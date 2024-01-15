#include "framework/formatter.h"
#include "framework/parsing.h"
#include "framework/systems/memory.h"

#include "wfobj_lexer.h"


//
#include "wfobj.h"

// ----- ----- ----- ----- -----
//     personal
// ----- ----- ----- ----- -----

struct WFObj wfobj_init(void) {
	return (struct WFObj){
		.positions = array_init(sizeof(float)),
		.texcoords = array_init(sizeof(float)),
		.normals   = array_init(sizeof(float)),
		.triangles = array_init(sizeof(uint32_t)),
	};
}

void wfobj_free(struct WFObj * obj) {
	array_free(&obj->positions);
	array_free(&obj->texcoords);
	array_free(&obj->normals);
	array_free(&obj->triangles);
	// zero_out(CBMP_(obj));
}

// ----- ----- ----- ----- -----
//     parsing
// ----- ----- ----- ----- -----

static void wfobj_error_at(struct WFObj_Token * token, struct CString message) {
	static struct CString c_wfobj_token_names[] = {
		[WFOBJ_TOKEN_ERROR_UNKNOWN_CHARACTER] = S__("unknown character"),
		[WFOBJ_TOKEN_EOF]                     = S__("eof"),
	};

	struct CString const reason = c_wfobj_token_names[token->type];

	LOG("[wfobj]");
	LOG(" [line: %u]", token->line + 1);
	LOG(" [context: '%.*s']", token->text.length, token->text.data);
	if (!cstring_empty(reason)) { LOG(" [%.*s]", reason.length, reason.data); }
	if (!cstring_empty(message)) { LOG(": %.*s", message.length, message.data); }
	LOG("\n");
}

static void wfobj_advance(struct WFObj_Lexer * lexer, struct WFObj_Token * token) {
	while (token->type != WFOBJ_TOKEN_EOF) {
		*token = wfobj_lexer_next(lexer);
		switch (token->type) {
			case WFOBJ_TOKEN_COMMENT: continue;

			case WFOBJ_TOKEN_ERROR_UNKNOWN_CHARACTER:
				wfobj_error_at(token, S_("lexer error"));
				continue;

			default: return;
		}
	}
}

static bool wfobj_consume_float(struct WFObj_Lexer * lexer, struct WFObj_Token * token, float * value) {
#define ADVANCE() wfobj_advance(lexer, token)

	if (token->type == WFOBJ_TOKEN_NUMBER) {
		*value = parse_r32(token->text);
		ADVANCE();
		return true;
	}

	wfobj_error_at(token, S_("expected a number"));
	return false;

#undef ADVANCE
}

static bool wfobj_consume_s32(struct WFObj_Lexer * lexer, struct WFObj_Token * token, int32_t * value) {
#define ADVANCE() wfobj_advance(lexer, token)

	if (token->type == WFOBJ_TOKEN_NUMBER) {
		*value = parse_s32(token->text);
		ADVANCE();
		return true;
	}

	wfobj_error_at(token, S_("expected a number"));
	return false;

#undef ADVANCE
}

static void wfobj_do_name(struct WFObj_Lexer * lexer, struct WFObj_Token * token) {
	while (token->type != WFOBJ_TOKEN_EOF && token->type != WFOBJ_TOKEN_NEW_LINE) {
		wfobj_advance(lexer, token);
	}
}

static void wfobj_do_vertex(struct WFObj_Lexer * lexer, struct WFObj_Token * token, struct Array * buffer, uint32_t limit) {
#define ADVANCE() wfobj_advance(lexer, token)

	uint32_t entries = 0;
	while (token->type != WFOBJ_TOKEN_EOF && token->type != WFOBJ_TOKEN_NEW_LINE) {
		if (entries >= limit) { wfobj_error_at(token, S_("expected less elements")); }

		float value;
		if (wfobj_consume_float(lexer, token, &value)) {
			if (entries >= limit) { continue; } entries++;
			array_push_many(buffer, 1, &value);
		}
		else { ADVANCE(); }
	}

	if (entries < limit) { wfobj_error_at(token, S_("expected more elements")); }

#undef ADVANCE
}

inline static uint32_t wfobj_translate_index(int32_t value, uint32_t base) {
	return (value > 0) ? (uint32_t)(value - 1) : (uint32_t)((int32_t)base + value);
}

