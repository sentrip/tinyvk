//
// Created by Djordje on 8/21/2021.
//

#ifndef TINYSTD_MEMORY_H
#define TINYSTD_MEMORY_H

#include "tinystd_config.h"

namespace tinystd {

void* malloc(size_t size);

void free(void* ptr);

void memcpy(void* dst, const void* src, size_t size);

void memset(void* dst, u8 byte, size_t size);

bool memeq(const void* l, const void* r, size_t size);

bool streq(const char* l, const char* r);

void exit(int code);

int print(const char* fmt, ...);

int error(const char* fmt, ...);

}


#endif //TINYSTD_MEMORY_H
