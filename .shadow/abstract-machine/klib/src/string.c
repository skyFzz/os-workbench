#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
	size_t res = 0;
	for (const char *p = s; *p != '\0'; p++) res++;
	return res;
}

size_t strnlen(const char *s, size_t maxlen) {
	size_t res = strlen(s);
	return res < maxlen ? res : maxlen;
}

char *strcpy(char *dst, const char *src) {
	size_t dlen = strlen(src);
	memcpy(dst, src, dlen);
	return dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
	stpncpy(dst, src, n);
	return dst;
}

char *stpncpy(char *dst, const char *src, size_t n) {
	size_t dlen;
	dlen = strnlen(src, n);
	// add padding if src is smaller than dst
	return memset(mempcpy(dst, src, dlen), 0, n - dlen);
	
}

char *strcat(char *dst, const char *src) {
	size_t dlen = strlen(dst);
	size_t slen = strlen(src);

	// "abc" 3
	char *dste = dst + dlen;
	memcpy(dste, src, slen);
	return dst;
}

int strcmp(const char *s1, const char *s2) {
	size_t l1 = strlen(s1);
	size_t l2 = strlen(s2);
	if (l1 < l2) {
		return -1;
	} else if (l1 > l2) {
		return 1;
	} else {
		return memcmp(s1, s2, l1);
	}
}

int strncmp(const char *s1, const char *s2, size_t n) {
	return memcmp(s1, s2, n);
}

void *memset(void *s, int c, size_t n) {
	char *s_cp = (char *)s;

	for (int i = 0; i < n; i++) {
		s_cp += i;
		*s_cp = c;
	}
	return s;
}

void *memmove(void *dst, const void *src, size_t n) {
	// memory areas may overlap, could overwrite data 
	/*
	// malloc requires extra implementation, use memcpy first
	void *tmp = malloc(n);
	for (int i = 0; i < n; i++) {
		*tmp += i;
		*tmp = *(src + i);
	}
	mempcpy(dst, tmp, n);
	free(tmp);
	*/
	memcpy(dst, src, n);
	return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
	// check if memory areas overlap
	if ((out >= in && out < in + n) || (in >= out && in < out + n)) {
		putch('E');
		putch('\n');
	}
	mempcpy(out, in, n);
	return out;
}

void *mempcpy(void *out, const void *in, size_t n) {
	size_t i;
	char *out_cp = (char *)out;
	const char *in_cp = (char *)in;

	for (i = 0; i < n; i++) {
		*out_cp += i;
		*out_cp = *(in_cp + i);
	}
	return ++out_cp;
}

int memcmp(const void *s1, const void *s2, size_t n) {
	const char *s1_cp = s1;
	const char *s2_cp = s2;

	for (int i = 0; i < n; i++) {
		s1_cp += i;
		s2_cp += i;
		if (*s1_cp < *s2_cp) {
			return -1;
		} else if (*s1_cp > *s2_cp) {
			return 1;
		}
	}
	return 0;
}

#endif
