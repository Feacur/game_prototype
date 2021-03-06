#include "memory.h"

#include <string.h>
#include <math.h>

//
#include "asset_mesh_obj.h"

static bool is_digit(char c) {
	return '0' <= c && c <= '9';
}

static char const * skip_line(char const * text) {
	while (*text != '\0' && *text != '\n') { text++; }
	return text;
}

static char const * skip_whitespace(char const * text) {
	for (;;) {
		switch (*text) {
			case ' ':
			case '\t':
			case '\r':
			case '\n':
				text++;
				break;

			case '#': {
					text = skip_line(text);
				break;
			}

			default: return text;
		}
	}
}

static float make_float(bool sign, uint32_t mantissa, int32_t exponent_10) {
	// result = (2 * sign - 1) * mantissa * 10^exponent_10
	// result = (2 * sign - 1) * mantissa * 5^exponent_10 * 2^exponent_10
	if (mantissa == 0) { return 0; }
	if (exponent_10 == 0) { return (float)mantissa * (sign * 2 - 1); }

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
	return ldexpf((float)mantissa, exponent_2) * (sign * 2 - 1);
}

static char const * parse_float(char const * text, float * result) {
	bool sign = true;
	uint32_t mantissa = 0;
	int32_t exponent = 0;

	text = skip_whitespace(text);

	if (*text == '-') { text++; sign = false; }
	while (is_digit(*text)) {
		mantissa = mantissa * 10 + (uint32_t)(*text - '0');
		text++;
	}

	if (*text == '.') {
		text++;
		while (is_digit(*text)) {
			mantissa = mantissa * 10 + (uint32_t)(*text - '0');
			exponent--;
			text++;
		}
	}

	if (*text == 'e' || *text == 'E') {
		bool exp_sign = true;
		uint32_t exp_value = 0;

		text++;
		if (*text == '-') { text++; exp_sign = false; }
		while (is_digit(*text)) {
			exp_value = exp_value * 10 + (uint32_t)(*text - '0');
			text++;
		}

		exponent += exp_value * (exp_sign * 2 - 1);
	}

	*result = make_float(sign, mantissa, exponent);
	return text;
}

void asset_mesh_obj_init(struct Asset_Mesh_Obj * obj, char const * text) {
	char const * cur;

	uint32_t position_lines = 0;
	uint32_t texcoord_lines = 0;
	uint32_t normal_lines = 0;
	uint32_t face_lines = 0;

	cur = text;
	while (*cur) {
		cur = skip_whitespace(cur);
		switch (*cur) {
			case 'v': {
				cur++;
				switch (*cur) {
					case ' ': position_lines++; break;
					case 't': texcoord_lines++; break;
					case 'n': normal_lines++; break;
				}
				break;
			}
			case 'f': face_lines++; break;
		}
		cur = skip_line(cur);
	}

	array_float_init(&obj->positions);
	array_float_init(&obj->texcoords);
	array_float_init(&obj->normals);
	array_s32_init(&obj->faces);

	array_float_resize(&obj->positions, position_lines * 3);
	array_float_resize(&obj->texcoords, texcoord_lines * 2);
	array_float_resize(&obj->normals, normal_lines * 3);
	array_s32_resize(&obj->faces, face_lines * 4);

	//
	cur = text;
	while (*cur) {
		cur = skip_whitespace(cur);
		switch (*cur) {
			case 'v': {
				cur++;
				switch (*cur) {
					case ' ': {
						float position[3];
						cur = parse_float(cur, position + 0);
						cur = parse_float(cur, position + 1);
						cur = parse_float(cur, position + 2);
						array_float_write_many(&obj->positions, position, 3);
						break;
					}
					case 't': {
						float texcoord[2];
						cur = parse_float(cur, texcoord + 0);
						cur = parse_float(cur, texcoord + 1);
						array_float_write_many(&obj->texcoords, texcoord, 2);
						break;
					}
					case 'n': {
						float normal[3];
						cur = parse_float(cur, normal + 0);
						cur = parse_float(cur, normal + 1);
						cur = parse_float(cur, normal + 2);
						array_float_write_many(&obj->normals, normal, 3);
						break;
					}
				}
				break;
			}
			case 'f': {
				break;
			}
		}
		cur = skip_line(cur);
	}
}

void asset_mesh_obj_free(struct Asset_Mesh_Obj * obj) {
	memset(obj, 0, sizeof(*obj));
}
