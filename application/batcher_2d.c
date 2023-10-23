#include "framework/common.h"
#include "framework/memory.h"
#include "framework/unicode.h"
#include "framework/formatter.h"

#include "framework/containers/hashset.h"
#include "framework/containers/array.h"
#include "framework/containers/array.h"
#include "framework/containers/array.h"

#include "framework/assets/mesh.h"
#include "framework/assets/font.h"

#include "framework/graphics/material.h"
#include "framework/graphics/gpu_objects.h"
#include "framework/graphics/gpu_command.h"

#include "framework/systems/string_system.h"
#include "framework/systems/asset_system.h"
#include "framework/systems/material_system.h"

#include "framework/maths.h"

#include "asset_types.h"

// @todo: UTF-8 edge cases, different languages, LTR/RTL, ligatures, etc.
// @idea: point to a `Gfx_Material` with an `Asset_Handle` instead
// @idea: static batching option; cache vertices and stuff until changed
// @idea: add a 3d batcher

// quad layout
// 3-----------2
// |         / |
// |       /   |
// |     /     |
// |   /       |
// | /         |
// 0-----------1

#define BATCHER_2D_BUFFERS_COUNT 2

struct Batcher_2D_Word {
	uint32_t codepoints_offset, codepoints_end;
	uint32_t buffer_vertices_offset;
	struct Handle font_asset_handle; float size;
	// @note: local to `batcher_2d_add_text`
	uint32_t breaker_codepoint;
	struct vec2 position;
	float full_size_x;
};

struct Batcher_2D_Batch {
	struct Handle material;
	struct Handle shader;
	enum Blend_Mode blend_mode;
	enum Depth_Mode depth_mode;
	uint32_t indices_offset, indices_length;
	uint32_t uniform_offset, uniform_length;
};

struct Batcher_2D_Vertex {
	struct vec2 position;
	struct vec2 tex_coord;
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
	struct Array buffer_vertices; // `struct Batcher_2D_Vertex`
	struct Array buffer_indices;  // uint32_t
	//
	struct Mesh_Parameters mesh_parameters[BATCHER_2D_BUFFERS_COUNT];
	struct Handle gpu_mesh_handle;
};

//
#include "batcher_2d.h"

struct Batcher_2D * batcher_2d_init(void) {
	struct Batcher_2D * batcher = MEMORY_ALLOCATE(struct Batcher_2D);
	*batcher = (struct Batcher_2D){
		.uniforms = gfx_uniforms_init(),
		.color = (struct vec4){1, 1, 1, 1},
		.matrix = c_mat4_identity,
		.mesh_parameters = {
			(struct Mesh_Parameters){
				.type = DATA_TYPE_R32_F,
				.flags = MESH_FLAG_MUTABLE | MESH_FLAG_WRITE | MESH_FLAG_FREQUENT,
				.attributes = {
					ATTRIBUTE_TYPE_POSITION,
					SIZE_OF_MEMBER(struct Batcher_2D_Vertex, position) / sizeof(float),
					ATTRIBUTE_TYPE_TEXCOORD,
					SIZE_OF_MEMBER(struct Batcher_2D_Vertex, tex_coord) / sizeof(float),
					ATTRIBUTE_TYPE_COLOR,
					SIZE_OF_MEMBER(struct Batcher_2D_Vertex, color) / sizeof(float),
				},
			},
			(struct Mesh_Parameters){
				.type = DATA_TYPE_R32_U,
				.flags = MESH_FLAG_INDEX | MESH_FLAG_MUTABLE | MESH_FLAG_WRITE | MESH_FLAG_FREQUENT,
			},
		},
		.codepoints      = array_init(sizeof(uint32_t)),
		.batches         = array_init(sizeof(struct Batcher_2D_Batch)),
		.words           = array_init(sizeof(struct Batcher_2D_Word)),
		.fonts           = hashset_init(&hash32, sizeof(struct Handle)),
		.buffer_vertices = array_init(sizeof(struct Batcher_2D_Vertex)),
		.buffer_indices  = array_init(sizeof(uint32_t)),
	};

	//
	batcher->gpu_mesh_handle = gpu_mesh_init(&(struct Mesh){
		.count      = BATCHER_2D_BUFFERS_COUNT,
		.buffers    = (struct Buffer[BATCHER_2D_BUFFERS_COUNT]){0},
		.parameters = batcher->mesh_parameters,
	});

	return batcher;
}

void batcher_2d_free(struct Batcher_2D * batcher) {
	gpu_mesh_free(batcher->gpu_mesh_handle);
	//
	array_free(&batcher->codepoints);
	array_free(&batcher->batches);
	array_free(&batcher->words);
	hashset_free(&batcher->fonts);
	array_free(&batcher->buffer_vertices);
	array_free(&batcher->buffer_indices);
	//
	gfx_uniforms_free(&batcher->uniforms);
	//
	common_memset(batcher, 0, sizeof(*batcher));
	MEMORY_FREE(batcher);
}

