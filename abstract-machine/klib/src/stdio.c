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
    
    // 解析格式控制符
    char pad_char = ' ';
    int width = 0;

    // 检查填充字符, 如 %08d 中的 0
    if (*fmt == '0') {
      pad_char = '0';
      ++fmt;
    }

    // 解析宽度, 如 %08d 中的 8
    while (*fmt >= '0' && *fmt <= '9') {
      width = width * 10 + (*fmt - '0');
      ++fmt;
    }

    switch(*fmt++){
      case 's': {
        s = va_arg(ap, char*);
        int len = strlen(s);
        // 处理左侧填充
        while (width > len) {
            if (out && count < n - 1) out[count] = ' ';
            ++count;
            width--;
        }
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

        // 计算实际数字长度
        int len = i;
        // %04d, -1 -> -001 (填充在符号后)
        // %4d, -1 ->   -1 (填充在符号前，空格填充)
        
        // 如果是0填充且有负号，先输出负号，再填充0
        bool handle_sign_first = (pad_char == '0' && buf[i-1] == '-');
        if (handle_sign_first) {
            if(out && count < n - 1) out[count] = '-';
            ++count;
            len--; // 符号已经输出了，剩余长度减1
            i--;   // buf中最后一个字符是'-'，不需要再输出了
        }

        // 填充
        while (width > len) {
            if (out && count < n - 1) out[count] = pad_char;
            ++count;
            width--;
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
      case 'x': {
        unsigned int x = va_arg(ap, unsigned int);
        char buf[32];
        int i = 0;
        if (x == 0) {
          buf[i++] = '0';
        } else {
          while (x > 0) {
            int digit = x % 16;
            if (digit < 10) buf[i++] = digit + '0';
            else buf[i++] = digit - 10 + 'a';
            x /= 16;
          }
        }

        int len = i;
        // 填充
        while (width > len) {
            if (out && count < n - 1) out[count] = pad_char;
            ++count;
            width--;
        }

        // Reverse the string
        for (int j = i - 1; j >= 0; j--) {
          if (out && count < n - 1) {
            out[count] = buf[j];
          }
          ++count;
        }
        break;
      }
      case 'c': {
        char ch = (char)va_arg(ap, int);
        if(out && count < n - 1){
          out[count] = ch;
        }
        ++count;
        break;
      }
      case '%': {
        if(out && count < n - 1){
          out[count] = '%';
        }
        ++count;
        break;
      }
      default: 
        printf("vsnprintf: unknown format control %%%c\n", *(fmt - 1));
        assert(0);
    }
  }
  if(out && n > 0){
    size_t terminate_idx = (count < n - 1) ? count : (n - 1);
    out[terminate_idx] = '\0';
  }
  return count;
}

#endif
