#if !defined(FRAMEWORK_PARSING)
#define FRAMEWORK_PARSING

#include "common.h"

float parse_float(char const * text);
uint32_t parse_u32(char const * text);
int32_t parse_s32(char const * text);
uint32_t parse_hex_u32(char const * text);

double parse_double(char const * text);

#endif
