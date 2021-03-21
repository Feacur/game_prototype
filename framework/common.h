#if !defined(GAME_FRAMEWORK_COMMON)
#define GAME_FRAMEWORK_COMMON

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define GROWTH_FACTOR 2
#define GROW_CAPACITY(capacity) \
	((capacity) < 8 ? 8 : (capacity) * GROWTH_FACTOR) \

#define INDEX_EMPTY UINT32_MAX

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
