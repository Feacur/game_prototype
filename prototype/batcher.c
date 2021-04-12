#include "framework/unicode.h"
#include "framework/containers/array_any.h"
#include "framework/graphics/batch_mesh_2d.h"
#include "framework/graphics/gpu_objects.h"
#include "framework/graphics/graphics.h"
#include "framework/graphics/material.h"
#include "framework/graphics/pass.h"

#include "framework/graphics/font_image.h"
#include "framework/graphics/font_image_glyph.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
#include "batcher.h"

void batcher_init(struct Batcher * batcher) {
	memset(batcher, 0, sizeof(*batcher));
	batcher->passes   = array_any_init(sizeof(struct Batch));
	batcher->buffer   = batch_mesh_2d_init();
	batcher->gpu_mesh = gpu_mesh_init(batch_mesh_2d_get_asset(batcher->buffer));
}

void batcher_free(struct Batcher * batcher) {
	batch_mesh_2d_free(batcher->buffer);
	array_any_free(batcher->passes);
	gpu_mesh_free(batcher->gpu_mesh);
	memset(batcher, 0, sizeof(*batcher));
}

static void batcher_bake_pass(struct Batcher * batcher) {
	uint32_t const offset = batch_mesh_2d_get_offset(batcher->buffer);
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
	batch_mesh_2d_add_quad(batcher->buffer, rect, uv);
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
		batch_mesh_2d_add_quad(
			batcher->buffer,
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
	uint32_t const texture_id   = graphics_add_uniform("u_Texture");
	uint32_t const camera_id    = graphics_add_uniform("u_Camera");
	uint32_t const transform_id = graphics_add_uniform("u_Transform");

	gpu_mesh_update(batcher->gpu_mesh, batch_mesh_2d_get_asset(batcher->buffer));

	graphics_draw(&(struct Render_Pass){
		.size_x = size_x, .size_y = size_y,
		.target = gpu_target,
		.blend_mode = {.mask = COLOR_CHANNEL_FULL},
		.depth_mode = {.enabled = true, .mask = true},
		//
		.clear_mask = TEXTURE_TYPE_COLOR | TEXTURE_TYPE_DEPTH,
		.clear_rgba = 0x303030ff,
	});

	batcher_bake_pass(batcher);
	uint32_t passes_count = array_any_get_count(batcher->passes);
	for (uint32_t i = 0; i < passes_count; i++) {
		struct Batch * batch = array_any_at(batcher->passes, i);

		if (batch->texture != NULL) { gfx_material_set_texture(batch->material, texture_id, 1, &batch->texture); }

		// @todo: do CPU-side?
		gfx_material_set_float(batch->material, camera_id, 4*4, &batch->camera.x.x);
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

	memset(&batcher->pass, 0, sizeof(batcher->pass));
	array_any_clear(batcher->passes);
	batch_mesh_2d_clear(batcher->buffer);
}
