#include <NDL.h>
#include <sdl-video.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void SDL_BlitSurface(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect) {
  assert(dst && src);
  assert(dst->format->BitsPerPixel == src->format->BitsPerPixel);

  // 若 srcrect 为 NULL, 则拷贝整个源表面
  int src_x, src_y, src_w, src_h;
  if (srcrect) {
    src_x = srcrect->x; src_y = srcrect->y; src_w = srcrect->w; src_h = srcrect->h;
  } else {
    src_x = 0; src_y = 0; src_w = src->w; src_h = src->h;
  }

  // 若 dstrect 为 NULL, 则拷贝到目标表面的 (0,0) 位置
  int dst_x, dst_y;
  if (dstrect) {
    dst_x = dstrect->x; dst_y = dstrect->y;
  } else {
    dst_x = 0; dst_y = 0;
  }

  uint8_t *rec_start_pixel_src = src->pixels + src_y * src->pitch + src_x * src->format->BytesPerPixel;
  uint8_t *rec_start_pixel_dst = dst->pixels + dst_y * dst->pitch + dst_x * dst->format->BytesPerPixel;
  for (int i = 0; i < src_h; i ++) {
    memcpy(rec_start_pixel_dst + i * dst->pitch,
           rec_start_pixel_src + i * src->pitch,
           src_w * src->format->BytesPerPixel);
  }
}

void SDL_FillRect(SDL_Surface *dst, SDL_Rect *dstrect, uint32_t color) {
  int x, y, w, h;
  if (dstrect) {
    x = dstrect->x; y = dstrect->y; w = dstrect->w; h = dstrect->h;
  } else {
    x = 0; y = 0; w = dst->w; h = dst->h;
  }

  for (int i = 0; i < h; i ++) {
    for (int j = 0; j < w; j ++) {
      uint8_t *pixel = dst->pixels + (y + i) * dst->pitch + (x + j) * dst->format->BytesPerPixel;
      if (dst->format->BitsPerPixel == 8) {
        // printf("8bit\n");
        // 8位色通过调色板映射
        uint8_t r = (color >> 16) & 0xff;
        uint8_t g = (color >> 8) & 0xff;
        uint8_t b = color & 0xff;
        // 找到最接近的颜色索引
        uint8_t index = SDL_MapRGBA(dst->format, r, g, b, 255);
        *pixel = index;
      } else {
        assert(dst->format->BitsPerPixel == 32);
        *((uint32_t *)pixel) = color;
      }
    }
  }
}

void SDL_UpdateRect(SDL_Surface *s, int x, int y, int w, int h) {
  assert(s);
  if (w == 0 && h == 0) {
    w = s->w;
    h = s->h;
  }
  
  // 如果是32位色且全屏更新, 直接透传像素指针, 避免malloc和memcpy
  if (s->format->BitsPerPixel == 32 && w == s->w && s->pitch == w * 4) {
    NDL_DrawRect((uint32_t *)((uint8_t *)s->pixels + y * s->pitch + x * 4), x, y, w, h);
    return;
  }

  uint32_t *pixels = malloc(w * h * sizeof(uint32_t));
  assert(pixels);

  if (s->format->BitsPerPixel == 8) {
    // printf("8bit\n");
    // 8位色: 通过调色板转换为 32 位色
    SDL_Color *palette = s->format->palette->colors;
    for (int i = 0; i < h; i++) {
      for (int j = 0; j < w; j++) {
        uint8_t index = *((uint8_t *)s->pixels + (y + i) * s->pitch + (x + j));
        SDL_Color color = palette[index];
        // NDL 期望 00RRGGBB 格式
        pixels[i * w + j] = (color.a << 24) | (color.r << 16) | (color.g << 8) | color.b;
      }
    }
  } else {
    assert(s->format->BitsPerPixel == 32);
    // 32位色: 直接复制
    for (int i = 0; i < h; i++) {
      memcpy(pixels + i * w, 
             (uint8_t *)s->pixels + (y + i) * s->pitch + x * 4, 
             w * 4);
    }
  }

  NDL_DrawRect(pixels, x, y, w, h);

  free(pixels);
}

