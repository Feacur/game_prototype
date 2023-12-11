#include "framework/common.h"
#include "framework/memory.h"
#include "framework/unicode.h"
#include "framework/formatter.h"

#include "framework/containers/hashset.h"
#include "framework/containers/array.h"
#include "framework/containers/buffer.h"

#include "framework/assets/mesh.h"
#include "framework/assets/font.h"

#include "framework/graphics/gfx_material.h"
#include "framework/graphics/gfx_objects.h"
#include "framework/graphics/command.h"
#include "framework/graphics/misc.h"

#include "framework/systems/string_system.h"
#include "framework/systems/asset_system.h"
#include "framework/systems/material_system.h"

#include "framework/maths.h"

#include "asset_types.h"

// @todo: UTF-8 edge cases, different languages, LTR/RTL, ligatures, etc.
// @idea: point to a `Gfx_Material` with an `Asset_Handle` instead
// @idea: static batching option; cache vertices and stuff until changed
// @idea: add a 3d batcher

struct Batcher_2D_Word {
	uint32_t codepoints_offset, codepoints_end;
	size_t buffer_offset;
	struct Handle ah_font; float size;
	// @note: local to `batcher_2d_add_text`
	uint32_t breaker_codepoint;
	struct vec2 position;
	float full_size_x;
};

struct Batcher_2D_Batch {
	struct Handle mh_mat;
	struct Handle gh_program;
	enum Blend_Mode blend_mode;
	enum Depth_Mode depth_mode;
	size_t buffer_offset, buffer_length;
	uint32_t uniform_offset, uniform_length;
};

struct Batcher_Instance {
	struct rect quad;
	struct rect uv;
	struct vec4 color;
};

struct Batcher_2D {
	struct Gfx_Uniforms uniforms;
	//
	struct Batcher_2D_Batch batch;
	struct Array codepoints; // uint32_t
	struct Array batches;    // `struct Batcher_2D_Batch`
	struct Array words;      // `struct Batcher_2D_Word`
	struct Hashset fonts;    // `struct Handle`
	//
	struct vec4 color;
	struct mat4 matrix;
	//
	struct Buffer buffer;
	struct Handle gh_buffer;
	struct Handle gh_mesh;
};

//
#include "batcher_2d.h"

struct Batcher_2D * batcher_2d_init(void) {
	struct Batcher_2D * batcher = ALLOCATE(struct Batcher_2D);
	*batcher = (struct Batcher_2D){
		.uniforms = gfx_uniforms_init(),
		.color = (struct vec4){1, 1, 1, 1},
		.matrix = c_mat4_identity,
		.codepoints      = array_init(sizeof(uint32_t)),
		.batches         = array_init(sizeof(struct Batcher_2D_Batch)),
		.words           = array_init(sizeof(struct Batcher_2D_Word)),
		.fonts           = hashset_init(&hash32, sizeof(struct Handle)),
		.buffer          = buffer_init(),
		.gh_buffer       = gpu_buffer_init(&(struct Buffer){0}),
		.gh_mesh         = gpu_mesh_init(&(struct Mesh){0}),
	};


	return batcher;
}

void batcher_2d_free(struct Batcher_2D * batcher) {
	gpu_buffer_free(batcher->gh_buffer);
	gpu_mesh_free(batcher->gh_mesh);
	//
	array_free(&batcher->codepoints);
	array_free(&batcher->batches);
	array_free(&batcher->words);
	hashset_free(&batcher->fonts);
	buffer_free(&batcher->buffer);
	//
	gfx_uniforms_free(&batcher->uniforms);
	//
	common_memset(batcher, 0, sizeof(*batcher));
	FREE(batcher);
}

void batcher_2d_set_color(struct Batcher_2D * batcher, struct vec4 value) {
	batcher->color = value;
}

void batcher_2d_set_matrix(struct Batcher_2D * batcher, struct mat4 const value) {
	batcher->matrix = value;
}

