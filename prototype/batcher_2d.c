#include "framework/common.h"
#include "framework/memory.h"
#include "framework/unicode.h"
#include "framework/logger.h"

#include "framework/containers/hash_set_u64.h"
#include "framework/containers/array_any.h"
#include "framework/containers/array_float.h"
#include "framework/containers/array_u32.h"
#include "framework/containers/ref.h"

#include "framework/assets/mesh.h"

#include "framework/graphics/material.h"
#include "framework/graphics/font_image.h"
#include "framework/graphics/gpu_objects.h"
#include "framework/graphics/gpu_misc.h"
#include "framework/graphics/gpu_pass.h"
#include "framework/maths.h"

#include "asset_types.h"

// @todo: UTF-8 edge cases, different languages, LTR/RTL, ligatures, etc.
// @idea: point to a `Gfx_Material` with an `Asset_Ref` instead
// @idea: static batching option; cache vertices and stuff until changed
// @idea: add a 3d batcher

#define BATCHER_2D_BUFFERS_COUNT 2
#define BATCHER_2D_ATTRIBUTES_COUNT 2

struct Batcher_2D_Text {
	uint32_t length;
	uint8_t const * data;
	struct Asset_Font const * font; // @idea: use `Asset_Ref` instead
	uint32_t vertices_offset, indices_offset;
	struct mat4 matrix;
	struct vec2 rect_min, rect_max, local_offset;
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
	struct Array_Any batches;
	struct Array_Any texts;
	//
	struct mat4 matrix;
	struct Array_Any matrices;
	//
	struct Array_Any buffer_vertices;
	struct Array_U32 buffer_indices;
	//
	struct Array_Byte      mesh_buffers[BATCHER_2D_BUFFERS_COUNT];
	struct Mesh_Parameters mesh_parameters[BATCHER_2D_BUFFERS_COUNT];
	struct Ref gpu_mesh_ref;
};

static void batcher_2d_update_asset(struct Batcher_2D * batcher);

//
#include "batcher_2d.h"

struct Batcher_2D * batcher_2d_init(void) {
	struct Batcher_2D * batcher = MEMORY_ALLOCATE(NULL, struct Batcher_2D);
	*batcher = (struct Batcher_2D){
		.matrix = mat4_identity,
	};
	array_any_init(&batcher->batches,  sizeof(struct Batcher_2D_Batch));
	array_any_init(&batcher->texts,    sizeof(struct Batcher_2D_Text));
	array_any_init(&batcher->matrices, sizeof(struct mat4));
	array_any_init(&batcher->buffer_vertices, sizeof(struct Batcher_2D_Vertex));
	array_u32_init(&batcher->buffer_indices);

	for (uint32_t i = 0; i < BATCHER_2D_BUFFERS_COUNT; i++) {
		array_byte_init(batcher->mesh_buffers + i);
	}

	//
	batcher->mesh_parameters[0] = (struct Mesh_Parameters){
		.type = DATA_TYPE_R32,
		.flags = MESH_FLAG_MUTABLE | MESH_FLAG_WRITE | MESH_FLAG_FREQUENT,
		.attributes_count = BATCHER_2D_ATTRIBUTES_COUNT,
		.attributes[0] = ATTRIBUTE_TYPE_POSITION,
		.attributes[1] = SIZE_OF_MEMBER(struct Batcher_2D_Vertex, position) / sizeof(float),
		.attributes[2] = ATTRIBUTE_TYPE_TEXCOORD,
		.attributes[3] = SIZE_OF_MEMBER(struct Batcher_2D_Vertex, tex_coord) / sizeof(float),
	};
	batcher->mesh_parameters[1] = (struct Mesh_Parameters){
		.type = DATA_TYPE_U32,
		.flags = MESH_FLAG_INDEX | MESH_FLAG_MUTABLE | MESH_FLAG_WRITE | MESH_FLAG_FREQUENT,
	};

