#include "framework/memory.h"
#include "parsing.h"

#include <string.h>

//
#include "mesh_obj_scanner.h"

void asset_mesh_obj_scanner_init(struct Mesh_Obj_Scanner * scanner, char const * text) {
	*scanner = (struct Mesh_Obj_Scanner){
		.start = text,
		.current = text,
		.line = 0,
	};
}

void asset_mesh_obj_scanner_free(struct Mesh_Obj_Scanner * scanner) {
	asset_mesh_obj_scanner_init(scanner, NULL);
}

inline static struct Mesh_Obj_Token asset_mesh_obj_scanner_next_internal(struct Mesh_Obj_Scanner * scanner);
struct Mesh_Obj_Token asset_mesh_obj_scanner_next(struct Mesh_Obj_Scanner * scanner) {
	return asset_mesh_obj_scanner_next_internal(scanner);
}

//

inline static char asset_mesh_obj_scanner_peek(struct Mesh_Obj_Scanner * scanner) {
	return *scanner->current;
}

inline static void asset_mesh_obj_scanner_advance(struct Mesh_Obj_Scanner * scanner) {
	scanner->current++;
}

inline static char asset_mesh_obj_scanner_is_at_end(struct Mesh_Obj_Scanner * scanner) {
	return *scanner->current == '\0';
}

#define PEEK() asset_mesh_obj_scanner_peek(scanner)
#define ADVANCE() asset_mesh_obj_scanner_advance(scanner)
#define IS_AT_END() asset_mesh_obj_scanner_is_at_end(scanner)

static struct Mesh_Obj_Token asset_mesh_obj_scanner_make_token(struct Mesh_Obj_Scanner * scanner, enum Mesh_Obj_Token_Type type) {
	return (struct Mesh_Obj_Token){
		.type = type,
		.start = scanner->start,
		.length = (uint32_t)(scanner->current - scanner->start),
		.line = scanner->line,
	};
}

static struct Mesh_Obj_Token asset_mesh_obj_scanner_make_new_line_token(struct Mesh_Obj_Scanner * scanner) {
	struct Mesh_Obj_Token token = asset_mesh_obj_scanner_make_token(scanner, MESH_OBJ_TOKEN_NEW_LINE);
	token.line--;
	return token;
}

static struct Mesh_Obj_Token asset_mesh_obj_scanner_make_number_token(struct Mesh_Obj_Scanner * scanner) {
	if (PEEK() == '-') { ADVANCE(); }
	while (is_digit(PEEK())) { ADVANCE(); }

	if (PEEK() == '.') {
		ADVANCE();
		while (is_digit(PEEK())) { ADVANCE(); }
	}

	if (PEEK() == 'e' || PEEK() == 'E') {
		ADVANCE();
		if (PEEK() == '-') { ADVANCE(); }
		while (is_digit(PEEK())) { ADVANCE(); }
	}

	return asset_mesh_obj_scanner_make_token(scanner, MESH_OBJ_TOKEN_NUMBER);
}

static enum Mesh_Obj_Token_Type asset_mesh_obj_scanner_check_keyword(struct Mesh_Obj_Scanner * scanner, uint32_t start, uint32_t length, char const * rest, enum Mesh_Obj_Token_Type type) {
	if (scanner->current - scanner->start != start + length) { return MESH_OBJ_TOKEN_IDENTIFIER; }
	if (memcmp(scanner->start + start, rest, length) != 0) { return MESH_OBJ_TOKEN_IDENTIFIER; }
	return type;
}

static enum Mesh_Obj_Token_Type asset_mesh_obj_scanner_identifier_type(struct Mesh_Obj_Scanner * scanner) {
	switch (scanner->start[0]) {
		case 'f': return asset_mesh_obj_scanner_check_keyword(scanner, 1, 0, "", MESH_OBJ_TOKEN_FACE);
		case 'g': return asset_mesh_obj_scanner_check_keyword(scanner, 1, 0, "", MESH_OBJ_TOKEN_GROUP);
		case 'o': return asset_mesh_obj_scanner_check_keyword(scanner, 1, 0, "", MESH_OBJ_TOKEN_OBJECT);
		case 's': return asset_mesh_obj_scanner_check_keyword(scanner, 1, 0, "", MESH_OBJ_TOKEN_SMOOTH);
		case 'v':
			if (scanner->current - scanner->start > 1) {
				switch (scanner->start[1]) {
					case 't': return asset_mesh_obj_scanner_check_keyword(scanner, 2, 0, "", MESH_OBJ_TOKEN_TEXCOORD);
					case 'n': return asset_mesh_obj_scanner_check_keyword(scanner, 2, 0, "", MESH_OBJ_TOKEN_NORMAL);
				}
			}
			return asset_mesh_obj_scanner_check_keyword(scanner, 1, 0, "", MESH_OBJ_TOKEN_POSITION);
	}
	return MESH_OBJ_TOKEN_IDENTIFIER;
}

static struct Mesh_Obj_Token asset_mesh_obj_scanner_make_error_token(struct Mesh_Obj_Scanner * scanner, char const * message) {
	return (struct Mesh_Obj_Token) {
		.type = MESH_OBJ_TOKEN_ERROR,
		.start = message,
		.length = (uint32_t)strlen(message),
		.line = scanner->line,
	};
}

static struct Mesh_Obj_Token asset_mesh_obj_scanner_make_identifier_token(struct Mesh_Obj_Scanner * scanner) {
	while (is_alpha(PEEK()) || is_digit(PEEK())) { ADVANCE(); }
	return asset_mesh_obj_scanner_make_token(scanner, asset_mesh_obj_scanner_identifier_type(scanner));
}

inline static struct Mesh_Obj_Token asset_mesh_obj_scanner_next_internal(struct Mesh_Obj_Scanner * scanner) {
	scanner->current = parse_whitespace(scanner->current);
	scanner->start = scanner->current;

	if (PEEK() == '#') {
		while (PEEK() != '\0' && PEEK() != '\n') { ADVANCE(); }
		scanner->start = scanner->current;
	}

	if (IS_AT_END()) { return asset_mesh_obj_scanner_make_token(scanner, MESH_OBJ_TOKEN_EOF); }

	if (is_alpha(PEEK())) { return asset_mesh_obj_scanner_make_identifier_token(scanner); }
	if (is_digit(PEEK())) { return asset_mesh_obj_scanner_make_number_token(scanner); }

	switch (PEEK()) {
		case '.': ADVANCE();
			return asset_mesh_obj_scanner_make_token(scanner, MESH_OBJ_TOKEN_DOT);
		case '-': ADVANCE();
			return asset_mesh_obj_scanner_make_token(scanner, MESH_OBJ_TOKEN_MINUS);
		case '/': ADVANCE();
			return asset_mesh_obj_scanner_make_token(scanner, MESH_OBJ_TOKEN_SLASH);
		case '\n': ADVANCE(); scanner->line++;
			return asset_mesh_obj_scanner_make_new_line_token(scanner);
	}

	return asset_mesh_obj_scanner_make_error_token(scanner, "unexpected character");
}

#undef PEEK
#undef ADVANCE
#undef IS_AT_END
