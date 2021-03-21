#include "framework/memory.h"
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

#include "framework/containers/array_byte.h"
#include "framework/assets/asset_mesh.h"
#include "framework/assets/asset_image.h"
#include "framework/assets/asset_font.h"

#include "application/application.h"

#include "batch_mesh.h"

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

static struct Game_Content {
	struct Gpu_Program * gpu_program_test;
	struct Gpu_Program * gpu_program_font;
	struct Gpu_Program * gpu_program_target;
	struct Gpu_Texture * gpu_texture_test;
	struct Gpu_Mesh * gpu_mesh_cube;
	//
	struct Asset_Font * asset_font;
	//
	struct {
		struct Gfx_Material test;
		struct Gfx_Material font;
		struct Gfx_Material target;
	} materials;
} content;


static struct Game_Font {
	struct Asset_Image buffer;
	struct Gpu_Texture * gpu_texture;
} font;

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
		struct Array_Byte asset_shader_test;
		platform_file_init(&asset_shader_test, "assets/test.glsl");

		struct Array_Byte asset_shader_font;
		platform_file_init(&asset_shader_font, "assets/font.glsl");
		
		struct Array_Byte asset_shader_target;
		platform_file_init(&asset_shader_target, "assets/target.glsl");

		struct Asset_Image asset_image_test;
		asset_image_init(&asset_image_test, "assets/test.png");

		struct Asset_Mesh asset_mesh_cube;
		asset_mesh_init(&asset_mesh_cube, "assets/cube.obj");

		content.gpu_program_test = gpu_program_init(&asset_shader_test);
		content.gpu_program_font = gpu_program_init(&asset_shader_font);
		content.gpu_program_target = gpu_program_init(&asset_shader_target);
		content.gpu_texture_test = gpu_texture_init(&asset_image_test);
		content.gpu_mesh_cube = gpu_mesh_init(&asset_mesh_cube);

		platform_file_free(&asset_shader_test);
		platform_file_free(&asset_shader_font);
		platform_file_free(&asset_shader_target);
		asset_image_free(&asset_image_test);
		asset_mesh_free(&asset_mesh_cube);

		content.asset_font = asset_font_init("assets/OpenSans-Regular.ttf");

		gfx_material_init(&content.materials.test, content.gpu_program_test);
		gfx_material_init(&content.materials.font, content.gpu_program_font);
		gfx_material_init(&content.materials.target, content.gpu_program_target);
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

	// init font
	{
		font.buffer = (struct Asset_Image){
			.size_x = 256,
			.size_y = 256,
			.data = MEMORY_ALLOCATE_ARRAY(uint8_t, 256 * 256),
			.parameters = {
				.texture_type = TEXTURE_TYPE_COLOR,
				.data_type = DATA_TYPE_U8,
				.channels = 1,
			},
		};

		float font_scale = asset_font_get_scale(content.asset_font, 32);

		struct Glyph_Params glyph_params;
		uint32_t glyph_id = asset_font_get_glyph_id(content.asset_font, (uint32_t)'h');
		asset_font_get_glyph_parameters(content.asset_font, &glyph_params, glyph_id);
		if (!glyph_params.is_empty) {
			uint32_t const font_size_x = (uint32_t)(((float)glyph_params.bmp_size_x) * font_scale);
			uint32_t const font_size_y = (uint32_t)(((float)glyph_params.bmp_size_y) * font_scale);
			if (font.buffer.size_x > font_size_x && font.buffer.size_y > font_size_y) {
				asset_font_fill_buffer(
					content.asset_font,
					font.buffer.data, 256,
					glyph_id,
					font_size_x, font_size_y, font_scale
				);
			}
		}

		font.gpu_texture = gpu_texture_init(&font.buffer);
	}

	// init batch mesh
	{
		batch.buffer = batch_mesh_init(2, (uint32_t[]){0, 3, 1, 2});
		batch.gpu_mesh = gpu_mesh_init(batch_mesh_get_mesh(batch.buffer));
	}

	// fill materials
	{
		gfx_material_set_texture(&content.materials.test, uniforms.texture, 1, &content.gpu_texture_test);
		gfx_material_set_float(&content.materials.test, uniforms.color, 4, &(struct vec4){0.2f, 0.6f, 1, 1}.x);

		gfx_material_set_texture(&content.materials.font, uniforms.font, 1, &font.gpu_texture);
		gfx_material_set_float(&content.materials.font, uniforms.color, 4, &(struct vec4){1, 1, 1, 1}.x);

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
	gpu_program_free(content.gpu_program_test);
	gpu_program_free(content.gpu_program_font);
	gpu_program_free(content.gpu_program_target);
	gpu_texture_free(content.gpu_texture_test);
	gpu_mesh_free(content.gpu_mesh_cube);
	asset_font_free(content.asset_font);
	gfx_material_free(&content.materials.test);
	gfx_material_free(&content.materials.font);
	gfx_material_free(&content.materials.target);

	gpu_texture_free(font.gpu_texture);
	asset_image_free(&font.buffer);

	gpu_mesh_free(target.gpu_mesh);
	gpu_target_free(target.gpu_target);

	batch_mesh_free(batch.buffer);
	gpu_mesh_free(batch.gpu_mesh);

	memset(&uniforms, 0, sizeof(uniforms));
	memset(&content, 0, sizeof(content));
	memset(&font, 0, sizeof(font));
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
	uint32_t target_size_x, target_size_y;
	gpu_target_get_size(target.gpu_target, &target_size_x, &target_size_y);

	// draw into the batch mesh
	batch_mesh_clear(batch.buffer);
	batch_mesh_add(
		batch.buffer,
		(3 + 2) * 4, (float[]){
			 0,  0, 0, 0, 1,
			 0, 256, 0, 0, 0,
			256,  0, 0, 1, 1,
			256, 256, 0, 1, 0,
		},
		3 * 2, (uint32_t[]){1, 0, 2, 1, 2, 3}
	);

	struct Asset_Mesh * batch_mesh = batch_mesh_get_mesh(batch.buffer);
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
		.mesh = content.gpu_mesh_cube,
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
		.material = &content.materials.font,
		.mesh = batch.gpu_mesh,
		// draw at the nearest point, map to screen buffer coords
		.camera_id = uniforms.camera,
		.transform_id = uniforms.transform,
		.camera = mat4_set_projection((struct vec2){-1, -1}, (struct vec2){2 / (float)size_x, 2 / (float)size_y}, 0, 1, 1),
		.transform = mat4_identity,
	});

	// screen buffer: draw target
	graphics_draw(&(struct Render_Pass){
		.size_x = size_x, .size_y = size_y,
		.blend_mode = {.mask = COLOR_CHANNEL_FULL},
		.depth_enabled = true,
		//
		.material = &content.materials.target,
		.mesh = target.gpu_mesh,
		// draw at the farthest point, map to normalized device coords
		.camera_id = uniforms.camera,
		.transform_id = uniforms.transform,
		.camera = mat4_identity,
		.transform = mat4_set_transformation((struct vec3){0, 0, FLOAT_ALMSOST_1}, (struct vec3){1, 1, 1}, (struct vec4){0, 0, 0, 1}),
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
	return 0;
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
