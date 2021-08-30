#include "framework/memory.h"
#include "framework/logger.h"
#include "framework/parsing.h"

#include "wfobj_scanner.h"

//
#include "wfobj.h"

inline static void wfobj_init_internal(struct WFObj * obj, char const * text);
void wfobj_init(struct WFObj * obj, char const * text) {
	wfobj_init_internal(obj, text);
}

void wfobj_free(struct WFObj * obj) {
	array_float_free(&obj->positions);
	array_float_free(&obj->texcoords);
	array_float_free(&obj->normals);
	array_u32_free(&obj->triangles);
}

//

static void wfobj_error_at(struct WFObj_Token * token, char const * message) {
	DEBUG_BREAK();
	// if (parser->panic_mode) { return; }
	// parser->panic_mode = true;

	logger_to_console("[line %u] error", token->line + 1);

	switch (token->type) {
		case WFOBJ_TOKEN_ERROR: break;
		case WFOBJ_TOKEN_EOF: logger_to_console(" at the end of file"); break;
		case WFOBJ_TOKEN_NEW_LINE: logger_to_console(" at the end of line"); break;
		default: logger_to_console(" at '%.*s'", token->length, token->data); break;
	}

	logger_to_console(": %s\n", message);
	// parser->had_error = true;
}

static void wfobj_advance(struct WFObj_Scanner * scanner, struct WFObj_Token * token) {
	for (;;) {
		*token = wfobj_scanner_next(scanner);
		if (token->type != WFOBJ_TOKEN_ERROR) { break; }
		wfobj_error_at(token, "scan error");
	}
}

static bool wfobj_consume_float(struct WFObj_Scanner * scanner, struct WFObj_Token * token, float * value) {
#define ADVANCE() wfobj_advance(scanner, token)

	if (token->type == WFOBJ_TOKEN_NUMBER) {
		// *value = strtof(token->start, NULL);
		*value = parse_float(token->data);
		ADVANCE();
		return true;
	}

	wfobj_error_at(token, "expected a number");
	return false;

#undef ADVANCE
}

static bool wfobj_consume_s32(struct WFObj_Scanner * scanner, struct WFObj_Token * token, int32_t * value) {
#define ADVANCE() wfobj_advance(scanner, token)

	if (token->type == WFOBJ_TOKEN_NUMBER) {
		// *value = (int32_t)strtoul(token->start, NULL, 10);
		*value = parse_s32(token->data);
		ADVANCE();
		return true;
	}

	wfobj_error_at(token, "expected a number");
	return false;

#undef ADVANCE
}

static void wfobj_do_name(struct WFObj_Scanner * scanner, struct WFObj_Token * token) {
	while (token->type != WFOBJ_TOKEN_EOF && token->type != WFOBJ_TOKEN_NEW_LINE) {
		wfobj_advance(scanner, token);
	}
}

static void wfobj_do_vertex(struct WFObj_Scanner * scanner, struct WFObj_Token * token, struct Array_Float * buffer, uint32_t limit) {
#define ADVANCE() wfobj_advance(scanner, token)

	uint32_t entries = 0;
	while (token->type != WFOBJ_TOKEN_EOF && token->type != WFOBJ_TOKEN_NEW_LINE) {
		if (entries >= limit) { wfobj_error_at(token, "expected less elements"); }

		float value;
		if (wfobj_consume_float(scanner, token, &value)) {
			if (entries >= limit) { continue; } entries++;
			array_float_push(buffer, value);
		}
		else { ADVANCE(); }
	}

	if (entries < limit) { wfobj_error_at(token, "expected more elements"); }

#undef ADVANCE
}

inline static uint32_t wfobj_translate_index(int32_t value, uint32_t base) {
	return (value > 0) ? (uint32_t)(value - 1) : (uint32_t)((int32_t)base + value);
}

