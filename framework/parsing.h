#if !defined(FRAMEWORK_PARSING)
#define FRAMEWORK_PARSING

#include "common.h"

uint32_t parse_u32(char const * text);
uint32_t parse_h32(char const * text);

int32_t parse_s32(char const * text);

float parse_r32(char const * text);
double parse_r64(char const * text);

#endif
