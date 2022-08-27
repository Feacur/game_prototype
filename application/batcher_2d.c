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
	uint32_t length, strings_offset;
	struct Asset_Font const * font; // @idea: use `Asset_Ref` instead
	uint32_t vertices_offset, indices_offset;
	struct mat4 matrix;
	struct rect rect;
	float size;
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
	struct Buffer strings;
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
			[0] = (struct Mesh_Parameters){
				.type = DATA_TYPE_R32_F,
				.flags = MESH_FLAG_MUTABLE | MESH_FLAG_WRITE | MESH_FLAG_FREQUENT,
				.attributes_count = BATCHER_2D_ATTRIBUTES_COUNT,
				.attributes = {
					[0] = ATTRIBUTE_TYPE_POSITION,
					[1] = SIZE_OF_MEMBER(struct Batcher_2D_Vertex, position) / sizeof(float),
					[2] = ATTRIBUTE_TYPE_TEXCOORD,
					[3] = SIZE_OF_MEMBER(struct Batcher_2D_Vertex, tex_coord) / sizeof(float),
				},
			},
			[1] = (struct Mesh_Parameters){
				.type = DATA_TYPE_R32_U,
				.flags = MESH_FLAG_INDEX | MESH_FLAG_MUTABLE | MESH_FLAG_WRITE | MESH_FLAG_FREQUENT,
			},
		},
		.strings         = buffer_init(NULL),
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
	buffer_free(&batcher->strings);
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

// @todo: deduplicate quad methods?
static void batcher_2d_fill_quad(
	struct Batcher_2D * batcher,
	struct mat4 matrix,
	uint32_t vertices_offset, uint32_t indices_offset,
	struct rect rect,
	struct rect uv
) {
	array_any_set_many(&batcher->buffer_vertices, vertices_offset, 4, (struct Batcher_2D_Vertex[]){
		batcher_2d_make_vertex(matrix, rect.min, uv.min),
		batcher_2d_make_vertex(matrix, (struct vec2){rect.min.x, rect.max.y}, (struct vec2){uv.min.x, uv.max.y}),
		batcher_2d_make_vertex(matrix, (struct vec2){rect.max.x, rect.min.y}, (struct vec2){uv.max.x, uv.min.y}),
		batcher_2d_make_vertex(matrix, rect.max, uv.max),
	});
	array_u32_set_many(&batcher->buffer_indices, indices_offset, 3 * 2, (uint32_t[]){
		vertices_offset + 1, vertices_offset + 0, vertices_offset + 2,
		vertices_offset + 1, vertices_offset + 2, vertices_offset + 3
	});
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
	struct rect rect,
	struct Asset_Font const * font, uint32_t length, uint8_t const * data, float size
) {
	// @todo: introduce rect bounds parameter
	//        introduce scroll offset parameter
	//        check bounds, reserve only visible
	//        >
	//        need to know vertices at this point
	//        UVs can be postponed
	// @idea: decode UTF-8 once into an array
	uint32_t codepoints_count = 0;
	FOR_UTF8 (length, data, it) {
		if (!codepoint_is_visible(it.codepoint)) { continue; }
		codepoints_count++;
	}
	if (codepoints_count == 0) { return; }

	//

	uint32_t const strings_offset = (uint32_t)batcher->strings.count;
	buffer_push_many(&batcher->strings, length + 1, data);

	array_any_push_many(&batcher->texts, 1, &(struct Batcher_2D_Text) {
		.length = length,
		.strings_offset = strings_offset,
		.font = font,
		.vertices_offset = batcher->buffer_vertices.count,
		.indices_offset  = batcher->buffer_indices.count,
		.matrix = batcher->matrix,
		.rect = rect,
		.size = size,
	});

	// reserve blanks for the text
	array_any_push_many(&batcher->buffer_vertices, codepoints_count * 4, NULL);
	array_u32_push_many(&batcher->buffer_indices, codepoints_count * 3 * 2, NULL);
}

