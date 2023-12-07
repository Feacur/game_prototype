#include "maths.h"

#define PARSE_INTEGER(type, value) \
	while (is_digit(*text)) { \
		type const next_value = value * 10 + (type)(*text - '0'); \
		if (next_value < value) { REPORT_CALLSTACK(); DEBUG_BREAK(); } \
		value = next_value; \
		text++; \
	} \

//
#include "parsing.h"

static float make_float(uint32_t mantissa, int32_t exponent_10);
float parse_float(char const * text) {
	bool positive = true;
	if (*text == '-') { text++;
		positive = false;
	}

	uint32_t mantissa = 0;
	PARSE_INTEGER(uint32_t, mantissa)

	int32_t exponent = 0;
	if (*text == '.') { text++;
		char const * const text_start = text;
		PARSE_INTEGER(uint32_t, mantissa)
		exponent += (int32_t)(text_start - text);
	}

	if (*text == 'e' || *text == 'E') { text++;
		bool exp_positive = true;
		switch (*text) {
			case '-': text++; exp_positive = false; break;
			case '+': text++; exp_positive = true; break;
		}

		uint32_t exp_value = 0;
		PARSE_INTEGER(uint32_t, exp_value)
		exponent += exp_value * (exp_positive * 2 - 1);
	}

	// result = mantissa * 10^exponent
	return make_float(mantissa, exponent) * (positive * 2 - 1);
}

uint32_t parse_u32(char const * text) {
	uint32_t value = 0;
	PARSE_INTEGER(uint32_t, value)
	return value;
}

int32_t parse_s32(char const * text) {
	bool positive = true;
	if (*text == '-') { text++;
		positive = false;
	}

	int32_t value = 0;
	PARSE_INTEGER(int32_t, value)
	return value * (positive * 2 - 1);
}

uint32_t parse_hex_u32(char const * text) {
	static uint8_t const c_table_hex[] = { 
		16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
		16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
		16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
		 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,16,16,16,16,16,16,
		16,10,11,12,13,14,15,16,16,16,16,16,16,16,16,16,
		16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
		16,10,11,12,13,14,15,16,16,16,16,16,16,16,16,16,
		16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
		16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
		16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
		16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
		16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
		16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
		16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
		16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
		16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
	};
	uint32_t value = 0;
	for (;;) {
		uint8_t const digit = c_table_hex[*(uint8_t const *)text];
		if (digit < 16) {
			uint32_t const next_value = (value << 4) | digit;
			if (next_value < value) { REPORT_CALLSTACK(); DEBUG_BREAK(); }
			value = next_value;
			text++;
		}
		else { break; }
	}
	return value;
}


static double make_double(uint64_t mantissa, int32_t exponent_10);
double parse_double(char const * text) {
	bool positive = true;
	if (*text == '-') { text++;
		positive = false;
	}

	uint64_t mantissa = 0;
	PARSE_INTEGER(uint64_t, mantissa)

	int32_t exponent = 0;
	if (*text == '.') { text++;
		char const * const text_start = text;
		PARSE_INTEGER(uint64_t, mantissa)
		exponent += (int32_t)(text_start - text);
	}

	if (*text == 'e' || *text == 'E') { text++;
		bool exp_positive = true;
		switch (*text) {
			case '-': text++; exp_positive = false; break;
			case '+': text++; exp_positive = true; break;
		}

		uint32_t exp_value = 0;
		PARSE_INTEGER(uint32_t, exp_value)
		exponent += exp_value * (exp_positive * 2 - 1);
	}

	// result = mantissa * 10^exponent
	return make_double(mantissa, exponent) * (positive * 2 - 1);
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
		// 0b1110_0000_0000_0000_0000_0000_0000_0000
		uint32_t const mask = UINT32_C(0xE0000000);
		while (mantissa & mask) { mantissa >>= 1; ++exponent_2; }
		mantissa *= 5; --exponent_10;
	}

	while (exponent_10 < 0) {
		// 0b1000_0000_0000_0000_0000_0000_0000_0000
		uint32_t const mask = UINT32_C(0x80000000);
		while (!(mantissa & mask)) { mantissa <<= 1; --exponent_2; }
		mantissa /= 5; ++exponent_10;
	}

	return r32_ldexp((float)mantissa, exponent_2);
}

static double make_double(uint64_t mantissa, int32_t exponent_10) {
	// result = mantissa * 10^exponent_10
	// result = mantissa * 5^exponent_10 * 2^exponent_10
	if (mantissa == 0) { return 0; }
	if (exponent_10 == 0) { return (double)mantissa; }

	// > mantissa_2 * 2^exponent_2 == mantissa * 10^exponent_10
	// > [start with] exponent_2 == exponent_10
	// > [calculate]  mantissa_2 == mantissa * 5^exponent_10
	// > preserve significance by transferring powers of 2 from mantissa to exponent_2
	int32_t exponent_2 = exponent_10;

	while (exponent_10 > 0) {
		// 0b1110_0000_0000_0000_0000_0000_0000_0000_0000_0000_0000_0000_0000_0000_0000_0000
		uint64_t const mask = UINT64_C(0xE000000000000000);
		while (mantissa & mask) { mantissa >>= 1; ++exponent_2; }
		mantissa *= 5; --exponent_10;
	}

	while (exponent_10 < 0) {
		// 0b1000_0000_0000_0000_0000_0000_0000_0000_0000_0000_0000_0000_0000_0000_0000_0000
		uint64_t const mask = UINT64_C(0x8000000000000000);
		while (!(mantissa & mask)) { mantissa <<= 1; --exponent_2; }
		mantissa /= 5; ++exponent_10;
	}

	return r64_ldexp((double)mantissa, exponent_2);
}

#undef PARSE_INTEGER
