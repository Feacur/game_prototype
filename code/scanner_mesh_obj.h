#if !defined(GAME_SCANNER_MESH_OBJ)
#define GAME_SCANNER_MESH_OBJ

#include "common.h"

enum Mesh_Obj_Token_Type {
	MESH_OBJ_TOKEN_NONE,

	// single-character tokens
	MESH_OBJ_TOKEN_SLASH, MESH_OBJ_TOKEN_NEW_LINE,

	// literals
	MESH_OBJ_TOKEN_NUMBER,

	// keywords
	MESH_OBJ_TOKEN_POSITION, MESH_OBJ_TOKEN_TEXCOORD, MESH_OBJ_TOKEN_NORMAL,

	//
	MESH_OBJ_TOKEN_ERROR,
	MESH_OBJ_TOKEN_EOF,
};

struct Scanner_Mesh_Obj {
	char const * start;
	char const * current;
	uint32_t line;
};

struct Mesh_Obj_Token {
	enum Mesh_Obj_Token_Type type;
	char const * start;
	uint32_t length;
	uint32_t line;
};

void scanner_mesh_obj_init(struct Scanner_Mesh_Obj * scanner, char const * text);
void scanner_mesh_obj_free(struct Scanner_Mesh_Obj * scanner);

struct Mesh_Obj_Token scanner_mesh_obj_next(struct Scanner_Mesh_Obj * scanner);

#endif
