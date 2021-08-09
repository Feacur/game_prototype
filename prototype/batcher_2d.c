
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

#include "framework/graphics/gpu_objects.h"
#include "framework/graphics/graphics.h"
#include "framework/graphics/material.h"
#include "framework/graphics/pass.h"
#include "framework/graphics/font_image.h"
#include "framework/maths.h"

#include "asset_types.h"

#include <string.h>

struct Batcher_2D_Text {
	uint32_t length;
	WEAK_PTR(uint8_t const) data;
	WEAK_PTR(struct Asset_Font const) font; // @todo: use `Asset_Ref` instead?
	uint32_t vertices_offset, indices_offset;
	struct mat4 matrix;
	float x, y;
};

struct Batcher_2D_Batch {
	uint32_t offset, length;
	WEAK_PTR(struct Gfx_Material) material;
	struct Ref gpu_texture_ref;
	struct Blend_Mode blend_mode;
	struct Depth_Mode depth_mode;
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
	struct Array_Any vertices;
	struct Array_U32 indices;
	//
	struct Mesh mesh;
	struct Ref gpu_mesh_ref;
};

static void batcher_2d_update_asset(struct Batcher_2D * batcher);

//
#include "batcher_2d.h"

struct Batcher_2D * batcher_2d_init(void) {
	uint32_t const buffers_count = 2;

	struct Batcher_2D * batcher = MEMORY_ALLOCATE(NULL, struct Batcher_2D);
	*batcher = (struct Batcher_2D){
		.matrix = mat4_identity,
		.mesh = (struct Mesh){
			.count      = buffers_count,
			.buffers    = MEMORY_ALLOCATE_ARRAY(batcher, struct Array_Byte, buffers_count),
			.parameters = MEMORY_ALLOCATE_ARRAY(batcher, struct Mesh_Parameters, buffers_count),
		},
	};
	array_any_init(&batcher->batches,  sizeof(struct Batcher_2D_Batch));
	array_any_init(&batcher->texts,    sizeof(struct Batcher_2D_Text));
	array_any_init(&batcher->matrices, sizeof(struct mat4));
	array_any_init(&batcher->vertices, sizeof(struct Batcher_2D_Vertex));

	//
	uint32_t const attributes[] = {
		ATTRIBUTE_TYPE_POSITION, SIZE_OF_MEMBER(struct Batcher_2D_Vertex, position) / sizeof(float),
		ATTRIBUTE_TYPE_TEXCOORD, SIZE_OF_MEMBER(struct Batcher_2D_Vertex, tex_coord) / sizeof(float),
	};
	uint32_t const attribute_pairs_count = (sizeof(attributes) / sizeof(*attributes));

	array_byte_init(batcher->mesh.buffers + 0);
	array_byte_init(batcher->mesh.buffers + 1);
	batcher->mesh.parameters[0] = (struct Mesh_Parameters){
		.type = DATA_TYPE_R32,
		.flags = MESH_FLAG_MUTABLE | MESH_FLAG_WRITE | MESH_FLAG_FREQUENT,
		.attributes_count = attribute_pairs_count / 2,
	};
	batcher->mesh.parameters[1] = (struct Mesh_Parameters){
		.type = DATA_TYPE_U32,
		.flags = MESH_FLAG_INDEX | MESH_FLAG_MUTABLE | MESH_FLAG_WRITE | MESH_FLAG_FREQUENT,
	};
	memcpy(batcher->mesh.parameters[0].attributes, attributes, sizeof(uint32_t) * attribute_pairs_count);

	//
	array_u32_init(&batcher->indices);

	batcher_2d_update_asset(batcher);
	batcher->gpu_mesh_ref = gpu_mesh_init(&batcher->mesh);

	return batcher;
}

void batcher_2d_free(struct Batcher_2D * batcher) {
	gpu_mesh_free(batcher->gpu_mesh_ref);
	//
	MEMORY_FREE(batcher, batcher->mesh.buffers);
	MEMORY_FREE(batcher, batcher->mesh.parameters);
	array_u32_free(&batcher->indices);
	//
	array_any_free(&batcher->batches);
	array_any_free(&batcher->texts);
	array_any_free(&batcher->matrices);
	array_any_free(&batcher->vertices);
	//
	memset(batcher, 0, sizeof(*batcher));
	MEMORY_FREE(batcher, batcher);
}

