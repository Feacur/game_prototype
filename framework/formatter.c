#include <stdio.h>

#define FORMATTER_CALLBACK(name) char * (name)(char const * input, void * context_ptr, int length)
typedef FORMATTER_CALLBACK(Formatter_Callback);

#include "framework/__warnings_push.h"
	#define STB_SPRINTF_IMPLEMENTATION
	#include <stb/stb_sprintf.h>
#include "framework/__warnings_pop.h"

//
#include "formatter.h"

// ----- ----- ----- ----- -----
//     format into logger
// ----- ----- ----- ----- -----

struct Formatter_Log {
	char buffer[STB_SPRINTF_MIN];
};

static FORMATTER_CALLBACK(formatter_log_callback) {
	struct Formatter_Log * context = context_ptr;

	if (length > 0)
		fwrite(input, sizeof(*input), (size_t)length, stdout);

	return context->buffer;
}

uint32_t formatter_log(char const * format, ...) {
	struct Formatter_Log context = {0};
	va_list args; va_start(args, format);
	uint32_t length = (uint32_t)stbsp_vsprintfcb(
		formatter_log_callback, &context
		, formatter_log_callback(NULL, &context, 0)
		, format, args
	);
	va_end(args); return length;
}

// ----- ----- ----- ----- -----
//     format into buffer
// ----- ----- ----- ----- -----

struct Formatter_Fmt {
	char buffer[STB_SPRINTF_MIN];
	size_t capacity;
	char * output;
};

static FORMATTER_CALLBACK(formatter_fmt_callback) {
	struct Formatter_Fmt * context = context_ptr;

	size_t input_size = (size_t)length;
	if (input_size > context->capacity) {
		ERR("insufficient format space %zu / %zu\n", context->capacity, input_size);
		REPORT_CALLSTACK(); DEBUG_BREAK();
		input_size = context->capacity;
	}

	if (context->output != input)
		common_memcpy(context->output, input, input_size);
	context->output += input_size;
	context->capacity -= input_size;

	// @note: try and write directly into output
	return (context->capacity > 0)
		? (context->capacity >= sizeof(context->buffer))
			? context->output
			: context->buffer
		: NULL;
}

uint32_t formatter_fmt(size_t capacity, char * output, char const * format, ...) {
	struct Formatter_Fmt context = {.capacity = capacity, .output = output};
	va_list args; va_start(args, format);
	uint32_t length = (uint32_t)stbsp_vsprintfcb(
		formatter_fmt_callback, &context
		, formatter_fmt_callback(NULL, &context, 0)
		, format, args
	);
	va_end(args); return length;
}
