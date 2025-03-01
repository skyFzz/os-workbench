#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
	/*
	size_t res = 0;
	for (const char *p = s; *p != '\0'; p++) res++;
	return res;
	*/
	const char *sc;
	for (sc = s; *sc != '\0'; sc++);
	
	return (sc - s);
}

size_t strnlen(const char *s, size_t maxlen) {
	/*
	size_t res = strlen(s);
	return res < maxlen ? res : maxlen;
	*/
	const char *es = s;
	while (*es && maxlen) {
		es++;
		maxlen--;
	}
	return (es - s);
}

char *strcpy(char *dst, const char *src) {
	/*
	size_t dlen = strlen(src);
	memcpy(dst, src, dlen);
	return dst;
	*/
	char *tmp = dst;

	while ((*dst++ = *src++) != '\0');

	return tmp;
}

char *strncpy(char *dst, const char *src, size_t n) {
	/*
	stpncpy(dst, src, n);
	return dst;
	*/
	char *tmp = dst;
	while (n) {
		if ((*tmp = *src) != 0)
			src++;
		tmp++;
		n--;
	}
	return dst;
}

char *stpncpy(char *dst, const char *src, size_t n) {
	/*
	size_t dlen;
	dlen = strnlen(src, n);
	// add padding if src is smaller than dst
	return memset(mempcpy(dst, src, dlen), 0, n - dlen);
	*/
	while ((*dst++ = *src++) != '\0');

	return --dst;
	
}

char *strcat(char *dst, const char *src) {
	/*
	size_t dlen = strlen(dst);
	size_t slen = strlen(src);

	// "abc" 3
	char *dste = dst + dlen;
	memcpy(dste, src, slen);
	return dst;
	*/
	char *tmp = dst;
	
	while (*dst)
		dst++;
	while ((*dst++ = *src++) != '\0');

	return tmp;

} 

int strcmp(const char *s1, const char *s2) {
	/*
	size_t l1 = strlen(s1);
	size_t l2 = strlen(s2);
	if (l1 < l2) {
		return -1;
	} else if (l1 > l2) {
		return 1;
	} else {
		return memcmp(s1, s2, l1);
	}
	*/
	unsigned char c1, c2;
	while (1) {
		c1 = *s1++;
		c2 = *s2++;
		if (c1 != c2) 
			return c1 < c2 ? -1 : 1;
		if (!c1)
			break;
	}
	return 0;
}

int strncmp(const char *s1, const char *s2, size_t n) {
	// return memcmp(s1, s2, n);
	unsigned char c1, c2;
	while (n) {
		c1 = *s1++;
		c2 = *s2++;
		if (c1 != c2)
			return c1 < c2 ? -1 : 1;
		if (!c1)
			break;
	}
	return 0;
}

void *memset(void *s, int c, size_t n) {
	/*
	char *s_cp = (char *)s;

	for (int i = 0; i < n; i++) {
		s_cp += i;
		*s_cp = c;
	}
	return s;
	*/
	char *xs = s;
	
	while (n--)
		*xs++ = c;
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
	/*
	memcpy(dst, src, n);
	*/
	char *tmp;
	const char *s;
	if (dst <= src) {
		tmp = dst;
		s = src;
		while (n--)
			*tmp++ = *s++;
	} else {
		tmp = dst;
		tmp += n;
		s = src;
		s += n;
		while (n--)
			*--tmp = *--s;	
	}
	return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
	/*
	// check if memory areas overlap
	if ((out >= in && out < in + n) || (in >= out && in < out + n)) {
		putch('E');
		putch('\n');
	}
	mempcpy(out, in, n);
	return out;
	*/
	char *tmp = out;
	const char *s = in;
	
	while (n--)
		*tmp++ = *s++;
	return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
	/*
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
	*/
	const unsigned char *su1, *su2;
	int res = 0;

	for (su1 = s1, su2 = s2; 0 < n; ++su1, ++su2, n--)
		if ((res = *su1 - *su2) != 0)
			break;
	return res;
}

#endif
