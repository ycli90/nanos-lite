#include <common.h>
#include "syscall.h"
#include <fs.h>
#include <loader.h>

int sys_exit(int status) {
  printf("program exit code: %d\n", status);
  naive_uload(NULL, "/bin/nterm");
  // halt(status);
  return 0;
}

int sys_yield() {
  yield();
  return 0;
}

int sys_open(const char *pathname, int flags, int mode) {
  return fs_open(pathname, flags, mode);
}

size_t sys_read(int fd, void *buf, size_t len) {
  return fs_read(fd, buf, len);
}

size_t sys_write(int fd, const void *buf, size_t len) {
  return fs_write(fd, buf, len);
}

int sys_close(int fd) {
  return fs_close(fd);
}

size_t sys_lseek(int fd, size_t offset, int whence) {
  return fs_lseek(fd, offset, whence);
}

int sys_brk(void *addr) {
  if (addr < heap.end) return 0;
  else return -1;
}

int sys_gettimeofday(uint64_t *time) {
  *time = io_read(AM_TIMER_UPTIME).us;
  return 0;
}

int sys_execve(const char *fname, char * const argv[], char *const envp[]) {
  naive_uload(NULL, fname);
  return -1;
}

int sys_fstat(int fd, void *) {
  return -1;
}

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;
  a[1] = c->GPR2;
  a[2] = c->GPR3;
  a[3] = c->GPR4;
  switch (a[0]) {
    case SYS_exit: sys_exit(a[1]); break;
    case SYS_yield: c->GPRx = sys_yield(); break;
    case SYS_open: c->GPRx = sys_open((const char*)a[1], (int)a[2], (int)a[3]); break;
    case SYS_read: c->GPRx = sys_read((int)a[1], (void*)a[2], (size_t)a[3]); break;
    case SYS_write: c->GPRx = sys_write((int)a[1], (const void*)a[2], (size_t)a[3]); break;
    case SYS_close: c->GPRx = sys_close((int)a[1]); break;
    case SYS_lseek: c->GPRx = sys_lseek((int)a[1], (size_t)a[2], (int)a[3]); break;
    case SYS_brk: c->GPRx = sys_brk((void*)a[1]); break;
    case SYS_gettimeofday: c->GPRx = sys_gettimeofday((uint64_t*)a[1]); break;
    case SYS_execve: c->GPRx = sys_execve((const char *)a[1], (char * const*)a[2], (char * const*)a[3]); break;
    case SYS_fstat: c->GPRx = sys_fstat((int)a[1], (void*)a[2]); break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}
