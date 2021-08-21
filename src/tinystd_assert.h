//
// Created by jayjay on 20/8/21.
//

#ifndef TINYSTD_ASSERT_H
#define TINYSTD_ASSERT_H

namespace tinystd {

#define TINYSTD_DEBUG

#ifdef TINYSTD_DEBUG
#  define tassert(expr)							\
     (static_cast <bool> (expr)						\
      ? void (0)							\
      : tinystd::impl::assert_fail (#expr, __FILE__, __FUNCTION__, __LINE__))
#else
#  define tassert(expr)
#endif

namespace impl {
void assert_fail(const char* expr, const char* file, const char* function, int line);
}

}

#endif //TINYSTD_ASSERT_H
