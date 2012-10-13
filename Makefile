all:
	gcc -std=c99 -O2 -Wall -Wextra -D_POSIX_SOURCE -D_POSIX_C_SOURCE=2 -D_BSD_SOURCE -o bin/execsub src/execsub.c
#	gcc -std=c99 -O2 -Wall -Wextra -D_POSIX_SOURCE -D_POSIX_C_SOURCE=2 -o bin/confirmsub src/confirmsub.c

debug:
	gcc -std=c99 -ggdb -Wall -Wextra -D_POSIX_SOURCE -D_POSIX_C_SOURCE=2 -D_BSD_SOURCE -o bin/execsub src/execsub.c
#	gcc -std=c99 -ggdb -Wall -Wextra -D_POSIX_SOURCE -D_POSIX_C_SOURCE=2 -o bin/confirmsub src/confirmsub.c

clean:
	rm bin/execsub
#	rm bin/confirmsub
