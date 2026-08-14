/* Minimal newlib syscall stubs so the nano-newlib C library links cleanly
 * with no OS underneath. We never use file I/O or exit() on-target; these
 * exist purely to satisfy the linker for libc internals (e.g. vsnprintf's
 * reentrancy structures). Standard boilerplate, not EdgeFlex-specific. */
#include <sys/stat.h>
#include <errno.h>
#include <stdint.h>

int _close(int file) { (void)file; return -1; }
int _fstat(int file, struct stat *st) { (void)file; st->st_mode = S_IFCHR; return 0; }
int _isatty(int file) { (void)file; return 1; }
int _lseek(int file, int ptr, int dir) { (void)file; (void)ptr; (void)dir; return 0; }
int _read(int file, char *ptr, int len) { (void)file; (void)ptr; (void)len; return 0; }
int _write(int file, char *ptr, int len) { (void)file; (void)ptr; return len; }
void _exit(int status) { (void)status; while (1) { } }
int _kill(int pid, int sig) { (void)pid; (void)sig; errno = EINVAL; return -1; }
int _getpid(void) { return 1; }