static void batcher_2d_internal_bake_pass(struct Batcher_2D * batcher) {
	uint32_t const uniform_offset = batcher->uniforms.headers.count;
	if (batcher->batch.buffer_offset < batcher->buffer.size || batcher->batch.uniform_offset < uniform_offset) {
		batcher->batch.buffer_length  = batcher->buffer.size  - batcher->batch.buffer_offset;
		batcher->batch.uniform_length = uniform_offset - batcher->batch.uniform_offset;
		array_push_many(&batcher->batches, 1, &batcher->batch);

		graphics_buffer_align(&batcher->buffer, BUFFER_MODE_STORAGE);
		batcher->batch.buffer_offset = batcher->buffer.size;
		batcher->batch.buffer_length = 0;

		batcher->batch.uniform_offset = uniform_offset;
		batcher->batch.uniform_length = 0;
	}
}

void batcher_2d_set_material(struct Batcher_2D * batcher, struct Handle handle) {
	if (!handle_equals(batcher->batch.mh_mat, handle)) {
		batcher_2d_internal_bake_pass(batcher);
	}
	batcher->batch.mh_mat = handle;
	batcher->batch.gh_program = (struct Handle){0};
	batcher->batch.blend_mode = BLEND_MODE_NONE;
	batcher->batch.depth_mode = DEPTH_MODE_NONE;
}

void batcher_2d_set_shader(
	struct Batcher_2D * batcher,
	struct Handle gh_program,
	enum Blend_Mode blend_mode, enum Depth_Mode depth_mode
) {
	if (!handle_equals(batcher->batch.gh_program, gh_program)
		|| batcher->batch.blend_mode != blend_mode
		|| batcher->batch.depth_mode != depth_mode) {
		batcher_2d_internal_bake_pass(batcher);
	}
	batcher->batch.mh_mat = (struct Handle){0};
	batcher->batch.gh_program = gh_program;
	batcher->batch.blend_mode = blend_mode;
	batcher->batch.depth_mode = depth_mode;
}

void batcher_2d_uniforms_push(struct Batcher_2D * batcher, struct CString name, struct CArray value) {
	uint32_t const id = string_system_add(name);
	batcher_2d_uniforms_id_push(batcher, id, value);
}

void batcher_2d_uniforms_id_push(struct Batcher_2D * batcher, uint32_t id, struct CArray value) {
	if (id == 0) { return; }
	struct CArray_Mut const field = gfx_uniforms_id_get(&batcher->uniforms, id, batcher->batch.uniform_offset);
	if (field.data != NULL) {
		if (carray_equals(carray_const(field), value)) { return; }
		batcher_2d_internal_bake_pass(batcher);
	}
	gfx_uniforms_id_push(&batcher->uniforms, id, value);
}

void batcher_2d_add_quad(
	struct Batcher_2D * batcher,
	struct rect rect,
	struct rect uv
) {
	struct mat4 const m = batcher->matrix;
	struct Batcher_Instance const instance = {
		.quad = {
			{
				m.x.x * rect.min.x + m.y.x * rect.min.y + m.w.x,
				m.x.y * rect.min.x + m.y.y * rect.min.y + m.w.y,
			},
			{
				m.x.x * rect.max.x + m.y.x * rect.max.y + m.w.x,
				m.x.y * rect.max.x + m.y.y * rect.max.y + m.w.y,
			},
		},
		.uv = uv,
		.color = batcher->color,
	};
	buffer_push_many(&batcher->buffer, sizeof(instance), &instance);
}