static void wfobj_do_faces(
	struct WFObj_Scanner * scanner, struct WFObj_Token * token, struct Array_U32 * buffer,
	uint32_t positions_count, uint32_t texcoords_count, uint32_t normals_count
) {
#define ADVANCE() wfobj_advance(scanner, token)

	while (token->type != WFOBJ_TOKEN_EOF && token->type != WFOBJ_TOKEN_NEW_LINE) {
		int32_t value;
		uint32_t face[3] = {0};

		// face format: position
		wfobj_consume_s32(scanner, token, &value);
		face[0] = wfobj_translate_index(value, positions_count);

		if (token->type != WFOBJ_TOKEN_SLASH) {
			array_u32_push_many(buffer, 3, face);
			continue;
		}

		// face format: position//normal
		ADVANCE();
		if (token->type == WFOBJ_TOKEN_SLASH) {
			ADVANCE();

			wfobj_consume_s32(scanner, token, &value);
			face[2] = wfobj_translate_index(value, normals_count);

			array_u32_push_many(buffer, 3, face);
			continue;
		}

		// face format: position/texcoord
		wfobj_consume_s32(scanner, token, &value);
		face[1] = wfobj_translate_index(value, texcoords_count);

		if (token->type != WFOBJ_TOKEN_SLASH) {
			array_u32_push_many(buffer, 3, face);
			continue;
		}

		// face format: position/texcoord/normal
		ADVANCE();
		wfobj_consume_s32(scanner, token, &value);
		face[2] = wfobj_translate_index(value, normals_count);

		array_u32_push_many(buffer, 3, face);
	}

#undef ADVANCE
}

inline static void wfobj_init_internal(struct WFObj * obj, char const * text) {
#define ADVANCE() wfobj_advance(&scanner, &token)

	struct WFObj_Scanner scanner;
	struct WFObj_Token token;

	//
	uint32_t position_lines = 0;
	uint32_t texcoord_lines = 0;
	uint32_t normal_lines = 0;
	uint32_t face_lines = 0;

	wfobj_scanner_init(&scanner, text); ADVANCE();
	while (token.type != WFOBJ_TOKEN_EOF) {
		switch (token.type) {
			default: break;

			case WFOBJ_TOKEN_POSITION: position_lines++; break;
			case WFOBJ_TOKEN_TEXCOORD: texcoord_lines++; break;
			case WFOBJ_TOKEN_NORMAL: normal_lines++; break;
			case WFOBJ_TOKEN_FACE: face_lines++; break;
		}
		ADVANCE();
	}
	wfobj_scanner_free(&scanner);

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

	wfobj_scanner_init(&scanner, text); ADVANCE();
	while (token.type != WFOBJ_TOKEN_EOF) {
		switch (token.type) {
			// silent
			case WFOBJ_TOKEN_NEW_LINE: break;
			case WFOBJ_TOKEN_COMMENT: break;

			// errors
			case WFOBJ_TOKEN_IDENTIFIER: wfobj_error_at(&token, "unknown identifier"); break;
			case WFOBJ_TOKEN_ERROR: wfobj_error_at(&token, "scan error"); break;
			default: wfobj_error_at(&token, "unhandled input"); break;

			// valid
			case WFOBJ_TOKEN_POSITION: { ADVANCE();
				wfobj_do_vertex(&scanner, &token, &obj->positions, 3);
				break;
			}

			case WFOBJ_TOKEN_TEXCOORD: { ADVANCE();
				wfobj_do_vertex(&scanner, &token, &obj->texcoords, 2);
				break;
			}

			case WFOBJ_TOKEN_NORMAL: { ADVANCE();
				wfobj_do_vertex(&scanner, &token, &obj->normals, 3);
				break;
			}

			case WFOBJ_TOKEN_FACE: { ADVANCE();
				scratch_u32.count = 0;
				wfobj_do_faces(
					&scanner, &token, &scratch_u32,
					obj->positions.count, obj->texcoords.count, obj->normals.count
				);

				uint32_t indices_count = scratch_u32.count / 3;
				for (uint32_t i = 2; i < indices_count; i++) {
					array_u32_push_many(&obj->triangles, 3, scratch_u32.data + 0);
					array_u32_push_many(&obj->triangles, 3, scratch_u32.data + (i - 1) * 3);
					array_u32_push_many(&obj->triangles, 3, scratch_u32.data + i * 3);
				}
				break;
			}

			case WFOBJ_TOKEN_SMOOTH: ADVANCE(); break;
			case WFOBJ_TOKEN_OBJECT: wfobj_do_name(&scanner, &token); break;
			case WFOBJ_TOKEN_GROUP: wfobj_do_name(&scanner, &token); break;
		}
		ADVANCE();
	}
	wfobj_scanner_free(&scanner);

	array_u32_free(&scratch_u32);

#undef ADVANCE
}