	//
	batcher_2d_update_asset(batcher);
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
	array_any_free(&batcher->batches);
	array_any_free(&batcher->texts);
	array_any_free(&batcher->matrices);
	array_any_free(&batcher->buffer_vertices);
	array_u32_free(&batcher->buffer_indices);
	//
	common_memset(batcher, 0, sizeof(*batcher));
	MEMORY_FREE(batcher, batcher);
}

static void batcher_2d_bake_pass(struct Batcher_2D * batcher) {
	uint32_t const offset = batcher->buffer_indices.count;
	if (batcher->batch.offset < offset) {
		batcher->batch.length = offset - batcher->batch.offset;
		array_any_push(&batcher->batches, &batcher->batch);

		batcher->batch.offset = offset;
		batcher->batch.length = 0;
	}
}

void batcher_2d_push_matrix(struct Batcher_2D * batcher, struct mat4 matrix) {
	array_any_push(&batcher->matrices, &batcher->matrix);
	batcher->matrix = matrix;
}

void batcher_2d_pop_matrix(struct Batcher_2D * batcher) {
	struct mat4 const * matrix = array_any_pop(&batcher->matrices);
	if (matrix != NULL) { batcher->matrix = *matrix; return; }
	logger_to_console("non-matching matrices\n"); DEBUG_BREAK();
	batcher->matrix = mat4_identity;
}

void batcher_2d_set_material(struct Batcher_2D * batcher, struct Gfx_Material * material) {
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
	float const * rect, float const * uv
) {
	array_any_set_many(&batcher->buffer_vertices, vertices_offset, 4, (struct Batcher_2D_Vertex[]){
		batcher_2d_make_vertex(matrix, (struct vec2){rect[0], rect[1]}, (struct vec2){uv[0], uv[1]}),
		batcher_2d_make_vertex(matrix, (struct vec2){rect[0], rect[3]}, (struct vec2){uv[0], uv[3]}),
		batcher_2d_make_vertex(matrix, (struct vec2){rect[2], rect[1]}, (struct vec2){uv[2], uv[1]}),
		batcher_2d_make_vertex(matrix, (struct vec2){rect[2], rect[3]}, (struct vec2){uv[2], uv[3]}),
	});
	array_u32_set_many(&batcher->buffer_indices, indices_offset, 3 * 2, (uint32_t[]){
		vertices_offset + 1, vertices_offset + 0, vertices_offset + 2,
		vertices_offset + 1, vertices_offset + 2, vertices_offset + 3
	});
}

void batcher_2d_add_quad(
	struct Batcher_2D * batcher,
	struct vec2 rect_min, struct vec2 rect_max, struct vec2 pivot,
	float const * uv
) {
	float const local_rect[] = { // @note: offset content to the bottom-left corner
		rect_min.x - pivot.x,
		rect_min.y - pivot.y,
		rect_max.x - pivot.x,
		rect_max.y - pivot.y,
	};

	uint32_t const vertex_offset = batcher->buffer_vertices.count;
	array_any_push_many(&batcher->buffer_vertices, 4, (struct Batcher_2D_Vertex[]){
		batcher_2d_make_vertex(batcher->matrix, (struct vec2){local_rect[0], local_rect[1]}, (struct vec2){uv[0], uv[1]}),
		batcher_2d_make_vertex(batcher->matrix, (struct vec2){local_rect[0], local_rect[3]}, (struct vec2){uv[0], uv[3]}),
		batcher_2d_make_vertex(batcher->matrix, (struct vec2){local_rect[2], local_rect[1]}, (struct vec2){uv[2], uv[1]}),
		batcher_2d_make_vertex(batcher->matrix, (struct vec2){local_rect[2], local_rect[3]}, (struct vec2){uv[2], uv[3]}),
	});
	array_u32_push_many(&batcher->buffer_indices, 3 * 2, (uint32_t[]){
		vertex_offset + 1, vertex_offset + 0, vertex_offset + 2,
		vertex_offset + 1, vertex_offset + 2, vertex_offset + 3
	});
}

