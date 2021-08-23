//
// Created by Djordje on 8/21/2021.
//

#ifndef TINYSTD_CONFIG_H
#define TINYSTD_CONFIG_H

#ifdef _MSC_VER
#define TINYVK_PLATFORM_WINDOWS
#define TINYVK_COMPILER_MSVC
#elif defined(__linux__)
#define TINYVK_PLATFORM_LINUX
#else
#error TinyVk does not support this platform yet
#endif

#define NODISCARD [[nodiscard]]

namespace tinystd {

using i8        = signed char;
using i16       = short;
using i32       = int;
using i64       = long long;
using u8        = unsigned char;
using u16       = unsigned short;
using u32       = unsigned int;
using u64       = unsigned long long;
using size_t    = unsigned long long;
using ibool     = u32;

template<typename T1, typename T2 = T1>
struct pair { T1 first; T2 second; };

}

#endif //TINYSTD_CONFIG_H
