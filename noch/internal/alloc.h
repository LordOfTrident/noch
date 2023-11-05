#ifndef NOCH_INTERNAL_ALLOC_H_HEADER_GUARD
#define NOCH_INTERNAL_ALLOC_H_HEADER_GUARD

#include <stdlib.h> /* malloc, realloc, free, abort */
#include <stdio.h>  /* fprintf, stderr */

#ifndef nochAlloc
#	define nochAlloc(SIZE) malloc(SIZE)
#endif

#ifndef nochRealloc
#	define nochRealloc(PTR, SIZE) realloc(PTR, SIZE)
#endif

#ifndef nochFree
#	define nochFree(PTR) free(PTR)
#endif

#define NOCH_OUT_OF_MEM()                                              \
	do {                                                               \
		fprintf(stderr, "%s:%i: Out of memory\n", __FILE__, __LINE__); \
		abort();                                                       \
	} while (0)

#endif