void batcher_2d_add_text(
	struct Batcher_2D * batcher,
	struct rect rect, struct vec2 alignment, bool wrap,
	struct Handle gh_font, struct CString value, float size
) {
	struct Asset_Font const * font_asset = asset_system_get(gh_font);
	if (font_asset == NULL) { return; }
	if (font_asset->font == NULL) { return; }

	float const scale       = font_get_scale(font_asset->font, size);
	float const ascent      = font_get_ascent(font_asset->font, scale);
	float const descent     = font_get_descent(font_asset->font, scale);
	float const line_gap    = font_get_gap(font_asset->font, scale);
	float const line_height = ascent - descent + line_gap;

	uint32_t const words_offset = batcher->words.count;

	font_add_defaults(font_asset->font, size);
	struct Glyph const * glyph_error = font_get_glyph(font_asset->font, '\0', size);

	// break text into words
	{
		float word_width = 0;
		uint32_t codepoints_offset = batcher->codepoints.count;

		FOR_UTF8 (value.length, (uint8_t const *)value.data, it) {
			font_add_glyph(font_asset->font, it.codepoint, size);

			struct Glyph const * glyph = font_get_glyph(font_asset->font, it.codepoint, size);
			float const full_size_x = (glyph != NULL) ? glyph->params.full_size_x : glyph_error->params.full_size_x;

			word_width += full_size_x;
			if (codepoint_is_word_break(it.codepoint)) {
				// @todo: (?) arena/stack allocator
				array_push_many(&batcher->words, 1, &(struct Batcher_2D_Word) {
					.codepoints_offset = codepoints_offset, .codepoints_end = batcher->codepoints.count,
					.ah_font = gh_font, .size = size,
					.breaker_codepoint = it.codepoint,
					.full_size_x = word_width,
				});

				//
				word_width = 0;
				codepoints_offset = batcher->codepoints.count;
			}

			if (!codepoint_is_invisible(it.codepoint)) {
				// @todo: (?) arena/stack allocator
				array_push_many(&batcher->codepoints, 1, &it.codepoint);

				//
				float const kerning = font_get_kerning(font_asset->font, it.leading, it.codepoint, scale);
				word_width += kerning;
			}
		}

		if (batcher->codepoints.count > codepoints_offset) {
			// @todo: (?) arena/stack allocator
			array_push_many(&batcher->words, 1, &(struct Batcher_2D_Word) {
				.codepoints_offset = codepoints_offset, .codepoints_end = batcher->codepoints.count,
				.ah_font = gh_font, .size = size,
				.full_size_x = word_width,
			});
		}
	}

	// position words as for alignment {0, 1}
	{
		struct vec2 offset = {rect.min.x, rect.max.y};
		offset.y -= ascent;

		for (uint32_t word_i = words_offset; word_i < batcher->words.count; word_i++) {
			struct Batcher_2D_Word * word = array_at(&batcher->words, word_i);

			// break line if word overflows
			if (wrap && (offset.x + word->full_size_x > rect.max.x)) {
				offset.x = rect.min.x; // @note: auto `\r`
				offset.y -= line_height;
			}
			word->position = offset;

			// process word
			for (uint32_t strings_i = word->codepoints_offset; strings_i < word->codepoints_end; strings_i++) {
				char const char0 = '\0';
				uint32_t const * codepoint = array_at(&batcher->codepoints, strings_i);
				uint32_t const * previous = (strings_i > word->codepoints_offset) ? array_at(&batcher->codepoints, strings_i - 1) : &char0;

				struct Glyph const * glyph = font_get_glyph(font_asset->font, *codepoint, word->size);
				float const full_size_x = (glyph != NULL) ? glyph->params.full_size_x : glyph_error->params.full_size_x;

				float const kerning = font_get_kerning(font_asset->font, *previous, *codepoint, scale);
				offset.x += full_size_x + kerning;
			}

			// process breaker
			{
				struct Glyph const * glyph = font_get_glyph(font_asset->font, word->breaker_codepoint, word->size);
				offset.x += (glyph != NULL) ? glyph->params.full_size_x : 0;
				if (word->breaker_codepoint == '\n') {
					offset.x = rect.min.x; // @note: auto `\r`
					offset.y -= line_height;
				}
			}
		}
	}

	// align words
	{
		struct vec2 const error_margins = {
			0.0001f * (1 - 2*alignment.x),
			0.0001f * (1 - 2*alignment.y),
		};
		struct vec2 const rect_size = {
			rect.max.x - rect.min.x,
			rect.max.y - rect.min.y,
		};

		uint32_t line_offset = words_offset;
		uint32_t lines_count = 1;
		float line_position_y = 0;
		float line_width = 0;

		for (uint32_t word_i = words_offset; word_i < batcher->words.count; word_i++) {
			struct Batcher_2D_Word const * word = array_at(&batcher->words, word_i);

			if (line_position_y != word->position.y) {
				float const offset = lerp(0, rect_size.x - line_width, alignment.x) + error_margins.x;
				for (uint32_t i = line_offset; i < word_i; i++) {
					struct Batcher_2D_Word * aligned_text = array_at(&batcher->words, i);
					aligned_text->position.x += offset;
				}
				line_offset = word_i;
				lines_count++;

				//
				line_position_y = word->position.y;
				line_width = 0;
			}

			line_width += word->full_size_x;
		}

		{
			float const offset = lerp(0, rect_size.x - line_width, alignment.x) + error_margins.x;
			for (uint32_t i = line_offset; i < batcher->words.count; i++) {
				struct Batcher_2D_Word * word = array_at(&batcher->words, i);
				word->position.x += offset;
			}
		}

		{
			float const height = (float)lines_count * line_height;
			float const offset = lerp((height - rect_size.y) - line_height, 0, alignment.y) + error_margins.y;
			for (uint32_t i = words_offset; i < batcher->words.count; i++) {
				struct Batcher_2D_Word * word = array_at(&batcher->words, i);
				word->position.y += offset;
			}
		}
	}

	// translate words into vertices
	for (uint32_t word_i = words_offset; word_i < batcher->words.count; word_i++) {
		struct Batcher_2D_Word * word = array_at(&batcher->words, word_i);
		word->buffer_offset = batcher->buffer.size;

		struct vec2 offset = word->position;
		if (offset.y > rect.max.y) { word->codepoints_end = word->codepoints_offset; continue; } // void entry
		if (offset.y < rect.min.y) { batcher->words.count = word_i;                  break; }    // drop rest

		for (uint32_t strings_i = word->codepoints_offset; strings_i < word->codepoints_end; strings_i++) {
			char const char0 = '\0';
			uint32_t const * codepoint = array_at(&batcher->codepoints, strings_i);
			uint32_t const * previous = (strings_i > word->codepoints_offset) ? array_at(&batcher->codepoints, strings_i - 1) : &char0;

			struct Glyph const * glyph = font_get_glyph(font_asset->font, *codepoint, word->size);
			struct Glyph_Params const params = (glyph != NULL) ? glyph->params : glyph_error->params;

			float const kerning = font_get_kerning(font_asset->font, *previous, *codepoint, scale);
			float const offset_x = offset.x + kerning;
			offset.x += params.full_size_x + kerning;

			if (offset_x < rect.min.x) { word->codepoints_offset = strings_i + 1; continue; } // skip glyph
			if (offset.x > rect.max.x) { word->codepoints_end    = strings_i;     break; }    // drop rest

			if (!codepoint_is_invisible(*codepoint)) {
				batcher_2d_add_quad(
					batcher,
					(struct rect){
						.min = {
							((float)params.rect.min.x) + offset_x,
							((float)params.rect.min.y) + offset.y,
						},
						.max = {
							((float)params.rect.max.x) + offset_x,
							((float)params.rect.max.y) + offset.y,
						},
					},
					(struct rect){0} // @note: UVs are delayed, see `batcher_2d_bake_words`
				);
			}
		}
	}
}

