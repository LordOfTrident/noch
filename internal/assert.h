#ifndef NOCH_INTERNAL_ASSERT_H_HEADER_GUARD
#define NOCH_INTERNAL_ASSERT_H_HEADER_GUARD

#ifndef NOCH_ASSERT
#	include <assert.h> /* assert */

#	define NOCH_ASSERT(EXPR) assert(EXPR)
#endif

#endif
