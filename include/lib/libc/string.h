#if !defined(__INCLUDE_LIB_LIBC_STRING_H)
#define __INCLUDE_LIB_LIBC_STRING_H

#include <sys/types.h>

extern "C"
{
    void *memset(void *, size_t, size_t);
    int memcmp(const void *, const void *, size_t);
    void *memmove(void *, const void *, size_t);
    int strlen(const char *);
    int strncmp(const char *, const char *, size_t);
    char *strncpy(char *, const char *, size_t);
}

#endif // __INCLUDE_LIB_LIBC_STRING_H
