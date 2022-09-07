#include "framework/common.h"
#include "framework/memory.h"
#include "framework/unicode.h"
#include "framework/logger.h"

#include "framework/containers/hash_set_u64.h"
#include "framework/containers/array_any.h"
#include "framework/containers/array_flt.h"
#include "framework/containers/array_u32.h"
#include "framework/containers/ref.h"

#include "framework/assets/mesh.h"
#include "framework/assets/font_atlas.h"

#include "framework/graphics/material.h"
#include "framework/graphics/gpu_objects.h"
#include "framework/graphics/gpu_misc.h"
#include "framework/graphics/gpu_command.h"
#include "framework/maths.h"

#include "asset_types.h"

// @todo: UTF-8 edge cases, different languages, LTR/RTL, ligatures, etc.
// @idea: point to a `Gfx_Material` with an `Asset_Ref` instead
// @idea: static batching option; cache vertices and stuff until changed
// @idea: add a 3d batcher

// quad layout
// 1-----------3
// |         / |
// |       /   |
// |     /     |
// |   /       |
// | /         |
// 0-----------2

#define BATCHER_2D_BUFFERS_COUNT 2
#define BATCHER_2D_ATTRIBUTES_COUNT 2

struct Batcher_2D_Text {
	uint32_t codepoints_from, codepoints_to;
	uint32_t buffer_vertices_offset;
	// @idea: use `Asset_Ref` instead
	struct Asset_Font const * font; float size;
	// @note: local to `batcher_2d_add_text`
	uint32_t breaker_codepoint;
	struct vec2 position;
	float full_size_x;
};

struct Batcher_2D_Batch {
	uint32_t offset, length;
	struct Gfx_Material const * material; // @idea: internalize materials if async
};

struct Batcher_2D_Vertex {
	struct vec2 position;
	struct vec2 tex_coord;
};

struct Batcher_2D {
	struct Batcher_2D_Batch batch;
	struct Array_U32 codepoints;
	struct Array_Any batches;        // `struct Batcher_2D_Batch`
	struct Array_Any texts;          // `struct Batcher_2D_Text`
	struct Hash_Set_U64 fonts_cache; // `struct Asset_Font const *`
	//
	struct mat4 matrix;
	//
	struct Array_Any buffer_vertices; // `struct Batcher_2D_Vertex`
	struct Array_U32 buffer_indices;
	//
	struct Mesh_Parameters mesh_parameters[BATCHER_2D_BUFFERS_COUNT];
	struct Ref gpu_mesh_ref;
};

static void batcher_2d_bake_pass(struct Batcher_2D * batcher);

//
#include "batcher_2d.h"

struct Batcher_2D * batcher_2d_init(void) {
	struct Batcher_2D * batcher = MEMORY_ALLOCATE(struct Batcher_2D);
	*batcher = (struct Batcher_2D){
		.matrix = c_mat4_identity,
		.mesh_parameters = {
			(struct Mesh_Parameters){
				.type = DATA_TYPE_R32_F,
				.flags = MESH_FLAG_MUTABLE | MESH_FLAG_WRITE | MESH_FLAG_FREQUENT,
				.attributes_count = BATCHER_2D_ATTRIBUTES_COUNT,
				.attributes = {
					ATTRIBUTE_TYPE_POSITION,
					SIZE_OF_MEMBER(struct Batcher_2D_Vertex, position) / sizeof(float),
					ATTRIBUTE_TYPE_TEXCOORD,
					SIZE_OF_MEMBER(struct Batcher_2D_Vertex, tex_coord) / sizeof(float),
				},
			},
			(struct Mesh_Parameters){
				.type = DATA_TYPE_R32_U,
				.flags = MESH_FLAG_INDEX | MESH_FLAG_MUTABLE | MESH_FLAG_WRITE | MESH_FLAG_FREQUENT,
			},
		},
		.codepoints      = array_u32_init(),
		.batches         = array_any_init(sizeof(struct Batcher_2D_Batch)),
		.texts           = array_any_init(sizeof(struct Batcher_2D_Text)),
		.buffer_vertices = array_any_init(sizeof(struct Batcher_2D_Vertex)),
		.buffer_indices  = array_u32_init(),
	};

