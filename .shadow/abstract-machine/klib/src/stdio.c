#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>


#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...) {
	size_t cnt = 0;
	va_list ap;

	va_start(ap, fmt);	// init ap for subsequent use by va_arg() and va_end()
	while (fmt) {
		for (; *fmt; fmt++) {
			putch(*fmt);
			cnt++;
		}
		fmt = va_arg(ap, const char *);
	}
	va_end(ap);		// ap is undefined after this call
	return cnt;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  panic("Not implemented");
}

int sprintf(char *out, const char *fmt, ...) {
  panic("Not implemented");
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
