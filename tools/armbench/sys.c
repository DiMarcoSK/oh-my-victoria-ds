// sys.c — chamadas de sistema minimas da newlib do devkitARM (versoes _r,
// reentrantes) p/ o jogo rodar "nu" no emulador (tools/armbench). printf vai
// p/ OUTB; o run.py le e imprime no fim.
#include <sys/stat.h>
#include <reent.h>
#include <errno.h>
char OUTB[1<<16];unsigned outN;
static char HEAP[1<<20];static unsigned hp;
struct _reent *__getreent(void){return _impure_ptr;}
void __libc_lock_acquire_recursive(_LOCK_RECURSIVE_T *l){(void)l;}
void __libc_lock_release_recursive(_LOCK_RECURSIVE_T *l){(void)l;}
_ssize_t _write_r(struct _reent *r,int f,const void *p,size_t n){(void)r;(void)f;const char *c=p;for(size_t i=0;i<n;i++)if(outN<sizeof OUTB-1)OUTB[outN++]=c[i];return (_ssize_t)n;}
void *_sbrk_r(struct _reent *r,ptrdiff_t n){if(hp+n>sizeof HEAP){r->_errno=ENOMEM;return (void*)-1;}void *p=HEAP+hp;hp+=n;return p;}
int _close_r(struct _reent *r,int f){(void)r;(void)f;return -1;}
int _fstat_r(struct _reent *r,int f,struct stat *s){(void)r;(void)f;s->st_mode=S_IFCHR;return 0;}
int _isatty_r(struct _reent *r,int f){(void)r;(void)f;return 1;}
_off_t _lseek_r(struct _reent *r,int f,_off_t o,int w){(void)r;(void)f;(void)o;(void)w;return 0;}
_ssize_t _read_r(struct _reent *r,int f,void *p,size_t n){(void)r;(void)f;(void)p;(void)n;return 0;}
int _kill_r(struct _reent *r,int p,int s){(void)r;(void)p;(void)s;return -1;}
int _getpid_r(struct _reent *r){(void)r;return 1;}
void _exit(int c){(void)c;for(;;){}}