	//
	batcher->gpu_mesh_ref = gpu_mesh_init(&(struct Mesh){
		.count      = BATCHER_2D_BUFFERS_COUNT,
		.buffers    = (struct Buffer[BATCHER_2D_BUFFERS_COUNT]){0},
		.parameters = batcher->mesh_parameters,
	});

	return batcher;
}

void batcher_2d_free(struct Batcher_2D * batcher) {
	gpu_mesh_free(batcher->gpu_mesh_ref);
	//
	array_u32_free(&batcher->codepoints);
	array_any_free(&batcher->batches);
	array_any_free(&batcher->texts);
	hash_set_u64_free(&batcher->fonts_cache);
	array_any_free(&batcher->buffer_vertices);
	array_u32_free(&batcher->buffer_indices);
	//
	common_memset(batcher, 0, sizeof(*batcher));
	MEMORY_FREE(batcher);
}

void batcher_2d_set_matrix(struct Batcher_2D * batcher, struct mat4 const * matrix) {
	batcher->matrix = (matrix != NULL) ? *matrix : c_mat4_identity;
}

void batcher_2d_set_material(struct Batcher_2D * batcher, struct Gfx_Material const * material) {
	if (batcher->batch.material != material) {
		batcher_2d_bake_pass(batcher);
	}
	batcher->batch.material = material;
}

inline static struct Batcher_2D_Vertex batcher_2d_make_vertex(struct mat4 m, struct vec2 position, struct vec2 tex_coord);
void batcher_2d_add_quad(
	struct Batcher_2D * batcher,
	struct rect rect,
	struct rect uv
) {
	uint32_t const vertex_offset = batcher->buffer_vertices.count;
	array_any_push_many(&batcher->buffer_vertices, 4, (struct Batcher_2D_Vertex[]){
		batcher_2d_make_vertex(batcher->matrix, rect.min, uv.min),
		batcher_2d_make_vertex(batcher->matrix, (struct vec2){rect.min.x, rect.max.y}, (struct vec2){uv.min.x, uv.max.y}),
		batcher_2d_make_vertex(batcher->matrix, (struct vec2){rect.max.x, rect.min.y}, (struct vec2){uv.max.x, uv.min.y}),
		batcher_2d_make_vertex(batcher->matrix, rect.max, uv.max),
	});
	array_u32_push_many(&batcher->buffer_indices, 3 * 2, (uint32_t[]){
		vertex_offset + 3, vertex_offset + 1, vertex_offset + 0,
		vertex_offset + 0, vertex_offset + 2, vertex_offset + 3,
	});
}

