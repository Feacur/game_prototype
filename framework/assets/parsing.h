#if !defined(GAME_ASSETS_PARSING)
#define GAME_ASSETS_PARSING

#include "framework/common.h"

float parse_float_positive(char const * text);
uint32_t parse_u32(char const * text);

inline static bool parse_is_digit(char c) {
	return '0' <= c && c <= '9';
}

inline static bool parse_is_hex(char c) {
	return ('0' <= c && c <= '9')
	    || ('a' <= c && c <= 'f')
	    || ('A' <= c && c <= 'F');
}

inline static bool parse_is_alpha(char c) {
	return ('a' <= c && c <= 'z')
	    || ('A' <= c && c <= 'Z')
	    || c == '_';
}

#endif
