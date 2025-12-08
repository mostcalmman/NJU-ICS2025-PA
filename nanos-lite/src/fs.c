#include <fs.h>

size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);

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

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB};

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
  [FD_STDOUT] = {"stdout", 0, 0, invalid_read, invalid_write},
  [FD_STDERR] = {"stderr", 0, 0, invalid_read, invalid_write},
#include "files.h"
};

void init_fs() {
  // TODO: initialize the size of /dev/fb
}

int fs_open(const char *pathname, int flags, int mode) { 
  if (!pathname) {
    panic("pathname is NULL");
  }
  for(int i = 0; i < sizeof(file_table)/sizeof(file_table[0]); i++) {
    if(strcmp(file_table[i].name, pathname) == 0) {
      return i;
    }
  }
  panic("file %s not found", pathname);
}

size_t fs_read(int fd, void *buf, size_t len) {
  if ( fd == 0 || fd==1 || fd == 2 ) {
    panic("should not read from stdin/stdout/stderr");
  }
  if (len + file_table[fd].open_offset > file_table[fd].size) {
    len = file_table[fd].size - file_table[fd].open_offset;
  }
  size_t ret = ramdisk_read(buf, file_table[fd].disk_offset + file_table[fd].open_offset, len);
  file_table[fd].open_offset += ret;
  return ret;
}

size_t fs_write(int fd, const void *buf, size_t len) {
  if (!fd){
    panic("should not write to stdin");
  }
  if(fd == 1 || fd == 2){
    for(int i = 0; i < len; i++) {
      putch(((char *)buf)[i]);
    }
    return len;
  }
  if (len + file_table[fd].open_offset > file_table[fd].size) {
    len = file_table[fd].size - file_table[fd].open_offset;
  }
  size_t ret = ramdisk_write(buf, file_table[fd].disk_offset + file_table[fd].open_offset, len);
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