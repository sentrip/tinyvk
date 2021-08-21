//
// Created by jayjay on 20/8/21.
//

#ifndef TINYSTD_TRAITS_H
#define TINYSTD_TRAITS_H

#include "tinystd_config.h"

namespace tinystd {

template<typename...>
struct void_t {  };


template <bool Test, class T = void>
struct enable_if {};

template <class T>
struct enable_if<true, T> { using type = T; };

template <bool Test, class T = void>
using enable_if_t = typename enable_if<Test, T>::type;


template<bool Cond, typename True, typename False>
struct conditional {};

template<typename True, typename False>
struct conditional<true, True, False> { using type = True; };

template<typename True, typename False>
struct conditional<false, True, False> { using type = False; };

template<bool Cond, typename True, typename False>
using conditional_t = typename conditional<Cond, True, False>::type;


template<typename T>
struct remove_reference { using type = T; };

template<typename T>
struct remove_reference<T&> { using type = T; };

template<typename T>
using remove_reference_t = typename remove_reference<T>::type;


template<typename T>
struct is_const { static constexpr bool value = false; };

template<typename T>
struct is_const<const T> { static constexpr bool value = true; };

template<typename T>
static constexpr bool is_const_v = is_const<T>::value;


template <class T>
T&& declval() noexcept;


template<typename T>
NODISCARD constexpr T&& move(T& v) {
    return (T&&)v;
}

template <class T>
NODISCARD constexpr remove_reference_t<T>&& move(T&& a) noexcept {
    return static_cast<remove_reference_t<T>&&>(a);
}


template <class T>
NODISCARD constexpr T&& forward(remove_reference_t<T>& a) noexcept { // forward an lvalue as either an lvalue or an rvalue
    return static_cast<T&&>(a);
}

template <class T>
NODISCARD constexpr T&& forward(remove_reference_t<T>&& a) noexcept { // forward an rvalue as an rvalue
    return static_cast<T&&>(a);
}

}

#endif //TINYSTD_TRAITS_H
