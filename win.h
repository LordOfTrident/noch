/* WINVER and _WIN32_WINNT have to be greater or equal to 0x0600 for
   CreateSymbolicLinkA to work on MinGW */

#ifdef WINVER
#	if WINVER < 0x0600
#		undef  WINVER
#		define WINVER 0x0600
#	endif
#endif

#ifdef _WIN32_WINNT
#	if _WIN32_WINNT < 0x0600
#		undef  _WIN32_WINNT
#		define _WIN32_WINNT 0x0600
#	endif
#endif

#include <windows.h>
