#if !defined(GAME_FRAMEWORK_COMMON)
#define GAME_FRAMEWORK_COMMON

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define INDEX_EMPTY UINT32_MAX

// -- utility

#define STRINGIFY_A_VALUE(v) #v
#define STRINGIFY_A_MACRO(m) STRINGIFY_A_VALUE(m)
#define TOKENIZE_A_VALUE(v1, v2) v1 ## v2
#define TOKENIZE_A_MACRO(m1, m2) TOKENIZE_A_VALUE(m1, m2)

#define FILE_AND_LINE __FILE__ ":" STRINGIFY_A_MACRO(__LINE__)

// -- FLEXIBLE_ARRAY
#if __STDC_VERSION__ >= 199901L
	#if defined(__clang__)
		#define FLEXIBLE_ARRAY
	#endif
#endif

#if !defined(FLEXIBLE_ARRAY)
	#define FLEXIBLE_ARRAY 1
#endif

// -- DEBUG_BREAK
#if defined(GAME_TARGET_DEBUG)
	#if defined(__clang__)
		#define DEBUG_BREAK() __builtin_debugtrap()
	#elif defined(_MSC_VER)
		#define DEBUG_BREAK() __debugbreak()
	#endif
#endif

#if !defined(DEBUG_BREAK)
	#define DEBUG_BREAK() (void)0
#endif

#endif
