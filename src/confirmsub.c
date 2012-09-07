#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/file.h>
#include <sys/stat.h>

#define BUFFER 512

void write_score_log(char *team, char *prob, char *score_path, int remove);
void write_exec_log(char *team, char *prob, char *rev, char *exec_path, int remove);

int main(int argc, char **argv) {
    char *prog = (argc > 0) ? argv[0] : NULL;
    char *team;
    char *prob;
    char *rev;
    char *score_path;
    char *exec_path;
    char *diff_path;
    char *sub_out_path;
    char *cor_out_path;
    char action[BUFFER];
    char confirm[BUFFER];

    // Force www-data user
    if(strcmp(getenv("USER"), "www-data") != 0) {
        fprintf(stderr, "\nYou must be the www-data user to run this program. You are seen as: %s\n\n", getenv("USER"));
        return 1;
    }

    if(argc == 9) {
        team         = argv[1];
        prob         = argv[2];
        rev          = argv[3];
        score_path   = argv[4];
        exec_path    = argv[5];
        diff_path    = argv[6];
        sub_out_path = argv[7];
        cor_out_path = argv[8];

    } else {
        fprintf(stderr, "Usage: %s [teamNo] [probNo] [revNo] [path to score log] [path to exec log] [path to diff file] [path to submission output file] [path to correct output file]\n", prog);
        return 1;
    }

    // Open the diff, and output files
    FILE *diff    = fopen(diff_path, "r");
    FILE *sub_out = fopen(sub_out_path, "r");
    FILE *cor_out = fopen(cor_out_path, "r");

    if(diff == NULL) {
        fprintf(stderr, "diff.txt file descriptor is null\n");
        return 1;
    } else if(sub_out == NULL) {
        fprintf(stderr, "Submission output.txt file descriptor is null\n");
        return 1;
    } else if(cor_out == NULL) {
        fprintf(stderr, "Correct output.txt file descriptor is null\n");
        return 1;
    }

    // Deal with it
    if(system("clear")) {}

    // Output the diff file
    char line[BUFFER];
    while(fgets(line, BUFFER, diff) != NULL) {
        printf("%s", line);
    }
    printf("--------------------------------------\n^Diff output^\n\n");
    printf("Press enter to continue.");
    if(fgets(line, BUFFER, stdin) == NULL) return 1;

    if(system("clear")) {}

    // Output the correct output
    while(fgets(line, BUFFER, cor_out) != NULL) {
        printf("%s", line);
    }
    printf("--------------------------------------\n^Correct output^\n\n");
    printf("Press enter to continue.");
    if(fgets(line, BUFFER, stdin) == NULL) return 1;

    if(system("clear")) {}

    // Output the submission output
    while(fgets(line, BUFFER, sub_out) != NULL) {
        printf("%s", line);
    }
    printf("--------------------------------------\n^Submission output^\n\n");

    do {
        printf("\t1.) Confirm solution.\n\t2.) False positive. Remove points.\n\t3.) Do nothing.\n\n>> ");
        if(fgets(action, BUFFER, stdin) == NULL) return 1;

        printf("Are you sure? (y,n) ");
        if(fgets(confirm, BUFFER, stdin) == NULL) return 1;

    } while(strcmp(confirm, "n\n") == 0);

    if(strcmp(action, "1\n") == 0) {
        write_score_log(team, prob, score_path, 0);
        write_exec_log(team, prob, rev, exec_path, 0);
        printf("Confirmed solution to problem %s (revision %s) for team %s.\n", prob, rev, team);
    } else if(strcmp(action, "2\n") == 0) {
        write_score_log(team, prob, score_path, 1);
        write_exec_log(team, prob, rev, exec_path, 1);
        printf("Removed points for problem %s (revision %s) for team %s.\n", prob, rev, team);
    } else {
        printf("Taking no action on problem %s (revision %s) for team %s.\n", prob, rev, team);
    }

    return 0;
}


