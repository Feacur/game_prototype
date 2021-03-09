#include "framework/memory.h"
#include "parsing.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

//
#include "asset_mesh_obj.h"

inline static void asset_mesh_obj_init_internal(struct Asset_Mesh_Obj * obj, char const * text);
void asset_mesh_obj_init(struct Asset_Mesh_Obj * obj, char const * text) {
	asset_mesh_obj_init_internal(obj, text);
}

void asset_mesh_obj_free(struct Asset_Mesh_Obj * obj) {
	array_float_free(&obj->positions);
	array_float_free(&obj->texcoords);
	array_float_free(&obj->normals);
	array_u32_init(&obj->triangles);
}

//
#include "asset_mesh_obj_scanner.h"

static void asset_mesh_obj_error_at(struct Mesh_Obj_Token * token, char const * message) {
	DEBUG_BREAK();
	// if (parser->panic_mode) { return; }
	// parser->panic_mode = true;

	fprintf(stderr, "[line %d] error", token->line + 1);

	switch (token->type) {
		case MESH_OBJ_TOKEN_ERROR: break;
		case MESH_OBJ_TOKEN_EOF: fprintf(stderr, " at the end of file"); break;
		case MESH_OBJ_TOKEN_NEW_LINE: fprintf(stderr, " at the end of line"); break;
		default: fprintf(stderr, " at '%.*s'", token->length, token->start); break;
	}

	fprintf(stderr, ": %s\n", message);
	// parser->had_error = true;
}

static void asset_mesh_obj_advance(struct Mesh_Obj_Scanner * scanner, struct Mesh_Obj_Token * token) {
	for (;;) {
		*token = asset_mesh_obj_scanner_next(scanner);
		if (token->type != MESH_OBJ_TOKEN_ERROR) { break; }
		asset_mesh_obj_error_at(token, "scan error");
	}
}

static bool asset_mesh_obj_consume_float(struct Mesh_Obj_Scanner * scanner, struct Mesh_Obj_Token * token, float * value) {
#define ADVANCE() asset_mesh_obj_advance(scanner, token)

	bool negative = (token->type == MESH_OBJ_TOKEN_MINUS);
	if (negative) { ADVANCE(); }

	if (token->type == MESH_OBJ_TOKEN_NUMBER) {
		// *value = strtof(token->start, NULL) * (1 - negative * 2);
		*value = parse_float_positive(token->start) * (1 - negative * 2);
		ADVANCE();
		return true;
	}

	asset_mesh_obj_error_at(token, "expected a number");
	return false;

#undef ADVANCE
}

static bool asset_mesh_obj_consume_s32(struct Mesh_Obj_Scanner * scanner, struct Mesh_Obj_Token * token, int32_t * value) {
#define ADVANCE() asset_mesh_obj_advance(scanner, token)
	bool negative = (token->type == MESH_OBJ_TOKEN_MINUS);
	if (negative) { ADVANCE(); }

	if (token->type == MESH_OBJ_TOKEN_NUMBER) {
		// *value = (int32_t)strtoul(token->start, NULL, 10) * (1 - negative * 2);
		*value = (int32_t)parse_u32(token->start) * (1 - negative * 2);
		ADVANCE();
		return true;
	}

	asset_mesh_obj_error_at(token, "expected a number");
	return false;

#undef ADVANCE
}

static void asset_mesh_obj_do_vertex(struct Mesh_Obj_Scanner * scanner, struct Mesh_Obj_Token * token, struct Array_Float * buffer, uint32_t limit) {
#define ADVANCE() asset_mesh_obj_advance(scanner, token)

	uint32_t entries = 0;
	while (token->type != MESH_OBJ_TOKEN_EOF && token->type != MESH_OBJ_TOKEN_NEW_LINE) {
		if (entries >= limit) { asset_mesh_obj_error_at(token, "expected less elements"); }

		float value;
		if (asset_mesh_obj_consume_float(scanner, token, &value)) {
			if (entries >= limit) { continue; } entries++;
			array_float_write(buffer, value);
		}
		else { ADVANCE(); }
	}

	if (entries < limit) { asset_mesh_obj_error_at(token, "expected more elements"); }

#undef ADVANCE
}

inline static uint32_t asset_mesh_obj_translate_index(int32_t value, uint32_t base) {
	return (value > 0) ? (uint32_t)(value - 1) : (uint32_t)((int32_t)base + value);
}

