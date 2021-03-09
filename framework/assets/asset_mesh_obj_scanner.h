#if !defined(GAME_SCANNER_MESH_OBJ)
#define GAME_SCANNER_MESH_OBJ

#include "framework/common.h"

enum Mesh_Obj_Token_Type {
	MESH_OBJ_TOKEN_NONE,

	// single-character tokens
	MESH_OBJ_TOKEN_MINUS, MESH_OBJ_TOKEN_SLASH, MESH_OBJ_TOKEN_NEW_LINE,

	// literals
	MESH_OBJ_TOKEN_IDENTIFIER, MESH_OBJ_TOKEN_NUMBER,

	// keywords
	MESH_OBJ_TOKEN_POSITION, MESH_OBJ_TOKEN_TEXCOORD, MESH_OBJ_TOKEN_NORMAL,
	MESH_OBJ_TOKEN_FACE,

	//
	MESH_OBJ_TOKEN_ERROR,
	MESH_OBJ_TOKEN_EOF,
};

struct Mesh_Obj_Scanner {
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

void asset_mesh_obj_scanner_init(struct Mesh_Obj_Scanner * scanner, char const * text);
void asset_mesh_obj_scanner_free(struct Mesh_Obj_Scanner * scanner);

struct Mesh_Obj_Token asset_mesh_obj_scanner_next(struct Mesh_Obj_Scanner * scanner);

#endif
