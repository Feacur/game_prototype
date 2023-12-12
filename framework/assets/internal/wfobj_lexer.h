#if !defined(FRAMEWORK_ASSETS_WFOBJ_LEXER)
#define FRAMEWORK_ASSETS_WFOBJ_LEXER

#include "framework/common.h"

enum WFObj_Token_Type {
	WFOBJ_TOKEN_NONE,

	// tokens
	WFOBJ_TOKEN_COMMENT,
	WFOBJ_TOKEN_NEW_LINE,
	WFOBJ_TOKEN_SLASH,

	// literals
	WFOBJ_TOKEN_IDENTIFIER,
	WFOBJ_TOKEN_NUMBER,

	// keywords
	WFOBJ_TOKEN_POSITION,
	WFOBJ_TOKEN_TEXCOORD,
	WFOBJ_TOKEN_NORMAL,
	WFOBJ_TOKEN_FACE,
	WFOBJ_TOKEN_SMOOTH,
	WFOBJ_TOKEN_OBJECT,
	WFOBJ_TOKEN_GROUP,

	// errors
	WFOBJ_TOKEN_ERROR_UNKNOWN_CHARACTER,

	//
	WFOBJ_TOKEN_EOF,
};

struct WFObj_Token {
	enum WFObj_Token_Type type;
	struct CString text;
	uint32_t line;
};

struct WFObj_Lexer {
	char const * start, * current;
	uint32_t line_start, line_current;
};

struct WFObj_Lexer wfobj_lexer_init(struct CString text);
void wfobj_lexer_free(struct WFObj_Lexer * lexer);

struct WFObj_Token wfobj_lexer_next(struct WFObj_Lexer * lexer);

#endif
