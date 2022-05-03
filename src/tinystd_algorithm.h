//
// Created by Djordje on 8/21/2021.
//

#ifndef TINYSTD_ALGORITHM_H
#define TINYSTD_ALGORITHM_H

#ifndef TINYVK_SORT_STACK_SIZE
#define TINYVK_SORT_STACK_SIZE          512
#endif

#ifdef TINYVK_SORT_ALLOCATE_MEMORY

#include "tinystd_memory.h"

#endif

namespace tinystd {

template<typename T>
constexpr void swap(T& l, T& r)
{
    T temp{move(l)};
    l = move(r);
    r = move(temp);
}


template<typename T>
constexpr const T& min(const T& l, const T& r)
{
    return l < r ? l : r;
}


template<typename T>
constexpr const T& max(const T& l, const T& r)
{
    return l < r ? r : l;
}

template<typename T>
constexpr T round_up(T value, T multiple)
{
    return ((value + multiple - T(1)) / multiple) * multiple;
}


template<typename It, typename T>
constexpr It find(It begin, It end, const T& v)
{
    for (auto it = begin; it != end; ++it)
        if (*it == v)
            return it;
    return end;
}


template<typename It, typename F>
constexpr It find_if(It begin, It end, F&& cond)
{
    for (auto it = begin; it != end; ++it)
        if (cond(*it))
            return it;
    return end;
}


template<typename It, typename F>
constexpr bool any_of(It begin, It end, F&& cond)
{
    for (auto it = begin; it != end; ++it)
        if (cond(*it))
            return true;
    return false;
}


template<typename It, typename F>
constexpr bool all_of(It begin, It end, F&& cond)
{
    for (auto it = begin; it != end; ++it)
        if (!cond(*it))
            return false;
    return true;
}


template<typename It, typename Compare>
constexpr It find_element(It begin, It end, Compare&& compare) noexcept
{
    if ((end - begin) == 1) return begin;
    It v = begin, result = end;
    for (auto it = begin; it != end; ++it)
        if (compare(*it, *v))
            v = result = it;
    return result;
}


template<typename It>
constexpr It min_element(It begin, It end) noexcept
{
    return find_element(begin, end, [](const auto& v, const auto& a){ return v < a; });
}


template<typename It>
constexpr It max_element(It begin, It end) noexcept
{
    return find_element(begin, end, [](const auto& v, const auto& a){ return v > a; });
}


template<typename C, typename V>
constexpr bool contains(const C& container, const V& value)
{
    return find(container.begin(), container.end(), value) != container.end();
}


namespace sort_impl {

template<typename T, typename Compare>
void do_sort(T* begin, T* end, Compare&& compare);

}

template<typename T>
void sort(T* begin, T* end)
{
    if (begin != end)
        sort_impl::do_sort(begin, end, [](const T& l, const T& r){ return l <= r; });
}

template<typename T, typename Compare>
void sort(T* begin, T* end, Compare&& compare)
{
    if (begin != end)
        sort_impl::do_sort(begin, end, forward<Compare>(compare));
}



/// https://www.geeksforgeeks.org/iterative-quick-sort/
namespace sort_impl {

template<typename T, typename Compare>
int partition(T* arr, int l, int h, Compare&& compare)
{
    T x = arr[h];
    int i = (l - 1);

    for (int j = l; j <= h - 1; j++) {
        if (compare(arr[j], x)) {
            i++;
            swap(arr[i], arr[j]);
        }
    }
    swap(arr[i + 1], arr[h]);
    return (i + 1);
}

template<typename T, typename Compare>
void do_sort(T* begin, T* end, Compare&& compare)
{
    int l = 0;
    int h = int(end - begin) - 1;

    // Create an auxiliary stack
    int stack_on_stack[TINYVK_SORT_STACK_SIZE]{};
    int* stack{stack_on_stack};

#ifdef TINYVK_SORT_ALLOCATE_MEMORY
    if ((h - l + 1) > TINYVK_SORT_STACK_SIZE) {
        size_t size = (h - l + 1) * sizeof(int);
        stack = (int*)TINYSTD_MALLOC(size);
        for (size_t i = 0; i < size; ++i) stack[i] = 0;
    }
#else
    tassert((h - l + 1) < TINYVK_SORT_STACK_SIZE &&
        "Array size too large for sort - define TINYVK_SORT_ALLOCATE_MEMORY to dynamically allocate a large enough stack");
#endif

    // initialize top of stack
    int top = -1;

    // push initial values of l and h to stack
    stack[++top] = l;
    stack[++top] = h;

    // Keep popping from stack while is not empty
    while (top >= 0) {
        // Pop h and l
        h = stack[top--];
        l = stack[top--];

        // Set pivot element at its correct position
        // in sorted array
        int p = partition(begin, l, h, forward<Compare>(compare));

        // If there are elements on left side of pivot,
        // then push left side to stack
        if (p - 1 > l) {
            stack[++top] = l;
            stack[++top] = p - 1;
        }

        // If there are elements on right side of pivot,
        // then push right side to stack
        if (p + 1 < h) {
            stack[++top] = p + 1;
            stack[++top] = h;
        }
    }

    if (stack != stack_on_stack)
        free(stack);
}

}


inline void hash_combine(size_t& seed, size_t h) noexcept
{
    seed ^= h + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

inline u32 hash_crc32(span<const u8> data)
{
   int i{}, j{};
   u32 byte{}, crc{}, mask{};
   static u32 table[256]{};

   /* Set up the table, if necessary. */

   if (table[1] == 0) {
      for (byte = 0; byte <= 255; byte++) {
         crc = byte;
         for (j = 7; j >= 0; j--) {    // Do eight times.
            mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
         }
         table[byte] = crc;
      }
   }

   /* Through with table setup, now calculate the CRC. */

//   i = 0;
   crc = 0xFFFFFFFF;
   for (const auto b: data) {
//   while ((byte = data[i]) != 0) {
       crc = (crc >> 8) ^ table[(crc ^ b) & 0xFF];
      crc = (crc >> 8) ^ table[(crc ^ b) & 0xFF];
//      i = i + 1;
   }
   return ~crc;
}

template<size_t N = 16>
struct monotonic_increasing_string {
    size_t  index{};
    char    string[N]{'a'};

    constexpr span<const char> inc() noexcept
    {
        if (++string[index] > 'z') {
            --string[index];
            u32 to_flip = ++index;
            string[index] = 'a';
            while (to_flip) {
                string[to_flip - 1] = 'a';
                if (string[--to_flip] != 'z')
                    break;
            }
        }
        return {string, index + 1};
    }
};

}

// integer-vector dependencies
namespace tinystd {
namespace dependencies {

template<typename T, size_t NOuter, size_t NInner>
using vec_vec = stack_vector<stack_vector<T, NInner>, NOuter>;


template<typename T, size_t Nodes, size_t Deps>
constexpr bool calculate(const vec_vec<T, Nodes, Deps>& graph, vec_vec<T, Nodes, Nodes>& stages)
{
    const size_t n_nodes = graph.size();

    // nodes without dependencies must have a count of 1 to start
    u64 counts[Nodes]{};
    u64 new_counts[Nodes]{};
    for (size_t i = 0; i < n_nodes; ++i)
        if (graph[i].empty())
            counts[i] = 1;

    // repeat for n_nodes iterations
    //      for each node in the graph
    //          sum counts of dependant nodes into new_counts
    for (size_t i = 0; i < n_nodes; ++i) {
        memcpy(new_counts, counts, sizeof(new_counts));
        for (size_t v = 0; v < n_nodes; ++v)
            for (auto d: graph[v])
                new_counts[v] += (counts[d] << 1);
        memcpy(counts, new_counts, sizeof(new_counts));
    }

    // create stages
    size_t done_count{};
    bitset<Nodes> done{};
    stack_vector<T, Nodes> stage{};
    while (done_count < n_nodes) {
        // get minimum count that is not done
        size_t min_i{-1ul};
        u64 min_count{-1ul};
        for (size_t i = 0; i < n_nodes; ++i) {
            if (done.test(i)) continue;
            if (counts[i] < min_count) {
                min_i = i;
                min_count = counts[i];
            }
        }
        tassert(min_i != -1ul);

        // get all nodes that share the count
        for (size_t i = 0; i < n_nodes; ++i)
            if (counts[i] == counts[min_i])
                stage.push_back(T(i));

        // add stage with nodes and update done info
        stages.push_back(stage);
        done_count += stage.size();
        for (auto s: stage) done.set(s, true);
        stage.clear();
    }

    return true;
}

}
}

#endif //TINYSTD_ALGORITHM_H
