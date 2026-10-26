/* Bounded heap for newlib's float formatting. No peripherals are used by libc. */
#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
extern char _heap_start[],_heap_end[];
void *_sbrk(ptrdiff_t increment) {
 static char *current;
 char *previous;
 if (!current) current=_heap_start;
 if ((increment>=0 && (size_t)increment>(size_t)(_heap_end-current)) ||
     (increment<0 && increment < -(ptrdiff_t)(current-_heap_start))) {
  errno=ENOMEM; return (void *)-1;
 }
 previous=current; current+=increment; return previous;
}
#include <sys/stat.h>
int _close(int fd) { (void)fd; errno=EBADF; return -1; }
int _fstat(int fd,struct stat *st) { (void)fd; (void)st; errno=EBADF; return -1; }
int _isatty(int fd) { (void)fd; return 0; }
off_t _lseek(int fd,off_t pos,int whence) { (void)fd;(void)pos;(void)whence;errno=EBADF;return -1; }
int _read(int fd,void *buf,size_t len) { (void)fd;(void)buf;(void)len;errno=EBADF;return -1; }
int _write(int fd,const void *buf,size_t len) { (void)fd;(void)buf;(void)len;errno=EBADF;return -1; }
int _getpid(void) { return 1; }
int _kill(int pid,int signal) { (void)pid;(void)signal;errno=EINVAL;return -1; }
