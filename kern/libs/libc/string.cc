
#include "lib/libc/string.h"
#include "arch/amd64/x86.h"

#include "sys/memlayout.h"
#include "sys/mmu.h"

#include "drivers/debug/kdebug.h"

#include "lib/libc/stdio.h"



extern "C" void *memset(void *s, int c, size_t n)
{
    printf("old memset.\n");
    
    if (reinterpret_cast<size_t>(s) % 4 == 0 && n % 4 == 0)
    {
        c &= 0xFF;
        stosl(s, (c << 24) | (c << 16) | (c << 8) | c, n / 4);
    }
    else
    {
        stosb(s, c, n);
    }

    return s;
}

extern "C" int memcmp(const void *v1, const void *v2, size_t n)
{
    const uint8_t *s1, *s2;

    s1 = reinterpret_cast<decltype(s1)>(v1);
    s2 = reinterpret_cast<decltype(s2)>(v2);
    while (n-- > 0)
    {
        if (*s1 != *s2)
            return *s1 - *s2;
        s1++, s2++;
    }

    return 0;
}

extern "C" void *memmove(void *dst, const void *src, size_t n)
{
    const char *s;
    char *d;

    s = reinterpret_cast<decltype(s)>(src);
    d = reinterpret_cast<decltype(d)>(dst);

    if (s < d && s + n > d)
    {
        s += n;
        d += n;
        while (n-- > 0)
            *--d = *--s;
    }
    else
        while (n-- > 0)
            *d++ = *s++;

    return dst;
}

extern "C" int strlen(const char *s)
{
    int n;

    for (n = 0; s[n]; n++)
        ;
    return n;
}

extern "C" int strncmp(const char *p, const char *q, size_t n)
{
    while (n > 0 && *p && *p == *q)
        n--, p++, q++;
    if (n == 0)
        return 0;
    return (uint8_t)*p - (uint8_t)*q;
}

extern "C" char *strncpy(char *s, const char *t, size_t _n)
{
    char *os;
    int64_t n = _n; //signed, for below code may get into infinite loop because overflowing to INTMAX

    os = s;
    while (n-- > 0 && (*s++ = *t++) != 0)
        ;
    while (n-- > 0)
        *s++ = 0;
    return os;
}

// These functions are only for the requirement of compiler
// Never be called explicitly
extern "C" void *memcpy(void *dst, const void *src, size_t n)
{
    return memmove(dst, src, n);
}

extern "C" int strcmp(const char *s1, const char *s2)
{
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;

    while (*p1 && *p1 == *p2)
        ++p1, ++p2;

    return (*p1 > *p2) - (*p2 > *p1);
}
