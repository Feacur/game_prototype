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

#include <string.h>

struct Batch {
	uint32_t offset, length;
	struct Gfx_Material * material;
	struct Gpu_Texture * texture;
	struct Blend_Mode blend_mode;
	struct Depth_Mode depth_mode;
	struct mat4 camera, transform;
};

struct Batcher {
	struct Batch pass;
	struct Array_Any * passes;
	//
	struct Asset_Mesh mesh;
	struct Array_Float vertices;
	struct Array_U32 indices;
	uint32_t vertex_index, element_index;
	//
	struct Gpu_Mesh * gpu_mesh;
};

static void batcher_update_asset(struct Batcher * batcher);

//
#include "batcher.h"

struct Batcher * batcher_init(void) {
	uint32_t const buffers_count = 2;

	struct Batcher * batcher = MEMORY_ALLOCATE(NULL, struct Batcher);
	*batcher = (struct Batcher){
		.passes = array_any_init(sizeof(struct Batch)),
		.mesh = (struct Asset_Mesh){
			.count      = buffers_count,
			.buffers    = MEMORY_ALLOCATE_ARRAY(batcher, struct Array_Byte, buffers_count),
			.parameters = MEMORY_ALLOCATE_ARRAY(batcher, struct Mesh_Parameters, buffers_count),
		},
	};

	//
	uint32_t const attributes[] = {
		ATTRIBUTE_TYPE_POSITION, 2,
		ATTRIBUTE_TYPE_TEXCOORD, 2,
	};
	uint32_t const attributes_count = (sizeof(attributes) / sizeof(*attributes)) / 2;

	array_byte_init(batcher->mesh.buffers + 0);
	array_byte_init(batcher->mesh.buffers + 1);
	batcher->mesh.parameters[0] = (struct Mesh_Parameters){
		.type = DATA_TYPE_R32,
		.flags = MESH_FLAG_MUTABLE | MESH_FLAG_WRITE | MESH_FLAG_FREQUENT,
		.attributes_count = attributes_count,
	};
	batcher->mesh.parameters[1] = (struct Mesh_Parameters){
		.type = DATA_TYPE_U32,
		.flags = MESH_FLAG_INDEX | MESH_FLAG_MUTABLE | MESH_FLAG_WRITE | MESH_FLAG_FREQUENT,
	};
	memcpy(batcher->mesh.parameters[0].attributes, attributes, attributes_count * 2 * sizeof(uint32_t));

	//
	array_float_init(&batcher->vertices);
	array_u32_init(&batcher->indices);

	batcher_update_asset(batcher);
	batcher->gpu_mesh = gpu_mesh_init(&batcher->mesh);

	return batcher;
}

void batcher_free(struct Batcher * batcher) {
	gpu_mesh_free(batcher->gpu_mesh);
	//
	MEMORY_FREE(batcher, batcher->mesh.buffers);
	MEMORY_FREE(batcher, batcher->mesh.parameters);
	array_float_free(&batcher->vertices);
	array_u32_free(&batcher->indices);
	//
	array_any_free(batcher->passes);
	//
	memset(batcher, 0, sizeof(*batcher));
	MEMORY_FREE(batcher, batcher);
}

static void batcher_bake_pass(struct Batcher * batcher) {
	uint32_t const offset = batcher->element_index;
	if (batcher->pass.offset < offset) {
		batcher->pass.length = offset - batcher->pass.offset;
		array_any_push(batcher->passes, &batcher->pass);

		batcher->pass.offset = offset;
		batcher->pass.length = 0;
	}
}

void batcher_set_camera(struct Batcher * batcher, struct mat4 camera) {
	if (memcmp(&batcher->pass.camera, &camera, sizeof(camera)) != 0) {
		batcher_bake_pass(batcher);
	}
	batcher->pass.camera = camera;
}

void batcher_set_transform(struct Batcher * batcher, struct mat4 transform) {
	if (memcmp(&batcher->pass.transform, &transform, sizeof(transform)) != 0) {
		batcher_bake_pass(batcher);
	}
	batcher->pass.transform = transform;
}

