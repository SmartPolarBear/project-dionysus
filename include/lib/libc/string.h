#pragma once

#include <sys/types.h>

extern "C" void *memset(void *, int, size_t);
extern "C" int memcmp(const void *, const void *, size_t);
extern "C" void *memmove(void *, const void *, size_t);
extern "C" int strlen(const char *);
extern "C" int strncmp(const char *, const char *, size_t);
extern "C" char *strncpy(char *, const char *, size_t);