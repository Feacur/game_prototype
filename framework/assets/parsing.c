#include <math.h>

#define PARSE_INTEGER(type, value) \
	while (parse_is_digit(*text)) { \
		value = value * 10 + (type)(*text - '0'); \
		text++; \
	} \

//
#include "parsing.h"

char const * parse_whitespace(char const * text) {
	for (;;) {
		switch (*text) {
			case ' ':
			case '\t':
			case '\r':
				text++;
				break;

			default:
				return text;
		}
	}
}

static float make_float(uint32_t mantissa, int32_t exponent_10);
float parse_float_positive(char const * text) {
	// result = mantissa * 10^exponent
	uint32_t mantissa = 0;
	int32_t exponent = 0;
	PARSE_INTEGER(uint32_t, mantissa)

	if (*text == '.') { text++;
		char const * const text_start = text;
		PARSE_INTEGER(uint32_t, mantissa)
		exponent += (text_start - text);
	}

	if (*text == 'e' || *text == 'E') { text++;
		bool exp_sign = true;
		switch (*text) {
			case '-': text++; exp_sign = false; break;
			case '+': text++; exp_sign = true; break;
		}

		uint32_t exp_value = 0;
		PARSE_INTEGER(uint32_t, exp_value)
		exponent += exp_value * (exp_sign * 2 - 1);
	}

	return make_float(mantissa, exponent);
}

uint32_t parse_u32(char const * text) {
	uint32_t value = 0;
	PARSE_INTEGER(uint32_t, value)
	return value;
}

//

static float make_float(uint32_t mantissa, int32_t exponent_10) {
	// result = mantissa * 10^exponent_10
	// result = mantissa * 5^exponent_10 * 2^exponent_10
	if (mantissa == 0) { return 0; }
	if (exponent_10 == 0) { return (float)mantissa; }

	// > mantissa_2 * 2^exponent_2 == mantissa * 10^exponent_10
	// > [start with] exponent_2 == exponent_10
	// > [calculate]  mantissa_2 == mantissa * 5^exponent_10
	// > preserve significance by transferring powers of 2 from mantissa to exponent_2
	int32_t exponent_2 = exponent_10;

	while (exponent_10 > 0) {
		uint32_t const mask = 0xE0000000; // 0b1110_0000_0000_0000_0000_0000_0000_0000
		while (mantissa & mask) { mantissa >>= 1; ++exponent_2; }
		mantissa *= 5; --exponent_10;
	}

	while (exponent_10 < 0) {
		uint32_t const mask = 0x80000000; // 0b1000_0000_0000_0000_0000_0000_0000_0000
		while (!(mantissa & mask)) { mantissa <<= 1; --exponent_2; }
		mantissa /= 5; ++exponent_10;
	}

	// ldexp(a, b) == a * 2^b
	return ldexpf((float)mantissa, exponent_2);
}

#undef PARSE_INTEGER
