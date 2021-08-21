//
// Created by jayjay on 20/8/21.
//

#include "tinystd_assert.h"
#include "tinystd_stdlib.h"

namespace tinystd { namespace impl {
void assert_fail(const char* expr, const char* file, const char* function, int line)
{
    error("ASSERT - %s:%d: %s: Assertion `%s` failed.\n", file, line, function, expr);
    exit(1);
}
}}
