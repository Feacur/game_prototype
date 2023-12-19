#if !defined(FRAMEWORK_FORMATTER)
#define FRAMEWORK_FORMATTER

#include "common.h"

PRINTF_LIKE(1, 2)
uint32_t formatter_log(char const * format, ...);
#define LOG(format, ...) formatter_log(""       format,      ## __VA_ARGS__)
#define TRC(format, ...) formatter_log("[trc] " format "\n", ## __VA_ARGS__)
#define WRN(format, ...) formatter_log("[wrn] " format "\n", ## __VA_ARGS__)
#define ERR(format, ...) formatter_log("[err] " format "\n", ## __VA_ARGS__)

PRINTF_LIKE(3, 4)
uint32_t formatter_fmt(size_t capacity, char * output, char const * format, ...);

/*

`*`    - might be a number or a leading parameter

`%#zo` - size_t as oct with leading `0`
`%#zx` - size_t as hex with leading `0x`
`%#f`  - float with decimal point
`%zu`  - size_t as dec

`%*`   - pad a with leading ` `

`%.*d` - pad a with leading `0`
`%.*f` - minimum precision after `.`
`%.*s` - maximum string size

*/

#endif