void batcher_2d_add_text(
	struct Batcher_2D * batcher,
	struct rect rect, struct vec2 alignment, bool wrap,
	struct Asset_Font const * font, struct CString value, float size
) {
	float const scale        = font_atlas_get_scale(font->font_atlas, size);
	float const font_ascent  = font_atlas_get_ascent(font->font_atlas, scale);
	float const font_descent = font_atlas_get_descent(font->font_atlas, scale);
	float const line_gap     = font_atlas_get_gap(font->font_atlas, scale);
	float const line_height  = font_ascent - font_descent + line_gap;

	uint32_t const texts_offset = batcher->texts.count;

	font_atlas_add_default_glyphs(font->font_atlas, size);
	struct Font_Glyph const * glyph_error = font_atlas_get_glyph(font->font_atlas, '\0', size);

	// translate text data into crude text blocks
	{
		float block_width = 0;
		uint32_t block_strings_offset = batcher->codepoints.count;

		FOR_UTF8 (value.length, (uint8_t const *)value.data, it) {
			font_atlas_add_glyph(font->font_atlas, it.codepoint, size);

			struct Font_Glyph const * glyph = font_atlas_get_glyph(font->font_atlas, it.codepoint, size);
			float const full_size_x = (glyph != NULL) ? glyph->params.full_size_x : glyph_error->params.full_size_x;

			block_width += full_size_x;
			if (codepoint_is_block_break(it.codepoint)) {
				// @todo: (?) arena/stack allocator
				array_any_push_many(&batcher->texts, 1, &(struct Batcher_2D_Text) {
					.codepoints_from = block_strings_offset, .codepoints_to = batcher->codepoints.count,
					.font = font, .size = size,
					.breaker_codepoint = it.codepoint,
					.full_size_x = block_width,
				});

				//
				block_width = 0;
				block_strings_offset = batcher->codepoints.count;
			}

			if (!codepoint_is_invisible(it.codepoint)) {
				// @todo: (?) arena/stack allocator
				array_u32_push_many(&batcher->codepoints, 1, &it.codepoint);

				//
				float const kerning = font_atlas_get_kerning(font->font_atlas, it.previous, it.codepoint, scale);
				block_width += kerning;
			}
		}

		if (batcher->codepoints.count > block_strings_offset) {
			// @todo: (?) arena/stack allocator
			array_any_push_many(&batcher->texts, 1, &(struct Batcher_2D_Text) {
				.codepoints_from = block_strings_offset, .codepoints_to = batcher->codepoints.count,
				.font = font, .size = size,
				.full_size_x = block_width,
			});
		}
	}

	// position text blocks as for alignment {0, 1}
	{
		struct vec2 offset = {rect.min.x, rect.max.y};
		offset.y -= font_ascent;

		for (uint32_t block_i = texts_offset; block_i < batcher->texts.count; block_i++) {
			struct Batcher_2D_Text * text = array_any_at(&batcher->texts, block_i);

			// break line if block overflows
			if (wrap && (offset.x + text->full_size_x > rect.max.x)) {
				offset.x = rect.min.x; // @note: auto `\r`
				offset.y -= line_height;
			}
			text->position = offset;

			// process block
			for (uint32_t strings_i = text->codepoints_from; strings_i < text->codepoints_to; strings_i++) {
				uint32_t const codepoint = array_u32_at(&batcher->codepoints, strings_i);
				uint32_t const previous = (strings_i > text->codepoints_from) ? array_u32_at(&batcher->codepoints, strings_i - 1) : '\0';

				struct Font_Glyph const * glyph = font_atlas_get_glyph(text->font->font_atlas, codepoint, text->size);
				float const full_size_x = (glyph != NULL) ? glyph->params.full_size_x : glyph_error->params.full_size_x;

				float const kerning = font_atlas_get_kerning(font->font_atlas, previous, codepoint, scale);
				offset.x += full_size_x + kerning;
			}

			// process breaker
			{
				struct Font_Glyph const * glyph = font_atlas_get_glyph(text->font->font_atlas, text->breaker_codepoint, text->size);
				offset.x += glyph->params.full_size_x;
				if (text->breaker_codepoint == '\n') {
					offset.x = rect.min.x; // @note: auto `\r`
					offset.y -= line_height;
				}
			}
		}
	}

	// align text blocks
	{
		struct vec2 const error_margins = {
			0.0001f * (1 - 2*alignment.x),
			0.0001f * (1 - 2*alignment.y),
		};
		struct vec2 const rect_size = {
			rect.max.x - rect.min.x,
			rect.max.y - rect.min.y,
		};

		uint32_t line_offset = texts_offset;
		uint32_t lines_count = 1;
		float line_position_y = 0;
		float line_width = 0;

		for (uint32_t block_i = texts_offset; block_i < batcher->texts.count; block_i++) {
			struct Batcher_2D_Text const * text = array_any_at(&batcher->texts, block_i);

			if (line_position_y != text->position.y) {
				float const offset = lerp(0, rect_size.x - line_width, alignment.x) + error_margins.x;
				for (uint32_t i = line_offset; i < block_i; i++) {
					struct Batcher_2D_Text * aligned_text = array_any_at(&batcher->texts, i);
					aligned_text->position.x += offset;
				}
				line_offset = block_i;
				lines_count++;

				//
				line_position_y = text->position.y;
				line_width = 0;
			}

			line_width += text->full_size_x;
		}

		{
			float const offset = lerp(0, rect_size.x - line_width, alignment.x) + error_margins.x;
			for (uint32_t i = line_offset; i < batcher->texts.count; i++) {
				struct Batcher_2D_Text * aligned_text = array_any_at(&batcher->texts, i);
				aligned_text->position.x += offset;
			}
		}

		{
			float const height = (float)lines_count * line_height;
			float const offset = lerp((height - rect_size.y) - line_height, 0, alignment.y) + error_margins.y;
			for (uint32_t i = texts_offset; i < batcher->texts.count; i++) {
				struct Batcher_2D_Text * aligned_text = array_any_at(&batcher->texts, i);
				aligned_text->position.y += offset;
			}
		}
	}

	// translate text blocks into crude vertices
	for (uint32_t block_i = texts_offset; block_i < batcher->texts.count; block_i++) {
		struct Batcher_2D_Text * text = array_any_at(&batcher->texts, block_i);
		text->buffer_vertices_offset = batcher->buffer_vertices.count;

		struct vec2 offset = text->position;
		if (offset.y > rect.max.y) { text->codepoints_to = text->codepoints_from; continue; } // void entry
		if (offset.y < rect.min.y) { batcher->texts.count = block_i;              break; }    // drop rest

		for (uint32_t strings_i = text->codepoints_from; strings_i < text->codepoints_to; strings_i++) {
			uint32_t const codepoint = array_u32_at(&batcher->codepoints, strings_i);
				uint32_t const previous = (strings_i > text->codepoints_from) ? array_u32_at(&batcher->codepoints, strings_i - 1) : '\0';

			struct Font_Glyph const * glyph = font_atlas_get_glyph(text->font->font_atlas, codepoint, text->size);
			struct Font_Glyph_Params const params = (glyph != NULL) ? glyph->params : glyph_error->params;

			float const kerning = font_atlas_get_kerning(font->font_atlas, previous, codepoint, scale);
			float const offset_x = offset.x + kerning;
			offset.x += params.full_size_x + kerning;

			if (offset_x < rect.min.x) { text->codepoints_from = strings_i + 1; continue; } // skip glyph
			if (offset.x > rect.max.x) { text->codepoints_to   = strings_i;     break; }    // drop rest

			if (!codepoint_is_invisible(codepoint)) {
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
					(struct rect){0} // @note: UVs are delayed, see `batcher_2d_bake_texts`
				);
			}
		}
	}
}

