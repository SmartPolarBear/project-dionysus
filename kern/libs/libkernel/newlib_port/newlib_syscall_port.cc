#include "system/types.h"
#include "system/kmalloc.h"

#include "drivers/debug/kdebug.h"

// Finished:
//void _exit();
//int kill(int pid, int sig);
//int getpid();
//caddr_t sbrk(int incr);
//char **environ; /* pointer to array of char * strings that define the current environment variables */

// To be finished
//int close(int file);
//int execve(char *name, char **argv, char **env);
//int fork();
//int fstat(int file, struct stat *st);
//int isatty(int file);
//int link(char *old, char *new);
//int lseek(int file, int ptr, int dir);
//int open(const char *name, int flags, ...);
//int read(int file, char *ptr, int len);
//int stat(const char *file, struct stat *st);
//clock_t times(struct tms *buf);
//int unlink(char *name);
//int wait(int *status);
//int write(int file, char *ptr, int len);
//int gettimeofday(struct timeval *p, struct timezone *z);

// leave it empty now
extern "C" char* environ[] =
	{
		(char*)"",
		(char*)"",
	};

extern "C" [[maybe_unused]] int kill([[maybe_unused]]int pid, int sig)
{
	KDEBUG_RICHPANIC("Some of libraries called kill for kernel.", "KILL CALLED", true, "Signal %d", sig);
	return 0;
}

extern "C" [[maybe_unused]] void _exit()
{
	KDEBUG_RICHPANIC("Some of libraries called exit for kernel.", "EXIT CALLED", true, "");
}

extern "C" [[maybe_unused]]int getpid()
{
	return INT32_MAX;
}

// caddr_t is the alias of char*
extern "C" [[maybe_unused]] char* sbrk(int inc)
{
	return (char*)memory::kmalloc(inc, 0);
}