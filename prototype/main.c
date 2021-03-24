#include "framework/memory.h"
#include "framework/utf8.h"
#include "framework/platform_timer.h"
#include "framework/platform_file.h"
#include "framework/platform_system.h"

#include "framework/maths.h"
#include "framework/input.h"

#include "framework/graphics/types.h"
#include "framework/graphics/gpu_objects.h"
#include "framework/graphics/material.h"
#include "framework/graphics/pass.h"
#include "framework/graphics/graphics.h"
#include "framework/graphics/batch_mesh.h"
#include "framework/graphics/font_image.h"
#include "framework/graphics/font_image_glyph.h"

#include "framework/containers/array_byte.h"
#include "framework/containers/array_any.h"
#include "framework/containers/hash_table.h"
#include "framework/containers/hash_table_any.h"

#include "framework/assets/asset_mesh.h"
#include "framework/assets/asset_image.h"
#include "framework/assets/asset_font.h"

#include "application/application.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Transform {
	struct vec3 position;
	struct vec3 scale;
	struct vec4 rotation;
};

static struct Game_Uniforms {
	uint32_t color;
	uint32_t texture;
	uint32_t font;
	uint32_t camera;
	uint32_t transform;
} uniforms;

struct Game_Font {
	struct Font_Image * buffer;
	struct Gpu_Texture * gpu_texture;
	struct Gfx_Material material;
};

static struct Game_Content {
	struct {
		struct Asset_Font * font_sans;
		struct Asset_Font * font_mono;
		struct Array_Byte text_test;
	} assets;
	//
	struct {
		struct Gpu_Program * program_test;
		struct Gpu_Program * program_font;
		struct Gpu_Program * program_target;
		struct Gpu_Texture * texture_test;
		struct Gpu_Mesh * mesh_cube;
	} gpu;
	//
	struct {
		struct Game_Font sans, mono;
	} fonts;
	//
	struct {
		struct Gfx_Material test;
		struct Gfx_Material target;
	} materials;
} content;

static struct Game_Target {
	struct Gpu_Mesh * gpu_mesh;
	struct Gpu_Target * gpu_target;
} target;

static struct Game_Batch {
	struct Batch_Mesh * buffer;
	struct Gpu_Mesh * gpu_mesh;
} batch;

static struct Game_State {
	struct Transform camera;
	struct Transform object;
} state;

static struct mat4 const mat4_identity = {
	{1,0,0,0},
	{0,1,0,0},
	{0,0,1,0},
	{0,0,0,1},
};

static void asset_mesh_init__target_quad(struct Asset_Mesh * asset_mesh);

