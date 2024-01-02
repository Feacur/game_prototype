#include "maths.h"

static uint8_t const LUT_base10[256] = { 
	['0'] =  0, ['1'] =  1,
	['2'] =  2, ['3'] =  3,
	['4'] =  4, ['5'] =  5,
	['6'] =  6, ['7'] =  7,
	['8'] =  8, ['9'] =  9,
};

static uint8_t const LUT_base16[256] = { 
	['0'] =  0, ['1'] =  1,
	['2'] =  2, ['3'] =  3,
	['4'] =  4, ['5'] =  5,
	['6'] =  6, ['7'] =  7,
	['8'] =  8, ['9'] =  9,
	['A'] = 10, ['a'] = 10,
	['B'] = 11, ['b'] = 11,
	['C'] = 12, ['c'] = 12,
	['D'] = 13, ['d'] = 13,
	['E'] = 14, ['e'] = 14,
	['F'] = 15, ['f'] = 15,
};

#define PARSE_SIGN(value) do { \
		switch (*text) { \
			default:          value = 0; break; \
			case '+': text++; value = 0; break; \
			case '-': text++; value = 1; break; \
		} \
		value <<= sizeof(value) * 8 - 1; \
	} while (0) \

#define PARSE_BASE10(value) do { \
		while (is_digit(*text)) { \
			uint8_t const digit = LUT_base10[*(uint8_t const *)text]; \
			value = value * 10 + digit; \
			text++; \
		} \
	} while (0) \

#define PARSE_BASE16(value) do { \
		while (is_digit(*text)) { \
			uint8_t const digit = LUT_base16[*(uint8_t const *)text]; \
			value = (value << 4) | digit; \
			text++; \
		} \
	} while (0) \

// `result == mantissa * 10^exponent == mantissa * 2^exponent * 5^exponent_10`
// eliminate `exponent_10 -> 0` preserving overall significances
#define CONVERT_M10E_M2E(mantissa, exponent) do { \
		int32_t exponent_10 = exponent; \
		while (exponent_10 > 0) { \
			uint8_t const offset = sizeof(mantissa) * 8 - 3; \
			while (mantissa >> offset) { mantissa >>= 1; ++exponent; } \
			mantissa *= 5; --exponent_10; \
		} \
		while (exponent_10 < 0) { \
			uint8_t const offset = sizeof(mantissa) * 8 - 1; \
			while (!(mantissa >> offset)) { mantissa <<= 1; --exponent; } \
			mantissa /= 5; ++exponent_10; \
		} \
	} while (0) \

//
#include "parsing.h"

uint32_t parse_u32(char const * text) {
	uint32_t value = 0; PARSE_BASE10(value);
	return value;
}

uint32_t parse_h32(char const * text) {
	uint32_t value = 0; PARSE_BASE16(value);
	return value;
}

int32_t parse_s32(char const * text) {
	uint32_t     sign;        PARSE_SIGN(sign);
	union Bits32 value = {0}; PARSE_BASE10(value.as_s);
	value.as_u |= sign;
	return value.as_s;
}

float parse_r32(char const * text) {
	uint32_t     sign;           PARSE_SIGN(sign);
	union Bits32 mantissa = {0}; PARSE_BASE10(mantissa.as_u);

	int32_t exponent = 0;
	if (*text == '.') { text++;
		char const * const text_start = text;
		PARSE_BASE10(mantissa.as_u);
		exponent += (int32_t)(text_start - text);
	}

	if (*text == 'e' || *text == 'E') { text++;
		uint32_t     e_sign;  PARSE_SIGN(e_sign);
		union Bits32 e = {0}; PARSE_BASE10(e.as_s);
		e.as_u |= e_sign;
		exponent += e.as_s;
	}

	if (mantissa.as_u != 0) {
		if (exponent == 0) {
			mantissa.as_r = (float)mantissa.as_u;
		}
		else {
			CONVERT_M10E_M2E(mantissa.as_u, exponent);
			mantissa.as_r = r32_ldexp((float)mantissa.as_u, exponent);
		}
	}

	mantissa.as_u |= sign;
	return mantissa.as_r;
}

double parse_r64(char const * text) {
	uint64_t     sign;           PARSE_SIGN(sign);
	union Bits64 mantissa = {0}; PARSE_BASE10(mantissa.as_u);

	int32_t exponent = 0;
	if (*text == '.') { text++;
		char const * const text_start = text;
		PARSE_BASE10(mantissa.as_u);
		exponent += (int32_t)(text_start - text);
	}

	if (*text == 'e' || *text == 'E') { text++;
		uint32_t     e_sign;  PARSE_SIGN(e_sign);
		union Bits32 e = {0}; PARSE_BASE10(e.as_s);
		e.as_u |= e_sign;
		exponent += e.as_s;
	}

	if (mantissa.as_u != 0) {
		if (exponent == 0) {
			mantissa.as_r = (double)mantissa.as_u;
		}
		else {
			CONVERT_M10E_M2E(mantissa.as_u, exponent);
			mantissa.as_r = r64_ldexp((double)mantissa.as_u, exponent);
		}
	}

	mantissa.as_u |= sign;
	return mantissa.as_r;
}

#undef PARSE_SIGN
#undef PARSE_BASE10
#undef PARSE_BASE16
#undef CONVERT_M10E_M2E