static void batcher_2d_bake_texts(struct Batcher_2D * batcher) {
	if (batcher->texts.count == 0) { return; }

	// add all glyphs
	for (uint32_t i = 0; i < batcher->texts.count; i++) {
		struct Batcher_2D_Text const * text = array_any_at(&batcher->texts, i);
		uint8_t const * text_data = (uint8_t *)batcher->strings.data + text->strings_offset;
		font_atlas_add_glyph_error(text->font->font_atlas, text->size);
		font_atlas_add_glyphs_from_text(text->font->font_atlas, text->length, text_data, text->size);
	}

	// render an upload the atlases
	{
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

	// fill quads
	for (uint32_t i = 0; i < batcher->texts.count; i++) {
		struct Batcher_2D_Text const * text = array_any_at(&batcher->texts, i);
		uint8_t const * text_data = (uint8_t *)batcher->strings.data + text->strings_offset;

		uint32_t vertices_offset = text->vertices_offset;
		uint32_t indices_offset  = text->indices_offset;

		float const scale        = font_atlas_get_scale(text->font->font_atlas, text->size);
		float const font_ascent  = font_atlas_get_ascent(text->font->font_atlas, scale);
		float const font_descent = font_atlas_get_descent(text->font->font_atlas, scale);
		float const line_gap     = font_atlas_get_gap(text->font->font_atlas, scale);
		float const line_height  = font_ascent - font_descent + line_gap;

		struct vec2 offset = {
			text->rect.min.x,
			text->rect.max.y - font_ascent,
		};

		struct Font_Glyph const * glyph_space = font_atlas_get_glyph(text->font->font_atlas, ' ', text->size);
		struct Font_Glyph const * glyph_error = font_atlas_get_glyph(text->font->font_atlas, CODEPOINT_EMPTY, text->size);

		float const space_size = (glyph_space != NULL) ? glyph_space->params.full_size_x : 0;
		float const tab_size = space_size * 4; // @todo: expose tab scale

		uint32_t previous_codepoint = 0;
		FOR_UTF8 (text->length, text_data, it) {
			switch (it.codepoint) {
				case CODEPOINT_ZERO_WIDTH_SPACE: break;

				case ' ':
				case CODEPOINT_NON_BREAKING_SPACE:
					offset.x += space_size;
					break;

				case '\t':
					offset.x += tab_size;
					break;

				case '\r':
					// offset.x = text->offset.x;
					break;

				case '\n':
					offset.x = text->rect.min.x; // @note: auto `\r`
					offset.y -= line_height;
					break;

				default: if (it.codepoint > ' ') {
					struct Font_Glyph const * glyph = font_atlas_get_glyph(text->font->font_atlas, it.codepoint, text->size);
					if (glyph == NULL) { glyph = glyph_error; }

					if (glyph->params.is_empty) { logger_to_console("codepoint '0x%x' has empty glyph\n", it.codepoint); DEBUG_BREAK(); }

					offset.x += font_atlas_get_kerning(text->font->font_atlas, previous_codepoint, it.codepoint, scale);

					batcher_2d_fill_quad(
						batcher,
						text->matrix,
						vertices_offset, indices_offset,
						(struct rect){
							.min = {
								((float)glyph->params.rect.min.x) + offset.x,
								((float)glyph->params.rect.min.y) + offset.y,
							},
							.max = {
								((float)glyph->params.rect.max.x) + offset.x,
								((float)glyph->params.rect.max.y) + offset.y,
							},
						},
						glyph->uv
					);
					vertices_offset += 4;
					indices_offset += 3 * 2;

					offset.x += glyph->params.full_size_x;
				} break;
			}

			previous_codepoint = it.codepoint;
		}
	}
}

void batcher_2d_clear(struct Batcher_2D * batcher) {
	common_memset(&batcher->batch, 0, sizeof(batcher->batch));
	buffer_clear(&batcher->strings);
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
