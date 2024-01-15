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
//     constants
// ----- ----- ----- ----- -----

#define INDEX_EMPTY UINT32_MAX

// ----- ----- ----- ----- -----
//     forward
// ----- ----- ----- ----- -----

struct JSON;

// ----- ----- ----- ----- -----
//     conversion
// ----- ----- ----- ----- -----

union Bits32 {
	uint32_t as_u;
	int32_t  as_s;
	float    as_r;
};

union Bits64 {
	uint64_t as_u;
	int64_t  as_s;
	double   as_r;
};

inline static float bits_u32_r32(uint32_t value) {
	return (union Bits32){.as_u = value}.as_r;
}

inline static uint32_t bits_r32_u32(float value) {
	return (union Bits32){.as_r = value}.as_u;
}

inline static double bits_u64_r64(uint64_t value) {
	return (union Bits64){.as_u = value}.as_r;
}

inline static uint64_t bits_r64_u64(double value) {
	return (union Bits64){.as_r = value}.as_u;
}

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
//     delegates
// ----- ----- ----- ----- -----

#define PREDICATE(func) bool (func)(void const * value)
typedef PREDICATE(Predicate);

#define COMPARATOR(func) int (func)(void const * v1, void const * v2)
typedef COMPARATOR(Comparator);

#define ALLOCATOR(func) void * (func)(void * pointer, size_t size)
typedef ALLOCATOR(Allocator);

#define HASHER(func) uint32_t (func)(void const * value)
typedef HASHER(Hasher);

#define JSON_PROCESSOR(func) void (func)(struct JSON const * json, void * data)
typedef JSON_PROCESSOR(JSON_Processor);

#define HANDLE_ACTION(func) void (func)(struct Handle handle)
typedef HANDLE_ACTION(Handle_Action);

// ----- ----- ----- ----- -----
//     array
// ----- ----- ----- ----- -----

struct CArray {
	uint32_t value_size;
	uint32_t count;
	void const * data;
};

struct CArray_Mut {
	uint32_t value_size;
	uint32_t count;
	void * data;
};

struct CArray carray_const(struct CArray_Mut value);
bool carray_equals(struct CArray v1, struct CArray v2);
bool carray_empty(struct CArray value);

#define CA_(value) (struct CArray){.value_size = sizeof(value), .count = 1, .data = &value}
#define CAM_(value) (struct CArray_Mut){.value_size = sizeof(value), .count = 1, .data = &value}
#define CAMP_(pointer) (struct CArray_Mut){.value_size = sizeof(*pointer), .count = 1, .data = pointer}

// ----- ----- ----- ----- -----
//     buffer
// ----- ----- ----- ----- -----

struct CBuffer {
	size_t size;
	void const * data;
};

struct CBuffer_Mut {
	size_t size;
	void * data;
};

struct CBuffer cbuffer_const(struct CBuffer_Mut value);
bool cbuffer_equals(struct CBuffer v1, struct CBuffer v2);
bool cbuffer_empty(struct CBuffer value);

#define CB_(value) (struct CBuffer){.size = sizeof(value), .data = &value}
#define CBM_(value) (struct CBuffer_Mut){.size = sizeof(value), .data = &value}
#define CBMP_(pointer) (struct CBuffer_Mut){.size = sizeof(*pointer), .data = pointer}

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
bool cstring_equals(struct CString v1, struct CString v2);
bool cstring_empty(struct CString value);

bool cstring_contains(struct CString v1, struct CString v2);
bool cstring_starts(struct CString v1, struct CString v2);
bool cstring_ends(struct CString v1, struct CString v2);

// @note: MSVC issues `C2099: initializer is not a constant`
//        when a designated initializer type is specified
#define S__(value) {.length = sizeof(value) - 1, .data = "" value}
#define S_(value) (struct CString)S__(value)

// ----- ----- ----- ----- -----
//     standard
// ----- ----- ----- ----- -----

uint32_t find_null(char const * buffer);
bool equals(void const * v1, void const * v2, size_t size);

void common_exit_success(void);
void common_exit_failure(void);

void common_memcpy(void * target, void const * source, size_t size);
void common_qsort(void * data, size_t count, size_t value_size, Comparator * compare);
char const * common_strstr(char const * buffer, char const * value);

// ----- ----- ----- ----- -----
//     utilities
// ----- ----- ----- ----- -----

void zero_out(struct CBuffer_Mut value);

bool contains_full_word(char const * container, struct CString value);

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
	return *(uint32_t const *)value;
}

inline static HASHER(hash64) {
	struct Data { uint32_t v1, v2; } const * data = value;
	// kinda FNV-1
	uint32_t const prime = 16777619u;
	uint32_t hash = 2166136261u;
	hash = (hash * prime) ^ data->v1;
	hash = (hash * prime) ^ data->v2;
	return hash;
}

// ----- ----- ----- ----- -----
//     debug
// ----- ----- ----- ----- -----

struct Callstack {
	uint64_t data[64];
};

void print_callstack(uint32_t padding, struct Callstack callstack);
void report_callstack(void);

#if !defined(GAME_TARGET_RELEASE)
	#define PRINT_CALLSTACK(padding, callstack) print_callstack(padding, callstack)
	#define REPORT_CALLSTACK() report_callstack()
#endif

#if !defined(PRINT_CALLSTACK)
	#define PRINT_CALLSTACK(padding, callstack) (void)0
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