static void asset_mesh_obj_do_faces(
	struct Mesh_Obj_Scanner * scanner, struct Mesh_Obj_Token * token, struct Array_U32 * buffer,
	uint32_t positions_count, uint32_t texcoords_count, uint32_t normals_count
) {
#define ADVANCE() asset_mesh_obj_advance(scanner, token)

	while (token->type != MESH_OBJ_TOKEN_EOF && token->type != MESH_OBJ_TOKEN_NEW_LINE) {
		int32_t value;
		uint32_t face[3] = {0};

		// face format: position
		asset_mesh_obj_consume_s32(scanner, token, &value);
		face[0] = asset_mesh_obj_translate_index(value, positions_count);

		if (token->type != MESH_OBJ_TOKEN_SLASH) {
			array_u32_write_many(buffer, 3, face);
			continue;
		}

		// face format: position//normal
		ADVANCE();
		if (token->type == MESH_OBJ_TOKEN_SLASH) {
			ADVANCE();

			asset_mesh_obj_consume_s32(scanner, token, &value);
			face[2] = asset_mesh_obj_translate_index(value, normals_count);

			array_u32_write_many(buffer, 3, face);
			continue;
		}

		// face format: position/texcoord
		asset_mesh_obj_consume_s32(scanner, token, &value);
		face[1] = asset_mesh_obj_translate_index(value, texcoords_count);

		if (token->type != MESH_OBJ_TOKEN_SLASH) {
			array_u32_write_many(buffer, 3, face);
			continue;
		}

		// face format: position/texcoord/normal
		asset_mesh_obj_consume_s32(scanner, token, &value);
		face[2] = asset_mesh_obj_translate_index(value, normals_count);

		array_u32_write_many(buffer, 3, face);
	}

#undef ADVANCE
}

inline static void asset_mesh_obj_init_internal(struct Asset_Mesh_Obj * obj, char const * text) {
#define ADVANCE() asset_mesh_obj_advance(&scanner, &token)

	struct Mesh_Obj_Scanner scanner;
	struct Mesh_Obj_Token token;

	//
	uint32_t position_lines = 0;
	uint32_t texcoord_lines = 0;
	uint32_t normal_lines = 0;
	uint32_t face_lines = 0;

	asset_mesh_obj_scanner_init(&scanner, text); ADVANCE();
	while (token.type != MESH_OBJ_TOKEN_EOF) {
		switch (token.type) {
			default: break;

			case MESH_OBJ_TOKEN_POSITION: position_lines++; break;
			case MESH_OBJ_TOKEN_TEXCOORD: texcoord_lines++; break;
			case MESH_OBJ_TOKEN_NORMAL: normal_lines++; break;
			case MESH_OBJ_TOKEN_FACE: face_lines++; break;
		}
		ADVANCE();
	}

	array_float_init(&obj->positions);
	array_float_init(&obj->texcoords);
	array_float_init(&obj->normals);
	array_u32_init(&obj->triangles);

	array_float_resize(&obj->positions, position_lines * 3);
	array_float_resize(&obj->texcoords, texcoord_lines * 2);
	array_float_resize(&obj->normals, normal_lines * 3);
	array_u32_resize(&obj->triangles, face_lines * 3 * 2);

	//
	struct Array_U32 scratch_u32;
	array_u32_init(&scratch_u32);
	array_u32_resize(&scratch_u32, 3 * 4);

	asset_mesh_obj_scanner_init(&scanner, text); ADVANCE();
	while (token.type != MESH_OBJ_TOKEN_EOF) {
		switch (token.type) {
			// silent
			case MESH_OBJ_TOKEN_NEW_LINE: break;

			// errors
			case MESH_OBJ_TOKEN_IDENTIFIER: asset_mesh_obj_error_at(&token, "unknown identifier"); break;
			case MESH_OBJ_TOKEN_ERROR: asset_mesh_obj_error_at(&token, "scan error"); break;
			default: asset_mesh_obj_error_at(&token, "unhandled input"); break;

			// valid
			case MESH_OBJ_TOKEN_POSITION: { ADVANCE();
				asset_mesh_obj_do_vertex(&scanner, &token, &obj->positions, 3);
				break;
			}

			case MESH_OBJ_TOKEN_TEXCOORD: { ADVANCE();
				asset_mesh_obj_do_vertex(&scanner, &token, &obj->texcoords, 2);
				break;
			}

			case MESH_OBJ_TOKEN_NORMAL: { ADVANCE();
				asset_mesh_obj_do_vertex(&scanner, &token, &obj->normals, 3);
				break;
			}

			case MESH_OBJ_TOKEN_FACE: { ADVANCE();
				scratch_u32.count = 0;
				asset_mesh_obj_do_faces(
					&scanner, &token, &scratch_u32,
					obj->positions.count, obj->texcoords.count, obj->normals.count
				);

				uint32_t indices_count = scratch_u32.count / 3;
				for (uint32_t i = 2; i < indices_count; i++) {
					array_u32_write_many(&obj->triangles, 3, scratch_u32.data + 0);
					array_u32_write_many(&obj->triangles, 3, scratch_u32.data + (i - 1) * 3);
					array_u32_write_many(&obj->triangles, 3, scratch_u32.data + i * 3);
				}
				break;
			}
		}
		ADVANCE();
	}

	array_u32_free(&scratch_u32);

#undef ADVANCE
}
