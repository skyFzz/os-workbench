#include <string.h>
#include <stdio.h>

int main() {
	//char *a = "abeaa";
	//char *b = "abecaa";
	char a[] = "abeaa";
	char b[] = "abecza";
//	memmove(b, a, 4);
	strncpy(b, a, 4);
//	strcpy(b, a);
//	memcpy(b, a, 4);
	printf("a: %s\n", a);
	printf("b: %s\n", b);
	printf("sizeof(size_t): %ld\n", sizeof(size_t));

	
	return 0;
}