static void game_init(void) {
	// init uniforms ids
	uniforms.color = graphics_add_uniform("u_Color");
	uniforms.texture = graphics_add_uniform("u_Texture");
	uniforms.font = graphics_add_uniform("u_Font");
	uniforms.camera = graphics_add_uniform("u_Camera");
	uniforms.transform = graphics_add_uniform("u_Transform");

	// load content
	{
		content.assets.font_sans = asset_font_init("assets/OpenSans-Regular.ttf");
		content.assets.font_mono = asset_font_init("assets/JetBrainsMono-Regular.ttf");

		platform_file_read("assets/test.txt", &content.assets.text_test);
		content.assets.text_test.data[content.assets.text_test.count] = '\0';

		struct Array_Byte asset_shader_test;
		platform_file_read("assets/test.glsl", &asset_shader_test);

		struct Array_Byte asset_shader_font;
		platform_file_read("assets/font.glsl", &asset_shader_font);
		
		struct Array_Byte asset_shader_target;
		platform_file_read("assets/target.glsl", &asset_shader_target);

		struct Asset_Image asset_image_test;
		asset_image_init(&asset_image_test, "assets/test.png");

		struct Asset_Mesh asset_mesh_cube;
		asset_mesh_init(&asset_mesh_cube, "assets/cube.obj");

		content.gpu.program_test = gpu_program_init(&asset_shader_test);
		content.gpu.program_font = gpu_program_init(&asset_shader_font);
		content.gpu.program_target = gpu_program_init(&asset_shader_target);
		content.gpu.texture_test = gpu_texture_init(&asset_image_test);
		content.gpu.mesh_cube = gpu_mesh_init(&asset_mesh_cube);

		array_byte_free(&asset_shader_test);
		array_byte_free(&asset_shader_font);
		array_byte_free(&asset_shader_target);
		asset_image_free(&asset_image_test);
		asset_mesh_free(&asset_mesh_cube);

		struct Array_Byte asset_codepoints;
		platform_file_read("assets/additional_codepoints_french.txt", &asset_codepoints);
		asset_codepoints.data[asset_codepoints.count] = '\0';

		uint32_t codepoints_count = 0;
		uint32_t * codepoints = MEMORY_ALLOCATE_ARRAY(uint32_t, (asset_codepoints.count + 4) * 2 + 1);
		codepoints[codepoints_count++] = 0x20;
		codepoints[codepoints_count++] = 0x7e;
		codepoints[codepoints_count++] = 0x401;
		codepoints[codepoints_count++] = 0x401;
		codepoints[codepoints_count++] = 0x410;
		codepoints[codepoints_count++] = 0x44f;
		codepoints[codepoints_count++] = 0x451;
		codepoints[codepoints_count++] = 0x451;
		for (size_t i = 0; i < asset_codepoints.count;) {
			uint32_t const octets_count = utf8_length(asset_codepoints.data + i);
			if (octets_count == 0) { i++; continue; }

			uint32_t const codepoint = utf8_decode(asset_codepoints.data + i, octets_count);
			if (codepoint == UTF8_EMPTY) { i++; continue; }

			i += octets_count;

			if (codepoint == '\r') { continue; }
			if (codepoint == '\n') { continue; }

			codepoints[codepoints_count++] = codepoint;
			codepoints[codepoints_count++] = codepoint;
		}
		codepoints[codepoints_count++] = 0;

		array_byte_free(&asset_codepoints);

		content.fonts.sans.buffer = font_image_init(content.assets.font_sans, 32);
		font_image_build(content.fonts.sans.buffer, codepoints);
		content.fonts.sans.gpu_texture = gpu_texture_init(font_image_get_asset(content.fonts.sans.buffer));
		gfx_material_init(&content.fonts.sans.material, content.gpu.program_font);

		MEMORY_FREE(codepoints);

		content.fonts.mono.buffer = font_image_init(content.assets.font_mono, 32);
		font_image_build(content.fonts.mono.buffer, (uint32_t[]){0x20, 0x7e, 0});
		content.fonts.mono.gpu_texture = gpu_texture_init(font_image_get_asset(content.fonts.mono.buffer));
		gfx_material_init(&content.fonts.mono.material, content.gpu.program_font);

		gfx_material_init(&content.materials.test, content.gpu.program_test);
		gfx_material_init(&content.materials.target, content.gpu.program_target);
	}

	// init target
	{
		struct Asset_Mesh asset_mesh_target;
		asset_mesh_init__target_quad(&asset_mesh_target);
		target.gpu_mesh = gpu_mesh_init(&asset_mesh_target);
		target.gpu_target = gpu_target_init(
			320, 180,
			(struct Texture_Parameters[]){
				[0] = {
					.texture_type = TEXTURE_TYPE_COLOR,
					.data_type = DATA_TYPE_U8,
					.channels = 4,
				},
				[1] = {
					.texture_type = TEXTURE_TYPE_DEPTH,
					.data_type = DATA_TYPE_R32,
				},
			},
			(bool[]){true, false}, // readable color texture
			2
		);
	}

	// init batch mesh
	{
		batch.buffer = batch_mesh_init(2, (uint32_t[]){0, 3, 1, 2});
		batch.gpu_mesh = gpu_mesh_init(batch_mesh_get_asset(batch.buffer));
	}

	// fill materials
	{
		gfx_material_set_texture(&content.fonts.sans.material, uniforms.font, 1, &content.fonts.sans.gpu_texture);
		gfx_material_set_float(&content.fonts.sans.material, uniforms.color, 4, &(struct vec4){1, 1, 1, 1}.x);

		gfx_material_set_texture(&content.fonts.mono.material, uniforms.font, 1, &content.fonts.mono.gpu_texture);
		gfx_material_set_float(&content.fonts.mono.material, uniforms.color, 4, &(struct vec4){1, 1, 1, 1}.x);

		gfx_material_set_texture(&content.materials.test, uniforms.texture, 1, &content.gpu.texture_test);
		gfx_material_set_float(&content.materials.test, uniforms.color, 4, &(struct vec4){0.2f, 0.6f, 1, 1}.x);

		struct Gpu_Texture * target_texture = gpu_target_get_texture(target.gpu_target, TEXTURE_TYPE_COLOR, 0);
		gfx_material_set_texture(&content.materials.target, uniforms.texture, 1, &target_texture);
	}

	// init state
	{
		state.camera = (struct Transform){
			.position = (struct vec3){0, 3, -5},
			.scale = (struct vec3){1, 1, 1},
			.rotation = quat_set_radians((struct vec3){MATHS_TAU / 16, 0, 0}),
		};

		state.object = (struct Transform){
			.position = (struct vec3){0, 0, 0},
			.scale = (struct vec3){1, 1, 1},
			.rotation = (struct vec4){0, 0, 0, 1},
		};
	}
}

