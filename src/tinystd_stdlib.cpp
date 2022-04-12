//
// Created by Djordje on 8/21/2021.
//

#include "tinystd_stdlib.h"

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cstdarg>

namespace tinystd {

void* malloc(size_t size)                               { return ::malloc(size); }
void  free(void* ptr)                                   { return ::free(ptr); }
void  memcpy(void* dst, const void* src, size_t size)   { ::memcpy(dst, src, size); }
void  memset(void* dst, u8 byte, size_t size)           { ::memset(dst, byte, size); }
void  exit(int code)                                    { ::exit(code); }
bool memeq(const void* l, const void* r, size_t size)   { return ::memcmp(l, r, size) == 0; }
bool streq(const char* l, const char* r)                { return ::strcmp(l, r) == 0; }

int print(const char* fmt, ...)
{
    va_list a{};
    va_start(a, fmt);
    int r = vprintf(fmt, a);
    va_end(a);
    return r;
}

int error(const char* fmt, ...)
{
    va_list a;
    va_start(a, fmt);
    int r = vfprintf(stderr, fmt, a);
    va_end(a);
    return r;
}

}
