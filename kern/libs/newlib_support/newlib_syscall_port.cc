#include "system/types.h"
#include "system/kmalloc.hpp"

#include "debug/kdebug.h"
#include "drivers/cmos/rtc.hpp"
#include "drivers/apic/timer.h"

// Finished:
//void _exit();
//int kill_locked(int pid, int sig);
//int getpid();
//caddr_t sbrk(int incr);
//char **environ; /* pointer to array of char * strings that define the current environment variables */
//int gettimeofday(struct timeval *p, struct timezone *z);
//clock_t times(struct tms *buf);
//int fork();
//int wait(int *status);

// To be finished
//int close(int file);
//int execve(char *name_, char **argv, char **env);
//int fstat(int file, struct stat *st);
//int isatty(int file);
//int link(char *old, char *new);
//int lseek(int file, int ptr, int dir);
//int open(const char *name_, int flags_, ...);
//int read(int file, char *ptr, int len);
//int stat(const char *file, struct stat *st);
//int unlink(char *name_);
//int write(int file, char *ptr, int len);


// leave it empty temporarily
extern "C" char* environ[] =
	{
		(char*)"",
		(char*)"",
		(char*)"",
		(char*)"",
		(char*)"",
	};



extern "C" [[maybe_unused]] int kill([[maybe_unused]]int pid, int sig)
{
	KDEBUG_RICHPANIC("Some of libraries called kill_locked for kernel.", "KILL CALLED", true, "Signal %d", sig);
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

struct timeval
{
	long tv_sec;     /* seconds */
	long tv_usec;    /* microseconds */
};

struct timezone
{
	int tz_minuteswest;     /* minutes west of Greenwich */
	int tz_dsttime;         /* type of DST correction */
};

extern "C" [[maybe_unused]] int gettimeofday(timeval* p, [[deprecated, maybe_unused]]timezone* z)
{
	p->tv_sec = cmos::cmos_read_rtc_timestamp();
	p->tv_usec = p->tv_sec * 1000;
	return 0;
}

using clock_t = uint64_t;

struct tms
{
	clock_t tms_utime;  /* user time */
	clock_t tms_stime;  /* system time */
	clock_t tms_cutime; /* user time of children */
	clock_t tms_cstime; /* system time of children */
};

extern "C" [[maybe_unused]] clock_t times(tms* buf)
{
	buf->tms_cstime = buf->tms_cutime = buf->tms_stime = buf->tms_utime = 0;
	return timer::get_ticks();
}

// TODO: after the implementation of kernel threads, they may have things to do.
extern "C" [[maybe_unused]] int wait([[maybe_unused]]int* status)
{
	return -1;
}

extern "C" [[maybe_unused]]  int fork()
{
	return -1;
}
