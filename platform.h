#ifndef NOCH_PLATFORM_H_HEADER_GUARD
#define NOCH_PLATFORM_H_HEADER_GUARD

#if defined(__clang__)
#	define COMPILER_CLANG
#elif defined(__GNUC__)
#	define COMPILER_GCC
#elif defined(_MSC_VER)
#	define COMPILER_MSVC
#else
#	define COMPILER_UNKNOWN
#endif

#if defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#	define PLATFORM_WINDOWS
#elif defined(__APPLE__)
#	define PLATFORM_APPLE
#elif defined(__linux__) || defined(__gnu_linux__) || defined(linux)
#	define PLATFORM_LINUX
#elif defined(__unix__) || defined(unix)
#	define PLATFORM_UNIX
#else
#	define PLATFORM_UNKNOWN
#endif

#if defined(__x86_64__) || defined(_M_X64)
#	define TARGET_X64
#elif defined(__i386) || defined(_M_IX86)
#	define TARGET_X86
#endif

#endif
