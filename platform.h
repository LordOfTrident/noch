#ifndef NOCH_PLATFORM_H_HEADER_GUARD
#define NOCH_PLATFORM_H_HEADER_GUARD

#if defined(__clang__)
#	define NOCH_COMPILER_CLANG
#elif defined(__GNUC__)
#	define NOCH_COMPILER_GCC
#elif defined(_MSC_VER)
#	define NOCH_COMPILER_MSVC
#else
#	define NOCH_COMPILER_UNKNOWN
#endif

#if defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#	define NOCH_PLATFORM_WINDOWS
#elif defined(__APPLE__)
#	define NOCH_PLATFORM_APPLE
#elif defined(__linux__) || defined(__gnu_linux__) || defined(linux)
#	define NOCH_PLATFORM_LINUX
#elif defined(__unix__) || defined(unix)
#	define NOCH_PLATFORM_UNIX
#else
#	define NOCH_PLATFORM_UNKNOWN
#endif

#if defined(__x86_64__) || defined(_M_X64)
#	define NOCH_TARGET_X64
#elif defined(__i386) || defined(_M_IX86)
#	define NOCH_TARGET_X86
#endif

#endif
