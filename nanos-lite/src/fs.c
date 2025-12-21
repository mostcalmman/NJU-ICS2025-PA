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
  off_t open_offset;
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_EVENT, FD_FB, FD_CANVAS};

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

char *fs_getname(int fd) {
  return file_table[fd].name;
}

void init_fs() {
  AM_GPU_CONFIG_T display = io_read(AM_GPU_CONFIG);
  assert(display.present);
  int w = display.width;
  int h = display.height;
  file_table[FD_FB].size = w * h * sizeof(uint32_t);
}

int fs_open(const char *pathname, int flags, int mode) { 
  if (!pathname) {
    panic("pathname is NULL");
  }
  for(int i = 0; i < sizeof(file_table)/sizeof(file_table[0]); i++) {
    if(strcmp(file_table[i].name, pathname) == 0) {
      if (file_table[i].read == NULL)  file_table[i].read  = ramdisk_read;
      if (file_table[i].write == NULL) file_table[i].write = ramdisk_write;

      file_table[i].open_offset = 0;
      return i;
    }
  }
  Log("file %s not found", pathname);
  // assert(0);
  halt(0);
}

ssize_t fs_read(int fd, void *buf, size_t len) {
  if (file_table[fd].size != 0) {
    if (file_table[fd].open_offset < 0) return 0;
    if (file_table[fd].open_offset >= file_table[fd].size) return 0;

    size_t avail = file_table[fd].size - file_table[fd].open_offset;
    if (len > avail) len = avail;
  }

  ssize_t ret = file_table[fd].read(buf, file_table[fd].disk_offset + file_table[fd].open_offset, len);
  file_table[fd].open_offset += ret;
  return ret;
}

ssize_t fs_write(int fd, const void *buf, size_t len) {
  if (file_table[fd].size != 0) {
    if (file_table[fd].open_offset < 0) return 0;
    if (file_table[fd].open_offset >= file_table[fd].size) return 0;

    size_t avail = file_table[fd].size - file_table[fd].open_offset;
    if (len > avail) len = avail;
  }

  ssize_t ret = file_table[fd].write(buf, file_table[fd].disk_offset + file_table[fd].open_offset, len);
  file_table[fd].open_offset += ret;
  return ret;
}

off_t fs_lseek(int fd, off_t offset, int whence) {

  off_t base = 0;
  switch (whence) {
    case SEEK_SET: base = 0; break;
    case SEEK_CUR: base = file_table[fd].open_offset; break;
    case SEEK_END: base = (off_t)file_table[fd].size; break;
    default: panic("whence = %d is invalid", whence);
  }

  off_t new_off = base + offset;
  if (new_off < 0) new_off = 0;

  file_table[fd].open_offset = new_off;
  return new_off;
}

int fs_close(int fd) {
  return 0;
}