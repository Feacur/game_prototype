#include <stdio.h>

#include "framework/__warnings_push.h"
	#define STB_SPRINTF_IMPLEMENTATION
	#include <stb/stb_sprintf.h>
#include "framework/__warnings_pop.h"

//
#include "logger.h"

struct Logger_Context {
	char buffer[STB_SPRINTF_MIN];
};

struct Logger_Buffer {
	struct Logger_Context temp;
	size_t capacity;
	char * output;
};

static LOGGER_CALLBACK(logger_callback_stderr) {
	struct Logger_Context * data = user;

	if (length > 0)
		fwrite(input, sizeof(*input), (size_t)length, stderr);

	return data->buffer;
}

static LOGGER_CALLBACK(logger_callback_buffer) {
	struct Logger_Buffer * data = user;

	size_t input_size = (size_t)length;
	if (input_size > data->capacity) {
		logger_to_console("insufficient buffer space %zu / %zu\n", data->capacity, input_size);
		REPORT_CALLSTACK(); DEBUG_BREAK();
		input_size = data->capacity;
	}

	if (input == data->temp.buffer)
		common_memcpy(data->output, data->temp.buffer, input_size);
	data->output += input_size;
	data->capacity -= input_size;

	// @note: try and write directly into output
	return (data->capacity > 0)
		? (data->capacity >= sizeof(data->temp.buffer))
			? data->output
			: data->temp.buffer
		: NULL;
}

uint32_t logger_to_console(char const * format, ...) {
	struct Logger_Context context = {0};
	va_list args; va_start(args, format);
	uint32_t length = (uint32_t)stbsp_vsprintfcb(
		logger_callback_stderr, &context
		, logger_callback_stderr(NULL, &context, 0)
		, format, args
	);
	va_end(args); return length;
}

uint32_t logger_to_buffer(size_t size, char * buffer, char const * format, ...) {
	struct Logger_Buffer context = { .capacity = size, .output = buffer };
	va_list args; va_start(args, format);
	uint32_t length = (uint32_t)stbsp_vsprintfcb(
		logger_callback_buffer, &context
		, logger_callback_buffer(NULL, &context, 0)
		, format, args
	);
	va_end(args); return length;
}