static void batcher_2d_bake_words(struct Batcher_2D * batcher) {
	if (batcher->words.count == 0) { return; }

	// render and upload the atlases
	{
		// @todo: (?) arena/stack allocator
		hashset_clear(&batcher->fonts, false);

		FOR_ARRAY(&batcher->words, it) {
			struct Batcher_2D_Word const * word = it.value;
			hashset_set(&batcher->fonts, &word->ah_font);
		}

		FOR_HASHSET (&batcher->fonts, it) {
			struct Handle const * ah_font = it.key;
			struct Asset_Font const * font = asset_system_get(*ah_font);
			font_render(font->font);
		}

		FOR_HASHSET (&batcher->fonts, it) {
			struct Handle const * ah_font = it.key;
			struct Asset_Font const * font = asset_system_get(*ah_font);
			gpu_texture_update(font->gh_texture, font_get_asset(font->font));
		}
	}

	// fill quads UVs
	FOR_ARRAY(&batcher->words, it) {
		struct Batcher_2D_Word const * word = it.value;
		size_t b_offset = word->buffer_offset;

		struct Asset_Font const * font_asset = asset_system_get(word->ah_font);
		struct Glyph const * glyph_error = font_get_glyph(font_asset->font, '\0', word->size);
		struct rect const glyph_error_uv = glyph_error->uv;

		for (uint32_t i = word->codepoints_offset; i < word->codepoints_end; i++) {
			uint32_t const * codepoint = array_at(&batcher->codepoints, i);

			if (codepoint_is_invisible(*codepoint)) { continue; }

			struct Glyph const * glyph = font_get_glyph(font_asset->font, *codepoint, word->size);
			struct rect const uv = (glyph != NULL) ? glyph->uv : glyph_error_uv;

			struct Batcher_Instance * instance = buffer_at(&batcher->buffer, b_offset);
			instance->uv = uv;

			b_offset += sizeof(struct Batcher_Instance);
		}
	}
}