static void game_free(void) {
	asset_font_free(content.assets.font_sans);
	asset_font_free(content.assets.font_mono);
	array_byte_free(&content.assets.text_test);

	gpu_program_free(content.gpu.program_test);
	gpu_program_free(content.gpu.program_font);
	gpu_program_free(content.gpu.program_target);
	gpu_texture_free(content.gpu.texture_test);
	gpu_mesh_free(content.gpu.mesh_cube);
	gfx_material_free(&content.materials.test);
	gfx_material_free(&content.materials.target);

	font_image_free(content.fonts.sans.buffer);
	gpu_texture_free(content.fonts.sans.gpu_texture);
	gfx_material_free(&content.fonts.sans.material);

	font_image_free(content.fonts.mono.buffer);
	gpu_texture_free(content.fonts.mono.gpu_texture);
	gfx_material_free(&content.fonts.mono.material);

	gpu_mesh_free(target.gpu_mesh);
	gpu_target_free(target.gpu_target);

	batch_mesh_free(batch.buffer);
	gpu_mesh_free(batch.gpu_mesh);

	memset(&uniforms, 0, sizeof(uniforms));
	memset(&content, 0, sizeof(content));
	memset(&target, 0, sizeof(target));
	memset(&batch, 0, sizeof(batch));
	memset(&state, 0, sizeof(state));
}

static void game_fixed_update(uint64_t elapsed, uint64_t per_second) {
	float const delta_time = (float)((double)elapsed / (double)per_second);
	(void)delta_time;
}

static void game_update(uint64_t elapsed, uint64_t per_second) {
	float const delta_time = (float)((double)elapsed / (double)per_second);

	if (input_mouse(MC_LEFT)) {
		int32_t x, y;
		input_mouse_delta(&x, &y);
		printf("delta: %d %d\n", x, y);
	}

	state.object.rotation = quat_mul(state.object.rotation, quat_set_radians(
		(struct vec3){0 * delta_time, 1 * delta_time, 0 * delta_time}
	));
}

