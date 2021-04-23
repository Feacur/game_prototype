
#include "framework/common.h"
#include "framework/memory.h"
#include "framework/unicode.h"

#include "framework/containers/array_any.h"
#include "framework/containers/array_float.h"
#include "framework/containers/array_u32.h"

#include "framework/assets/asset_mesh.h"

#include "framework/graphics/gpu_objects.h"
#include "framework/graphics/graphics.h"
#include "framework/graphics/material.h"
#include "framework/graphics/pass.h"
#include "framework/graphics/font_image.h"
#include "framework/graphics/font_image_glyph.h"
#include "framework/maths.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct Batcher_2D_Batch {
	uint32_t offset, length;
	struct Gfx_Material * material;
	struct Gpu_Texture * texture;
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
	//
	struct mat4 matrix;
	struct Array_Any matrices;
	//
	struct Array_Any vertices;
	struct Array_U32 indices;
	uint32_t vertex_offset, element_offset;
	//
	struct Asset_Mesh mesh;
	struct Gpu_Mesh * gpu_mesh;
};

static void batcher_2d_update_asset(struct Batcher_2D * batcher);

//
#include "batcher_2d.h"

struct Batcher_2D * batcher_2d_init(void) {
	uint32_t const buffers_count = 2;

	struct Batcher_2D * batcher = MEMORY_ALLOCATE(NULL, struct Batcher_2D);
	*batcher = (struct Batcher_2D){
		.matrix = mat4_identity,
		.mesh = (struct Asset_Mesh){
			.count      = buffers_count,
			.buffers    = MEMORY_ALLOCATE_ARRAY(batcher, struct Array_Byte, buffers_count),
			.parameters = MEMORY_ALLOCATE_ARRAY(batcher, struct Mesh_Parameters, buffers_count),
		},
	};
	array_any_init(&batcher->batches,  sizeof(struct Batcher_2D_Batch));
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
	batcher->gpu_mesh = gpu_mesh_init(&batcher->mesh);

	return batcher;
}

void batcher_2d_free(struct Batcher_2D * batcher) {
	gpu_mesh_free(batcher->gpu_mesh);
	//
	MEMORY_FREE(batcher, batcher->mesh.buffers);
	MEMORY_FREE(batcher, batcher->mesh.parameters);
	array_u32_free(&batcher->indices);
	//
	array_any_free(&batcher->batches);
	array_any_free(&batcher->matrices);
	array_any_free(&batcher->vertices);
	//
	memset(batcher, 0, sizeof(*batcher));
	MEMORY_FREE(batcher, batcher);
}

