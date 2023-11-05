#ifndef NOCH_ATTRS_H_HEADER_GUARD
#define NOCH_ATTRS_H_HEADER_GUARD

#include "platform.h"

/* The following should be used instead of the PACK macro:
 *
 *     #pragma pack(push, 1)
 *     struct {...};
 *     #pragma pack(pop)
 */

#if __cplusplus - 0 >= 201402L
#	define DEPRECATED(MSG) [[deprecated(MSG)]]
#elif defined(COMPILER_GCC) || defined(COMPILER_CLANG)
#	define DEPRECATED(MSG) __attribute__((deprecated))
#elif defined(COMPILER_MSVC)
#	define DEPRECATED(MSG) __declspec(deprecated(MSG))
#else
#	define DEPRECATED(MSG)
#endif

#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
#	define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#	define INLINE             __attribute__((always_inline)) inline
#	define PACK(STRUCT)       STRUCT __attribute__((__packed__))
#	define EXPORT
#elif defined(COMPILER_MSVC)
#	define WARN_UNUSED_RESULT _Check_return_
#	define INLINE(...)        __forceinline
#	define PACK(STRUCT)       __pragma(pack(push, 1)) STRUCT __pragma(pack(pop))
#	define EXPORT             extern __declspec(dllexport)
#else
#	define WARN_UNUSED_RESULT
#	define INLINE
#	define PACK(STRUCT)
#	define EXPORT
#endif

#endif
