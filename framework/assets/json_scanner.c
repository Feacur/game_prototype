#include "framework/memory.h"
#include "framework/parsing.h"

#include <string.h>

//
#include "json_scanner.h"

void json_scanner_init(struct JSON_Scanner * scanner, char const * text) {
	*scanner = (struct JSON_Scanner){
		.start = text,
		.current = text,
	};
}

void json_scanner_free(struct JSON_Scanner * scanner) {
	memset(scanner, 0, sizeof(*scanner));
}

inline static struct JSON_Token json_scanner_next_internal(struct JSON_Scanner * scanner);
struct JSON_Token json_scanner_next(struct JSON_Scanner * scanner) {
	return json_scanner_next_internal(scanner);
}

//

inline static char json_scanner_peek(struct JSON_Scanner * scanner) {
	return *scanner->current;
}

inline static char json_scanner_peek_offset(struct JSON_Scanner * scanner, uint32_t offset) {
	return *(scanner->current + offset);
}

inline static char json_scanner_advance(struct JSON_Scanner * scanner) {
	return *(scanner->current++);
}

#define PEEK() json_scanner_peek(scanner)
#define PEEK_OFFSET(offset) json_scanner_peek_offset(scanner, offset)
#define ADVANCE() json_scanner_advance(scanner)

static void json_scanner_skip_whitespace(struct JSON_Scanner * scanner) {
	for (;;) {
		switch (PEEK()) {
			case ' ':
			case '\t':
			case '\r':
				ADVANCE();
				break;

			case '\n':
				ADVANCE();
				scanner->line_current++;
				break;

			default:
				return;
		}
	}
}

static struct JSON_Token json_scanner_make_token(struct JSON_Scanner * scanner, enum JSON_Token_Type type) {
	return (struct JSON_Token){
		.type = type,
		.length = (uint32_t)(scanner->current - scanner->start),
		.data = scanner->start,
		.line = scanner->line_start,
	};
}

static struct JSON_Token json_scanner_make_number_token(struct JSON_Scanner * scanner) {
	if (PEEK() == '-') { ADVANCE(); }
	while (parse_is_digit(PEEK())) { ADVANCE(); }

	if (PEEK() == '.') { ADVANCE();
		while (parse_is_digit(PEEK())) { ADVANCE(); }
	}

	if (PEEK() == 'e' || PEEK() == 'E') { ADVANCE();
		switch (PEEK()) {
			case '-': ADVANCE(); break;
			case '+': ADVANCE(); break;
		}
		while (parse_is_digit(PEEK())) { ADVANCE(); }
	}

	return json_scanner_make_token(scanner, JSON_TOKEN_NUMBER);
}

static enum JSON_Token_Type json_scanner_check_keyword(struct JSON_Scanner * scanner, uint32_t start, uint32_t length, char const * rest, enum JSON_Token_Type type) {
	if (scanner->current - scanner->start != start + length) { return JSON_TOKEN_IDENTIFIER; }
	if (memcmp(scanner->start + start, rest, length) != 0) { return JSON_TOKEN_IDENTIFIER; }
	return type;
}

static enum JSON_Token_Type json_scanner_identifier_type(struct JSON_Scanner * scanner) {
	switch (scanner->start[0]) {
		case 't': return json_scanner_check_keyword(scanner, 1, 3, "rue", JSON_TOKEN_TRUE);
		case 'f': return json_scanner_check_keyword(scanner, 1, 4, "alse", JSON_TOKEN_FALSE);
		case 'n': return json_scanner_check_keyword(scanner, 1, 3, "ull", JSON_TOKEN_NULL);
	}
	return JSON_TOKEN_IDENTIFIER;
}

static struct JSON_Token json_scanner_make_error_token(struct JSON_Scanner * scanner, char const * message) {
	return (struct JSON_Token) {
		.type = JSON_TOKEN_ERROR,
		.length = (uint32_t)strlen(message),
		.data = message,
		.line = scanner->line_start,
	};
}

static struct JSON_Token json_scanner_make_identifier_token(struct JSON_Scanner * scanner) {
	while (parse_is_alpha(PEEK()) || parse_is_digit(PEEK())) { ADVANCE(); }
	return json_scanner_make_token(scanner, json_scanner_identifier_type(scanner));
}

static struct JSON_Token json_scanner_make_string(struct JSON_Scanner * scanner) {
	while (PEEK() != '\0' && PEEK() != '\n' && PEEK() != '"') {
		if (PEEK() == '\\' && PEEK_OFFSET(1) == '"') {
			ADVANCE(); ADVANCE(); continue;
		}
		ADVANCE();
	}
	if (PEEK() != '"') { json_scanner_make_error_token(scanner, "unterminated string"); }
	ADVANCE();
	return json_scanner_make_token(scanner, JSON_TOKEN_STRING);
}

inline static struct JSON_Token json_scanner_next_internal(struct JSON_Scanner * scanner) {
	json_scanner_skip_whitespace(scanner);
	scanner->start = scanner->current;
	scanner->line_start = scanner->line_current;

	char const c = ADVANCE();
	switch (c) {
		case '/': if (PEEK() == '/') { ADVANCE();
			while (PEEK() != '\0' && PEEK() != '\n') { ADVANCE(); }
			return json_scanner_make_token(scanner, JSON_TOKEN_COMMENT);
		} break;

		case '\0': return json_scanner_make_token(scanner, JSON_TOKEN_EOF);
		case ':': return json_scanner_make_token(scanner, JSON_TOKEN_COLON);
		case '{': return json_scanner_make_token(scanner, JSON_TOKEN_LEFT_BRACE);
		case '}': return json_scanner_make_token(scanner, JSON_TOKEN_RIGHT_BRACE);
		case '[': return json_scanner_make_token(scanner, JSON_TOKEN_LEFT_SQUARE);
		case ']': return json_scanner_make_token(scanner, JSON_TOKEN_RIGHT_SQUARE);
		case ',': return json_scanner_make_token(scanner, JSON_TOKEN_COMMA);
		case '"': return json_scanner_make_string(scanner);
	}

	if (parse_is_alpha(c)) { return json_scanner_make_identifier_token(scanner); }
	if (c == '-' || parse_is_digit(c)) { return json_scanner_make_number_token(scanner); }

	return json_scanner_make_error_token(scanner, "unexpected character");
}

#undef PEEK
#undef PEEK_OFFSET
#undef ADVANCE
