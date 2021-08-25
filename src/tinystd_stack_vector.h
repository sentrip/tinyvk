//
// Created by jayjay on 20/8/21.
//

#ifndef TINYSTD_STACK_VECTOR_H
#define TINYSTD_STACK_VECTOR_H

#include "tinystd_stdlib.h"

namespace tinystd {

template<typename T, size_t N>
struct stack_vector {
    T*      m_begin{};
    size_t  m_size{};
    size_t  m_capacity{};
    T       m_data[N]{};

    stack_vector() : m_begin{m_data}, m_size{}, m_capacity{N} {}
    ~stack_vector() { if (m_begin != m_data) tinystd::free(m_begin); }

    bool        empty() const               { return m_size == 0; }
    size_t      size() const                { return m_size; }

    T*          data()                      { return m_begin; }
    const T*    data() const                { return m_begin; }

    T*          begin()                     { return m_begin; }
    const T*    begin() const               { return m_begin; }
    T*          end()                       { return m_begin + m_size; }
    const T*    end() const                 { return m_begin + m_size; }

    T&          front()                     { return m_begin[0]; }
    const T&    front() const               { return m_begin[0]; }
    T&          back()                      { return m_begin[m_size-1]; }
    const T&    back() const                { return m_begin[m_size-1]; }

    T&          operator[](size_t i)        { return m_begin[i]; }
    const T&    operator[](size_t i)  const { return m_begin[i]; }

    void        clear()                     { m_size = 0; }

    T           pop_back()                  { return m_begin[m_size--]; }

    void        push_back(const T& v)
    {
        ensure_capacity(1);
        m_begin[m_size++] = v;
    }

    void        insert(const T* it, const T& v)
    {
        const size_t begin = it - m_begin;
        ensure_capacity(1);
        for (size_t i = m_size; i > begin; --i)
            m_data[i] = m_data[i - 1];
        m_data[begin] = v;
        ++m_size;
    }

    void        erase(const T* it)
    {
        for (size_t i = (it - m_begin); i < m_size - 1; ++i)
            m_data[i] = m_data[i + 1];
        --m_size;
    }

    void        resize(size_t n)
    {
        auto* old_begin = m_begin;
        if (m_begin != m_data)
            tinystd::free(m_begin);
        m_begin = n > N ? (T*)tinystd::malloc(n * sizeof(T)) : m_data;
        if (n > 0 && old_begin != m_begin)
            tinystd::memcpy(m_begin, old_begin, n * sizeof(T));
        m_size = n;
    }

    stack_vector(const stack_vector& o) : stack_vector() { *this = o; }
    stack_vector(stack_vector&& o) noexcept : stack_vector() { *this = move(o); }

    stack_vector& operator=(const stack_vector& o)
    {
        if (this == &o) return *this;
        if (m_begin != m_data && o.m_begin == o.m_data) {
            tinystd::free(m_begin);
            m_begin = m_data;
        }
        else if (m_begin == m_data && o.m_begin != o.m_data) {
            m_begin = (T*)tinystd::malloc(o.m_capacity * sizeof(T));
        }
        else if (m_begin != m_data && m_capacity < o.m_capacity) {
            tinystd::free(m_begin);
            m_begin = (T*)tinystd::malloc(o.m_capacity * sizeof(T));
        }
        m_capacity = o.m_capacity;
        m_size = o.m_size;
        for (size_t i = 0; i < o.m_size; ++i)
            m_begin[i] = o.m_begin[i];
        return *this;
    }

    stack_vector& operator=(stack_vector&& o) noexcept
    {
        if (this == &o) return *this;
        if (o.m_begin == o.m_data) {
            *this = (const stack_vector<T, N>&)o;
        }
        else {
            m_begin = o.m_begin;
            m_size = o.m_size;
            m_capacity = o.m_capacity;
            o.m_begin = o.m_data;
            o.m_capacity = N;
            o.m_size = 0;
        }
        return *this;
    }

private:
    void ensure_capacity(size_t n)
    {
        const size_t size = m_size + n;
        if (size <= m_capacity)
            return;
        while (size >= m_capacity)
            m_capacity *= 2;
        auto* new_memory = (T*)tinystd::malloc(m_capacity * sizeof(T));
        tinystd::memcpy(m_begin, new_memory, m_size * sizeof(T));
        if (m_begin != m_data)
            tinystd::free(m_begin);
        m_begin = new_memory;
    }

};

}

#endif //TINYSTD_STACK_VECTOR_H
