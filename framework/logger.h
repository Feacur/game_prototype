#if !defined(FRAMEWORK_LOGGER)
#define FRAMEWORK_LOGGER

#include "common.h"

#define LOGGER_CALLBACK(name) char * (name)(char const * input, void * user, int length)
typedef LOGGER_CALLBACK(Logger_Callback);

#if defined(__clang__)
	#define PRINTF_LIKE(position, count) __attribute__((format(printf, position, count)))
#else
	#define PRINTF_LIKE(position, count)
#endif // __clang__

PRINTF_LIKE(1, 2)
uint32_t logger_to_console(char const * format, ...);

PRINTF_LIKE(3, 4)
uint32_t logger_to_buffer(size_t size, char * buffer, char const * format, ...);

#endif
