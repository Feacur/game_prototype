#include <stdio.h>

// better to compile third-parties as separate units
#if defined(__clang__)
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wlanguage-extension-token"
	#pragma clang diagnostic ignored "-Wextra-semi-stmt"
	#pragma clang diagnostic ignored "-Wcast-align"
	#pragma clang diagnostic ignored "-Wcast-qual"
	#pragma clang diagnostic ignored "-Wsign-conversion"
	#pragma clang diagnostic ignored "-Wdouble-promotion"
	#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
	#pragma clang diagnostic ignored "-Wconditional-uninitialized"
#elif defined(_MSC_VER)
	#pragma warning(push, 0)
#endif

#define STB_SPRINTF_IMPLEMENTATION
#include <stb/stb_sprintf.h>

#if defined(__clang__)
	#pragma clang diagnostic pop
#elif defined(_MSC_VER)
	#pragma warning(pop)
#endif

//
static char gs_logger_buffer[STB_SPRINTF_MIN];
static char * logger_callback_fputs_stderr(char const * buffer, void * user, int length) {
	(void)user;
	fwrite(buffer, sizeof(*buffer), (size_t)length, stderr);
	return gs_logger_buffer;
}

//
#include "logger.h"

void logger_to_console(char const * format, ...) {
	va_list args;
	va_start(args, format);
	stbsp_vsprintfcb(logger_callback_fputs_stderr, NULL, gs_logger_buffer, format, args);
	va_end(args);
}

uint32_t logger_to_buffer(char * buffer, char const * format, ...) {
	va_list args;
	va_start(args, format);
	int length = stbsp_vsprintfcb(NULL, NULL, buffer, format, args);
	va_end(args);
	return (uint32_t)length;
}