// APIs below are already implemented.

static inline int maskToShift(uint32_t mask) {
  switch (mask) {
    case 0x000000ff: return 0;
    case 0x0000ff00: return 8;
    case 0x00ff0000: return 16;
    case 0xff000000: return 24;
    case 0x00000000: return 24; // hack
    default: assert(0);
  }
}

SDL_Surface* SDL_CreateRGBSurface(uint32_t flags, int width, int height, int depth,
    uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask) {
  assert(depth == 8 || depth == 32);
  SDL_Surface *s = malloc(sizeof(SDL_Surface));
  assert(s);
  s->flags = flags;
  s->format = malloc(sizeof(SDL_PixelFormat));
  assert(s->format);
  if (depth == 8) {
    s->format->palette = malloc(sizeof(SDL_Palette));
    assert(s->format->palette);
    s->format->palette->colors = malloc(sizeof(SDL_Color) * 256);
    assert(s->format->palette->colors);
    memset(s->format->palette->colors, 0, sizeof(SDL_Color) * 256);
    s->format->palette->ncolors = 256;
  } else {
    s->format->palette = NULL;
    s->format->Rmask = Rmask; s->format->Rshift = maskToShift(Rmask); s->format->Rloss = 0;
    s->format->Gmask = Gmask; s->format->Gshift = maskToShift(Gmask); s->format->Gloss = 0;
    s->format->Bmask = Bmask; s->format->Bshift = maskToShift(Bmask); s->format->Bloss = 0;
    s->format->Amask = Amask; s->format->Ashift = maskToShift(Amask); s->format->Aloss = 0;
  }

  s->format->BitsPerPixel = depth;
  s->format->BytesPerPixel = depth / 8;

  s->w = width;
  s->h = height;
  s->pitch = width * depth / 8;
  assert(s->pitch == width * s->format->BytesPerPixel);

  if (!(flags & SDL_PREALLOC)) {
    s->pixels = malloc(s->pitch * height);
    assert(s->pixels);
  }

  return s;
}

SDL_Surface* SDL_CreateRGBSurfaceFrom(void *pixels, int width, int height, int depth,
    int pitch, uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask) {
  SDL_Surface *s = SDL_CreateRGBSurface(SDL_PREALLOC, width, height, depth,
      Rmask, Gmask, Bmask, Amask);
  assert(pitch == s->pitch);
  s->pixels = pixels;
  return s;
}

void SDL_FreeSurface(SDL_Surface *s) {
  if (s != NULL) {
    if (s->format != NULL) {
      if (s->format->palette != NULL) {
        if (s->format->palette->colors != NULL) free(s->format->palette->colors);
        free(s->format->palette);
      }
      free(s->format);
    }
    if (s->pixels != NULL && !(s->flags & SDL_PREALLOC)) free(s->pixels);
    free(s);
  }
}

SDL_Surface* SDL_SetVideoMode(int width, int height, int bpp, uint32_t flags) {
  if (flags & SDL_HWSURFACE) NDL_OpenCanvas(&width, &height);
  return SDL_CreateRGBSurface(flags, width, height, bpp,
      DEFAULT_RMASK, DEFAULT_GMASK, DEFAULT_BMASK, DEFAULT_AMASK);
}

void SDL_SoftStretch(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect) {
  assert(src && dst);
  assert(dst->format->BitsPerPixel == src->format->BitsPerPixel);
  assert(dst->format->BitsPerPixel == 8);

  int x = (srcrect == NULL ? 0 : srcrect->x);
  int y = (srcrect == NULL ? 0 : srcrect->y);
  int w = (srcrect == NULL ? src->w : srcrect->w);
  int h = (srcrect == NULL ? src->h : srcrect->h);

  assert(dstrect);
  if(w == dstrect->w && h == dstrect->h) {
    /* The source rectangle and the destination rectangle
     * are of the same size. If that is the case, there
     * is no need to stretch, just copy. */
    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;
    SDL_BlitSurface(src, &rect, dst, dstrect);
  }
  else {
    assert(0);
  }
}

