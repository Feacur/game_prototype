#include <stdio.h>

#include "framework/__warnings_push.h"
	#define STB_SPRINTF_IMPLEMENTATION
	#include <stb/stb_sprintf.h>
#include "framework/__warnings_pop.h"

//
static char gs_logger_buffer[STB_SPRINTF_MIN];
static char * logger_callback_stderr(char const * buffer, void * user, int length) {
	(void)user;
	fwrite(buffer, sizeof(*buffer), (size_t)length, stderr);
	return gs_logger_buffer;
}

//
#include "logger.h"

void logger_to_console(char const * format, ...) {
	va_list args;
	va_start(args, format);
	stbsp_vsprintfcb(logger_callback_stderr, NULL, gs_logger_buffer, format, args);
	va_end(args);
}

uint32_t logger_to_buffer(uint32_t size, char * buffer, char const * format, ...) {
	va_list args;
	va_start(args, format);
	uint32_t length = (uint32_t)stbsp_vsnprintf(buffer, (int)size, format, args);
	va_end(args);
	if (length > size) {
		logger_to_console("insufficient buffer space\n");
		REPORT_CALLSTACK(); DEBUG_BREAK();
	}
	return length;
}
