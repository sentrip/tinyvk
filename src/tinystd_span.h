//
// Created by jayjay on 20/8/21.
//

#ifndef TINYSTD_SPAN_H
#define TINYSTD_SPAN_H

#include "tinystd_config.h"
#include "tinystd_traits.h"

namespace tinystd {

template<typename T>
struct span {
    T*      m_data{};
    size_t  m_size{};

    constexpr span() = default;
    constexpr span(T* d, size_t s) noexcept : m_data{d}, m_size{s} {}
    constexpr span(T* b, T* e) noexcept : m_data{b}, m_size{e-b} {}

    template<size_t N>
    constexpr span(T(&v)[N])        noexcept : m_data{&v[0]}, m_size{N} {}
    template<typename U, typename = void_t<decltype(declval<U&>().data() + declval<U&>().size())>>
    constexpr span(U&& v)           noexcept : m_data{v.data()}, m_size{v.size()} {}

    constexpr bool        empty()               const noexcept { return m_size == 0; }
    constexpr size_t      size()                const noexcept { return m_size; }
    template<typename = void>
    constexpr T*          data()                      noexcept { return m_data; }
    template<typename = enable_if_t<!is_const_v<T>, void>>
    constexpr const T*    data()                const noexcept { return m_data; }
    template<typename = void>
    constexpr T*          begin()                     noexcept { return m_data; }
    template<typename = void>
    constexpr T*          end()                       noexcept { return m_data + m_size; }
    template<typename = enable_if_t<!is_const_v<T>, void>>
    constexpr const T*    begin()               const noexcept { return m_data; }
    template<typename = enable_if_t<!is_const_v<T>, void>>
    constexpr const T*    end()                 const noexcept { return m_data + m_size; }
    template<typename = void>
    constexpr T&          operator[](size_t i)        noexcept { return m_data[i]; }
    template<typename = enable_if_t<!is_const_v<T>, void>>
    constexpr const T&    operator[](size_t i)  const noexcept { return m_data[i]; }
};

}

#endif //TINYSTD_SPAN_H
