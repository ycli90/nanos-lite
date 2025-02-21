#include <fs.h>
#include <ramdisk.h>
#include <device.h>

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

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_EVENTS, FD_DISPINFO, FD_SBCFG, FD_SBCTL, FD_SB,
  FD_RANDOM_ACCESS_BEGIN, FD_FB = FD_RANDOM_ACCESS_BEGIN, FD_INIT_END = FD_FB};
size_t screen_width, screen_height;

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
  [FD_EVENTS] = {"/dev/events", 0, 0, events_read, invalid_write},
  [FD_DISPINFO] = {"/proc/dispinfo", 0, 0, dispinfo_read, invalid_write},
  [FD_SBCFG] = {"/dev/sbcfg", 0, 0, sbcfg_read, invalid_write},
  [FD_SBCTL] = {"/dev/sbctl", 0, 0, sbctl_read, sbctl_write},
  [FD_SB] = {"/dev/sb", 0, 0, invalid_read, sb_write},
  [FD_FB] = {"/dev/fb", 0, 0, invalid_read, fb_write},
#include "files.h"
};

int fs_open(const char *pathname, int flags, int mode) {
  assert(pathname);
  for (int i = 0; i < LENGTH(file_table); i++) {
    if (strcmp(pathname, file_table[i].name) == 0) {
      file_table[i].open_offset = 0;
      return i;
    }
  }
  printf("Error: file %s does not exist\n", pathname);
  return -1;
}

size_t fs_read(int fd, void *buf, size_t len) {
  assert(fd >= 0 && fd < LENGTH(file_table));
  if (fd >= FD_RANDOM_ACCESS_BEGIN) {
    assert(file_table[fd].open_offset <= file_table[fd].size); // for develop
    size_t remain_len = file_table[fd].size - file_table[fd].open_offset;
    if (len > remain_len) len = remain_len;
  }
  size_t read_len = file_table[fd].read(buf, file_table[fd].disk_offset + file_table[fd].open_offset, len);
  if (fd >= FD_RANDOM_ACCESS_BEGIN) {
    file_table[fd].open_offset += read_len;
  }
  return read_len;
}

size_t fs_write(int fd, const void *buf, size_t len) {
  assert(fd >= 0 && fd < LENGTH(file_table));
  if (fd >= FD_RANDOM_ACCESS_BEGIN) {
    assert(file_table[fd].open_offset <= file_table[fd].size); // for develop
    size_t remain_len = file_table[fd].size - file_table[fd].open_offset;
    if (len > remain_len) len = remain_len;
  }
  size_t write_len = file_table[fd].write(buf, file_table[fd].disk_offset + file_table[fd].open_offset, len);
  if (fd >= FD_RANDOM_ACCESS_BEGIN) {
    file_table[fd].open_offset += write_len;
  }
  return write_len;
}

size_t fs_lseek(int fd, size_t offset, int whence) {
  assert(fd >= FD_RANDOM_ACCESS_BEGIN && fd < LENGTH(file_table));
  size_t base;
  switch (whence) {
  case SEEK_SET: base = 0; break;
  case SEEK_CUR: base = file_table[fd].open_offset; break;
  case SEEK_END: base = file_table[fd].size; break;
  default: panic("lseek: invalid whence %d", whence);
  }
  size_t new_offset = base + offset;
  assert(new_offset <= file_table[fd].size); // for develop
  file_table[fd].open_offset = new_offset;
  return new_offset;
}

int fs_close(int fd) {
  return 0;
}

void init_fs() {
  // initialize the size of /dev/fb
  screen_width = io_read(AM_GPU_CONFIG).width;
  screen_height = io_read(AM_GPU_CONFIG).height;
  file_table[FD_FB].size = screen_width * screen_height * 4;
  for (int i = FD_INIT_END + 1; i < LENGTH(file_table); i++) {
    file_table[i].read = ramdisk_read;
    file_table[i].write = ramdisk_write;
  }
}
