#if !defined(GAME_ASSETS_WFOBJ_SCANNER)
#define GAME_ASSETS_WFOBJ_SCANNER

#include "framework/common.h"

enum WFObj_Token_Type {
	WFOBJ_TOKEN_NONE,

	// tokens
	WFOBJ_TOKEN_NEW_LINE,
	WFOBJ_TOKEN_COMMENT,
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

	//
	WFOBJ_TOKEN_ERROR,
	WFOBJ_TOKEN_EOF,
};

struct WFObj_Token {
	enum WFObj_Token_Type type;
	struct CString text;
	uint32_t line;
};

struct WFObj_Scanner {
	char const * start, * current;
	uint32_t line_start, line_current;
};

void wfobj_scanner_init(struct WFObj_Scanner * scanner, char const * text);
void wfobj_scanner_free(struct WFObj_Scanner * scanner);

struct WFObj_Token wfobj_scanner_next(struct WFObj_Scanner * scanner);

#endif
