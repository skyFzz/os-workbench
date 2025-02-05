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

struct Node {
	char *name;
	pid_t pid;
	pid_t ppid;
	struct Node *next;

	// refer to CLRS p.246 on a left-child, right-sibling representation for rooted trees
	struct Node *parent;
	struct Node *left;
	struct Node *risi;
};

struct List {
	struct Node *head;
	struct Node *tail;
};

unsigned int hash(pid_t pid) {
	return pid % HASH_SIZE;
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

void getArgs(int argc, char *argv[]) {
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
					free(path); // free path before exiting
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

struct List *makeLists() {
	struct List *lists = (struct List *)malloc(HASH_SIZE * sizeof(struct List));
	assert(lists);
	struct List *tmp;	// local worker
	DIR *dir;
	DIR *subdir;
	char path[20] = "/proc/";	// worst case: /proc/1234567/stat
	int ret;
	struct dirent *entry;
	struct dirent *subent;
	FILE *fp;	// $ulimit -Hn $1048576
	char pid_s[8];
	char ppid_s[8];
	int i;

	for (i = 0; i < HASH_SIZE; i++) {
		lists[i].head = NULL;
		lists[i].tail= NULL;
	}
	i = 0;

	dir = opendir(path);
	assert(dir);

	errno = 0;
	entry = readdir(dir);
	while (entry != NULL) {
		assert(entry->d_type != DT_UNKNOWN);	// only some fs fully support d_type
		if (entry->d_type == DT_DIR && entry->d_name[0] >= '0' && entry->d_name[0] <= '9') {
			struct Node *node = (struct Node *)malloc(sizeof(struct Node)); 
			node->next = NULL;
			node->name = (char *)malloc(128 * sizeof(char)); // on htis machine /proc/pid/stat can display over 15 bytes

			strncat(path, entry->d_name, 8);	// pid_max literal has 8 digits
			strncat(path, "/", 2);
			subdir = opendir(path); // /proc/pid/
			assert(subdir != NULL);
			errno = 0;
			subent = readdir(subdir); // /proc/pid/.
			printf("subent d_name is %s\n", subent->d_name);
			while (subent) {
				if (strncmp(subent->d_name, "stat", 5) == 0) {
					printf("hi\n");
					strncat(path, "stat", 5);
					printf("hi\n");
					fp = fopen(path, "r");
					if (!fp) {
						fclose(fp);
						exit(-1);
					}

					ret = fgetc(fp);
					while(ret != ' ') {
						pid_s[i++] = ret;
						ret = fgetc(fp);
					}
					printf("hi\n");
					pid_s[i] = '\0';
					node->pid = atoi(pid_s);
					printf("node->pid is %d\n", node->pid);
					
					i = 0;
					ret = fgetc(fp);
					ret = fgetc(fp);
					while(ret != ')') {
						node->name[i++] = ret;
						ret = fgetc(fp);
					}
					node->name[i] = '\0';
					printf("node->name is %s\n", node->name);

					i = 0;
					ret = fgetc(fp);	// omit ' '
					ret = fgetc(fp);	// omit 'S'
					ret = fgetc(fp);	// omit ' '
					ret = fgetc(fp);	// get ppid
					while(ret != ' ') {
						ppid_s[i++] = ret;
						ret = fgetc(fp);
					}
					ppid_s[i] = '\0';
					node->ppid = atoi(ppid_s);
					printf("node ppid is %d\n", node->ppid);

					// init the linked list otherwise just append to the end
					// tmp = lists[hash(node->pid)]; // this is wrong, together with struct List tmp; rather than using pointer
					tmp = &lists[hash(node->pid)]; // this is correct 
					printf("node name is %s\n", tmp->head->next->name);
					if (tmp->head == NULL) {
						// init dummy nodes if list is empty
						tmp->head = (struct Node *)malloc(sizeof(struct Node));
						tmp->tail = (struct Node *)malloc(sizeof(struct Node));
						tmp->head->next = node;
						tmp->tail->next = node;
					} else {
						tmp->tail->next->next = node;
						tmp->tail->next = node;
					}
					printf("node name is %s\n", tmp->head->next->name);
								
					ret = fclose(fp);
					assert(ret == 0);

					i = 0;	// reset
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
	assert(errno == 0);	// could be either EOF or error

	ret = closedir(dir);
	assert(ret == 0);

	return lists;
}

void freeLists(struct List *lists) {
	struct Node *nxt;
	struct Node *tar;
	for (int i = 0; i < HASH_SIZE; i++) {
		if (lists[i].head == NULL) continue;	// only free the allocated ones
		tar = lists[i].head->next;
		printf("head->next name is %s\n", tar->name);
		do {
			nxt = tar->next;
			free(tar->name); // don't forget about name
			free(tar);	
			tar = nxt;
		} while (tar != NULL);
		free(lists[i].head);	 
		free(lists[i].tail);
		lists[i].head = NULL;	// avoid dangling pointers
		lists[i].tail = NULL;
	}
	free(lists);
	lists = NULL;
}

int main(int argc, char *argv[]) {
	getArgs(argc, argv);

	struct List *lists = makeLists();
	assert(lists != NULL);

	freeLists(lists);
  	return 1;
}
