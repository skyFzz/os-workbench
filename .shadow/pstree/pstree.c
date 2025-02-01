#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define PATH "/proc/"

bool P = false;
bool N = false;
bool V = false;

struct proc {
	char *name;
	uint32_t pid;
	uint32_t ppid;
	uint32_t child[];
}

void usage(char *argv) {
	printf("pstree: unsupported option '%s'\n", argv);
	printf("Usage: pstree [ -p | --show-pids ] [ -n | --numeric-sort ] [ -V | --version ]\n");
	printf("\n");
	printf("Display a tree of processes.\n");
	printf("\n");
	printf("\t-p, --show-pids\t\tshow PIDs\n");
	printf("\t-n, --numeric-sort\tsort output by PID\n");
	printf("\t-V, --version\t\tdisplay version information\n");
}

void valid(int argc, char *argv[]) {
	char *tmp;

	for (int i = 0; i < argc; i++) {
    		assert(argv[i]);
		if (i == 0) continue;

		int argl = strlen(argv[i]);
		if (argl == 2) {
			if (argv[i][0] == '-') {
				switch(argv[i][1]) {
					case 'p':
						P = true;
						break;
					case 'n':
						N = true;
						break;
					case 'V':
						V = true;
						break;
					default:
						usage(argv[i]);
						exit(-1);
				}
			} else {
				usage(argv[i]);
				exit(-1);
			} 
		} else if (argl > 2) {
			if (argv[i][0] == '-' && argv[i][1] == '-') {
				// tmp = (char *)malloc((argl - 2) * sizeof(char));	// buffer overflow
				tmp = (char *)malloc((argl - 2) + 1 * sizeof(char));
				assert(tmp);
				strcpy(tmp, argv[i] + 2);
				// if (strcmp(tmp, "show-pids")) {	// true for all non-matching strings
				if (strcmp(tmp, "show-pids") == 0) {
					P = true;
				} else if (strcmp(tmp, "numeric-sort") == 0) {
					N = true;
				} else if (strcmp(tmp, "version") == 0) {
					V = true;
				} else {
					usage(argv[i]);
					exit(-1);
				}
				free(tmp);
			} else {
				usage(argv[i]);
				exit(-1);
			}
		} else {
			usage(argv[i]);
			exit(-1);
		}
  	}
  	assert(!argv[argc]);
}


int main(int argc, char *argv[]) {
	valid(argc, argv);

  	return 0;
}