static void batcher_2d_bake_texts(struct Batcher_2D * batcher) {
	if (batcher->texts.count == 0) { return; }

	// render and upload the atlases
	{
		// @todo: (?) arena/stack allocator
		hash_set_u64_clear(&batcher->fonts_cache);

		for (uint32_t i = 0; i < batcher->texts.count; i++) {
			struct Batcher_2D_Text const * text = array_any_at(&batcher->texts, i);
			hash_set_u64_set(&batcher->fonts_cache, (uint64_t)text->font);
		}

		FOR_HASH_SET_U64 (&batcher->fonts_cache, it) {
			struct Asset_Font const * font = (void *)it.key_hash;
			font_atlas_render(font->font_atlas);
		}

		FOR_HASH_SET_U64 (&batcher->fonts_cache, it) {
			struct Asset_Font const * font = (void *)it.key_hash;
			gpu_texture_update(font->gpu_ref, font_atlas_get_asset(font->font_atlas));
		}
	}

	// fill quads UVs
	for (uint32_t block_i = 0; block_i < batcher->texts.count; block_i++) {
		struct Batcher_2D_Text const * text = array_any_at(&batcher->texts, block_i);
		uint32_t vertices_offset = text->buffer_vertices_offset;

		struct Font_Glyph const * glyph_error = font_atlas_get_glyph(text->font->font_atlas, '\0', text->size);
		struct rect const glyph_error_uv = glyph_error->uv;

		for (uint32_t strings_i = text->codepoints_from; strings_i < text->codepoints_to; strings_i++) {
			uint32_t const codepoint = array_u32_at(&batcher->codepoints, strings_i);

			if (codepoint_is_invisible(codepoint)) { continue; }

			struct Font_Glyph const * glyph = font_atlas_get_glyph(text->font->font_atlas, codepoint, text->size);
			struct rect const uv = (glyph != NULL) ? glyph->uv : glyph_error_uv;

			struct Batcher_2D_Vertex * vertices = array_any_at(&batcher->buffer_vertices, vertices_offset);
			vertices[0].tex_coord = uv.min;
			vertices[1].tex_coord = (struct vec2){uv.min.x, uv.max.y};
			vertices[2].tex_coord = (struct vec2){uv.max.x, uv.min.y};
			vertices[3].tex_coord = uv.max;

			vertices_offset += 4;
		}
	}
}