static void batcher_2d_bake_pass(struct Batcher_2D * batcher) {
	uint32_t const offset = batcher->element_offset;
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
	fprintf(stderr, "non-matching matrices\n"); DEBUG_BREAK();
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

void batcher_2d_set_texture(struct Batcher_2D * batcher, struct Gpu_Texture * texture) {
	if (batcher->batch.texture != texture) {
		batcher_2d_bake_pass(batcher);
	}
	batcher->batch.texture = texture;
}

static struct Batcher_2D_Vertex batcher_2d_make_vertex(struct mat4 m, struct vec2 position, struct vec2 tex_coord);

void batcher_2d_add_quad(
	struct Batcher_2D * batcher,
	float const * rect, float const * uv
) {
	uint32_t const vertex_offset = batcher->vertex_offset;
	array_any_push_many(&batcher->vertices, 4, (struct Batcher_2D_Vertex[]){
		batcher_2d_make_vertex(batcher->matrix, (struct vec2){rect[0], rect[1]}, (struct vec2){uv[0], uv[1]}),
		batcher_2d_make_vertex(batcher->matrix, (struct vec2){rect[0], rect[3]}, (struct vec2){uv[0], uv[3]}),
		batcher_2d_make_vertex(batcher->matrix, (struct vec2){rect[2], rect[1]}, (struct vec2){uv[2], uv[1]}),
		batcher_2d_make_vertex(batcher->matrix, (struct vec2){rect[2], rect[3]}, (struct vec2){uv[2], uv[3]}),
	});
	array_u32_write_many(&batcher->indices, 3 * 2, (uint32_t[]){
		vertex_offset + 1, vertex_offset + 0, vertex_offset + 2,
		vertex_offset + 1, vertex_offset + 2, vertex_offset + 3
	});
	batcher->vertex_offset += 4;
	batcher->element_offset += 3 * 2;
}

void batcher_2d_add_text(struct Batcher_2D * batcher, struct Font_Image * font, uint64_t length, uint8_t const * data, float x, float y) {
	float const line_height = font_image_get_height(font) + font_image_get_gap(font);
	float offset_x = x, offset_y = y;

	struct Font_Glyph const * glyph_space = font_image_get_glyph(font, ' ');
	struct Font_Glyph const * glyph_error = font_image_get_glyph(font, CODEPOINT_EMPTY);

	float const space_size = (glyph_space != NULL) ? glyph_space->params.full_size_x : 0;
	float const tab_size = space_size * 4;

	uint32_t codepoint = 0;
	for (uint64_t i = 0; i < length;) {
		uint32_t previous_codepoint = codepoint;

		uint32_t const octets_count = utf8_length(data + i);
		if (octets_count == 0) { codepoint = 0; i++; continue; }

		codepoint = utf8_decode(data + i, octets_count);
		if (codepoint == CODEPOINT_EMPTY) { i++; continue; }

		i += octets_count;

		// white space
		switch (codepoint) {
			case ' ':
			case 0xa0: // non-breaking space
				offset_x += space_size;
				continue;

			case '\t':
				offset_x += tab_size;
				continue;

			case '\n':
				offset_x = x;
				offset_y -= line_height;
				continue;

			case 0x200b: // zero-width space
				continue;
		}

		if (codepoint < ' ') { continue; }

		// possible glyph
		struct Font_Glyph const * glyph = font_image_get_glyph(font, codepoint);
		if (glyph == NULL) { glyph = glyph_error; }

		offset_x += font_image_get_kerning(font, previous_codepoint, codepoint);
		batcher_2d_add_quad(
			batcher,
			(float[]){
				((float)glyph->params.rect[0]) + offset_x,
				((float)glyph->params.rect[1]) + offset_y,
				((float)glyph->params.rect[2]) + offset_x,
				((float)glyph->params.rect[3]) + offset_y,
			},
			glyph->uv
		);

		offset_x += glyph->params.full_size_x;
	}
}

void batcher_2d_draw(struct Batcher_2D * batcher, uint32_t size_x, uint32_t size_y, struct Gpu_Target * gpu_target) {
	uint32_t const texture_id   = graphics_add_uniform("u_Texture");
	uint32_t const color_id     = graphics_add_uniform("u_Color");

	//
	batcher_2d_update_asset(batcher);
	gpu_mesh_update(batcher->gpu_mesh, &batcher->mesh);

	batcher_2d_bake_pass(batcher);
	uint32_t passes_count = array_any_get_count(&batcher->batches);

	for (uint32_t i = 0; i < passes_count; i++) {
		struct Batcher_2D_Batch * batch = array_any_at(&batcher->batches, i);

		// @todo: do matrix computations CPU-side
		gfx_material_set_texture(batch->material, texture_id, 1, &batch->texture);
		gfx_material_set_float(batch->material, color_id, 4, &(struct vec4){1, 1, 1, 1}.x);

		graphics_draw(&(struct Render_Pass){
			.size_x = size_x, .size_y = size_y,
			.target = gpu_target,
			.blend_mode = batch->blend_mode,
			.depth_mode = batch->depth_mode,
			//
			.material = batch->material,
			.mesh = batcher->gpu_mesh,
			.offset = batch->offset, .length = batch->length,
		});
	}

	//
	memset(&batcher->batch, 0, sizeof(batcher->batch));
	array_any_clear(&batcher->batches);
	array_any_clear(&batcher->vertices);
	batcher->indices.count  = 0;
	batcher->vertex_offset   = 0;
	batcher->element_offset  = 0;
}

//

static struct Batcher_2D_Vertex batcher_2d_make_vertex(struct mat4 m, struct vec2 position, struct vec2 tex_coord) {
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
		.count = sizeof(struct Batcher_2D_Vertex) * array_any_get_count(&batcher->vertices),
	};
	batcher->mesh.buffers[1] = (struct Array_Byte){
		.data = (uint8_t *)batcher->indices.data,
		.count = sizeof(uint32_t) * batcher->indices.count,
	};
}
