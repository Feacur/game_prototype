#if !defined(GAME_FRAMEWORK_UNICODE)
#define GAME_FRAMEWORK_UNICODE

#include "common.h"

#define CODEPOINT_EMPTY              UINT32_MAX
#define CODEPOINT_ZERO_WIDTH_SPACE   0x0000200b
#define CODEPOINT_NON_BREAKING_SPACE 0x000000a0

struct UTF8_Iterator {
	uint32_t current, next;
	uint32_t codepoint;
};

uint32_t utf8_length(uint8_t const * value);
uint32_t utf8_decode(uint8_t const * value, uint32_t length);

bool utf8_iterate(uint32_t length, uint8_t const * data, struct UTF8_Iterator * it);

#endif
