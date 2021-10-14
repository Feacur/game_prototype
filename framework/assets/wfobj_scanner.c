#include "framework/memory.h"
#include "framework/parsing.h"

//
#include "wfobj_scanner.h"

void wfobj_scanner_init(struct WFObj_Scanner * scanner, char const * text) {
	*scanner = (struct WFObj_Scanner){
		.start = text,
		.current = text,
	};
}

void wfobj_scanner_free(struct WFObj_Scanner * scanner) {
	common_memset(scanner, 0, sizeof(*scanner));
}

inline static struct WFObj_Token wfobj_scanner_next_internal(struct WFObj_Scanner * scanner);
struct WFObj_Token wfobj_scanner_next(struct WFObj_Scanner * scanner) {
	return wfobj_scanner_next_internal(scanner);
}

//

inline static char wfobj_scanner_peek(struct WFObj_Scanner * scanner) {
	return *scanner->current;
}

inline static char wfobj_scanner_advance(struct WFObj_Scanner * scanner) {
	return *(scanner->current++);
}

#define PEEK() wfobj_scanner_peek(scanner)
#define ADVANCE() wfobj_scanner_advance(scanner)

static void wfobj_scanner_skip_whitespace(struct WFObj_Scanner * scanner) {
	for (;;) {
		switch (PEEK()) {
			case ' ':
			case '\t':
			case '\r':
				ADVANCE();
				break;

			default:
				return;
		}
	}
}

static struct WFObj_Token wfobj_scanner_make_token(struct WFObj_Scanner * scanner, enum WFObj_Token_Type type) {
	return (struct WFObj_Token){
		.type = type,
		.text = {
			.length = (uint32_t)(scanner->current - scanner->start),
			.data = scanner->start,
		},
		.line = scanner->line_start,
	};
}

static struct WFObj_Token wfobj_scanner_make_number_token(struct WFObj_Scanner * scanner) {
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

	return wfobj_scanner_make_token(scanner, WFOBJ_TOKEN_NUMBER);
}

static enum WFObj_Token_Type wfobj_scanner_check_keyword(struct WFObj_Scanner * scanner, uint32_t start, struct CString rest, enum WFObj_Token_Type type) {
	if (scanner->current - scanner->start != start + rest.length) { return WFOBJ_TOKEN_IDENTIFIER; }
	if (common_memcmp(scanner->start + start, rest.data, rest.length) != 0) { return WFOBJ_TOKEN_IDENTIFIER; }
	return type;
}

static enum WFObj_Token_Type wfobj_scanner_identifier_type(struct WFObj_Scanner * scanner) {
	switch (scanner->start[0]) {
		case 'f': return wfobj_scanner_check_keyword(scanner, 1, S_EMPTY, WFOBJ_TOKEN_FACE);
		case 'g': return wfobj_scanner_check_keyword(scanner, 1, S_EMPTY, WFOBJ_TOKEN_GROUP);
		case 'o': return wfobj_scanner_check_keyword(scanner, 1, S_EMPTY, WFOBJ_TOKEN_OBJECT);
		case 's': return wfobj_scanner_check_keyword(scanner, 1, S_EMPTY, WFOBJ_TOKEN_SMOOTH);
		case 'v':
			if (scanner->current - scanner->start > 1) {
				switch (scanner->start[1]) {
					case 't': return wfobj_scanner_check_keyword(scanner, 2, S_EMPTY, WFOBJ_TOKEN_TEXCOORD);
					case 'n': return wfobj_scanner_check_keyword(scanner, 2, S_EMPTY, WFOBJ_TOKEN_NORMAL);
				}
			}
			return wfobj_scanner_check_keyword(scanner, 1, S_EMPTY, WFOBJ_TOKEN_POSITION);
	}
	return WFOBJ_TOKEN_IDENTIFIER;
}

static struct WFObj_Token wfobj_scanner_make_error_token(struct WFObj_Scanner * scanner, struct CString message) {
	return (struct WFObj_Token) {
		.type = WFOBJ_TOKEN_ERROR,
		.text = message,
		.line = scanner->line_start,
	};
}

static struct WFObj_Token wfobj_scanner_make_identifier_token(struct WFObj_Scanner * scanner) {
	while (parse_is_alpha(PEEK()) || parse_is_digit(PEEK())) { ADVANCE(); }
	return wfobj_scanner_make_token(scanner, wfobj_scanner_identifier_type(scanner));
}

inline static struct WFObj_Token wfobj_scanner_next_internal(struct WFObj_Scanner * scanner) {
	wfobj_scanner_skip_whitespace(scanner);
	scanner->start = scanner->current;
	scanner->line_start = scanner->line_current;

	if (PEEK() == '\0') { return wfobj_scanner_make_token(scanner, WFOBJ_TOKEN_EOF); }

	char const c = ADVANCE();
	switch (c) {
		case '#':
			while (PEEK() != '\0' && PEEK() != '\n') { ADVANCE(); }
			return wfobj_scanner_make_token(scanner, WFOBJ_TOKEN_COMMENT);

		case '/': return wfobj_scanner_make_token(scanner, WFOBJ_TOKEN_SLASH);

		case '\n':
			scanner->line_current++;
			return wfobj_scanner_make_token(scanner, WFOBJ_TOKEN_NEW_LINE);
	}

	if (parse_is_alpha(c)) { return wfobj_scanner_make_identifier_token(scanner); }
	if (c == '-' || parse_is_digit(c)) { return wfobj_scanner_make_number_token(scanner); }

	DEBUG_BREAK();
	return wfobj_scanner_make_error_token(scanner, S_("unexpected character"));
}

#undef PEEK
#undef ADVANCE
