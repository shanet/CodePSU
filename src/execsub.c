#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/file.h>

#define BUFFER 512

void write_results(char *team, char *prob, char *rev, char *message, int skip_log);
void write_score_log(char *team, char *prob);
char* add_leading_zero(char *string);

int main(int argc, char **argv) {
    char *prog = (argc > 0) ? argv[0] : NULL;
    char *team;
    char *prob;
    char *rev;
    char *lang;
    char *binary_path;
    char *binary;
    char *input_path;
    char *output_path;
    char *time_path;

    FILE *_time;
    int time_limit;
    time_t start_time;

    // Force www-data user
    /*if(strcmp(getenv("USER"), "www-data") != 0) {
        fprintf(stderr, "\nYou must be the www-data user to run this program. You are seen as: %s\n\n", getenv("USER"));
        return 1;
    }*/

    if(argc == 10) {
        team        = argv[1];
        prob        = argv[2];
        rev         = argv[3];
        lang        = argv[4];
        binary_path = argv[5];
        binary      = argv[6];
        input_path  = argv[7];
        output_path = argv[8];
        time_path   = argv[9];
    } else {
        printf("Usage: %s [teamNo] [probNo] [revNo] [language] [binary path] [binary] [path to input file] [path to output file] [path to time limit file]\n", prog);
        return 1;
    }

    // Read the time limit from the file
    _time = fopen(time_path, "r");
    if(fscanf(_time, "%d", &time_limit) == EOF) return 1;
    fclose(_time);

    // Check time limit is > 0
    if(time_limit <= 0) {
        fprintf(stderr, "%s: Time limit must be greater than 0.\n", prog);
        write_results(team, prob, rev, "ERROR: Time limit must be greater than 0.", 0);
        return 0;
    }

    // Change the directory the binary is in
    if(chdir(binary_path) == -1) {
        fprintf(stderr, "%s: Failed to change directory\n", prog);
        write_results(team, prob, rev, "ERROR: Failed to change directory.", 0);
        return 1;
    }

    // Show that the test is being run
    write_results(team, prob, rev, "Executing code...", 1);

    // Fork and exec the requested binary
    pid_t pid = fork();
    if(!pid) {
        // Connect stdin to the input file
        if(freopen(input_path, "r", stdin) == NULL) {
            fprintf(stderr, "%s: Failed to redirect input file to stdin\n", prog);
            write_results(team, prob, rev, "ERROR: Failed to redirect input file to stdin.", 0);
            return 1;
        }

        // Connect stdout to the output file
        if(freopen("output.txt", "w", stdout) == NULL) {
            fprintf(stderr, "%s: Failed to redirect stdout to output file\n", prog);
            write_results(team, prob, rev, "ERROR: Failed to redirect stdout to output file.", 0);
            return 1;
        }

        // Execute the binary
        if(strcmp(lang, "java") == 0) {
            execlp("java", "java", "main", NULL);
        } else {
            execl(binary, binary, NULL);
        }
        fprintf(stderr, "%s: Failed to execute binary: %s\n", prog, strerror(errno));
        write_results(team, prob, rev, "ERROR: Failed to execute binary.", 0);
        return 1;
    } else if(pid < 0) {
        fprintf(stderr, "%s: Failed to fork process: %s\n", prog, strerror(errno));
        write_results(team, prob, rev, "ERROR: Failed to fork process.", 0);
    }

    // Get the current time
    start_time = time(NULL);

    // While under the time limit, wait for the child to return
    int status;
    int returned = 0;
    while(time(NULL) < start_time + time_limit) {
        if(waitpid(pid, &status, WNOHANG) == pid) {
            returned = 1;
            break;
        }
        usleep(1000);
    }

    // Write a newline to the output file in case the submission did not
    FILE *output = fopen("output.txt", "a");
    fprintf(output, "\n");
    fclose(output);

    // If the child didn't return in the time limit, sigkill it
    if(!returned) {
        kill(pid, SIGKILL);
        write_results(team, prob, rev, "Time limit execeeded.", 0);
        fprintf(stderr, "%s: Time limit execeeded.\n", prog);
    // If it did return, run a diff on the outputs
    } else {
        char cmd[BUFFER];
        sprintf(cmd, "diff --strip-trailing-cr --ignore-blank-lines %s output.txt", output_path);

        FILE *diff = popen(cmd, "r");
        FILE *diff_output = fopen("diff.txt", "w");

        // Copy the diff output to the diff output file
        char buf[BUFFER];
        while(fgets(buf, BUFFER, diff) != NULL) {
            fprintf(diff_output, "%s\n", buf);
        }

        int exit_code = WEXITSTATUS(pclose(diff));
        fclose(diff_output);

        // Exit code 0 from diff means files are indentical
        if(exit_code == 0) {
            write_results(team, prob, rev, "Correct output.", 0);
            write_score_log(team, prob);
            fprintf(stderr, "%s: Correct output.\n", prog);
        } else {
            write_results(team, prob, rev, "Incorrect output.", 0);
            fprintf(stderr, "%s: Incorrect output.\n", prog);
        }
    }

    fprintf(stderr, "%s: Finished with team %s, problem %s (revision %s).\n", prog, team, prob, rev);

    return 0;
}