void batcher_2d_clear(struct Batcher_2D * batcher) {
	common_memset(&batcher->batch, 0, sizeof(batcher->batch));
	array_u32_clear(&batcher->codepoints);
	array_any_clear(&batcher->batches);
	array_any_clear(&batcher->texts);
	array_any_clear(&batcher->buffer_vertices);
	array_u32_clear(&batcher->buffer_indices);
}

void batcher_2d_issue_commands(struct Batcher_2D * batcher, struct Array_Any * gpu_commands) {
	batcher_2d_bake_pass(batcher);
	for (uint32_t i = 0; i < batcher->batches.count; i++) {
		struct Batcher_2D_Batch const * batch = array_any_at(&batcher->batches, i);

		array_any_push_many(gpu_commands, 2, (struct GPU_Command[]){
			(struct GPU_Command){
				.type = GPU_COMMAND_TYPE_MATERIAL,
				.as.material = {
					.material = batch->material,
				},
			},
			(struct GPU_Command){
				.type = GPU_COMMAND_TYPE_DRAW,
				.as.draw = {
					.gpu_mesh_ref = batcher->gpu_mesh_ref,
					.offset = batch->offset, .length = batch->length,
				},
			},
		});
	}
	array_any_clear(&batcher->batches);
}

void batcher_2d_bake(struct Batcher_2D * batcher) {
	if (batcher->batches.count > 0) {
		logger_to_console("unissued batches\n"); DEBUG_BREAK();
	}
	if (batcher->batch.offset < batcher->buffer_indices.count) {
		logger_to_console("unissued indices\n"); DEBUG_BREAK();
	}

	batcher_2d_bake_texts(batcher);
	gpu_mesh_update(batcher->gpu_mesh_ref, &(struct Mesh){
		.count = BATCHER_2D_BUFFERS_COUNT,
		.buffers = (struct Buffer[BATCHER_2D_BUFFERS_COUNT]){
			(struct Buffer){
				.data = batcher->buffer_vertices.data,
				.count = sizeof(struct Batcher_2D_Vertex) * batcher->buffer_vertices.count,
			},
			(struct Buffer){
				.data = (uint8_t *)batcher->buffer_indices.data,
				.count = sizeof(*batcher->buffer_indices.data) * batcher->buffer_indices.count,
			},
		},
		.parameters = batcher->mesh_parameters,
	});
}

//

inline static struct Batcher_2D_Vertex batcher_2d_make_vertex(struct mat4 m, struct vec2 position, struct vec2 tex_coord) {
	return (struct Batcher_2D_Vertex){
		.position = {
			m.x.x * position.x + m.y.x * position.y + m.w.x,
			m.x.y * position.x + m.y.y * position.y + m.w.y,
		},
		.tex_coord = tex_coord,
	};
}

static void batcher_2d_bake_pass(struct Batcher_2D * batcher) {
	uint32_t const offset = batcher->buffer_indices.count;
	if (batcher->batch.offset < offset) {
		batcher->batch.length = offset - batcher->batch.offset;
		array_any_push_many(&batcher->batches, 1, &batcher->batch);

		batcher->batch.offset = offset;
		batcher->batch.length = 0;
	}
}
