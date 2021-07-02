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
static char logger_buffer[STB_SPRINTF_MIN];
static char * logger_callback_fputs_stderr(char const * buffer, void * user, int length) {
	(void)user;
	fwrite(buffer, sizeof(*buffer), (size_t)length, stderr);
	return logger_buffer;
}

//
#include "logger.h"

void logger_to_console(char const * format, ...) {
	va_list args;
	va_start(args, format);
	stbsp_vsprintfcb(logger_callback_fputs_stderr, NULL, logger_buffer, format, args);
	va_end(args);
}

int logger_to_buffer(char * buffer, char const * format, ...) {
	va_list args;
	va_start(args, format);
	int length = stbsp_vsprintfcb(NULL, NULL, buffer, format, args);
	va_end(args);
	return length;
}
