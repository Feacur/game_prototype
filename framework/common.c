#include "formatter.h"
#include "parsing.h"
#include "platform/debug.h"

#include <stdlib.h>
#include <string.h>


//
#include "common.h"

// ----- ----- ----- ----- -----
//     array
// ----- ----- ----- ----- -----

struct CArray carray_const(struct CArray_Mut value) {
	return (struct CArray){
		.value_size = value.value_size,
		.count = value.count,
		.data = value.data,
	};
}

bool carray_equals(struct CArray v1, struct CArray v2) {
	if (v1.value_size != v2.value_size) { return false; }
	if (v1.count != v2.count) { return false; }
	return equals(v1.data, v2.data, v2.value_size * v2.count);
}

bool carray_empty(struct CArray value) {
	if (value.value_size == 0) { return true; }
	if (value.count == 0) { return true; }
	if (value.data == NULL) { return true; }
	return false;
}

void carray_clear(struct CArray_Mut value) {
	if (value.value_size == 0) { return; }
	if (value.count == 0) { return; }
	if (value.data == NULL) { return; }
	memset(value.data, 0, value.value_size * value.count);
}

// ----- ----- ----- ----- -----
//     buffer
// ----- ----- ----- ----- -----

struct CBuffer cbuffer_const(struct CBuffer_Mut value) {
	return (struct CBuffer){
		.size = value.size,
		.data = value.data,
	};
}

bool cbuffer_equals(struct CBuffer v1, struct CBuffer v2) {
	if (v1.size != v2.size) { return false; }
	return equals(v1.data, v2.data, v2.size);
}

bool cbuffer_empty(struct CBuffer value) {
	if (value.size == 0) { return true; }
	if (value.data == NULL) { return true; }
	return false;
}

void cbuffer_clear(struct CBuffer_Mut value) {
	if (value.size == 0) { return; }
	if (value.data == NULL) { return; }
	memset(value.data, 0, value.size);
}

// ----- ----- ----- ----- -----
//     string
// ----- ----- ----- ----- -----

struct CString cstring_const(struct CString_Mut value) {
	return (struct CString){
		.length = value.length,
		.data = value.data,
	};
}

bool cstring_equals(struct CString v1, struct CString v2) {
	if (v1.length != v2.length) { return false; }
	return equals(v1.data, v2.data, v2.length);
}

bool cstring_empty(struct CString value) {
	if (value.length == 0) { return true; }
	if (value.data == NULL) { return true; }
	return false;
}

void cstring_clear(struct CString_Mut value) {
	if (value.length == 0) { return; }
	if (value.data == NULL) { return; }
	memset(value.data, 0, value.length);
}

bool cstring_contains(struct CString v1, struct CString v2) {
	if (v1.length < v2.length) { return false; }
	return strstr(v1.data, v2.data) != NULL;
}

bool cstring_starts(struct CString v1, struct CString v2) {
	if (v1.length < v2.length) { return false; }
	return equals(v1.data, v2.data, v2.length);
}

bool cstring_ends(struct CString v1, struct CString v2) {
	if (v1.length < v2.length) { return false; }
	return equals(v1.data + v1.length - v2.length, v2.data, v2.length);
}

// ----- ----- ----- ----- -----
//     standard
// ----- ----- ----- ----- -----

uint32_t find_null(char const * buffer) {
	return (uint32_t)strlen(buffer);
}

bool equals(void const * v1, void const * v2, size_t size) {
	if (v1 == v2) { return true; }
	return memcmp(v1, v2, size) == 0;
}

__declspec(noreturn) void common_exit_success(void) { exit(EXIT_SUCCESS); }
__declspec(noreturn) void common_exit_failure(void) { exit(EXIT_FAILURE); }

void common_memcpy(void * target, void const * source, size_t size) {
	if (source == NULL) { return; }
	memcpy(target, source, size);
}

void common_qsort(void * data, size_t count, size_t value_size, Comparator * compare) {
	qsort(data, count, value_size, compare);
}

char const * common_strstr(char const * buffer, char const * value) {
	return strstr(buffer, value);
}

// ----- ----- ----- ----- -----
//     utilities
// ----- ----- ----- ----- -----

bool contains_full_word(char const * container, struct CString value) {
	if (container == NULL)  { return false; }
	if (value.data == NULL) { return false; }

	for (char const * start = container, * end = container; *start; start = end) {
		while (*start == ' ') { start++; }

		end = strchr(start, ' ');
		if (end == NULL) { end = container + strlen(container); }

		struct CString const it = {
			.length = (uint32_t)(end - start),
			.data = start,
		};
		if (cstring_equals(it, value)) { return true; }
	}

	return false;
}

// ----- ----- ----- ----- -----
//     debug
// ----- ----- ----- ----- -----

void print_callstack(uint32_t padding, struct Callstack callstack) {
	struct CString stacktrace = platform_debug_get_stacktrace(callstack);
	do {
		while (stacktrace.length > 0 && is_line(stacktrace.data[0])) {
			stacktrace.length--;
			stacktrace.data++;
		}
		uint32_t line_length = 0;
		while (line_length < stacktrace.length && !is_line(stacktrace.data[line_length])) {
			line_length++;
		}
		if (line_length > 0) {
			LOG("%*s" , padding, "");
			LOG("%.*s\n" , line_length, stacktrace.data);
			stacktrace.length -= line_length;
			stacktrace.data += line_length;
		}
	} while (stacktrace.length > 0);
}

void report_callstack(void) {
	// @note: skip `report_callstack`
	struct Callstack const callstack = platform_debug_get_callstack(1);
	LOG("> callstack:\n");
	print_callstack(2, callstack);
}
