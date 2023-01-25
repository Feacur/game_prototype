#if !defined(FRAMEWORK_UNICODE)
#define FRAMEWORK_UNICODE

#include "common.h"

#define CODEPOINT_ZERO_WIDTH_SPACE   0x0000200b
#define CODEPOINT_NON_BREAKING_SPACE 0x000000a0

struct UTF8_Iterator {
	uint32_t current, next;
	uint32_t codepoint, previous;
};

uint32_t utf8_codepoint_length(uint8_t const * value);
uint32_t utf8_codepoint_decode(uint8_t const * value, uint32_t length);

inline static bool codepoint_is_invisible(uint32_t codepoint) {
	switch (codepoint) {
		case '\0':
			return false;

		case CODEPOINT_ZERO_WIDTH_SPACE:
		case CODEPOINT_NON_BREAKING_SPACE:
			return true;
	}
	if (codepoint <= ' ') { return true; }
	return false;
}

inline static bool codepoint_is_word_break(uint32_t codepoint) {
	switch (codepoint) {
		case CODEPOINT_ZERO_WIDTH_SPACE:
		case ' ':
		case '\t':
		case '\n':
			return true;
	}
	return false;
}

bool utf8_iterate(uint32_t length, uint8_t const * data, struct UTF8_Iterator * it);

#define FOR_UTF8(length, data, it) for ( \
	struct UTF8_Iterator it = {0}; \
	utf8_iterate(length, data, &it); \
) \

#endif