static void wfobj_do_faces(
	struct WFObj_Lexer * lexer, struct WFObj_Token * token, struct Array * buffer,
	uint32_t positions_count, uint32_t texcoords_count, uint32_t normals_count
) {
#define ADVANCE() wfobj_advance(lexer, token)

	while (token->type != WFOBJ_TOKEN_EOF && token->type != WFOBJ_TOKEN_NEW_LINE) {
		int32_t value;
		uint32_t face[3] = {0};

		// face format: position
		wfobj_consume_s32(lexer, token, &value);
		face[0] = wfobj_translate_index(value, positions_count);

		if (token->type != WFOBJ_TOKEN_SLASH) {
			array_push_many(buffer, 3, face);
			continue;
		}

		// face format: position//normal
		ADVANCE();
		if (token->type == WFOBJ_TOKEN_SLASH) {
			ADVANCE();

			wfobj_consume_s32(lexer, token, &value);
			face[2] = wfobj_translate_index(value, normals_count);

			array_push_many(buffer, 3, face);
			continue;
		}

		// face format: position/texcoord
		wfobj_consume_s32(lexer, token, &value);
		face[1] = wfobj_translate_index(value, texcoords_count);

		if (token->type != WFOBJ_TOKEN_SLASH) {
			array_push_many(buffer, 3, face);
			continue;
		}

		// face format: position/texcoord/normal
		ADVANCE();
		wfobj_consume_s32(lexer, token, &value);
		face[2] = wfobj_translate_index(value, normals_count);

		array_push_many(buffer, 3, face);
	}

#undef ADVANCE
}

struct WFObj wfobj_parse(struct CString text) {
#define ADVANCE() wfobj_advance(&lexer, &token)

	struct WFObj result = wfobj_init();

	struct WFObj_Lexer lexer;
	struct WFObj_Token token;

	//
	uint32_t position_lines = 0;
	uint32_t texcoord_lines = 0;
	uint32_t normal_lines = 0;
	uint32_t face_lines = 0;

	lexer = wfobj_lexer_init(text);
	token = (struct WFObj_Token){0}; ADVANCE();
	while (token.type != WFOBJ_TOKEN_EOF) {
		switch (token.type) {
			default: break;

			case WFOBJ_TOKEN_POSITION: position_lines++; break;
			case WFOBJ_TOKEN_TEXCOORD: texcoord_lines++; break;
			case WFOBJ_TOKEN_NORMAL:   normal_lines++;   break;
			case WFOBJ_TOKEN_FACE:     face_lines++;     break;
		}
		ADVANCE();
	}
	wfobj_lexer_free(&lexer);

	array_resize(&result.positions, position_lines * 3);
	array_resize(&result.texcoords, texcoord_lines * 2);
	array_resize(&result.normals,   normal_lines * 3);
	array_resize(&result.triangles, face_lines * 3 * 2);

	//
	struct Array scratch_u32 = array_init(sizeof(uint32_t));
	scratch_u32.allocate = realloc_arena;
	array_resize(&scratch_u32, 3 * 4);

	lexer = wfobj_lexer_init(text);
	token = (struct WFObj_Token){0}; ADVANCE();
	while (token.type != WFOBJ_TOKEN_EOF) {
		switch (token.type) {
			// silent
			case WFOBJ_TOKEN_COMMENT: break;
			case WFOBJ_TOKEN_NEW_LINE: break;

			// errors
			default: wfobj_error_at(&token, S_("lexer error")); break;

			// valid
			case WFOBJ_TOKEN_POSITION: { ADVANCE();
				wfobj_do_vertex(&lexer, &token, &result.positions, 3);
				break;
			}

			case WFOBJ_TOKEN_TEXCOORD: { ADVANCE();
				wfobj_do_vertex(&lexer, &token, &result.texcoords, 2);
				break;
			}

			case WFOBJ_TOKEN_NORMAL: { ADVANCE();
				wfobj_do_vertex(&lexer, &token, &result.normals, 3);
				break;
			}

			case WFOBJ_TOKEN_FACE: { ADVANCE();
				scratch_u32.count = 0;
				wfobj_do_faces(
					&lexer, &token, &scratch_u32,
					result.positions.count, result.texcoords.count, result.normals.count
				);

				uint32_t indices_count = scratch_u32.count / 3;
				for (uint32_t i = 2; i < indices_count; i++) {
					uint32_t const * data = scratch_u32.data;
					array_push_many(&result.triangles, 3, data + 0);
					array_push_many(&result.triangles, 3, data + (i - 1) * 3);
					array_push_many(&result.triangles, 3, data + i * 3);
				}
				break;
			}

			case WFOBJ_TOKEN_SMOOTH: ADVANCE(); break;
			case WFOBJ_TOKEN_OBJECT: wfobj_do_name(&lexer, &token); break;
			case WFOBJ_TOKEN_GROUP: wfobj_do_name(&lexer, &token); break;
		}
		ADVANCE();
	}
	wfobj_lexer_free(&lexer);
	array_free(&scratch_u32);

	return result;

#undef ADVANCE
}
