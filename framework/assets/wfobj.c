#include "framework/memory.h"
#include "framework/logger.h"
#include "framework/parsing.h"

#include "wfobj_scanner.h"

//
#include "wfobj.h"

inline static struct WFObj wfobj_init_internal(char const * text);
struct WFObj wfobj_init(char const * text) {
	return wfobj_init_internal(text);
}

void wfobj_free(struct WFObj * obj) {
	array_flt_free(&obj->positions);
	array_flt_free(&obj->texcoords);
	array_flt_free(&obj->normals);
	array_u32_free(&obj->triangles);
}

//

static void wfobj_error_at(struct WFObj_Token * token, char const * message) {
	DEBUG_BREAK();

	static char const * c_wfobj_token_names[] = {
		[WFOBJ_TOKEN_ERROR_UNKNOWN_CHARACTER] = "unknown character",
		[WFOBJ_TOKEN_EOF]                     = "eof",
	};

	char const * const reason = c_wfobj_token_names[token->type];

	logger_to_console("wfobj");
	logger_to_console(" [line: %u]", token->line + 1);
	logger_to_console(" [context: '%.*s']", token->text.length, token->text.data);
	if (reason != NULL) { logger_to_console(" [%s]", reason); }
	if (message != NULL) { logger_to_console(": %s", message); }
	logger_to_console("\n");
}

static void wfobj_advance(struct WFObj_Scanner * scanner, struct WFObj_Token * token) {
	while (token->type != WFOBJ_TOKEN_EOF) {
		*token = wfobj_scanner_next(scanner);
		switch (token->type) {
			case WFOBJ_TOKEN_COMMENT: continue;

			case WFOBJ_TOKEN_ERROR_UNKNOWN_CHARACTER:
				wfobj_error_at(token, "scanner error");
				continue;

			default: return;
		}
	}
}

static bool wfobj_consume_float(struct WFObj_Scanner * scanner, struct WFObj_Token * token, float * value) {
#define ADVANCE() wfobj_advance(scanner, token)

	if (token->type == WFOBJ_TOKEN_NUMBER) {
		// *value = strtof(token->start, NULL);
		*value = parse_float(token->text.data);
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
		*value = parse_s32(token->text.data);
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

static void wfobj_do_vertex(struct WFObj_Scanner * scanner, struct WFObj_Token * token, struct Array_Flt * buffer, uint32_t limit) {
#define ADVANCE() wfobj_advance(scanner, token)

	uint32_t entries = 0;
	while (token->type != WFOBJ_TOKEN_EOF && token->type != WFOBJ_TOKEN_NEW_LINE) {
		if (entries >= limit) { wfobj_error_at(token, "expected less elements"); }

		float value;
		if (wfobj_consume_float(scanner, token, &value)) {
			if (entries >= limit) { continue; } entries++;
			array_flt_push(buffer, value);
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

inline static struct WFObj wfobj_init_internal(char const * text) {
#define ADVANCE() wfobj_advance(&scanner, &token)

	struct WFObj result = {
		.positions = array_flt_init(),
		.texcoords = array_flt_init(),
		.normals   = array_flt_init(),
		.triangles = array_u32_init(),
	};

	struct WFObj_Scanner scanner;
	struct WFObj_Token token;

	//
	uint32_t position_lines = 0;
	uint32_t texcoord_lines = 0;
	uint32_t normal_lines = 0;
	uint32_t face_lines = 0;

	scanner = wfobj_scanner_init(text);
	token = (struct WFObj_Token){0}; ADVANCE();
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

	array_flt_resize(&result.positions, position_lines * 3);
	array_flt_resize(&result.texcoords, texcoord_lines * 2);
	array_flt_resize(&result.normals, normal_lines * 3);
	array_u32_resize(&result.triangles, face_lines * 3 * 2);

	//
	// @todo: arena/stack allocator
	struct Array_U32 scratch_u32 = array_u32_init();
	array_u32_resize(&scratch_u32, 3 * 4);

	scanner = wfobj_scanner_init(text);
	token = (struct WFObj_Token){0}; ADVANCE();
	while (token.type != WFOBJ_TOKEN_EOF) {
		switch (token.type) {
			// silent
			case WFOBJ_TOKEN_COMMENT: break;
			case WFOBJ_TOKEN_NEW_LINE: break;

			// errors
			default: wfobj_error_at(&token, "scanner error"); break;

			// valid
			case WFOBJ_TOKEN_POSITION: { ADVANCE();
				wfobj_do_vertex(&scanner, &token, &result.positions, 3);
				break;
			}

			case WFOBJ_TOKEN_TEXCOORD: { ADVANCE();
				wfobj_do_vertex(&scanner, &token, &result.texcoords, 2);
				break;
			}

			case WFOBJ_TOKEN_NORMAL: { ADVANCE();
				wfobj_do_vertex(&scanner, &token, &result.normals, 3);
				break;
			}

			case WFOBJ_TOKEN_FACE: { ADVANCE();
				scratch_u32.count = 0;
				wfobj_do_faces(
					&scanner, &token, &scratch_u32,
					result.positions.count, result.texcoords.count, result.normals.count
				);

				uint32_t indices_count = scratch_u32.count / 3;
				for (uint32_t i = 2; i < indices_count; i++) {
					array_u32_push_many(&result.triangles, 3, scratch_u32.data + 0);
					array_u32_push_many(&result.triangles, 3, scratch_u32.data + (i - 1) * 3);
					array_u32_push_many(&result.triangles, 3, scratch_u32.data + i * 3);
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

	return result;

#undef ADVANCE
}
