#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>
#include <limits.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

void out(const char *s);

/* A very poor and premature implementation of printf, supporting only %d, %c, %p, and %s */
int printf(const char *fmt, ...) {
  int ret=0;
  va_list ap;
  va_start(ap, fmt);
 
  for (; *fmt; fmt++) {
    if (*fmt == '%') {
      fmt++;
      if (*fmt == 'd') {
        int d = va_arg(ap, int);
        char buf[11];
        memset(buf, 0, 11);
        int i=0;
        
        if (d == 0) {
          putch('0');
          ret += sizeof(int);
          continue;
        }
  
        if (d < 0) {
          putch('-');
          d = -d;
        }

        while (d > 0) {
          buf[i++] = d%10 + '0';  
          d/=10;
        }

        char *p = buf+10;
        while (!*p) p--;
        for (; *p; p--) putch(*p);
        ret += sizeof(int);
      } else if (*fmt == 'l' && *(fmt + 1) == 'u') {
        unsigned long l = va_arg(ap, unsigned long);
        char buf[21];
        memset(buf, 0, 21);
        int i=0;

        if (l == 0) {
          putch('0');
          ret += sizeof(unsigned long);
          fmt++;
          continue;
        }

        while (l > 0) {
          buf[i++] = l%10 + '0';
          l/=10;
        }

        char *p = buf+20;
        while (!*p) p--;
        for (; *p; p--) putch(*p);
        ret += sizeof(unsigned long);
        fmt++; 
      } else if (*fmt == 'c') {
        char c = va_arg(ap, int);
        putch(c);
        ret += sizeof(char);
      } else if (*fmt == 's') {
        char *s = va_arg(ap, char *);
        out(s);
        ret += sizeof(char *);
      } else if (*fmt == 'p') {
        char buf[19];
        memset(buf, 0, 18);
        void *p = va_arg(ap, void *);
        uintptr_t ptr = (uintptr_t)p;
        const char hex[] = "0123456789abcdef";

        buf[0] = '0';
        buf[1] = 'x';
        for (int i=0; i<16; i++) {
          buf[17-i] = hex[ptr & 0xf];
          ptr >>= 4;
        }
        buf[18] = '\0';

        char *bp = buf;
        for (; *bp; bp++) putch(*bp);
        ret += sizeof(char *);
      } else {
        out("invalid format character\n");
      }
    } else {
      putch(*fmt);
      ret++;
    }
  }
  va_end(ap);
  return ret;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  return vsnprintf(out, INT_MAX, fmt, ap);
}

int sprintf(char *out, const char *fmt, ...) {
  int ret;
  va_list ap;
  va_start(ap, fmt);
  ret = vsprintf(out, fmt, ap);
  va_end(ap);
  return ret;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  int ret;
  va_list ap;
  va_start(ap, fmt);
  ret = vsnprintf(out, n, fmt, ap);
  va_end(ap);
  return ret;
}

void out(const char *s) {
  for (; *s; s++) putch(*s);
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("not implemented");
}

#endif
