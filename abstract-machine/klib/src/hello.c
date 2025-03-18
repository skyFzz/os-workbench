#include <am.h>
#include <stdarg.h>
#include <klib.h>

int main(const char *args) {
  
  char *root = (char *)malloc(26);
  root[0] = 'A';
  
  for (int i = 0; i < 26; i++) {
    root[i] = i + 'A';
  }
   
  return 0;
}