void SDL_SetPalette(SDL_Surface *s, int flags, SDL_Color *colors, int firstcolor, int ncolors) {
  assert(s);
  assert(s->format);
  assert(s->format->palette);
  assert(firstcolor == 0);

  s->format->palette->ncolors = ncolors;
  memcpy(s->format->palette->colors, colors, sizeof(SDL_Color) * ncolors);

  if(s->flags & SDL_HWSURFACE) {
    assert(ncolors == 256);
    for (int i = 0; i < ncolors; i ++) {
      uint8_t r = colors[i].r;
      uint8_t g = colors[i].g;
      uint8_t b = colors[i].b;
    }
    SDL_UpdateRect(s, 0, 0, 0, 0);
  }
}

static void ConvertPixelsARGB_ABGR(void *dst, void *src, int len) {
  int i;
  uint8_t (*pdst)[4] = dst;
  uint8_t (*psrc)[4] = src;
  union {
    uint8_t val8[4];
    uint32_t val32;
  } tmp;
  int first = len & ~0xf;
  for (i = 0; i < first; i += 16) {
#define macro(i) \
    tmp.val32 = *((uint32_t *)psrc[i]); \
    *((uint32_t *)pdst[i]) = tmp.val32; \
    pdst[i][0] = tmp.val8[2]; \
    pdst[i][2] = tmp.val8[0];

    macro(i + 0); macro(i + 1); macro(i + 2); macro(i + 3);
    macro(i + 4); macro(i + 5); macro(i + 6); macro(i + 7);
    macro(i + 8); macro(i + 9); macro(i +10); macro(i +11);
    macro(i +12); macro(i +13); macro(i +14); macro(i +15);
  }

  for (; i < len; i ++) {
    macro(i);
  }
}

SDL_Surface *SDL_ConvertSurface(SDL_Surface *src, SDL_PixelFormat *fmt, uint32_t flags) {
  assert(src->format->BitsPerPixel == 32);
  assert(src->w * src->format->BytesPerPixel == src->pitch);
  assert(src->format->BitsPerPixel == fmt->BitsPerPixel);

  SDL_Surface* ret = SDL_CreateRGBSurface(flags, src->w, src->h, fmt->BitsPerPixel,
    fmt->Rmask, fmt->Gmask, fmt->Bmask, fmt->Amask);

  assert(fmt->Gmask == src->format->Gmask);
  assert(fmt->Amask == 0 || src->format->Amask == 0 || (fmt->Amask == src->format->Amask));
  ConvertPixelsARGB_ABGR(ret->pixels, src->pixels, src->w * src->h);

  return ret;
}

uint32_t SDL_MapRGBA(SDL_PixelFormat *fmt, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  if (fmt->BytesPerPixel == 1) {
    assert(fmt->palette);
    int best_diff = 0x7fffffff;
    uint8_t best_index = 0;
    for (int i = 0; i < fmt->palette->ncolors; i ++) {
      SDL_Color *c = &fmt->palette->colors[i];
      // 计算曼哈顿距离寻找最接近的颜色
      int diff = abs((int)c->r - r) + abs((int)c->g - g) + abs((int)c->b - b);
      if (diff < best_diff) {
        best_diff = diff;
        best_index = i;
        if (diff == 0) break; // 找到精确匹配
      }
    }
    return best_index;
  }

  assert(fmt->BytesPerPixel == 4);
  uint32_t p = (r << fmt->Rshift) | (g << fmt->Gshift) | (b << fmt->Bshift);
  if (fmt->Amask) p |= (a << fmt->Ashift);
  return p;
}

int SDL_LockSurface(SDL_Surface *s) {
  assert(0);
  return 0;
}

void SDL_UnlockSurface(SDL_Surface *s) {
  assert(0);
}