void batcher_2d_add_text(
	struct Batcher_2D * batcher,
	struct Asset_Font const * font, uint32_t length, uint8_t const * data,
	struct vec2 rect_min, struct vec2 rect_max, struct vec2 pivot
) {
	// @todo: introduce rect bounds parameter
	//        introduce scroll offset parameter
	//        check bounds, reserve only visible
	//        >
	//        need to know vertices at this point
	//        UVs can be postponed
	// @idea: decode UTF-8 once into an array
	uint32_t codepoints_count = 0;
	for (struct UTF8_Iterator it = {0}; utf8_iterate(length, data, &it); /*empty*/) {
		switch (it.codepoint) {
			case CODEPOINT_ZERO_WIDTH_SPACE: break;

			case ' ':
			case CODEPOINT_NON_BREAKING_SPACE:
				break;

			case '\t': break;

			case '\r': break;
			case '\n': break;

			default: if (it.codepoint > ' ') {
				codepoints_count++;
			} break;
		}
	}

	if (codepoints_count == 0) { return; }

	//
	array_any_push(&batcher->texts, &(struct Batcher_2D_Text) {
		.length = length,
		.data = data,
		.font = font,
		.vertices_offset = batcher->buffer_vertices.count,
		.indices_offset  = batcher->buffer_indices.count,
		.matrix = batcher->matrix,
		.rect_min = rect_min, .rect_max = rect_max,
		.local_offset = { // @note: offset content to the top-left corner, as if for LTR/TTB text
			.x = rect_min.x - pivot.x,
			.y = rect_max.y - pivot.y,
		}
	});

	// reserve blanks for the text
	array_any_push_many(&batcher->buffer_vertices, codepoints_count * 4, NULL);
	array_u32_push_many(&batcher->buffer_indices, codepoints_count * 3 * 2, NULL);
}

static void batcher_2d_update_text(struct Batcher_2D * batcher) {
	if (batcher->texts.count == 0) { return; }

	// add all glyphs
	for (uint32_t i = 0; i < batcher->texts.count; i++) {
		struct Batcher_2D_Text const * text = array_any_at(&batcher->texts, i);
		font_image_add_glyphs_from_text(text->font->buffer, text->length, text->data);
	}

	// render an upload the atlases
	{
		struct Hash_Set_U64 fonts;
		hash_set_u64_init(&fonts);

		for (uint32_t i = 0; i < batcher->texts.count; i++) {
			struct Batcher_2D_Text const * text = array_any_at(&batcher->texts, i);
			hash_set_u64_set(&fonts, (uint64_t)text->font);
		}

		for (struct Hash_Set_U64_Iterator it = {0}; hash_set_u64_iterate(&fonts, &it); /*empty*/) {
			struct Asset_Font const * font = (void *)it.key_hash;
			font_image_add_kerning_all(font->buffer);
		}

		for (struct Hash_Set_U64_Iterator it = {0}; hash_set_u64_iterate(&fonts, &it); /*empty*/) {
			struct Asset_Font const * font = (void *)it.key_hash;
			font_image_render(font->buffer);
		}

		for (struct Hash_Set_U64_Iterator it = {0}; hash_set_u64_iterate(&fonts, &it); /*empty*/) {
			struct Asset_Font const * font = (void *)it.key_hash;
			gpu_texture_update(font->gpu_ref, font_image_get_asset(font->buffer));
		}

		hash_set_u64_free(&fonts);
	}

	// fill quads
	for (uint32_t i = 0; i < batcher->texts.count; i++) {
		struct Batcher_2D_Text const * text = array_any_at(&batcher->texts, i);
		uint32_t vertices_offset = text->vertices_offset;
		uint32_t indices_offset  = text->indices_offset;

		float const line_height = font_image_get_height(text->font->buffer) + font_image_get_gap(text->font->buffer);
		float offset_x = text->local_offset.x;
		float offset_y = text->local_offset.y - line_height;

		struct Font_Glyph const * glyph_space = font_image_get_glyph(text->font->buffer, ' ');
		struct Font_Glyph const * glyph_error = font_image_get_glyph(text->font->buffer, CODEPOINT_EMPTY);

		float const space_size = (glyph_space != NULL) ? glyph_space->params.full_size_x : 0;
		float const tab_size = space_size * 4; // @todo: expose tab scale

		uint32_t previous_codepoint = 0;
		for (struct UTF8_Iterator it = {0}; utf8_iterate(text->length, text->data, &it); /*empty*/) {
			switch (it.codepoint) {
				case CODEPOINT_ZERO_WIDTH_SPACE: break;

				case ' ':
				case CODEPOINT_NON_BREAKING_SPACE:
					offset_x += space_size;
					break;

				case '\t':
					offset_x += tab_size;
					break;

				case '\r':
					// offset_x = text->offset.x;
					break;

				case '\n':
					offset_x = text->local_offset.x; // @note: auto `\r`
					offset_y -= line_height;
					break;

				default: if (it.codepoint > ' ') {
					struct Font_Glyph const * glyph = font_image_get_glyph(text->font->buffer, it.codepoint);
					if (glyph == NULL) { glyph = glyph_error; }

					if (glyph->params.is_empty) { logger_to_console("codepoint '0x%x' has empty glyph\n", it.codepoint); DEBUG_BREAK(); }

					offset_x += font_image_get_kerning(text->font->buffer, previous_codepoint, it.codepoint);

					batcher_2d_fill_quad(
						batcher,
						text->matrix,
						vertices_offset, indices_offset,
						(float[]){
							((float)glyph->params.rect[0]) + offset_x,
							((float)glyph->params.rect[1]) + offset_y,
							((float)glyph->params.rect[2]) + offset_x,
							((float)glyph->params.rect[3]) + offset_y,
						},
						glyph->uv
					);
					vertices_offset += 4;
					indices_offset += 3 * 2;

					offset_x += glyph->params.full_size_x;
				} break;
			}

			previous_codepoint = it.codepoint;
		}
	}
}

