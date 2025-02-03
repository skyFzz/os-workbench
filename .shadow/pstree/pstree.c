#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>

#define HASH_SIZE 65536

bool P = false;
bool N = false;
bool V = false;

// refer to CLRS p.246 on a left-child, right-sibling representation for rooted trees
struct Node {
	char *name;
	pid_t pid;
	pid_t ppid;
	struct Node *parent;
	struct Node *left;
	struct Node *risi;
	struct Node *next;
};

unsigned int hash(pid_t pid) {
	return pid % HASH_SIZE;
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
	char *path;

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
				// path = (char *)malloc((argl - 2) * sizeof(char));	// buffer overflow
				path = (char *)malloc((argl - 2) + 1 * sizeof(char));
				assert(path);
				strcpy(path, argv[i] + 2);
				// if (strcmp(path, "show-pids")) {	// true for all non-matching strings
				if (strcmp(path, "show-pids") == 0) {
					P = true;
				} else if (strcmp(path, "numeric-sort") == 0) {
					N = true;
				} else if (strcmp(path, "version") == 0) {
					V = true;
				} else {
					usage(argv[i]);
					exit(-1);
				}
				free(path);
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

void read(struct Node *hashmap) {
	DIR *dir;
	DIR *subdir;
	char path[20] = "/proc/";	// worst case: /proc/1234567/stat
	int ret;
	struct dirent *entry;
	struct dirent *subent;
	FILE *fp;	// $ulimit -Hn $1048576
	char pid[8];
	int i;

	dir = opendir(path);
	assert(dir != NULL);

	errno = 0;
	entry = readdir(dir);
	while (entry != NULL) {
		assert(entry->d_type != DT_UNKNOWN);	// only some fs fully support d_type
		if (entry->d_type == DT_DIR && entry->d_name[0] >= '0' && entry->d_name[0] <= '9') {
			strncat(path, entry->d_name, 8);	// pid_max literal has 8 digits
			strncat(path, "/", 2);
			subdir = opendir(path);
			assert(subdir != NULL);
			errno = 0;
			subent = readdir(dir);
			while (subent != NULL) {
				if (strncmp(subent->d_name, "stat", 5) == 0) {
					strncat(path, "stat", 5);
					fp = fopen(path, "r");
					if (!fp) {
						fclose(fp);
						eixt(-1);
					}
					ret = fgetc(fp);
					while(ret != ' ') {
						pid[i++] = ret;
					}
					pid[i] = '\0';
					printf("pid is %s\n", pid);
					ret = fclose(fp);
					assert(ret == 0);
				}
				subent = readdir(subdir);
			}		
			assert(errno == 0);
			ret = closedir(subdir);
			assert(ret == 0);
			
			strcpy(path, "/proc/");		// reset root dir
		} 	
		entry = readdir(dir);	
	}
	assert(errno == 0);

	ret = closedir(dir);
	assert(ret == 0);
}

int main(int argc, char *argv[]) {
	struct Node hashmap[HASH_SIZE];
	assert(hashmap != NULL);

	valid(argc, argv);
	read(hashmap);

	free(hashmap);
  	return 0;
}
