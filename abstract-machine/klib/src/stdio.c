#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...) {
  void putch(char ch); 
  
  char print_buf[1024];
  va_list args;
  va_start(args, fmt);
  if( vsnprintf(print_buf, sizeof(print_buf), fmt, args) >= sizeof(print_buf)) {
    assert(0);
  }  
  va_end(args);

  int output_count = 0;
  while (print_buf[output_count] != '\0') {
    putch(print_buf[output_count]);
    output_count++;
  }
  
  return output_count;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  // 使用 (size_t)-1 表示无限大 (SIZE_MAX)
  return vsnprintf(out, (size_t)-1, fmt, ap);
}

int sprintf(char *out, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int ret = vsprintf(out, fmt, ap);
  va_end(ap);
  return ret;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int ret = vsnprintf(out, n, fmt, ap);
  va_end(ap);
  return ret;
}

// 通用可变参数列表处理器
int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  char* s;
  int d;
  size_t count = 0;
  // count用来记录已经处理的字符数, 不一定写入out(处理和写入分开)
  while(*fmt){
    if(*fmt != '%'){
      if(out && count < n - 1){
        out[count] = *fmt;
      }
      ++count;
      ++fmt;
      continue;
    }
    ++fmt;
    switch(*fmt++){
      case 's': {
        s = va_arg(ap, char*);
        while(*s){
          if(out && count < n - 1){
            out[count] = *s;
          }
          ++count;
          ++s;
        }
        break;
      }
      case 'd': {
        d = va_arg(ap, int);
        char buf[32];
        int i = 0;
        if(d == 0){ 
          buf[i++] = '0'; 
        }else{
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
        }
        // Reverse the string
        for(int j = i - 1; j >= 0; j--){
          char ch = buf[j];
          if(out && count < n - 1){
            out[count] = ch;
          }
          ++count;
        }
        break;
      }
      default: assert(0);
    }
  }
  if(out && n > 0){
    size_t terminate_idx = (count < n - 1) ? count : (n - 1);
    out[terminate_idx] = '\0';
  }
  return count;
}

#endif
