#include <cassert>

#define assert_always(expr) \
    ((expr) ? (void)0 : __assert_func(__FILE__, __LINE__, __ASSERT_FUNC, #expr))