void write_score_log(char *team, char *prob, char *score_path, int remove) {
    // WE DON'T NEED NO STINKIN' COPY FUNCTION!!!!!
    // Make a tmp file
    char score_tmp_path[BUFFER];
    strncpy(score_tmp_path, score_path, strlen(score_path));
    strncat(score_tmp_path, ".tmp", 4);

    FILE *log     = fopen(score_path, "r");
    FILE *tmp_log = fopen(score_tmp_path, "w");

    if(log == NULL) {
        fprintf(stderr, "score.log file descriptor is null\n");
        return;
    } else if(tmp_log == NULL) {
        fprintf(stderr, "Temp score.log file descriptor is null\n");
        return;
    }

    // Try to lock the log and if locked, wait for it to become unlocked
    int lock;
    do {
        lock = flock(fileno(log), LOCK_EX);
    } while(lock == -1 && errno == EWOULDBLOCK);

    // Construct the line to be removed/changed
    char needle[BUFFER];
    char new_line[BUFFER];
    sprintf(needle, "team_%s:%s\n", team, prob);
    sprintf(new_line, "team_%s:%sC\n", team, prob);

    // Copy the contents of the score log into the tmp log
    char line[BUFFER];
    int found = 0;
    while(fgets(line, BUFFER, log) != NULL) {
        // Skip the line to be removed / insert the new line
        if(strspn(line, needle) != 0) {
            found = 1;
            if(remove) continue;
            fprintf(tmp_log, "%s", new_line);
        } else {
            fprintf(tmp_log, "%s", line);
        }
    }

    // If the needle wasn't found because the points were previously marked
    // as a false positive, insert the line at the end of the file
    if(!found && !remove) {
        fprintf(tmp_log, "%s", new_line);
    }

    // Rename the tmp log to the real log
    rename(score_tmp_path, score_path);

    // Unlock the file
    flock(fileno(log), LOCK_UN);

    fclose(log);
    fclose(tmp_log);
}


void write_exec_log(char *team, char *prob, char *rev, char *exec_path, int remove) {
    // Make a tmp file
    char exec_tmp_path[BUFFER];
    strncpy(exec_tmp_path, exec_path, strlen(exec_path));
    strncat(exec_tmp_path, ".tmp", 4);

    FILE *log     = fopen(exec_path, "r");
    FILE *tmp_log = fopen(exec_tmp_path, "w");

    if(log == NULL) {
        fprintf(stderr, "score.log file descriptor is null\n");
        return;
    } else if(tmp_log == NULL) {
        fprintf(stderr, "Temp score.log file descriptor is null\n");
        return;
    }

    // Try to lock the log and if locked, wait for it to become unlocked
    int lock;
    do {
        lock = flock(fileno(log), LOCK_EX);
    } while(lock == -1 && errno == EWOULDBLOCK);

    // Construct the line to be changed
    char new_line[BUFFER];
    char needle[BUFFER];
    sprintf(needle, "Team %s: Problem %s (revision %s): Correct output.\n", team, prob, rev);
    if(remove) {
        sprintf(new_line, "Team %s: Problem %s (revision %s): Correct output. | FALSE POSITIVE\n", team, prob, rev);
    } else {
        sprintf(new_line, "Team %s: Problem %s (revision %s): Correct output. | CONFIRMED\n", team, prob, rev);
    }

    // Copy the contents of the score log into the tmp log
    char line[BUFFER];
    while(fgets(line, BUFFER, log) != NULL) {
        if(strspn(line, needle) != 0) {
            fprintf(tmp_log, "%s", new_line);
        } else {
            fprintf(tmp_log, "%s", line);
        }
    }

    // Rename the tmp log to the real log
    rename(exec_tmp_path, exec_path);

    // Correct the permissions of the new log file
    /*if(chmod(exec_path, S_IRWXU | S_IRWXG | S_IROTH) != 0) {
        fprintf(stderr, "Error changing permissions of exec log. This must be done manually or bad things will happen: %s\n", strerror(errno));
    }
    struct passwd *user_info = getpwnam("www-data");
    if(chown(exec_path, user_info->pw_uid, user_info->pw_gid) != 0) {
        fprintf(stderr, "Error changing ownership of exec log. This must be done manually or bad things will happen: %s\n", strerror(errno));
    }*/

    // Unlock the file
    flock(fileno(log), LOCK_UN);

    fclose(log);
    fclose(tmp_log);
}