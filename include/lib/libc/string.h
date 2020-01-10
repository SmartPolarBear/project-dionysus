#if !defined(__INCLUDE_LIB_LIBC_STRING_H)
#define __INCLUDE_LIB_LIBC_STRING_H

#include <sys/types.h>

template <typename T>
static inline auto memset2(T *s, int c, size_t n) -> T *
{
    // a nullptr should not be memset
    unsigned char *mem = reinterpret_cast<unsigned char *>(s);

    for (size_t i = 0; i < n; i++)
    {
        mem[i] = (T) static_cast<unsigned char>(c);
    }
    return s;
}

extern "C" void *memset(void *, int, size_t);
extern "C" int memcmp(const void *, const void *, size_t);
extern "C" void *memmove(void *, const void *, size_t);
extern "C" int strlen(const char *);
extern "C" int strncmp(const char *, const char *, size_t);
extern "C" char *strncpy(char *, const char *, size_t);

#endif // __INCLUDE_LIB_LIBC_STRING_H
