#if !defined(GAME_FRAMEWORK_LOGGER)
#define GAME_FRAMEWORK_LOGGER

#include "common.h"

#if defined(__clang__)
	#define PRINTF_LIKE(position, count) __attribute__((format(printf, position, count)))
#else
	#define PRINTF_LIKE(position, count)
#endif // __clang__

PRINTF_LIKE(1, 2)
void logger_to_console(char const * format, ...);

PRINTF_LIKE(2, 3)
uint32_t logger_to_buffer(char * buffer, char const * format, ...);

#endif
