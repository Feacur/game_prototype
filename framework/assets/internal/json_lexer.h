#if !defined(FRAMEWORK_ASSETS_JSON_LEXER)
#define FRAMEWORK_ASSETS_JSON_LEXER

#include "framework/common.h"

enum JSON_Token_Type {
	JSON_TOKEN_NONE,

	// tokens
	JSON_TOKEN_COMMENT,
	JSON_TOKEN_COLON,
	JSON_TOKEN_LEFT_BRACE, JSON_TOKEN_RIGHT_BRACE,
	JSON_TOKEN_LEFT_SQUARE, JSON_TOKEN_RIGHT_SQUARE,
	JSON_TOKEN_COMMA,

	// literals
	JSON_TOKEN_STRING,
	JSON_TOKEN_NUMBER,

	// keywords
	JSON_TOKEN_TRUE,
	JSON_TOKEN_FALSE,
	JSON_TOKEN_NULL,

	// errors
	JSON_TOKEN_ERROR_IDENTIFIER,
	JSON_TOKEN_ERROR_UNKNOWN_CHARACTER,
	JSON_TOKEN_ERROR_UNTERMINATED_STRING,
	JSON_TOKEN_ERROR_UNESCAPED_CONTROL,
	JSON_TOKEN_ERROR_MALFORMED_UNICODE,

	//
	JSON_TOKEN_EOF,
};

struct JSON_Token {
	enum JSON_Token_Type type;
	struct CString text;
	uint32_t line;
};

struct JSON_Lexer {
	char const * start, * current;
	uint32_t line_start, line_current;
};

struct JSON_Lexer json_lexer_init(struct CString text);
void json_lexer_free(struct JSON_Lexer * lexer);

struct JSON_Token json_lexer_next(struct JSON_Lexer * lexer);

#endif
