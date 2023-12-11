#if !defined(FRAMEWORK_COMMON)
#define FRAMEWORK_COMMON

#include "__compilation.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// ----- ----- ----- ----- -----
//     meta
// ----- ----- ----- ----- -----

#define STR_TKN(v) #v
#define STR_MCR(m) STR_TKN(m)
#define CAT_TKN(v1, v2) v1 ## v2
#define CAT_MCR(m1, m2) CAT_TKN(m1, m2)

#define FILE_AND_LINE __FILE__ ":" STR_MCR(__LINE__)
#define S_FILE_AND_LINE S_(FILE_AND_LINE)

#define SIZE_OF_ARRAY(array) (sizeof(array) / sizeof(*array))
#define SIZE_OF_MEMBER(type, name) sizeof(((type *)0)->name)

#define STATIC_ASSERT(condition, token) \
	typedef char CAT_MCR(static_assert__ ## token ## _, __LINE__)[(condition)?1:-1]

#if defined(__clang__)
	#define PRINTF_LIKE(position, count) __attribute__((format(printf, position, count)))
#else
	#define PRINTF_LIKE(position, count)
#endif // __clang__

#if __STDC_VERSION__ >= 199901L
	#define FLEXIBLE_ARRAY
#endif

#if !defined(FLEXIBLE_ARRAY)
	#define FLEXIBLE_ARRAY 1
#endif

// ----- ----- ----- ----- -----
//     delegates
// ----- ----- ----- ----- -----

#define PREDICATE(name) bool name(void const * value)
typedef PREDICATE(Predicate);

#define COMPARATOR(name) int name(void const * v1, void const * v2)
typedef COMPARATOR(Comparator);

#define ALLOCATOR(name) void * (name)(void * pointer, size_t size)
typedef ALLOCATOR(Allocator);

#define HASHER(name) uint32_t (name)(void const * v)
typedef HASHER(Hasher);

// ----- ----- ----- ----- -----
//     handle
// ----- ----- ----- ----- -----

struct Handle {
	uint32_t id  : 24; // ((1 << 24) - 1) == 0x00ffffff
	uint32_t gen :  8; // ((1 <<  8) - 1) == 0x000000ff
};
STATIC_ASSERT(sizeof(struct Handle) == sizeof(uint32_t), common);

inline static bool handle_is_null(struct Handle h) { return h.id == 0; }
inline static bool handle_equals(struct Handle h1, struct Handle h2) {
	return h1.gen == h2.gen && h1.id == h2.id;
}

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

#define INDEX_EMPTY UINT32_MAX

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

bool equals(void const * v1, void const * v2, size_t size);

void common_exit_success(void);
void common_exit_failure(void);

void common_memset(void * target, uint8_t value, size_t size);
void common_memcpy(void * target, void const * source, size_t size);
void common_qsort(void * data, size_t count, size_t value_size, Comparator * compare);
char const * common_strstr(char const * buffer, char const * value);

// ----- ----- ----- ----- -----
//     utilities
// ----- ----- ----- ----- -----

bool contains_full_word(char const * container, struct CString value);

inline static uint32_t align_u32(uint32_t value) {
	uint32_t const alignment = 8 - 1;
	return ((value | alignment) & ~alignment);
}

inline static uint64_t align_u64(uint64_t value) {
	uint64_t const alignment = 8 - 1;
	return ((value | alignment) & ~alignment);
}

inline static bool is_line(char c) {
	return c == '\n' || c == '\r';
}

inline static bool is_digit(char c) {
	return '0' <= c && c <= '9';
}

inline static bool is_hex(char c) {
	return ('0' <= c && c <= '9')
	    || ('a' <= c && c <= 'f')
	    || ('A' <= c && c <= 'F');
}

inline static bool is_alpha(char c) {
	return ('a' <= c && c <= 'z')
	    || ('A' <= c && c <= 'Z')
	    || c == '_';
}

inline static HASHER(hash32) {
	return *(uint32_t const *)v;
}

inline static HASHER(hash64) {
	struct Data { uint32_t v1, v2; };
	struct Data const * data = v;
	// kinda FNV-1
	uint32_t const prime = UINT32_C(16777619);
	uint32_t hash = UINT32_C(2166136261);
	hash = (hash * prime) ^ data->v1;
	hash = (hash * prime) ^ data->v2;
	return hash;
}

// ----- ----- ----- ----- -----
//     debug
// ----- ----- ----- ----- -----

void report_callstack(void);

#if !defined(GAME_TARGET_RELEASE)
	#define REPORT_CALLSTACK() report_callstack()
#endif

#if !defined(REPORT_CALLSTACK)
	#define REPORT_CALLSTACK() (void)0
#endif

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
//     documentation
// ----- ----- ----- ----- -----

// https://sourceforge.net/p/predef/wiki/Home/

#endif
