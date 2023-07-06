#ifndef NOCH_INTERNAL_ALLOC_H_HEADER_GUARD
#define NOCH_INTERNAL_ALLOC_H_HEADER_GUARD

#if    defined(NOCH_ALLOC) &&  defined(NOCH_REALLOC) &&  defined(NOCH_FREE)
#elif !defined(NOCH_ALLOC) && !defined(NOCH_REALLOC) && !defined(NOCH_FREE)
#else
#	error "Please, either define all NOCH_ALLOC, NOCH_REALLOC and NOCH_FREE or none"
#endif

#ifndef NOCH_ALLOC
#	include <stdlib.h> /* malloc, realloc, free */

#	define NOCH_ALLOC(SIZE) malloc(SIZE)
#	define NOCH_REALLOC(PTR, SIZE) realloc(PTR, SIZE)
#	define NOCH_FREE(PTR) free(PTR)
#endif

#endif
