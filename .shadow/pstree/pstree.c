#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <assert.h>
#include <errno.h>

bool P = false;
bool N = false;
bool V = false;

struct proc {
	char *name;
	uint32_t pid;
	uint32_t ppid;
	uint32_t child[];
};

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

void read(struct proc *procs) {
	DIR *dir;
	DIR *subdir;
	const char path[] = "/proc/";
	char tmp[16] = "/proc/";
	int ret;
	struct dirent *entry;
	struct dirent *subent;

	dir = opendir(path);
	assert(dir != NULL);

	errno = 0;
	entry = readdir(dir);
	while (entry != NULL) {
		assert(entry->d_type != DT_UNKNOWN);	// only some fs fully support d_type
		if (entry->d_type == DT_DIR && entry->d_name[0] >= '0' && entry->d_name[0] <= '9') {
			printf("%s\n", entry->d_name);
			strncat(tmp, entry->d_name, 8);				
			printf("%s\n", tmp);
			subdir = opendir(tmp);
			printf("%d\n", errno);
			assert(subdir != NULL);
			errno = 0;
			subent = readdir(dir);
			while (subent != NULL) {
				printf("Entry name: %s\n", subent->d_name);
				subent = readdir(subdir);
			}		
			assert(errno == 0);
			ret = closedir(subdir);
			assert(ret == 0);
			
			tmp = "/proc/";
		} 	
		entry = readdir(dir);	
	}
	assert(errno == 0);

	ret = closedir(dir);
	assert(ret == 0);
}

int main(int argc, char *argv[]) {
	struct proc *procs = malloc(50000 * sizeof(struct proc));
	assert(procs != NULL);
	valid(argc, argv);
	read(procs);

	free(procs);
  	return 0;
}