void batcher_2d_set_color(struct Batcher_2D * batcher, struct vec4 value) {
	batcher->color = value;
}

void batcher_2d_set_matrix(struct Batcher_2D * batcher, struct mat4 const value) {
	batcher->matrix = value;
}

static void batcher_2d_internal_bake_pass(struct Batcher_2D * batcher) {
	uint32_t const offset = batcher->buffer_indices.count;
	uint32_t const u_offset = batcher->uniforms.headers.count;
	if (batcher->batch.indices_offset < offset || batcher->batch.uniform_offset < offset) {
		batcher->batch.indices_length = offset - batcher->batch.indices_offset;
		batcher->batch.uniform_length = u_offset - batcher->batch.uniform_offset;
		array_push_many(&batcher->batches, 1, &batcher->batch);

		batcher->batch.indices_offset = offset;
		batcher->batch.uniform_offset = u_offset;

		batcher->batch.indices_length = 0;
		batcher->batch.uniform_length = 0;
	}
}

void batcher_2d_set_material(struct Batcher_2D * batcher, struct Handle handle) {
	if (!handle_equals(batcher->batch.material, handle)) {
		batcher_2d_internal_bake_pass(batcher);
	}
	batcher->batch.material = handle;
	batcher->batch.shader = (struct Handle){0};
	batcher->batch.blend_mode = BLEND_MODE_NONE;
	batcher->batch.depth_mode = DEPTH_MODE_NONE;
}

void batcher_2d_set_shader(
	struct Batcher_2D * batcher,
	struct Handle handle,
	enum Blend_Mode blend_mode, enum Depth_Mode depth_mode
) {
	if (!handle_equals(batcher->batch.shader, handle)
		|| batcher->batch.blend_mode != blend_mode
		|| batcher->batch.depth_mode != depth_mode) {
		batcher_2d_internal_bake_pass(batcher);
	}
	batcher->batch.material = (struct Handle){0};
	batcher->batch.shader = handle;
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

inline static struct Batcher_2D_Vertex batcher_2d_internal_make_vertex(struct mat4 m, struct vec2 position, struct vec2 tex_coord, struct vec4 color) {
	return (struct Batcher_2D_Vertex){
		.position = {
			m.x.x * position.x + m.y.x * position.y + m.w.x,
			m.x.y * position.x + m.y.y * position.y + m.w.y,
		},
		.tex_coord = tex_coord,
		.color = color,
	};

/*
	.position = mat4_mul_vec(m, position as vec4);
*/
}

void batcher_2d_add_quad(
	struct Batcher_2D * batcher,
	struct rect rect,
	struct rect uv
) {
	uint32_t const vertex_offset = batcher->buffer_vertices.count;
	struct vec4 const color = batcher->color;
	array_push_many(&batcher->buffer_vertices, 4, (struct Batcher_2D_Vertex[]){
		batcher_2d_internal_make_vertex(batcher->matrix, rect.min, uv.min, color),
		batcher_2d_internal_make_vertex(batcher->matrix, (struct vec2){rect.max.x, rect.min.y}, (struct vec2){uv.max.x, uv.min.y}, color),
		batcher_2d_internal_make_vertex(batcher->matrix, rect.max, uv.max, color),
		batcher_2d_internal_make_vertex(batcher->matrix, (struct vec2){rect.min.x, rect.max.y}, (struct vec2){uv.min.x, uv.max.y}, color),
	});
	array_push_many(&batcher->buffer_indices, 3 * 2, (uint32_t[]){
		vertex_offset + 0, vertex_offset + 1, vertex_offset + 2,
		vertex_offset + 2, vertex_offset + 3, vertex_offset + 0,
	});
}

void batcher_2d_add_text(
	struct Batcher_2D * batcher,
	struct rect rect, struct vec2 alignment, bool wrap,
	struct Handle font_asset_handle, struct CString value, float size
) {
	struct Asset_Font const * font_asset = asset_system_get(font_asset_handle);
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
					.font_asset_handle = font_asset_handle, .size = size,
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
				.font_asset_handle = font_asset_handle, .size = size,
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
		word->buffer_vertices_offset = batcher->buffer_vertices.count;

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
		hashset_clear(&batcher->fonts);

		for (uint32_t i = 0; i < batcher->words.count; i++) {
			struct Batcher_2D_Word const * word = array_at(&batcher->words, i);
			hashset_set(&batcher->fonts, &word->font_asset_handle);
		}

		FOR_HASHSET (&batcher->fonts, it) {
			struct Handle const * handle = it.key;
			struct Asset_Font const * font_asset = asset_system_get(*handle);
			font_render(font_asset->font);
		}

		FOR_HASHSET (&batcher->fonts, it) {
			struct Handle const * handle = it.key;
			struct Asset_Font const * font_asset = asset_system_get(*handle);
			gpu_texture_update(font_asset->gpu_handle, font_get_asset(font_asset->font));
		}
	}

	// fill quads UVs
	for (uint32_t word_i = 0; word_i < batcher->words.count; word_i++) {
		struct Batcher_2D_Word const * word = array_at(&batcher->words, word_i);
		uint32_t vertices_offset = word->buffer_vertices_offset;

		struct Asset_Font const * font_asset = asset_system_get(word->font_asset_handle);
		struct Glyph const * glyph_error = font_get_glyph(font_asset->font, '\0', word->size);
		struct rect const glyph_error_uv = glyph_error->uv;

		for (uint32_t strings_i = word->codepoints_offset; strings_i < word->codepoints_end; strings_i++) {
			uint32_t const * codepoint = array_at(&batcher->codepoints, strings_i);

			if (codepoint_is_invisible(*codepoint)) { continue; }

			struct Glyph const * glyph = font_get_glyph(font_asset->font, *codepoint, word->size);
			struct rect const uv = (glyph != NULL) ? glyph->uv : glyph_error_uv;

			struct Batcher_2D_Vertex * vertices = array_at(&batcher->buffer_vertices, vertices_offset);
			vertices[0].tex_coord = uv.min;
			vertices[1].tex_coord = (struct vec2){uv.max.x, uv.min.y};
			vertices[2].tex_coord = uv.max;
			vertices[3].tex_coord = (struct vec2){uv.min.x, uv.max.y};

			vertices_offset += 4;
		}
	}
}

