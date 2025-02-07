#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...) {
	/*
    va_list ap;
    int d;
    char c;
    char *s;

    va_start(ap, fmt);
    while (*fmt) {
        if (*fmt == '%') {                         // formatted string
            fmt++;
            switch (*fmt++) {
                case 's':
                    s = va_arg(ap, char *);         // get the next type
                    while (*s) putch(*s++);
                    break;
                case 'd':
                    d = va_arg(ap, int);
                    putch(d);
                    break;
                case 'c':
                    c = (char) va_arg(ap, int);     // va_arg only takes fully promted types
                    putch(c);
                    break;
            }
        } else {
            putch(*fmt++);                          // normal string
        }
    }
    va_end(ap);
    return 0;
	*/
	panic("Not implemented");
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
