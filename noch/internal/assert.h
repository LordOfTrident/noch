#ifndef NOCH_INTERNAL_ASSERT_H_HEADER_GUARD
#define NOCH_INTERNAL_ASSERT_H_HEADER_GUARD

#ifndef NOCH_ASSERT
#	include <assert.h> /* assert */

#	define nochAssert(EXPR) assert(EXPR)
#endif

#endif
