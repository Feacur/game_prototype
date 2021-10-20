#include "framework/logger.h"

// @note: UTF-8 is based off of rfc3629

//
#include "unicode.h"

uint32_t utf8_codepoint_length(uint8_t const * value) {
	uint8_t const octet = value[0];
	if ((octet & 0x80) == 0x00) { return 1; }
	if ((octet & 0xe0) == 0xc0) { return 2; }
	if ((octet & 0xf0) == 0xe0) { return 3; }
	if ((octet & 0xf8) == 0xf0) { return 4; }
	return 0;
}

uint32_t utf8_codepoint_decode(uint8_t const * value, uint32_t length) {
	static uint8_t const masks[8] = {0x00, 0x7f, 0x1f, 0x0f, 0x07, 0x00, 0x00, 0x00};
	uint32_t codepoint = value[0] & masks[length];
	for (uint32_t i = 1; i < length; i++) {
		uint8_t const octet = value[i];
		if ((octet & 0xc0) == 0x80) {
			codepoint = (codepoint << 6) | (octet & 0x3f);
			continue;
		}
		return CODEPOINT_EMPTY;
	}
	return codepoint;
}

bool utf8_iterate(uint32_t length, uint8_t const * data, struct UTF8_Iterator * it) {
	while (it->next < length) {
		uint8_t const * value = data + it->next;
		uint32_t const octets_count = utf8_codepoint_length(value);

		it->current = it->next;
		it->next += (octets_count > 0) ? octets_count : 1;

		if (octets_count > 0) {
			it->codepoint = utf8_codepoint_decode(value, octets_count);
			if (it->codepoint != CODEPOINT_EMPTY) { return true; }
		}
		logger_to_console("UTF-8 sequence is malformed\n"); DEBUG_BREAK();
	}
	return false;
}
