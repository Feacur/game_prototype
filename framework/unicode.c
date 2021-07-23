#include "framework/logger.h"

#include <stdlib.h>

//
#include "unicode.h"

uint32_t utf8_length(uint8_t const * value) {
	uint8_t const octet = value[0];
	if (octet < 0x80) { return 1; }

	// if ((octet & 0xc0) == 0x80) { return 0; } // a continuation octet of a 1/2/3/4 byte sequence
	// if ((octet & 0xfc) == 0xf8) { return 0; } // a malformed octet, of a 5 byte sequence
	// if ((octet & 0xfe) == 0xfc) { return 0; } // a malformed octet, of a 6 byte sequence

	if ((octet & 0xe0) == 0xc0) { return 2; }
	if ((octet & 0xf0) == 0xe0) { return 3; }
	if ((octet & 0xf8) == 0xf0) { return 4; }
	logger_to_console("UTF-8 sequence is malformed\n"); DEBUG_BREAK();

	return 0;
}

uint32_t utf8_decode(uint8_t const * value, uint32_t length) {
	// @note: do range checks or not?
	static uint8_t const masks[8] = {0x00, 0x7f, 0x1f, 0x0f, 0x07, 0x00, 0x00, 0x00};
	// if (length >= sizeof(masks) / sizeof(*masks)) { return CODEPOINT_EMPTY; }

	uint32_t codepoint = value[0] & masks[length];
	for (uint32_t i = 1; i < length; i++) {
		uint8_t const octet = value[i];
		if ((octet & 0xc0) != 0x80) {
			logger_to_console("UTF-8 sequence is malformed\n"); DEBUG_BREAK();
			return CODEPOINT_EMPTY;
		}
		codepoint = (codepoint << 6) | (octet & 0x3f);
	}

	return codepoint;
}

bool utf8_iterate(uint32_t length, uint8_t const * data, struct UTF8_Iterator * it) {
	while (it->next < length) {
		uint8_t const * value = data + it->next;
		uint32_t const octets_count = utf8_length(value);

		it->current = it->next;
		it->next += (octets_count > 0) ? octets_count : 1;

		if (octets_count > 0) {
			it->codepoint = utf8_decode(value, octets_count);
			if (it->codepoint != CODEPOINT_EMPTY) { return true; }
		}
	}
	return false;
}
