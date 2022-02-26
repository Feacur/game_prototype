#if !defined(GAME_FRAMEWORK_COMMON)
#define GAME_FRAMEWORK_COMMON

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// #if defined(__clang__)
// #define NORETURN_POSTFIX __attribute__((noreturn))
// #else
// #define NORETURN_POSTFIX
// #endif

struct CString {
	uint32_t length;
	char const * data;
};

// ----- ----- ----- ----- -----
//     standard
// ----- ----- ----- ----- -----

__declspec(noreturn)
void common_exit_failure(void);

void common_memset(void * target, uint8_t value, size_t size);
void common_memcpy(void * target, void const * source, size_t size);
int common_memcmp(void const * buffer_1, void const * buffer_2, size_t size);
void common_qsort(void * data, size_t count, size_t value_size, int (* compare)(void const * v1, void const * v2));
char const * common_strstr(char const * buffer, char const * value);
int32_t common_strncmp(char const * buffer_1, char const * buffer_2, size_t size);

#define INDEX_EMPTY UINT32_MAX

// ----- ----- ----- ----- -----
//     utilities
// ----- ----- ----- ----- -----

#define STRINGIFY_A_VALUE(v) #v
#define STRINGIFY_A_MACRO(m) STRINGIFY_A_VALUE(m)
#define TOKENIZE_A_VALUE(v1, v2) v1 ## v2
#define TOKENIZE_A_MACRO(m1, m2) TOKENIZE_A_VALUE(m1, m2)

#define S_(value) (struct CString){.length = sizeof("" value) - 1, .data = value,}
#define S_NULL (struct CString){0}
#define S_EMPTY S_("")

#define FILE_AND_LINE __FILE__ ":" STRINGIFY_A_MACRO(__LINE__)
#define S_FILE_AND_LINE S_(FILE_AND_LINE)

#define SIZE_OF_ARRAY(array) (sizeof(array) / sizeof(*array))
#define SIZE_OF_MEMBER(type, name) sizeof(((type *)0)->name)

uint32_t align_u32(uint32_t value);
uint64_t align_u64(uint64_t value);

bool contains_full_word(char const * container, struct CString value);

// ----- ----- ----- ----- -----
//     flexible array
// ----- ----- ----- ----- -----

#if __STDC_VERSION__ >= 199901L
	#if defined(__clang__)
		#define FLEXIBLE_ARRAY
	#endif
#endif

#if !defined(FLEXIBLE_ARRAY)
	#define FLEXIBLE_ARRAY 1
#endif

// ----- ----- ----- ----- -----
//     debug break
// ----- ----- ----- ----- -----

#if defined(GAME_TARGET_DEVELOPMENT) || defined(GAME_TARGET_DEBUG)
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
