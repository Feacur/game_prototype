#include "framework/memory.h"
#include "framework/unicode.h"
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
#include "framework/graphics/font_image.h"
#include "framework/graphics/font_image_glyph.h"
#include "framework/graphics/batch_mesh_2d.h"
#include "framework/graphics/batch_mesh_3d.h"

#include "framework/containers/array_any.h"
#include "framework/containers/array_byte.h"
#include "framework/containers/hash_table_any.h"
#include "framework/containers/hash_table_u32.h"

#include "framework/assets/asset_mesh.h"
#include "framework/assets/asset_image.h"
#include "framework/assets/asset_font.h"

#include "application/application.h"
#include "transform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
	struct Gpu_Target * gpu_target;
} target;

static struct Game_Batch {
	struct Game_Batch_Mesh_2D * buffer;
	struct Gpu_Mesh * gpu_mesh;
} batch;

static struct Game_State {
	struct Transform camera;
	struct Transform object;
} state;

static void game_init(void) {
	// init uniforms ids
	uniforms.color = graphics_add_uniform("u_Color");
	uniforms.texture = graphics_add_uniform("u_Texture");
	uniforms.font = graphics_add_uniform("u_Font");
	uniforms.camera = graphics_add_uniform("u_Camera");
	uniforms.transform = graphics_add_uniform("u_Transform");

	// load content
	{
		content.assets.font_sans = asset_font_init("assets/fonts/OpenSans-Regular.ttf");
		content.assets.font_mono = asset_font_init("assets/fonts/JetBrainsMono-Regular.ttf");

		platform_file_read_entire("assets/sandbox/test.txt", &content.assets.text_test);
		content.assets.text_test.data[content.assets.text_test.count] = '\0';

		struct Array_Byte asset_shader_test;
		platform_file_read_entire("assets/shaders/test.glsl", &asset_shader_test);

		struct Array_Byte asset_shader_font;
		platform_file_read_entire("assets/shaders/font.glsl", &asset_shader_font);
		
		struct Array_Byte asset_shader_target;
		platform_file_read_entire("assets/shaders/target.glsl", &asset_shader_target);

		struct Asset_Image asset_image_test;
		asset_image_init(&asset_image_test, "assets/sandbox/test.png");

		struct Asset_Mesh asset_mesh_cube;
		asset_mesh_init(&asset_mesh_cube, "assets/sandbox/cube.obj");

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
		platform_file_read_entire("assets/sandbox/additional_codepoints_french.txt", &asset_codepoints);
		asset_codepoints.data[asset_codepoints.count] = '\0';

		uint32_t codepoints_count = 0;
		uint32_t * codepoints = MEMORY_ALLOCATE_ARRAY(NULL, uint32_t, (asset_codepoints.count + 4) * 2 + 1);
		codepoints[codepoints_count++] = ' ';
		codepoints[codepoints_count++] = '~';
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
			if (codepoint == CODEPOINT_EMPTY) { i++; continue; }

			i += octets_count;

			if (codepoint <= 0x7f) { continue; }
			codepoints[codepoints_count++] = codepoint;
			codepoints[codepoints_count++] = codepoint;
		}

		array_byte_free(&asset_codepoints);

		content.fonts.sans.buffer = font_image_init(content.assets.font_sans, 32);
		font_image_build(content.fonts.sans.buffer, codepoints_count / 2, codepoints);
		content.fonts.sans.gpu_texture = gpu_texture_init(font_image_get_asset(content.fonts.sans.buffer));
		gfx_material_init(&content.fonts.sans.material, content.gpu.program_font);

		MEMORY_FREE(NULL, codepoints);

		content.fonts.mono.buffer = font_image_init(content.assets.font_mono, 32);
		font_image_build(content.fonts.mono.buffer, 1, (uint32_t[]){' ', '~'});
		content.fonts.mono.gpu_texture = gpu_texture_init(font_image_get_asset(content.fonts.mono.buffer));
		gfx_material_init(&content.fonts.mono.material, content.gpu.program_font);

		gfx_material_init(&content.materials.test, content.gpu.program_test);
		gfx_material_init(&content.materials.target, content.gpu.program_target);
	}

	// init target
	{
		target.gpu_target = gpu_target_init(
			320, 180,
			(struct Texture_Parameters[]){
				[0] = {
					.texture_type = TEXTURE_TYPE_COLOR,
					.data_type = DATA_TYPE_U8,
					.channels = 4,
					.flags = TEXTURE_FLAG_READ
				},
				[1] = {
					.texture_type = TEXTURE_TYPE_DEPTH,
					.data_type = DATA_TYPE_R32,
				},
			},
			2
		);
	}

	// init batch mesh
	{
		batch.buffer = game_batch_mesh_2d_init();
		batch.gpu_mesh = gpu_mesh_init(game_batch_mesh_2d_get_asset(batch.buffer));
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

	gpu_target_free(target.gpu_target);

	game_batch_mesh_2d_free(batch.buffer);
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

	// target: clear
	graphics_draw(&(struct Render_Pass){
		.target = target.gpu_target,
		.blend_mode = {.mask = COLOR_CHANNEL_FULL},
		.depth_enabled = true, .depth_mask = true,
		//
		.clear_mask = TEXTURE_TYPE_COLOR | TEXTURE_TYPE_DEPTH,
		.clear_rgba = 0x303030ff,
	});

	// target: draw 3d
	graphics_draw(&(struct Render_Pass){
		.target = target.gpu_target,
		.blend_mode = {.mask = COLOR_CHANNEL_FULL},
		.depth_enabled = true, .depth_mask = true,
		//
		.material = &content.materials.test,
		.mesh = content.gpu.mesh_cube,
		// map to camera coords; draw transformed
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
	{
		// @todo: provide options to fit draw buffer into the window (useful for fullscreen?)
		// @todo: restrict window scaling by render buffer (useful for windowed?)
		game_batch_mesh_2d_clear(batch.buffer);
		game_batch_mesh_2d_add_quad(
			batch.buffer,
			(float[]){-1, -1, 1, 1},
			(float[]){0,0,1,1}
		);

		struct Asset_Mesh * batch_mesh = game_batch_mesh_2d_get_asset(batch.buffer);
		gpu_mesh_update(batch.gpu_mesh, batch_mesh);
	}

	graphics_draw(&(struct Render_Pass){
		.size_x = size_x, .size_y = size_y,
		.blend_mode = {.mask = COLOR_CHANNEL_FULL},
		//
		.material = &content.materials.target,
		.mesh = batch.gpu_mesh,
		// map to normalized device coords; draw at the farthest point
		.camera_id = uniforms.camera,
		.transform_id = uniforms.transform,
		.camera = mat4_identity,
		.transform = mat4_set_transformation((struct vec3){0, 0, 1}, (struct vec3){1, 1, 1}, (struct vec4){0, 0, 0, 1}),
	});

	// screen buffer: draw HUD
	{
		game_batch_mesh_2d_clear(batch.buffer);

		float const text_x = 50, text_y = 200;
		float const line_height = font_image_get_height(font->buffer) + font_image_get_gap(font->buffer);
		float offset_x = text_x, offset_y = text_y;
		uint32_t previous_glyph_id = 0;

		// struct Array_U32 const * codepoints = input_get_codepoints();
		// for (size_t i = 0; i < codepoints->count; i++) {
		// 	uint32_t const codepoint = codepoints->data[i];

		for (size_t i = 0; i < content.assets.text_test.count;) {
			uint32_t const octets_count = utf8_length(content.assets.text_test.data + i);
			if (octets_count == 0) { i++; continue; }

			uint32_t const codepoint = utf8_decode(content.assets.text_test.data + i, octets_count);
			if (codepoint == UTF8_EMPTY) { i++; continue; }

			i += octets_count;

			if (codepoint < ' ') {
				previous_glyph_id = 0;
				if (codepoint == '\t') {
					struct Font_Glyph const * glyph = font_image_get_glyph(font->buffer, ' ');
					if (glyph != NULL) { offset_x += glyph->params.full_size_x * 4; }
				}
				else if (codepoint == '\n') {
					offset_x = text_x;
					offset_y -= line_height;
				}
				continue;
			}

			struct Font_Glyph const * glyph = font_image_get_glyph(font->buffer, codepoint);
			if (glyph == NULL) { previous_glyph_id = 0; continue; }

			offset_x += (previous_glyph_id != 0) ? font_image_get_kerning(font->buffer, previous_glyph_id, glyph->id) : 0;
			float const rect[] = { // left, bottom, top, right
				((float)glyph->params.rect[0]) + offset_x,
				((float)glyph->params.rect[1]) + offset_y,
				((float)glyph->params.rect[2]) + offset_x,
				((float)glyph->params.rect[3]) + offset_y,
			};
			offset_x += glyph->params.full_size_x;

			game_batch_mesh_2d_add_quad(batch.buffer, rect, glyph->uv);
		}

		struct Asset_Image const * font_image = font_image_get_asset(font->buffer);
		game_batch_mesh_2d_add_quad(
			batch.buffer,
			(float[]){0, (float)(size_y - font_image->size_y), (float)font_image->size_x, (float)size_y},
			(float[]){0,0,1,1}
		);

		struct Asset_Mesh * batch_mesh = game_batch_mesh_2d_get_asset(batch.buffer);
		gpu_mesh_update(batch.gpu_mesh, batch_mesh);
	}

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
		//
		.material = &font->material,
		.mesh = batch.gpu_mesh,
		// map to screen buffer coords; draw at the nearest point
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
