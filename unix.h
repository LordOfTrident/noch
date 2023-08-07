#ifndef __USE_XOPEN_EXTENDED
#	define __USE_XOPEN_EXTENDED
#endif

#ifdef _POSIX_C_SOURCE
#	if _POSIX_C_SOURCE < 200112L
#		undef  _POSIX_C_SOURCE
#		define _POSIX_C_SOURCE 200112L
#	endif
#endif

#ifdef _XOPEN_SOURCE
#	if _XOPEN_SOURCE < 500
#		undef  _XOPEN_SOURCE
#		define _XOPEN_SOURCE 500
#	endif
#endif

#include <unistd.h>
