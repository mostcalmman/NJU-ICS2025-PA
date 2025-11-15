#include <klib.h>
#include <klib-macros.h>
#include <stddef.h>
#include <stdint.h>
#include <x86_64-linux-gnu/sys/types.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  size_t len = 0;
  while (s[len] != '\0') {
    len++;
  }
  return len;
}

char *strcpy(char *dst, const char *src) {
  size_t i = 0;
  while (src[i] != '\0') {
    dst[i] = src[i];
    i++;
  }
  dst[i] = '\0';
  return dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
  size_t i = 0;
  while(i < n && src[i] != '\0') {
    dst[i] = src[i];
    i++;
  }
  while(i < n) {
    dst[i] = '\0';
    i++;
  }
  return dst;
}

char *strcat(char *dst, const char *src) {
  strcpy(dst + strlen(dst), src);
  return dst;
}

int strcmp(const char *s1, const char *s2) {
  size_t i = 0;
  while(s1[i] != '\0' && s2[i] != '\0') {
    if(s1[i] != s2[i]) {
      return (unsigned char)s1[i] - (unsigned char)s2[i];
    }
    i++;
  }
  return (unsigned char)s1[i] - (unsigned char)s2[i];
}

int strncmp(const char *s1, const char *s2, size_t n) {
  size_t i = 0;
  while(s1[i] != '\0' && s2[i] != '\0' && i < n) {
    if(s1[i] != s2[i]) {
      return (unsigned char)s1[i] - (unsigned char)s2[i];
    }
    i++;
  }
  return i<n ? (unsigned char)s1[i] - (unsigned char)s2[i] : 0;
}

void *memset(void *s, int c, size_t n) {
  // for(size_t i = 0; i < n; i++) {
  //   ((char *)s)[i] = (char)c;
  // }
  // return s;
  uint32_t *s32 = (uint32_t *)s;
  size_t word_n = n / 4;
  uint32_t c32 = (uint32_t)c | ((uint32_t)c << 8) | ((uint32_t)c << 16) | ((uint32_t)c << 24);
  for(size_t i = 0; i < word_n; i++) {
    s32[i] = c32;
  }
  size_t byte_n = n % 4;
  uint8_t *s8 = (uint8_t *)(s32 + word_n);
  for(size_t i = 0; i < byte_n; i++) {
    s8[i] = (uint8_t)c;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  char *d = (char *)dst;
  const char *s = (const char *)src;
  if(d == s || n == 0) {
    return dst;
  }
  if(d > s && d < s + n) {
    for(size_t i = n - 1; i >= 0; i--) {
      d[i] = s[i];
    }
  } else {
    for(size_t i = 0; i < n; i++) {
      d[i] = s[i];
    }
  }
  return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
  // for(size_t i = 0; i < n; i++) {
  //   ((char *)out)[i] = ((char *)in)[i];
  // }
  // return out;
  uint32_t *d32 = (uint32_t *)out;
  const uint32_t *s32 = (const uint32_t *)in;
  size_t word_n = n / 4;
  for(size_t i = 0; i < word_n; i++) {
    d32[i] = s32[i];
  }
  size_t byte_n = n % 4;
  uint8_t *d8 = (uint8_t *)(d32 + word_n);
  const uint8_t *s8 = (const uint8_t *)(s32 + word_n);

  for(size_t i = 0; i < byte_n; i++) {
    d8[i] = s8[i];
  }
  return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  for(size_t i = 0; i < n; i++) {
    if(((unsigned char *)s1)[i] != ((unsigned char *)s2)[i]) {
      return ((unsigned char *)s1)[i] - ((unsigned char *)s2)[i];
    }
  }
  return 0;
}

#endif
