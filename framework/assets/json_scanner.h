#if !defined(GAME_ASSETS_JSON_SCANNER)
#define GAME_ASSETS_JSON_SCANNER

#include "framework/common.h"

enum JSON_Token_Type {
	JSON_TOKEN_NONE,

	// tokens
	JSON_TOKEN_COMMENT,
	JSON_TOKEN_LEFT_BRACE, JSON_TOKEN_RIGHT_BRACE,
	JSON_TOKEN_LEFT_SQUARE, JSON_TOKEN_RIGHT_SQUARE,
	JSON_TOKEN_COMMA,

	// literals
	JSON_TOKEN_IDENTIFIER,
	JSON_TOKEN_NUMBER,
	JSON_TOKEN_STRING,

	// keywords
	JSON_TOKEN_TRUE,
	JSON_TOKEN_FALSE,
	JSON_TOKEN_NULL,

	//
	JSON_TOKEN_ERROR,
	JSON_TOKEN_EOF,
};

struct JSON_Token {
	enum JSON_Token_Type type;
	char const * start;
	uint32_t length;
	uint32_t line;
};

struct JSON_Scanner {
	char const * start;
	char const * current;
	uint32_t line_start, line_current;
};

void json_scanner_init(struct JSON_Scanner * scanner, char const * text);
void json_scanner_free(struct JSON_Scanner * scanner);

struct JSON_Token json_scanner_next(struct JSON_Scanner * scanner);

#endif
