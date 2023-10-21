#if !defined(FRAMEWORK_FORMATTER)
#define FRAMEWORK_FORMATTER

#include "common.h"

PRINTF_LIKE(1, 2)
uint32_t formatter_log(char const * format, ...);
#define LOG(...) formatter_log("" __VA_ARGS__)

PRINTF_LIKE(3, 4)
uint32_t formatter_fmt(size_t capacity, char * output, char const * format, ...);

#endif
