#include <fs.h>

size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);
size_t serial_write(const void *buf, size_t offset, size_t len);
size_t events_read(void *buf, size_t offset, size_t len);
size_t dispinfo_read(void *buf, size_t offset, size_t len);
size_t fb_write(const void *buf, size_t offset, size_t len);
size_t sb_write(const void *buf, size_t offset, size_t len);
size_t sbctl_read(void *buf, size_t offset, size_t len);
size_t sbctl_write(const void *buf, size_t offset, size_t len);

typedef size_t (*ReadFn) (void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn) (const void *buf, size_t offset, size_t len);

typedef struct {
  char *name;
  size_t size;
  size_t disk_offset;
  ReadFn read;
  WriteFn write;
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_EVENT, FD_FB, FD_CANVAS};

#define NR_FD 32
#define FD_RESERVED (FD_CANVAS + 1)

typedef struct {
  int used;
  int file_idx;   // index into file_table[]
  off_t offset;   // per-fd offset
} FDinfo;

static FDinfo fd_table[NR_FD];

static inline void fd_check(int fd) {
  if (fd < 0 || fd >= NR_FD || !fd_table[fd].used) {
    panic("bad fd = %d", fd);
  }
}

static inline int fd_alloc(void) {
  for (int fd = FD_RESERVED; fd < NR_FD; fd++) {
    if (!fd_table[fd].used) return fd;
  }
  return -1;
}

size_t invalid_read(void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

size_t invalid_write(const void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}



/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  [FD_STDIN]  = {"stdin", 0, 0, invalid_read, invalid_write},
  [FD_STDOUT] = {"stdout", 0, 0, invalid_read, serial_write},
  [FD_STDERR] = {"stderr", 0, 0, invalid_read, serial_write},
  [FD_EVENT]  = {"/dev/events", 0, 0, events_read, invalid_write},
  [FD_FB]     = {"/dev/fb", 0, 0, invalid_read, fb_write},
  [FD_CANVAS] = {"unimplemented", 0, 0, invalid_read, fb_write},
  {"/proc/dispinfo", 0, 0, dispinfo_read, invalid_write},
  {"/dev/sbctl", 0, 0, sbctl_read, sbctl_write},
  {"/dev/sb",0 ,0, invalid_read, sb_write},
                
#include "files.h"
};

static inline Finfo *fd_finfo(int fd) {
  return &file_table[fd_table[fd].file_idx];
}

char *fs_getname(int fd) {
  fd_check(fd);
  return fd_finfo(fd)->name;
}

void init_fs() {
  AM_GPU_CONFIG_T display = io_read(AM_GPU_CONFIG);
  assert(display.present);
  int w = display.width;
  int h = display.height;
  file_table[FD_FB].size = w * h * sizeof(uint32_t);

  // init fd table: 保留 0..FD_CANVAS 为固定 fd
  for (int i = 0; i < NR_FD; i++) fd_table[i] = (FDinfo){0};
  for (int fd = 0; fd < FD_RESERVED; fd++) {
    fd_table[fd].used = 1;
    fd_table[fd].file_idx = fd;
    fd_table[fd].offset = 0;
  }
}

int fs_open(const char *pathname, int flags, int mode) {
  if (!pathname) panic("pathname is NULL");

  int file_idx = -1;
  for (int i = 0; i < (int)(sizeof(file_table) / sizeof(file_table[0])); i++) {
    if (strcmp(file_table[i].name, pathname) == 0) { file_idx = i; break; }
  }
  if (file_idx < 0) panic("file %s not found", pathname);

  if (file_table[file_idx].read == NULL)  file_table[file_idx].read  = ramdisk_read;
  if (file_table[file_idx].write == NULL) file_table[file_idx].write = ramdisk_write;

  // 对保留设备：返回固定 fd（避免破坏现有假设）
  if (file_idx < FD_RESERVED) {
    fd_table[file_idx].used = 1;
    fd_table[file_idx].file_idx = file_idx;
    fd_table[file_idx].offset = 0;
    return file_idx;
  }

  int fd = fd_alloc();
  if (fd < 0) panic("too many open files");

  fd_table[fd].used = 1;
  fd_table[fd].file_idx = file_idx;
  fd_table[fd].offset = 0;
  return fd;
}

ssize_t fs_read(int fd, void *buf, size_t len) {
  fd_check(fd);
  Finfo *f = fd_finfo(fd);

  if (f->size != 0) {
    if (fd_table[fd].offset < 0) return 0;
    if ((size_t)fd_table[fd].offset >= f->size) return 0;

    size_t avail = f->size - (size_t)fd_table[fd].offset;
    if (len > avail) len = avail;
  }

  size_t r = f->read(buf, f->disk_offset + (size_t)fd_table[fd].offset, len);
  fd_table[fd].offset += (off_t)r;
  return (ssize_t)r;
}

ssize_t fs_write(int fd, const void *buf, size_t len) {
  fd_check(fd);
  Finfo *f = fd_finfo(fd);

  if (f->size != 0) {
    if (fd_table[fd].offset < 0) return 0;
    if ((size_t)fd_table[fd].offset >= f->size) return 0;

    size_t avail = f->size - (size_t)fd_table[fd].offset;
    if (len > avail) len = avail;
  }

  size_t w = f->write(buf, f->disk_offset + (size_t)fd_table[fd].offset, len);
  fd_table[fd].offset += (off_t)w;
  return (ssize_t)w;
}

off_t fs_lseek(int fd, off_t offset, int whence) {
  fd_check(fd);
  Finfo *f = fd_finfo(fd);

  off_t base = 0;
  switch (whence) {
    case SEEK_SET: base = 0; break;
    case SEEK_CUR: base = fd_table[fd].offset; break;
    case SEEK_END: base = (off_t)f->size; break;
    default: panic("whence = %d is invalid", whence);
  }

  off_t new_off = base + offset;
  if (new_off < 0) new_off = 0;
  fd_table[fd].offset = new_off;
  return new_off;
}

int fs_close(int fd) {
  fd_check(fd);

  // 保留 fd 不真正释放，只重置 offset
  if (fd < FD_RESERVED) {
    fd_table[fd].offset = 0;
    return 0;
  }

  fd_table[fd].used = 0;
  fd_table[fd].file_idx = -1;
  fd_table[fd].offset = 0;
  return 0;
}