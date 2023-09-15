#include "framework/memory.h"
#include "framework/parsing.h"

//
#include "json_lexer.h"

struct JSON_Lexer json_lexer_init(char const * text) {
	if (text == NULL) { text = ""; }
	return (struct JSON_Lexer){
		.start = text,
		.current = text,
	};
}

void json_lexer_free(struct JSON_Lexer * lexer) {
	common_memset(lexer, 0, sizeof(*lexer));
}

inline static struct JSON_Token json_lexer_next_internal(struct JSON_Lexer * lexer);
struct JSON_Token json_lexer_next(struct JSON_Lexer * lexer) {
	return json_lexer_next_internal(lexer);
}

//

inline static char json_lexer_peek(struct JSON_Lexer * lexer) {
	return *lexer->current;
}

inline static char json_lexer_peek_offset(struct JSON_Lexer * lexer, uint32_t offset) {
	return *(lexer->current + offset);
}

inline static char json_lexer_advance(struct JSON_Lexer * lexer) {
	return *(lexer->current++);
}

#define PEEK() json_lexer_peek(lexer)
#define PEEK_OFFSET(offset) json_lexer_peek_offset(lexer, offset)
#define ADVANCE() json_lexer_advance(lexer)

static void json_lexer_skip_whitespace(struct JSON_Lexer * lexer) {
	for (;;) {
		switch (PEEK()) {
			case ' ':
			case '\t':
			case '\r':
				ADVANCE();
				break;

			case '\n':
				ADVANCE();
				lexer->line_current++;
				break;

			default:
				return;
		}
	}
}

static struct JSON_Token json_lexer_make_token(struct JSON_Lexer * lexer, enum JSON_Token_Type type) {
	return (struct JSON_Token){
		.type = type,
		.text = {
			.length = (uint32_t)(lexer->current - lexer->start),
			.data = lexer->start,
		},
		.line = lexer->line_start,
	};
}

static struct JSON_Token json_lexer_make_number_token(struct JSON_Lexer * lexer) {
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

	return json_lexer_make_token(lexer, JSON_TOKEN_NUMBER);
}

static enum JSON_Token_Type json_lexer_check_keyword(struct JSON_Lexer * lexer, uint32_t start, struct CString rest, enum JSON_Token_Type type) {
	if (lexer->current - lexer->start != start + rest.length) { return JSON_TOKEN_ERROR_IDENTIFIER; }
	if (common_memcmp(lexer->start + start, rest.data, rest.length) != 0) { return JSON_TOKEN_ERROR_IDENTIFIER; }
	return type;
}

static enum JSON_Token_Type json_lexer_identifier_type(struct JSON_Lexer * lexer) {
	switch (lexer->start[0]) {
		case 't': return json_lexer_check_keyword(lexer, 1, S_("rue"), JSON_TOKEN_TRUE);
		case 'f': return json_lexer_check_keyword(lexer, 1, S_("alse"), JSON_TOKEN_FALSE);
		case 'n': return json_lexer_check_keyword(lexer, 1, S_("ull"), JSON_TOKEN_NULL);
	}
	return JSON_TOKEN_ERROR_IDENTIFIER;
}

static struct JSON_Token json_lexer_make_identifier_token(struct JSON_Lexer * lexer) {
	while (parse_is_alpha(PEEK()) || parse_is_digit(PEEK())) { ADVANCE(); }
	return json_lexer_make_token(lexer, json_lexer_identifier_type(lexer));
}

static struct JSON_Token json_lexer_make_string(struct JSON_Lexer * lexer) {
	while (ADVANCE() != '\0') {
		if (PEEK() == '\n') { lexer->line_current++; break; }
		if (PEEK() == '"') { ADVANCE(); return json_lexer_make_token(lexer, JSON_TOKEN_STRING); }
		if (PEEK() == '\\') {
			switch (ADVANCE(), PEEK()) {
				case '"':
				case '\\':
				case '/':
				case 'b':
				case 'f':
				case 'n':
				case 'r':
				case 't':
					continue;

				case 'u':
					if (!parse_is_hex(PEEK_OFFSET(1))) { return json_lexer_make_token(lexer, JSON_TOKEN_ERROR_MALFORMED_UNICODE); }
					if (!parse_is_hex(PEEK_OFFSET(2))) { return json_lexer_make_token(lexer, JSON_TOKEN_ERROR_MALFORMED_UNICODE); }
					if (!parse_is_hex(PEEK_OFFSET(3))) { return json_lexer_make_token(lexer, JSON_TOKEN_ERROR_MALFORMED_UNICODE); }
					if (!parse_is_hex(PEEK_OFFSET(4))) { return json_lexer_make_token(lexer, JSON_TOKEN_ERROR_MALFORMED_UNICODE); }
					ADVANCE(); ADVANCE(); ADVANCE(); ADVANCE();
					continue;
			}
			return json_lexer_make_token(lexer, JSON_TOKEN_ERROR_UNESCAPED_CONTROL);
		}
	}
	return json_lexer_make_token(lexer, JSON_TOKEN_ERROR_UNTERMINATED_STRING);
}

inline static struct JSON_Token json_lexer_next_internal(struct JSON_Lexer * lexer) {
	json_lexer_skip_whitespace(lexer);
	lexer->start = lexer->current;
	lexer->line_start = lexer->line_current;

	if (PEEK() == '\0') { return json_lexer_make_token(lexer, JSON_TOKEN_EOF); }

	char const c = ADVANCE();
	switch (c) {
		case '/': {
			switch (PEEK()) {
				case '/': while (ADVANCE() != '\0') {
					if (PEEK() == '\n') { lexer->line_current++; break; }
				}
				return json_lexer_make_token(lexer, JSON_TOKEN_COMMENT);

				case '*': while (ADVANCE() != '\0') {
					if (PEEK() == '\n') { lexer->line_current++; }
					if (PEEK() == '*' && PEEK_OFFSET(1) == '/') { ADVANCE(); ADVANCE(); break; }
				}
				return json_lexer_make_token(lexer, JSON_TOKEN_COMMENT);
			}
		} break;

		case ':': return json_lexer_make_token(lexer, JSON_TOKEN_COLON);
		case '{': return json_lexer_make_token(lexer, JSON_TOKEN_LEFT_BRACE);
		case '}': return json_lexer_make_token(lexer, JSON_TOKEN_RIGHT_BRACE);
		case '[': return json_lexer_make_token(lexer, JSON_TOKEN_LEFT_SQUARE);
		case ']': return json_lexer_make_token(lexer, JSON_TOKEN_RIGHT_SQUARE);
		case ',': return json_lexer_make_token(lexer, JSON_TOKEN_COMMA);
		case '"': return json_lexer_make_string(lexer);
	}

	if (parse_is_alpha(c)) { return json_lexer_make_identifier_token(lexer); }
	if (c == '-' || parse_is_digit(c)) { return json_lexer_make_number_token(lexer); }

	return json_lexer_make_token(lexer, JSON_TOKEN_ERROR_UNKNOWN_CHARACTER);
}

#undef PEEK
#undef PEEK_OFFSET
#undef ADVANCE