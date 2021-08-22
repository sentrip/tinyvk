//
// Created by Djordje on 8/21/2021.
//

#ifndef TINYSTD_STRING_H
#define TINYSTD_STRING_H

// TODO: tinystd_string.h - does tinyvk really need dynamic strings?

#include "tinystd_assert.h"
#include "tinystd_stdlib.h"

namespace tinystd {

template<size_t N>  struct string_base      { char  m_data[N+1]{}; protected: void set_data(char*) {} };
template<>          struct string_base<0>   { char* m_data{};      protected: void set_data(char* s) { m_data = s; } };

template<size_t N>
struct sized_string : string_base<N> {
    using string_base<N>::  m_data;
    size_t                  m_size{};
    size_t                  m_capacity{};

    sized_string(): m_size{}, m_capacity{N} {}
    ~sized_string()                         { if (N == 0 && m_data) free(m_data); }

    ibool           empty()         const noexcept { return m_size == 0; }
    size_t          size()          const noexcept { return m_size; }
    char*           data()                noexcept { return m_data; }
    const char*     data()          const noexcept { return m_data; }
    char*           begin()               noexcept { return m_data; }
    const char*     begin()         const noexcept { return m_data; }
    char*           end()                 noexcept { return m_data + m_size; }
    const char*     end()           const noexcept { return m_data + m_size; }

    void            append(const char* s, size_t n)
    {
        if (N == 0) ensure_capacity(m_capacity + n, true);
        else        tassert(m_size + n <= m_capacity && "String too long");
        if (s != end()) memcpy(end(), s, n);
        m_size += n;
        *end() = 0;
    }

    sized_string(const sized_string& o) : sized_string() { *this = o; }

    sized_string(sized_string&& o) noexcept : sized_string() { *this = move(o); }

    sized_string& operator=(const sized_string& o)
    {
        m_size = o.m_size;
        if (N == 0) {
            ensure_capacity(o.m_capacity, true);
        }
        else {
            memcpy(m_data, o.m_data, m_size + 1);
        }
        return *this;
    }

    sized_string& operator=(sized_string&& o) noexcept
    {
        if (N == 0) {
            this->set_data(o.m_data);
            m_size = o.m_size;
            m_capacity = o.m_capacity;
            o.m_capacity = 0;
            o.set_data(nullptr);
        }
        else {
            *this = (const sized_string&)o;
        }
        o.m_size = 0;
        return *this;
    }

private:
    char*           ensure_capacity(size_t n, ibool copy)
    {
        if (N > 0) {
            tassert(n <= N && "String too long");
            return m_data;
        }

        if (n > m_capacity) {
            auto* old = m_data;
            if (old) free(m_data);
            this->set_data((char*)malloc(n));
            if (old && copy && m_size) memcpy(m_data, old, m_size + 1);
        }

        m_capacity = n;
    }
};

template<size_t N>
using small_string  = sized_string<N>;
using string        = sized_string<0>;

}

#endif //TINYSTD_STRING_H