static void batcher_2d_bake_pass(struct Batcher_2D * batcher) {
	uint32_t const offset = batcher->indices.count;
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

void batcher_2d_set_blend_mode(struct Batcher_2D * batcher, struct Blend_Mode blend_mode) {
	if (memcmp(&batcher->batch.blend_mode, &blend_mode, sizeof(blend_mode)) != 0) {
		batcher_2d_bake_pass(batcher);
	}
	batcher->batch.blend_mode = blend_mode;
}

void batcher_2d_set_depth_mode(struct Batcher_2D * batcher, struct Depth_Mode depth_mode) {
	if (memcmp(&batcher->batch.depth_mode, &depth_mode, sizeof(depth_mode)) != 0) {
		batcher_2d_bake_pass(batcher);
	}
	batcher->batch.depth_mode = depth_mode;
}

void batcher_2d_set_material(struct Batcher_2D * batcher, struct Gfx_Material * material) {
	if (batcher->batch.material != material) {
		batcher_2d_bake_pass(batcher);
	}
	batcher->batch.material = material;
}

void batcher_2d_set_texture(struct Batcher_2D * batcher, struct Ref gpu_texture_ref) {
	if (batcher->batch.gpu_texture_ref.id != gpu_texture_ref.id || batcher->batch.gpu_texture_ref.gen != gpu_texture_ref.gen) {
		batcher_2d_bake_pass(batcher);
	}
	batcher->batch.gpu_texture_ref = gpu_texture_ref;
}

inline static struct Batcher_2D_Vertex batcher_2d_make_vertex(struct mat4 m, struct vec2 position, struct vec2 tex_coord);

// @todo: deduplicate quad methods?
static void batcher_2d_fill_quad(
	struct Batcher_2D * batcher,
	struct mat4 matrix,
	uint32_t vertices_offset, uint32_t indices_offset,
	float const * rect, float const * uv
) {
	array_any_set_many(&batcher->vertices, vertices_offset, 4, (struct Batcher_2D_Vertex[]){
		batcher_2d_make_vertex(matrix, (struct vec2){rect[0], rect[1]}, (struct vec2){uv[0], uv[1]}),
		batcher_2d_make_vertex(matrix, (struct vec2){rect[0], rect[3]}, (struct vec2){uv[0], uv[3]}),
		batcher_2d_make_vertex(matrix, (struct vec2){rect[2], rect[1]}, (struct vec2){uv[2], uv[1]}),
		batcher_2d_make_vertex(matrix, (struct vec2){rect[2], rect[3]}, (struct vec2){uv[2], uv[3]}),
	});
	array_u32_set_many(&batcher->indices, indices_offset, 3 * 2, (uint32_t[]){
		vertices_offset + 1, vertices_offset + 0, vertices_offset + 2,
		vertices_offset + 1, vertices_offset + 2, vertices_offset + 3
	});
}

void batcher_2d_add_quad(
	struct Batcher_2D * batcher,
	float const * rect, float const * uv
) {
	uint32_t const vertex_offset = batcher->vertices.count;
	array_any_push_many(&batcher->vertices, 4, (struct Batcher_2D_Vertex[]){
		batcher_2d_make_vertex(batcher->matrix, (struct vec2){rect[0], rect[1]}, (struct vec2){uv[0], uv[1]}),
		batcher_2d_make_vertex(batcher->matrix, (struct vec2){rect[0], rect[3]}, (struct vec2){uv[0], uv[3]}),
		batcher_2d_make_vertex(batcher->matrix, (struct vec2){rect[2], rect[1]}, (struct vec2){uv[2], uv[1]}),
		batcher_2d_make_vertex(batcher->matrix, (struct vec2){rect[2], rect[3]}, (struct vec2){uv[2], uv[3]}),
	});
	array_u32_push_many(&batcher->indices, 3 * 2, (uint32_t[]){
		vertex_offset + 1, vertex_offset + 0, vertex_offset + 2,
		vertex_offset + 1, vertex_offset + 2, vertex_offset + 3
	});
}

void batcher_2d_add_text(struct Batcher_2D * batcher, struct Asset_Font const * font, uint32_t length, uint8_t const * data, float x, float y) {
	// @todo: do rect bounds check
	uint32_t codepoints_count = 0;
	for (struct UTF8_Iterator it = {0}; utf8_iterate(length, data, &it); /*empty*/) {
		switch (it.codepoint) {
			case CODEPOINT_ZERO_WIDTH_SPACE: break;

			case ' ':
			case CODEPOINT_NON_BREAKING_SPACE:
				break;

			case '\t': break;

			// case '\r': break;
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
		.vertices_offset = batcher->vertices.count,
		.indices_offset = batcher->indices.count,
		.matrix = batcher->matrix,
		.x = x,
		.y = y,
	});
	font_image_add_glyphs_from_text(font->buffer, length, data);

	// reserve blanks for the text
	array_any_push_many(&batcher->vertices, codepoints_count * 4, NULL);
	array_u32_push_many(&batcher->indices, codepoints_count * 3 * 2, NULL);
}

void batcher_2d_update_text(struct Batcher_2D * batcher) {
	for (uint32_t i = 0; i < batcher->texts.count; i++) {
		struct Batcher_2D_Text const * text = array_any_at(&batcher->texts, i);
		uint32_t vertices_offset = text->vertices_offset;
		uint32_t indices_offset  = text->indices_offset;

		float const line_height = font_image_get_height(text->font->buffer) + font_image_get_gap(text->font->buffer);
		float offset_x = text->x, offset_y = text->y;

		struct Font_Glyph const * glyph_space = font_image_get_glyph(text->font->buffer, ' ');
		struct Font_Glyph const * glyph_error = font_image_get_glyph(text->font->buffer, CODEPOINT_EMPTY);

		float const space_size = (glyph_space != NULL) ? glyph_space->params.full_size_x : 0;
		float const tab_size = space_size * 4;

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

				// case '\r':
				// 	offset_x = x;
				// 	break;

				case '\n':
					offset_x = text->x; // @idea: rely on [now outdated?] `\r`?
					offset_y -= line_height;
					break;

				default: if (it.codepoint > ' ') {
					struct Font_Glyph const * glyph = font_image_get_glyph(text->font->buffer, it.codepoint);
					if (glyph == NULL) { glyph = glyph_error; }

					// @todo: scale whole block, not only glyphs;
					//        spaces should be scaled too!
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

void batcher_2d_draw(struct Batcher_2D * batcher, uint32_t size_x, uint32_t size_y, struct Ref gpu_target_ref) {
	uint32_t const texture_id = graphics_add_uniform("u_Texture");
	uint32_t const color_id   = graphics_add_uniform("u_Color");

	// render text into the blanks
	{
		struct Hash_Set_U64 fonts;
		hash_set_u64_init(&fonts);

		for (uint32_t i = 0; i < batcher->texts.count; i++) {
			struct Batcher_2D_Text const * text = array_any_at(&batcher->texts, i);
			hash_set_u64_set(&fonts, (uint64_t)text->font);
		}

		for (struct Hash_Set_U64_Iterator it = {0}; hash_set_u64_iterate(&fonts, &it); /*empty*/) {
			struct Asset_Font const * font = (void *)it.key_hash;
			font_image_render(font->buffer);
			gpu_texture_update(font->gpu_ref, font_image_get_asset(font->buffer));
			font_image_add_kerning_all(font->buffer);
		}

		hash_set_u64_free(&fonts);
	}
	batcher_2d_update_text(batcher);

	// upload the batch mesh
	batcher_2d_update_asset(batcher);
	gpu_mesh_update(batcher->gpu_mesh_ref, &batcher->mesh);

	// bake leftovers
	batcher_2d_bake_pass(batcher);
	uint32_t passes_count = batcher->batches.count;

	// render all the passes
	for (uint32_t i = 0; i < passes_count; i++) {
		struct Batcher_2D_Batch * batch = array_any_at(&batcher->batches, i);

		gfx_material_set_texture(batch->material, texture_id, 1, &batch->gpu_texture_ref);
		gfx_material_set_float(batch->material, color_id, 4, &(struct vec4){1, 1, 1, 1}.x);

		graphics_draw(&(struct Render_Pass){
			.size_x = size_x, .size_y = size_y,
			.target = gpu_target_ref,
			.blend_mode = batch->blend_mode,
			.depth_mode = batch->depth_mode,
			//
			.material = batch->material,
			.mesh = batcher->gpu_mesh_ref,
			.offset = batch->offset, .length = batch->length,
		});
	}

	//
	memset(&batcher->batch, 0, sizeof(batcher->batch));
	array_any_clear(&batcher->batches);
	array_any_clear(&batcher->texts);
	array_any_clear(&batcher->vertices);
	batcher->indices.count  = 0;
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
	batcher->mesh.buffers[0] = (struct Array_Byte){
		.data = array_any_at(&batcher->vertices, 0),
		.count = sizeof(struct Batcher_2D_Vertex) * batcher->vertices.count,
	};
	batcher->mesh.buffers[1] = (struct Array_Byte){
		.data = (uint8_t *)batcher->indices.data,
		.count = sizeof(uint32_t) * batcher->indices.count,
	};
}
