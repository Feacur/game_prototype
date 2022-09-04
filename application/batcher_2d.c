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

#define BATCHER_2D_BUFFERS_COUNT 2
#define BATCHER_2D_ATTRIBUTES_COUNT 2

struct Batcher_2D_Text {
	uint32_t strings_from, strings_to;
	uint32_t vertices_offset;
	struct vec2 position_offset;
	// @idea: use `Asset_Ref` instead
	struct Asset_Font const * font; float size;
	uint32_t breaker_codepoint;
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
	struct Array_U32 strings;
	struct Array_Any batches;        // `struct Batcher_2D_Batch`
	struct Array_Any texts;          // `struct Batcher_2D_Text`
	struct Hash_Set_U64 fonts_cache; // `struct Asset_Font const *`
	//
	struct mat4 matrix;
	//
	struct Array_Any buffer_vertices; // `struct Batcher_2D_Vertex`
	struct Array_U32 buffer_indices;
	//
	struct Buffer          mesh_buffers[BATCHER_2D_BUFFERS_COUNT]; // a convenience temporary container
	struct Mesh_Parameters mesh_parameters[BATCHER_2D_BUFFERS_COUNT];
	struct Ref gpu_mesh_ref;
};

static void batcher_2d_bake_mesh_buffers(struct Batcher_2D * batcher);
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
		.strings         = array_u32_init(),
		.batches         = array_any_init(sizeof(struct Batcher_2D_Batch)),
		.texts           = array_any_init(sizeof(struct Batcher_2D_Text)),
		.buffer_vertices = array_any_init(sizeof(struct Batcher_2D_Vertex)),
		.buffer_indices  = array_u32_init(),
	};

	for (uint32_t i = 0; i < BATCHER_2D_BUFFERS_COUNT; i++) {
		batcher->mesh_buffers[i] = buffer_init(NULL);
	}

	//
	batcher_2d_bake_mesh_buffers(batcher);
	batcher->gpu_mesh_ref = gpu_mesh_init(&(struct Mesh){
		.count      = BATCHER_2D_BUFFERS_COUNT,
		.buffers    = batcher->mesh_buffers,
		.parameters = batcher->mesh_parameters,
	});

	return batcher;
}

void batcher_2d_free(struct Batcher_2D * batcher) {
	gpu_mesh_free(batcher->gpu_mesh_ref);
	//
	array_u32_free(&batcher->strings);
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

static void batcher_2d_fill_quad_uv(
	struct Batcher_2D * batcher,
	uint32_t vertices_offset,
	struct rect uv
) {
	struct Batcher_2D_Vertex * vertices = array_any_at(&batcher->buffer_vertices, vertices_offset);
	vertices[0].tex_coord = uv.min;
	vertices[1].tex_coord = (struct vec2){uv.min.x, uv.max.y};
	vertices[2].tex_coord = (struct vec2){uv.max.x, uv.min.y};
	vertices[3].tex_coord = uv.max;
}

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
		vertex_offset + 1, vertex_offset + 0, vertex_offset + 2,
		vertex_offset + 1, vertex_offset + 2, vertex_offset + 3
	});
}

