#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...) {
  panic("Not implemented");
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  panic("Not implemented");
}

int sprintf(char *out, const char *fmt, ...) {
  char* s;
  int d;
  va_list ap;
  va_start(ap, fmt);
  while(*fmt){
    if(*fmt != '%'){
      *out++ = *fmt++;
      continue;
    }
    fmt++;
    switch(*fmt++){
      case 's': {
        s = va_arg(ap, char*);
        while(*s){
          *out++ = *s++;
        }
        break;
      }
      case 'd': {
        d = va_arg(ap, int);
        char buf[15];
        int i = 0;
        if(d == 0){ buf[i++] = '0'; break; }

        bool negative = false;
        if(d < 0){
          negative = true;
          d = -d;
        }
        while(d > 0){
          buf[i++] = (d % 10) + '0';
          d /= 10;
        }
        if(negative){
          buf[i++] = '-';
        }
        
        // Reverse the string
        for(int j = i - 1; j >= 0; j--){
          *out++ = buf[j];
        }
        break;
      }
    }
  }
  *out = '\0';
  va_end(ap);
  return strlen(out);
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