void batcher_2d_clear(struct Batcher_2D * batcher) {
	common_memset(&batcher->batch, 0, sizeof(batcher->batch));
	array_clear(&batcher->codepoints, false);
	array_clear(&batcher->batches, false);
	array_clear(&batcher->words, false);
	buffer_clear(&batcher->buffer, false);
	gfx_uniforms_clear(&batcher->uniforms);
}

void batcher_2d_issue_commands(struct Batcher_2D * batcher, struct Array * gpu_commands) {
	batcher_2d_internal_bake_pass(batcher);
	FOR_ARRAY(&batcher->batches, it) {
		struct Batcher_2D_Batch const * batch = it.value;

		struct Handle gh_program = {0};
		if (!handle_is_null(batch->mh_mat)) {
			struct Gfx_Material const * material = material_system_take(batch->mh_mat);
			gh_program = material->gh_program;
			array_push_many(gpu_commands, 1, &(struct GPU_Command){
				.type = GPU_COMMAND_TYPE_MATERIAL,
				.as.material = {
					.mh_mat = batch->mh_mat,
				},
			});
		}

		if (!handle_is_null(batch->gh_program)) {
			gh_program = batch->gh_program;
			array_push_many(gpu_commands, 1, &(struct GPU_Command){
				.type = GPU_COMMAND_TYPE_SHADER,
				.as.shader = {
					.gh_program = batch->gh_program,
					.blend_mode = batch->blend_mode,
					.depth_mode = batch->depth_mode,
				},
			});
		}

		if (batch->uniform_length > 0) {
			array_push_many(gpu_commands, 1, &(struct GPU_Command){
				.type = GPU_COMMAND_TYPE_UNIFORM,
				.as.uniform = {
					.gh_program = gh_program,
					.uniforms = &batcher->uniforms,
					.offset = batch->uniform_offset,
					.count = batch->uniform_length,
				},
			});
		}

		if (batch->buffer_length > 0) {
			// @note: draw instances quads, each quad is 2 triangles,
			// thus we need 6 vertices; the rest is shader-handled
			array_push_many(gpu_commands, 2, (struct GPU_Command[]){
				{
					.type = GPU_COMMAND_TYPE_BUFFER,
					.as.buffer = {
						.gh_buffer = batcher->gh_buffer,
						.offset = batch->buffer_offset,
						.length = batch->buffer_length,
						.mode = BUFFER_MODE_STORAGE,
						.index = BLOCK_TYPE_DYNAMIC - 1,
					},
				},
				{
					.type = GPU_COMMAND_TYPE_DRAW,
					.as.draw = {
						.gh_mesh = batcher->gh_mesh,
						.count = 6, .mode = MESH_MODE_TRIANGLES,
						.instances = (uint32_t)(batch->buffer_length / sizeof(struct Batcher_Instance)),
					},
				},
			});
		}
	}
	array_clear(&batcher->batches, false);
}

void batcher_2d_bake(struct Batcher_2D * batcher) {
	if (batcher->batches.count > 0) {
		WRN("unissued batches");
		REPORT_CALLSTACK(); DEBUG_BREAK();
	}
	if (batcher->batch.buffer_offset < batcher->buffer.size) {
		WRN("unissued vertices");
		REPORT_CALLSTACK(); DEBUG_BREAK();
	}

	batcher_2d_bake_words(batcher);
	gpu_buffer_update(batcher->gh_buffer, &batcher->buffer);
}