void batcher_set_blend_mode(struct Batcher * batcher, struct Blend_Mode blend_mode) {
	if (memcmp(&batcher->pass.blend_mode, &blend_mode, sizeof(blend_mode)) != 0) {
		batcher_bake_pass(batcher);
	}
	batcher->pass.blend_mode = blend_mode;
}

void batcher_set_depth_mode(struct Batcher * batcher, struct Depth_Mode depth_mode) {
	if (memcmp(&batcher->pass.depth_mode, &depth_mode, sizeof(depth_mode)) != 0) {
		batcher_bake_pass(batcher);
	}
	batcher->pass.depth_mode = depth_mode;
}

void batcher_set_material(struct Batcher * batcher, struct Gfx_Material * material) {
	if (batcher->pass.material != material) {
		batcher_bake_pass(batcher);
	}
	batcher->pass.material = material;
}

void batcher_set_texture(struct Batcher * batcher, struct Gpu_Texture * texture) {
	if (batcher->pass.texture != texture) {
		batcher_bake_pass(batcher);
	}
	batcher->pass.texture = texture;
}

void batcher_add_quad(
	struct Batcher * batcher,
	float const * rect, float const * uv
) {
	uint32_t const vertex_index = batcher->vertex_index;
	array_float_write_many(&batcher->vertices, (2 + 2) * 4, (float[]){
		rect[0], rect[1], uv[0], uv[1],
		rect[0], rect[3], uv[0], uv[3],
		rect[2], rect[1], uv[2], uv[1],
		rect[2], rect[3], uv[2], uv[3],
	});
	array_u32_write_many(&batcher->indices, 3 * 2, (uint32_t[]){
		vertex_index + 1, vertex_index + 0, vertex_index + 2,
		vertex_index + 1, vertex_index + 2, vertex_index + 3
	});
	batcher->vertex_index += 4;
	batcher->element_index += 3 * 2;
}

void batcher_add_text(struct Batcher * batcher, struct Font_Image * font, uint64_t length, uint8_t const * data, float x, float y) {
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
		batcher_add_quad(
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

void batcher_draw(struct Batcher * batcher, uint32_t size_x, uint32_t size_y, struct Gpu_Target * gpu_target) {
	uint32_t const camera_id    = graphics_add_uniform("u_Camera");
	uint32_t const texture_id   = graphics_add_uniform("u_Texture");
	uint32_t const color_id     = graphics_add_uniform("u_Color");
	uint32_t const transform_id = graphics_add_uniform("u_Transform");

	//
	batcher_update_asset(batcher);
	gpu_mesh_update(batcher->gpu_mesh, &batcher->mesh);

	batcher_bake_pass(batcher);
	uint32_t passes_count = array_any_get_count(batcher->passes);

	for (uint32_t i = 0; i < passes_count; i++) {
		struct Batch * batch = array_any_at(batcher->passes, i);

		// @todo: do matrix computations CPU-side
		gfx_material_set_float(batch->material, camera_id, 4*4, &batch->camera.x.x);
		gfx_material_set_texture(batch->material, texture_id, 1, &batch->texture);
		gfx_material_set_float(batch->material, color_id, 4, &(struct vec4){1, 1, 1, 1}.x);
		gfx_material_set_float(batch->material, transform_id, 4*4, &batch->transform.x.x);

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
	memset(&batcher->pass, 0, sizeof(batcher->pass));
	array_any_clear(batcher->passes);
	batcher->vertices.count = 0;
	batcher->indices.count  = 0;
	batcher->vertex_index   = 0;
	batcher->element_index  = 0;
}

//

static void batcher_update_asset(struct Batcher * batcher) {
	batcher->mesh.buffers[0] = (struct Array_Byte){
		.data = (uint8_t *)batcher->vertices.data,
		.count = batcher->vertices.count * sizeof(float),
	};
	batcher->mesh.buffers[1] = (struct Array_Byte){
		.data = (uint8_t *)batcher->indices.data,
		.count = batcher->indices.count * sizeof(uint32_t),
	};
}
