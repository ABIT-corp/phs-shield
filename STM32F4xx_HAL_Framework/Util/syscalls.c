#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/reent.h>
 
#define ECHOBACK
 
extern unsigned char _end;
unsigned char *heap_end;
register unsigned char *stack_ptr asm ("sp");
 
int _read_r(struct _reent *r, int file, char *ptr, int len)
{
    return -1;
}
 
int _lseek_r(struct _reent *r, int file, int ptr, int dir)
{
  return -1;
}
 
int _write_r(struct _reent *r, int file, const void *ptr, size_t len)
{
    return -1;
}
 
int _close_r(struct _reent *r, int file)
{
  return -1;
}
 
caddr_t _sbrk_r(struct _reent *r, int incr)
{
    unsigned char *prev_heap_end;
 
	/* initialize */
    if( heap_end == 0 ) {
        heap_end = &_end;
    }
    prev_heap_end = heap_end;
  
    heap_end += incr;

    return (caddr_t) prev_heap_end;
}
 
int _fstat_r(struct _reent *r, int file, struct stat *st)
{
  return -1;
}
 
int _open_r(struct _reent *r, const char *path, int flags, int mode)
{
  return -1;
}
 
int _isatty(int fd)
{
  return 1;
}
 
char *__exidx_start;
char *__exidx_end;