void batcher_2d_clear(struct Batcher_2D * batcher) {
	common_memset(&batcher->batch, 0, sizeof(batcher->batch));
	array_clear(&batcher->codepoints);
	array_clear(&batcher->batches);
	array_clear(&batcher->words);
	array_clear(&batcher->buffer_vertices);
	array_clear(&batcher->buffer_indices);
	gfx_uniforms_clear(&batcher->uniforms);
}

void batcher_2d_issue_commands(struct Batcher_2D * batcher, struct Array * gpu_commands) {
	batcher_2d_internal_bake_pass(batcher);
	for (uint32_t i = 0; i < batcher->batches.count; i++) {
		struct Batcher_2D_Batch const * batch = array_at(&batcher->batches, i);

		struct Handle program_handle = {0};
		if (!handle_is_null(batch->material)) {
			struct Gfx_Material const * material = material_system_take(batch->material);
			program_handle = material->gpu_program_handle;
			array_push_many(gpu_commands, 1, &(struct GPU_Command){
				.type = GPU_COMMAND_TYPE_MATERIAL,
				.as.material = {
					.handle = batch->material,
				},
			});
		}

		if (!handle_is_null(batch->shader)) {
			program_handle = batch->shader;
			array_push_many(gpu_commands, 1, &(struct GPU_Command){
				.type = GPU_COMMAND_TYPE_SHADER,
				.as.shader = {
					.handle = batch->shader,
					.blend_mode = batch->blend_mode,
					.depth_mode = batch->depth_mode,
				},
			});
		}

		if (batch->uniform_length > 0) {
			array_push_many(gpu_commands, 1, &(struct GPU_Command){
				.type = GPU_COMMAND_TYPE_UNIFORM,
				.as.uniform = {
					.program_handle = program_handle,
					.uniforms = &batcher->uniforms,
					.offset = batch->uniform_offset,
					.count = batch->uniform_length,
				},
			});
		}

		if (batch->indices_length > 0) {
			array_push_many(gpu_commands, 1, &(struct GPU_Command){
				.type = GPU_COMMAND_TYPE_DRAW,
				.as.draw = {
					.mesh_handle = batcher->gpu_mesh_handle,
					.offset = batch->indices_offset, .length = batch->indices_length,
				},
			});
		}
	}
	array_clear(&batcher->batches);
}

void batcher_2d_bake(struct Batcher_2D * batcher) {
	if (batcher->batches.count > 0) {
		WRN("unissued batches");
		REPORT_CALLSTACK(); DEBUG_BREAK();
	}
	if (batcher->batch.indices_offset < batcher->buffer_indices.count) {
		WRN("unissued indices");
		REPORT_CALLSTACK(); DEBUG_BREAK();
	}

	batcher_2d_bake_words(batcher);
	gpu_mesh_update(batcher->gpu_mesh_handle, &(struct Mesh){
		.count = BATCHER_2D_BUFFERS_COUNT,
		.buffers = (struct Buffer[BATCHER_2D_BUFFERS_COUNT]){
			(struct Buffer){
				.data = batcher->buffer_vertices.data,
				.size = sizeof(struct Batcher_2D_Vertex) * batcher->buffer_vertices.count,
			},
			(struct Buffer){
				.data = batcher->buffer_indices.data,
				.size = sizeof(uint32_t) * batcher->buffer_indices.count,
			},
		},
		.parameters = batcher->mesh_parameters,
	});
}