static void game_render(uint32_t size_x, uint32_t size_y) {
	struct Game_Font * font = &content.fonts.sans;

	uint32_t target_size_x, target_size_y;
	gpu_target_get_size(target.gpu_target, &target_size_x, &target_size_y);

	// draw into the batch mesh
	batch_mesh_clear(batch.buffer);

	float const text_x = 50, text_y = 200;
	float offset_x = text_x, offset_y = text_y;
	uint32_t previous_glyph_id = 0;
	for (size_t i = 0; i < content.assets.text_test.count;) {
		uint32_t const octets_count = utf8_length(content.assets.text_test.data + i);
		if (octets_count == 0) { i++; continue; }

		uint32_t const codepoint = utf8_decode(content.assets.text_test.data + i, octets_count);
		if (codepoint == UTF8_EMPTY) { i++; continue; }

		i += octets_count;

		if (codepoint == '\r') {
			previous_glyph_id = 0;
			// offset_x = text_x;
			continue;
		}

		if (codepoint == '\n') {
			previous_glyph_id = 0;
			offset_x = text_x;
			offset_y -= font_image_get_height(font->buffer) + font_image_get_gap(font->buffer);
			continue;
		}

		struct Font_Glyph const * glyph = font_image_get_glyph(font->buffer, codepoint);
		if (glyph == NULL) { previous_glyph_id = 0; continue; }
		if (glyph->id == 0) { previous_glyph_id = 0; continue; }

		offset_x += (previous_glyph_id != 0) ? font_image_get_kerning(font->buffer, previous_glyph_id, glyph->id) : 0;
		float const rect[] = { // left, bottom, top, right
			((float)glyph->params.rect[0]) + offset_x,
			((float)glyph->params.rect[1]) + offset_y,
			((float)glyph->params.rect[2]) + offset_x,
			((float)glyph->params.rect[3]) + offset_y,
		};
		offset_x += glyph->params.full_size_x;

		batch_mesh_add_quad_xy(batch.buffer, rect, glyph->uv);
	}

	struct Asset_Image const * font_image = font_image_get_asset(font->buffer);
	batch_mesh_add_quad_xy(
		batch.buffer,
		(float[]){0, (float)(size_y - font_image->size_y), (float)font_image->size_x, (float)size_y},
		(float[]){0,0,1,1}
	);

	struct Asset_Mesh * batch_mesh = batch_mesh_get_asset(batch.buffer);
	gpu_mesh_update(batch.gpu_mesh, batch_mesh);

	// target: clear
	graphics_draw(&(struct Render_Pass){
		.target = target.gpu_target,
		.blend_mode = {.mask = COLOR_CHANNEL_FULL},
		.depth_enabled = true, .depth_mask = true,
		//
		.clear_mask = TEXTURE_TYPE_COLOR | TEXTURE_TYPE_DEPTH,
		.clear_rgba = 0x303030ff,
	});

	// target: draw HUD
	// graphics_draw(&(struct Render_Pass){
	// 	.target = target.gpu_target,
	// 	.blend_mode = {
	// 		.rgb = {
	// 			.op = BLEND_OP_ADD,
	// 			.src = BLEND_FACTOR_SRC_ALPHA,
	// 			.dst = BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
	// 		},
	// 		.mask = COLOR_CHANNEL_FULL
	// 	},
	// 	.depth_enabled = true, .depth_mask = true,
	// 	//
	// 	.material = &content.materials.font,
	// 	.mesh = batch.gpu_mesh,
	// 	// draw at the nearest point, map to target buffer coords
	// 	.camera_id = uniforms.camera,
	// 	.transform_id = uniforms.transform,
	// 	.camera = mat4_set_projection((struct vec2){-1, -1}, (struct vec2){2 / (float)target_size_x, 2 / (float)target_size_y}, 0, 1, 1),
	// 	.transform = mat4_identity,
	// });

	// target: draw 3d
	graphics_draw(&(struct Render_Pass){
		.target = target.gpu_target,
		.blend_mode = {.mask = COLOR_CHANNEL_FULL},
		.depth_enabled = true, .depth_mask = true,
		//
		.material = &content.materials.test,
		.mesh = content.gpu.mesh_cube,
		// draw transformed, map to camera coords
		.camera_id = uniforms.camera,
		.transform_id = uniforms.transform,
		.camera = mat4_mul_mat(
			mat4_set_projection((struct vec2){0, 0}, (struct vec2){1, (float)size_x / (float)size_y}, 0.1f, 10, 0),
			mat4_set_inverse_transformation(state.camera.position, state.camera.scale, state.camera.rotation)
		),
		.transform = mat4_set_transformation(state.object.position, state.object.scale, state.object.rotation),
	});

	// screen buffer: clear
	graphics_draw(&(struct Render_Pass){
		.size_x = size_x, .size_y = size_y,
		.blend_mode = {.mask = COLOR_CHANNEL_FULL},
		.depth_enabled = true, .depth_mask = true,
		//
		.clear_mask = TEXTURE_TYPE_COLOR | TEXTURE_TYPE_DEPTH,
		.clear_rgba = 0x303030ff,
	});

	// screen buffer: draw target
	graphics_draw(&(struct Render_Pass){
		.size_x = size_x, .size_y = size_y,
		.blend_mode = {.mask = COLOR_CHANNEL_FULL},
		//
		.material = &content.materials.target,
		.mesh = target.gpu_mesh,
		// draw at the farthest point, map to normalized device coords
		.camera_id = uniforms.camera,
		.transform_id = uniforms.transform,
		.camera = mat4_identity,
		.transform = mat4_identity,
		// .transform = mat4_set_transformation((struct vec3){0, 0, FLOAT_ALMSOST_1}, (struct vec3){1, 1, 1}, (struct vec4){0, 0, 0, 1}),
	});

	// screen buffer: draw HUD
	graphics_draw(&(struct Render_Pass){
		.size_x = size_x, .size_y = size_y,
		.blend_mode = {
			.rgb = {
				.op = BLEND_OP_ADD,
				.src = BLEND_FACTOR_SRC_ALPHA,
				.dst = BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			},
			.mask = COLOR_CHANNEL_FULL
		},
		.depth_enabled = true, .depth_mask = true,
		//
		.material = &font->material,
		.mesh = batch.gpu_mesh,
		// draw at the nearest point, map to screen buffer coords
		.camera_id = uniforms.camera,
		.transform_id = uniforms.transform,
		.camera = mat4_set_projection((struct vec2){-1, -1}, (struct vec2){2 / (float)size_x, 2 / (float)size_y}, 0, 1, 1),
		.transform = mat4_identity,
	});
}

