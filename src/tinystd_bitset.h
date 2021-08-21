//
// Created by jayjay on 20/8/21.
//

#ifndef TINYSTD_BITSET_H
#define TINYSTD_BITSET_H

#include "tinystd_config.h"

namespace tinystd {

template<size_t N>
struct bitset {
    uint64_t words[1 + ((N - 1) / 64)]{};

    [[nodiscard]] constexpr bool test  (size_t i)          const noexcept { return (words[i / N] & (1ull << (i % N))) > 0; }
                  constexpr void set   (size_t i, bool v = true) noexcept { words[i / N] |= (uint64_t(v) << (i % N)); }
                  constexpr void reset (size_t i)                noexcept { words[i / N] &= ~(1ull << (i % N)); }
};

}

#endif //TINYSTD_BITSET_H
