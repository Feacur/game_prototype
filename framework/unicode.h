#if !defined(GAME_FRAMEWORK_UTF8)
#define GAME_FRAMEWORK_UTF8

#include "common.h"

#define UTF8_EMPTY UINT32_MAX

uint32_t utf8_length(uint8_t const * value);
uint32_t utf8_decode(uint8_t const * value, uint32_t length);

#endif