//

int main (int argc, char * argv[]) {
	(void)argc; (void)argv;
	application_run(&(struct Application_Config){
		.callbacks = {
			.init = game_init,
			.free = game_free,
			.fixed_update = game_fixed_update,
			.update = game_update,
			.render = game_render,
		},
		.vsync = 1,
		.target_refresh_rate = 72,
		.fixed_refresh_rate = 50,
		.slow_frames_limit = 5,
	});
	return EXIT_SUCCESS;
}

//

static void asset_mesh_init__target_quad(struct Asset_Mesh * asset_mesh) {
	static float vertices[] = {
		-1, -1, 0, 0, 0,
		-1,  1, 0, 0, 1,
		 1, -1, 0, 1, 0,
		 1,  1, 0, 1, 1,
	};
	static uint32_t indices[] = {1, 0, 2, 1, 2, 3};

	//
	static struct Array_Byte buffers[] = {
		[0] = {
			.data = (uint8_t *)vertices,
			.count = sizeof(vertices),
		},
		[1] = {
			.data = (uint8_t *)indices,
			.count = sizeof(indices),
		},
	};

	static struct Mesh_Settings settings[] = {
		[0] = {
			.type = DATA_TYPE_R32,
			.frequency = MESH_FREQUENCY_STATIC,
			.access = MESH_ACCESS_DRAW,
			.attributes_count = 2,
			.attributes[0] = 0, .attributes[1] = 3,
			.attributes[2] = 1, .attributes[3] = 2,
		},
		[1] = {
			.type = DATA_TYPE_U32,
			.frequency = MESH_FREQUENCY_STATIC,
			.access = MESH_ACCESS_DRAW,
			.is_index = true,
		},
	};

	*asset_mesh = (struct Asset_Mesh){
		.count = 2,
		.buffers = buffers,
		.settings = settings,
	};
}
