#include <fs.h>

size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);
size_t serial_write(const void *buf, size_t offset, size_t len);
size_t events_read(void *buf, size_t offset, size_t len);
size_t dispinfo_read(void *buf, size_t offset, size_t len);
size_t fb_write(const void *buf, size_t offset, size_t len);

typedef size_t (*ReadFn) (void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn) (const void *buf, size_t offset, size_t len);

typedef struct {
  char *name;
  size_t size;
  size_t disk_offset;
  ReadFn read;
  WriteFn write;
  size_t open_offset;
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
      if (file_table[i].read == NULL || file_table[i].write == NULL) {
        file_table[i].read = ramdisk_read;
        file_table[i].write = ramdisk_write;
      }
      file_table[i].open_offset = 0;
      return i;
    }
  }
  Log("file %s not found", pathname);
  return -1;
}

size_t fs_read(int fd, void *buf, size_t len) {
  if (len + file_table[fd].open_offset > file_table[fd].size && file_table[fd].size!=0) {
    len = file_table[fd].size - file_table[fd].open_offset;
  }
  size_t ret = file_table[fd].read(buf, file_table[fd].disk_offset + file_table[fd].open_offset, len);
  file_table[fd].open_offset += ret;
  return ret;
}

size_t fs_write(int fd, const void *buf, size_t len) {
  if (len + file_table[fd].open_offset > file_table[fd].size && file_table[fd].size!=0) {
    len = file_table[fd].size - file_table[fd].open_offset;
  }
  size_t ret = file_table[fd].write(buf, file_table[fd].disk_offset + file_table[fd].open_offset, len);
  file_table[fd].open_offset += ret;
  return ret;
}

size_t fs_lseek(int fd, size_t offset, int whence) {
  switch (whence) {
    case SEEK_SET:
      file_table[fd].open_offset = offset;
      break;
    case SEEK_CUR:
      file_table[fd].open_offset += offset;
      break;
    case SEEK_END:
      file_table[fd].open_offset = file_table[fd].size + offset;
      break;
    default:
      panic("whence = %d is invalid", whence);
  }
  return file_table[fd].open_offset;
}

int fs_close(int fd) {
  return 0;
}