void write_results(char *team, char *prob, char *rev, char *message, int skip_log) {
    char path[BUFFER];
    sprintf(path, "../../../../results/team_%s.htm", team);

    // Read the template, writing each line to the result file and replace the message line
    // when we get to it
    FILE *template = fopen("../../../../results/submission-result.htm", "r");
    FILE *result   = fopen(path, "w");

    char line[BUFFER];
    while(fgets(line, BUFFER, template) != NULL) {
        if(strstr(line, "<!--REPLACE-->") != NULL) {
            fprintf(result, "Team %s: Problem %s (submission %s): %s\n", team, prob, rev, message);
        } else {
            fprintf(result, "%s", line);
        }
    }

    fclose(template);
    fclose(result);
    
    // Write the results to the exec log unless told not to
    if(!skip_log) {
        FILE *log = fopen("../../../../exec.log", "a");

        // Add a leading 0 to any number less than ten
        team = add_leading_zero(team);
        prob = add_leading_zero(prob);
        rev  = add_leading_zero(rev);

        // Try to lock the log and if locked, wait for it to become unlocked
        int lock;
        do {
            lock = flock(fileno(log), LOCK_EX);
        } while(lock == -1 && errno == EWOULDBLOCK);

        fprintf(log, "Team %s: Problem %s (revision %s): %s\n", team, prob, rev, message);

        // Unlock the file
        flock(fileno(log), LOCK_UN);

        fclose(log);

        free(team);
        free(prob);
        free(rev);
    }
}


void write_score_log(char *team, char *prob) {
    FILE *log = fopen("../../../../score.log", "a");

    // Add a leading 0 to any number less than ten
    team = add_leading_zero(team);
    prob = add_leading_zero(prob);

    // Try to lock the log and if locked, wait for it to become unlocked
    int lock;
    do {
        lock = flock(fileno(log), LOCK_EX);
    } while(lock == -1 && errno == EWOULDBLOCK);

    fprintf(log, "team_%s:%sU\n", team, prob);

    // Unlock the file
    flock(fileno(log), LOCK_UN);

    fclose(log);

    free(team);
    free(prob);
}


char* add_leading_zero(char *string) {
    char *new_string = malloc(sizeof(char)*3);
    if(new_string == NULL) return NULL;

    if(atoi(string) < 10) {
        new_string[0] = '0';
        new_string[1] = *string;
        new_string[2] = '\0';
    } else {
        new_string[0] = *string;
        new_string[1] = *(string+1);
        new_string[2] = '\0';
    }

    return new_string;
}