void batcher_2d_add_text(
	struct Batcher_2D * batcher,
	struct rect rect, struct vec2 alignment, bool wrap,
	struct Asset_Font const * font, uint32_t length, uint8_t const * data, float size
) {
	float const scale        = font_atlas_get_scale(font->font_atlas, size);
	float const font_ascent  = font_atlas_get_ascent(font->font_atlas, scale);
	float const font_descent = font_atlas_get_descent(font->font_atlas, scale);
	float const line_gap     = font_atlas_get_gap(font->font_atlas, scale);
	float const line_height  = font_ascent - font_descent + line_gap;

	uint32_t const texts_offset = batcher->texts.count;

	font_atlas_add_default_glyphs(font->font_atlas, size);
	struct Font_Glyph const * glyph_error = font_atlas_get_glyph(font->font_atlas, CODEPOINT_EMPTY, size);

	// @note: other values are not supported yet
	if (alignment.x != 0 || alignment.y != 1) { DEBUG_BREAK(); }
	alignment = (struct vec2){0, 1};

	// translate text data into crude text blocks
	{
		float block_width = 0;
		uint32_t block_strings_offset = batcher->strings.count;

		uint32_t previous_codepoint = CODEPOINT_EMPTY;
		FOR_UTF8 (length, data, it) {
			font_atlas_add_glyph(font->font_atlas, it.codepoint, size);

			struct Font_Glyph const * glyph = font_atlas_get_glyph(font->font_atlas, it.codepoint, size);
			float const full_size_x = (glyph != NULL) ? glyph->params.full_size_x : glyph_error->params.full_size_x;

			block_width += full_size_x;
			if (codepoint_is_block_break(it.codepoint)) {
				// @todo: (?) arena/stack allocator
				array_any_push_many(&batcher->texts, 1, &(struct Batcher_2D_Text) {
					.strings_from = block_strings_offset, .strings_to = batcher->strings.count,
					.font = font, .size = size,
					.breaker_codepoint = it.codepoint,
					.full_size_x = block_width,
				});

				//
				block_width = 0;
				block_strings_offset = batcher->strings.count;
			}

			if (!codepoint_is_invisible(it.codepoint)) {
				// @todo: (?) arena/stack allocator
				array_u32_push_many(&batcher->strings, 1, (uint32_t[]){
					(glyph != NULL) ? it.codepoint : CODEPOINT_EMPTY,
				});

				//
				float const kerning = font_atlas_get_kerning(font->font_atlas, previous_codepoint, it.codepoint, scale);
				block_width -= kerning;
			}

			previous_codepoint = it.codepoint;
		}

		if (batcher->strings.count > block_strings_offset) {
			// @todo: (?) arena/stack allocator
			array_any_push_many(&batcher->texts, 1, &(struct Batcher_2D_Text) {
				.strings_from = block_strings_offset, .strings_to = batcher->strings.count,
				.font = font, .size = size,
				.full_size_x = block_width,
			});
		}
	}

	// adjust positions of text blocks
	{
		struct vec2 offset = {
			lerp(rect.min.x, rect.max.x, alignment.x),
			lerp(rect.min.y, rect.max.y, alignment.y),
		};
		offset.y -= font_ascent;

		for (uint32_t block_i = texts_offset; block_i < batcher->texts.count; block_i++) {
			struct Batcher_2D_Text * text = array_any_at(&batcher->texts, block_i);

			// break line if block overflows
			if (wrap && (offset.x + text->full_size_x > rect.max.x)) {
				offset.x = lerp(rect.min.x, rect.max.x, alignment.x); // @note: auto `\r`
				offset.y -= line_height;
			}
			text->position_offset = offset;

			// process block
			uint32_t previous_codepoint = CODEPOINT_EMPTY;
			for (uint32_t strings_i = text->strings_from; strings_i < text->strings_to; strings_i++) {
				uint32_t const codepoint = array_u32_at(&batcher->strings, strings_i);

				struct Font_Glyph const * glyph = font_atlas_get_glyph(text->font->font_atlas, codepoint, text->size);

				float const kerning = font_atlas_get_kerning(font->font_atlas, previous_codepoint, codepoint, scale);
				previous_codepoint = codepoint;

				offset.x += glyph->params.full_size_x - kerning;
			}

			// process breaker
			{
				struct Font_Glyph const * glyph = font_atlas_get_glyph(text->font->font_atlas, text->breaker_codepoint, text->size);
				offset.x += glyph->params.full_size_x;
				if (text->breaker_codepoint == '\n') {
					offset.x = lerp(rect.min.x, rect.max.x, alignment.x); // @note: auto `\r`
					offset.y -= line_height;
				}
			}
		}
	}

	// translate text blocks into crude vertices
	for (uint32_t block_i = texts_offset; block_i < batcher->texts.count; block_i++) {
		struct Batcher_2D_Text * text = array_any_at(&batcher->texts, block_i);
		text->vertices_offset = batcher->buffer_vertices.count;

		struct vec2 offset = text->position_offset;
		if (offset.y > rect.max.y) { text->strings_to = text->strings_from; continue; } // void entry
		if (offset.y < rect.min.y) { batcher->texts.count = block_i;        break; }    // drop entry

		uint32_t previous_codepoint = CODEPOINT_EMPTY;
		for (uint32_t strings_i = text->strings_from; strings_i < text->strings_to; strings_i++) {
			uint32_t const codepoint = array_u32_at(&batcher->strings, strings_i);

			struct Font_Glyph const * glyph = font_atlas_get_glyph(text->font->font_atlas, codepoint, text->size);

			float const kerning = font_atlas_get_kerning(font->font_atlas, previous_codepoint, codepoint, scale);
			previous_codepoint = codepoint;

			float const offset_x = offset.x + kerning;
			offset.x += glyph->params.full_size_x - kerning;


			if (offset_x < rect.min.x) { text->strings_from = strings_i + 1; continue; } // skip glyph
			if (offset.x > rect.max.x) { text->strings_to   = strings_i + 1; break; }    // skip glyph

			if (!codepoint_is_invisible(codepoint)) {
				batcher_2d_add_quad(
					batcher,
					(struct rect){
						.min = {
							((float)glyph->params.rect.min.x) + offset_x,
							((float)glyph->params.rect.min.y) + offset.y,
						},
						.max = {
							((float)glyph->params.rect.max.x) + offset_x,
							((float)glyph->params.rect.max.y) + offset.y,
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
		uint32_t vertices_offset = text->vertices_offset;

		for (uint32_t strings_i = text->strings_from; strings_i < text->strings_to; strings_i++) {
			uint32_t const codepoint = array_u32_at(&batcher->strings, strings_i);

			if (codepoint_is_invisible(codepoint)) { continue; }

			struct Font_Glyph const * glyph = font_atlas_get_glyph(text->font->font_atlas, codepoint, text->size);
			batcher_2d_fill_quad_uv(batcher, vertices_offset, glyph->uv);
			vertices_offset += 4;
		}
	}
}

void batcher_2d_clear(struct Batcher_2D * batcher) {
	common_memset(&batcher->batch, 0, sizeof(batcher->batch));
	array_u32_clear(&batcher->strings);
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
	batcher_2d_bake_texts(batcher);
	batcher_2d_bake_mesh_buffers(batcher);
	gpu_mesh_update(batcher->gpu_mesh_ref, &(struct Mesh){
		.count      = BATCHER_2D_BUFFERS_COUNT,
		.buffers    = batcher->mesh_buffers,
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

static void batcher_2d_bake_mesh_buffers(struct Batcher_2D * batcher) {
	batcher->mesh_buffers[0] = (struct Buffer){
		.data = batcher->buffer_vertices.data,
		.count = sizeof(struct Batcher_2D_Vertex) * batcher->buffer_vertices.count,
	};
	batcher->mesh_buffers[1] = (struct Buffer){
		.data = (uint8_t *)batcher->buffer_indices.data,
		.count = sizeof(*batcher->buffer_indices.data) * batcher->buffer_indices.count,
	};
}
