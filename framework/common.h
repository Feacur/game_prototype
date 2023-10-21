#if !defined(FRAMEWORK_COMMON)
#define FRAMEWORK_COMMON

#include "__compilation.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// ----- ----- ----- ----- -----
//     array
// ----- ----- ----- ----- -----

struct CArray {
	size_t size;
	void const * data;
};

struct CArray_Mut {
	size_t size;
	void * data;
};

struct CArray carray_const(struct CArray_Mut value);
bool carray_equals(struct CArray v1, struct CArray v2);

#define A_(value) (struct CArray){.size = sizeof(value), .data = &value}

// ----- ----- ----- ----- -----
//     string
// ----- ----- ----- ----- -----

struct CString {
	uint32_t length;
	char const * data;
};

struct CString_Mut {
	uint32_t length;
	char * data;
};

struct CString cstring_const(struct CString_Mut value);
bool cstring_contains(struct CString v1, struct CString v2);
bool cstring_equals(struct CString v1, struct CString v2);
bool cstring_starts(struct CString v1, struct CString v2);
bool cstring_ends(struct CString v1, struct CString v2);

// @note: MSVC issues `C2099: initializer is not a constant`
//        when a designated initializer type is specified
#define S__(value) {.length = sizeof(value) - 1, .data = "" value}
#define S_(value) (struct CString)S__(value)

// ----- ----- ----- ----- -----
//     standard
// ----- ----- ----- ----- -----

void common_exit_success(void);
void common_exit_failure(void);

void common_memset(void * target, uint8_t value, size_t size);
void common_memcpy(void * target, void const * source, size_t size);
int32_t common_memcmp(void const * buffer_1, void const * buffer_2, size_t size);
void common_qsort(void * data, size_t count, size_t value_size, int (* compare)(void const * v1, void const * v2));
char const * common_strstr(char const * buffer, char const * value);
int32_t common_strcmp(char const * buffer_1, char const * buffer_2);
int32_t common_strncmp(char const * buffer_1, char const * buffer_2, size_t size);

#define INDEX_EMPTY UINT32_MAX

// ----- ----- ----- ----- -----
//     utilities
// ----- ----- ----- ----- -----

#define STR_TKN(v) #v
#define STR_MCR(m) STR_TKN(m)
#define CAT_TKN(v1, v2) v1 ## v2
#define CAT_MCR(m1, m2) CAT_TKN(m1, m2)

#define FILE_AND_LINE __FILE__ ":" STR_MCR(__LINE__)
#define S_FILE_AND_LINE S_(FILE_AND_LINE)

#define SIZE_OF_ARRAY(array) (sizeof(array) / sizeof(*array))
#define SIZE_OF_MEMBER(type, name) sizeof(((type *)0)->name)

uint32_t align_u32(uint32_t value);
uint64_t align_u64(uint64_t value);

bool contains_full_word(char const * container, struct CString value);

// ----- ----- ----- ----- -----
//     allocating
// ----- ----- ----- ----- -----

#define ALLOCATOR(name) void * (name)(void * pointer, size_t size)
typedef ALLOCATOR(Allocator);

// ----- ----- ----- ----- -----
//     hashing
// ----- ----- ----- ----- -----

#define HASHER(name) uint32_t (name)(void const * v)
typedef HASHER(Hasher);

inline static HASHER(hash32) { return *(uint32_t const *)v; }

// ----- ----- ----- ----- -----
//     flexible array
// ----- ----- ----- ----- -----

#if __STDC_VERSION__ >= 199901L
	#define FLEXIBLE_ARRAY
#endif

#if !defined(FLEXIBLE_ARRAY)
	#define FLEXIBLE_ARRAY 1
#endif

// ----- ----- ----- ----- -----
//     debug break
// ----- ----- ----- ----- -----

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

// ----- ----- ----- ----- -----
//     callstack
// ----- ----- ----- ----- -----

void report_callstack(void);

#if !defined(GAME_TARGET_RELEASE)
	#define REPORT_CALLSTACK() report_callstack()
#endif

#if !defined(REPORT_CALLSTACK)
	#define REPORT_CALLSTACK() (void)0
#endif

// ----- ----- ----- ----- -----
//     static assert
// ----- ----- ----- ----- -----

#define STATIC_ASSERT(condition, token) \
	typedef char CAT_MCR(static_assert__ ## token ## _, __LINE__)[(condition)?1:-1]

// ----- ----- ----- ----- -----
//     handle
// ----- ----- ----- ----- -----

struct Handle {
	uint32_t id : 24; // ((1 << 24) - 1) == 0x00ffffff
	uint32_t gen : 8; // ((1 <<  8) - 1) == 0x000000ff
};
STATIC_ASSERT(sizeof(struct Handle) == sizeof(uint32_t), common);

inline static bool handle_is_null(struct Handle h) { return h.id == 0; }
inline static bool handle_equals(struct Handle h1, struct Handle h2) {
	return h1.gen == h2.gen && h1.id == h2.id;
}

// ----- ----- ----- ----- -----
//     documentation
// ----- ----- ----- ----- -----

// https://sourceforge.net/p/predef/wiki/Home/

#endif