void batcher_2d_draw(struct Batcher_2D * batcher) {
	if (batcher->batches.count == 0) { return; }

	// render text into the blanks
	batcher_2d_update_text(batcher);

	// upload the batch mesh
	batcher_2d_update_asset(batcher);
	gpu_mesh_update(batcher->gpu_mesh_ref, &(struct Mesh){
		.count      = BATCHER_2D_BUFFERS_COUNT,
		.buffers    = batcher->mesh_buffers,
		.parameters = batcher->mesh_parameters,
	});

	// render all the passes
	batcher_2d_bake_pass(batcher);
	for (uint32_t i = 0; i < batcher->batches.count; i++) {
		struct Batcher_2D_Batch * batch = array_any_at(&batcher->batches, i);

		graphics_process(&(struct Render_Pass){
			.type = RENDER_PASS_TYPE_DRAW,
			.as.draw = {
				.material = batch->material,
				.gpu_mesh_ref = batcher->gpu_mesh_ref,
				.offset = batch->offset, .length = batch->length,
			},
		});
	}

	//
	common_memset(&batcher->batch, 0, sizeof(batcher->batch));
	array_any_clear(&batcher->batches);
	array_any_clear(&batcher->texts);
	array_any_clear(&batcher->buffer_vertices);
	array_u32_clear(&batcher->buffer_indices);
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

static void batcher_2d_update_asset(struct Batcher_2D * batcher) {
	batcher->mesh_buffers[0] = (struct Array_Byte){
		.data = batcher->buffer_vertices.data,
		.count = sizeof(struct Batcher_2D_Vertex) * batcher->buffer_vertices.count,
	};
	batcher->mesh_buffers[1] = (struct Array_Byte){
		.data = (uint8_t *)batcher->buffer_indices.data,
		.count = sizeof(*batcher->buffer_indices.data) * batcher->buffer_indices.count,
	};
}
