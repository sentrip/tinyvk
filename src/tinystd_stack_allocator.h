//
// Created by jayjay on 25/4/22.
//

#ifndef TINYSTD_STACK_ALLOCATOR_H
#define TINYSTD_STACK_ALLOCATOR_H

#include "tinystd_traits.h"

namespace tinystd {

template<size_t N>
struct stack_allocator {
    size_t  m_offset{};
    u8      m_buffer[N]{};

    constexpr u8* allocate(size_t align, size_t size) NEX {
        return align_advance(align, size);
    }

    template<typename T, typename... Args>
    constexpr T* construct(size_t n, Args&&... args) NEX {
        auto* ptr = allocate(alignof(T), n * sizeof(T));
        for (size_t i = 1; i < n; ++i) new (ptr + (i * sizeof(T))) T{forward<Args>(args)...};
        return new (ptr) T{forward<Args>(args)...};
    }

private:
    constexpr u8* align_advance(size_t align, size_t size) NEX {
        const size_t aligned_offset = (m_offset + align - 1) & ~(align - 1);
        if (aligned_offset + size > N) return nullptr;
        m_offset = aligned_offset + size;
        return m_buffer + aligned_offset;
    }
};

}

#endif //TINYSTD_STACK_ALLOCATOR_H
