#include "framework/parsing.h"
#include "framework/systems/memory.h"


//
#include "wfobj_lexer.h"

struct WFObj_Lexer wfobj_lexer_init(struct CString text) {
	if (cstring_empty(text)) { text = S_(""); }
	return (struct WFObj_Lexer){
		.start = text.data,
		.current = text.data,
	};
}

void wfobj_lexer_free(struct WFObj_Lexer * lexer) {
	zero_out(AMP_(lexer));
}

inline static struct WFObj_Token wfobj_lexer_next_internal(struct WFObj_Lexer * lexer);
struct WFObj_Token wfobj_lexer_next(struct WFObj_Lexer * lexer) {
	return wfobj_lexer_next_internal(lexer);
}

//

inline static char wfobj_lexer_peek(struct WFObj_Lexer * lexer) {
	return *lexer->current;
}

inline static char wfobj_lexer_advance(struct WFObj_Lexer * lexer) {
	return *(lexer->current++);
}

#define PEEK() wfobj_lexer_peek(lexer)
#define ADVANCE() wfobj_lexer_advance(lexer)

static void wfobj_lexer_skip_whitespace(struct WFObj_Lexer * lexer) {
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

static struct WFObj_Token wfobj_lexer_make_token(struct WFObj_Lexer * lexer, enum WFObj_Token_Type type) {
	return (struct WFObj_Token){
		.type = type,
		.text = {
			.length = (uint32_t)(lexer->current - lexer->start),
			.data = lexer->start,
		},
		.line = lexer->line_start,
	};
}

static struct WFObj_Token wfobj_lexer_make_number_token(struct WFObj_Lexer * lexer) {
	if (PEEK() == '-') { ADVANCE(); }
	while (is_digit(PEEK())) { ADVANCE(); }

	if (PEEK() == '.') { ADVANCE();
		while (is_digit(PEEK())) { ADVANCE(); }
	}

	if (PEEK() == 'e' || PEEK() == 'E') { ADVANCE();
		switch (PEEK()) {
			case '-': ADVANCE(); break;
			case '+': ADVANCE(); break;
		}
		while (is_digit(PEEK())) { ADVANCE(); }
	}

	return wfobj_lexer_make_token(lexer, WFOBJ_TOKEN_NUMBER);
}

static enum WFObj_Token_Type wfobj_lexer_check_keyword(struct WFObj_Lexer * lexer, uint32_t offset, struct CString rest, enum WFObj_Token_Type type) {
	struct CString const word = {
		.length = (uint32_t)(lexer->current - lexer->start - offset),
		.data = lexer->start + offset,
	};
	return cstring_equals(word, rest) ? type
		: WFOBJ_TOKEN_IDENTIFIER;
}

static enum WFObj_Token_Type wfobj_lexer_identifier_type(struct WFObj_Lexer * lexer) {
	switch (lexer->start[0]) {
		case 'f': return wfobj_lexer_check_keyword(lexer, 1, S_(""), WFOBJ_TOKEN_FACE);
		case 'g': return wfobj_lexer_check_keyword(lexer, 1, S_(""), WFOBJ_TOKEN_GROUP);
		case 'o': return wfobj_lexer_check_keyword(lexer, 1, S_(""), WFOBJ_TOKEN_OBJECT);
		case 's': return wfobj_lexer_check_keyword(lexer, 1, S_(""), WFOBJ_TOKEN_SMOOTH);
		case 'v':
			if (lexer->current - lexer->start > 1) {
				switch (lexer->start[1]) {
					case 't': return wfobj_lexer_check_keyword(lexer, 2, S_(""), WFOBJ_TOKEN_TEXCOORD);
					case 'n': return wfobj_lexer_check_keyword(lexer, 2, S_(""), WFOBJ_TOKEN_NORMAL);
				}
			}
			return wfobj_lexer_check_keyword(lexer, 1, S_(""), WFOBJ_TOKEN_POSITION);
	}
	return WFOBJ_TOKEN_IDENTIFIER;
}

static struct WFObj_Token wfobj_lexer_make_identifier_token(struct WFObj_Lexer * lexer) {
	while (is_alpha(PEEK()) || is_digit(PEEK())) { ADVANCE(); }
	return wfobj_lexer_make_token(lexer, wfobj_lexer_identifier_type(lexer));
}

inline static struct WFObj_Token wfobj_lexer_next_internal(struct WFObj_Lexer * lexer) {
	wfobj_lexer_skip_whitespace(lexer);
	lexer->start = lexer->current;
	lexer->line_start = lexer->line_current;

	if (PEEK() == '\0') { return wfobj_lexer_make_token(lexer, WFOBJ_TOKEN_EOF); }

	char const c = ADVANCE();
	switch (c) {
		case '#': while (ADVANCE() != '\0') {
			if (PEEK() == '\n') { lexer->line_current++; break; }
		}
		return wfobj_lexer_make_token(lexer, WFOBJ_TOKEN_COMMENT);

		case '/': return wfobj_lexer_make_token(lexer, WFOBJ_TOKEN_SLASH);

		case '\n':
			lexer->line_current++;
			return wfobj_lexer_make_token(lexer, WFOBJ_TOKEN_NEW_LINE);
	}

	if (is_alpha(c)) { return wfobj_lexer_make_identifier_token(lexer); }
	if (c == '-' || is_digit(c)) { return wfobj_lexer_make_number_token(lexer); }

	REPORT_CALLSTACK(); DEBUG_BREAK();
	return wfobj_lexer_make_token(lexer, WFOBJ_TOKEN_ERROR_UNKNOWN_CHARACTER);
}

#undef PEEK
#undef ADVANCE
