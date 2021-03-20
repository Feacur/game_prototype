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
	uint32_t camera;
	uint32_t transform;
} uniforms;

static struct Game_Content {
	struct Gpu_Program * gpu_program;
	struct Gpu_Program * gpu_program_target;
	struct Gpu_Texture * gpu_texture;
	struct Gpu_Mesh * gpu_mesh;
	struct Gpu_Mesh * target_mesh;
} content;

static struct Game_State {
	struct Transform camera;
	struct Transform object;
	struct Gfx_Material material;
	struct Gfx_Material target_material;
	struct Batch_Mesh * batch;
	struct Gpu_Mesh * gpu_mesh_batch;
	struct Gpu_Target * gpu_target;
} state;

static void asset_mesh_init__target_quad(struct Asset_Mesh * asset_mesh);

static void game_init(void) {
	// init uniforms ids
	uniforms.color = graphics_add_uniform("u_Color");
	uniforms.texture = graphics_add_uniform("u_Texture");
	uniforms.camera = graphics_add_uniform("u_Camera");
	uniforms.transform = graphics_add_uniform("u_Transform");

	// load content
	struct Array_Byte asset_shader;
	platform_file_init(&asset_shader, "assets/test.glsl");
	
	struct Array_Byte asset_shader_target;
	platform_file_init(&asset_shader_target, "assets/target.glsl");

	struct Asset_Image asset_image;
	asset_image_init(&asset_image, "assets/test.png");

	struct Asset_Mesh asset_mesh;
	asset_mesh_init(&asset_mesh, "assets/cube.obj");

	struct Asset_Mesh target_quad;
	asset_mesh_init__target_quad(&target_quad);

	content.gpu_program = gpu_program_init(&asset_shader);
	content.gpu_program_target = gpu_program_init(&asset_shader_target);
	content.gpu_texture = gpu_texture_init(&asset_image);
	content.gpu_mesh = gpu_mesh_init(&asset_mesh);
	content.target_mesh = gpu_mesh_init(&target_quad);

	platform_file_free(&asset_shader);
	platform_file_free(&asset_shader_target);
	asset_image_free(&asset_image);
	asset_mesh_free(&asset_mesh);

	// init state
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

	state.batch = batch_mesh_init(2, (uint32_t[]){3, 2}, (uint32_t[]){0, 1});
	state.gpu_mesh_batch = gpu_mesh_init(batch_mesh_get_mesh(state.batch));

	gfx_material_init(&state.material, content.gpu_program);
	gfx_material_set_texture(&state.material, uniforms.texture, 1, &content.gpu_texture);
	gfx_material_set_float(&state.material, uniforms.color, 4, &(struct vec4){0.2f, 0.6f, 1, 1}.x);

	state.gpu_target = gpu_target_init(
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
	struct Gpu_Texture * target_texture = gpu_target_get_texture(state.gpu_target, TEXTURE_TYPE_COLOR, 0);
	gfx_material_init(&state.target_material, content.gpu_program_target);
	gfx_material_set_texture(&state.target_material, uniforms.texture, 1, &target_texture);
}

static void game_free(void) {
	gpu_program_free(content.gpu_program);
	gpu_program_free(content.gpu_program_target);
	gpu_texture_free(content.gpu_texture);
	gpu_mesh_free(content.gpu_mesh);
	gpu_mesh_free(content.target_mesh);

	batch_mesh_free(state.batch);
	gpu_mesh_free(state.gpu_mesh_batch);
	gpu_target_free(state.gpu_target);

	memset(&uniforms, 0, sizeof(uniforms));
	memset(&content, 0, sizeof(content));
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
	struct mat4 const mat4_identity = {
		{1,0,0,0},
		{0,1,0,0},
		{0,0,1,0},
		{0,0,0,1},
	};

	//
	struct Asset_Mesh * batch_mesh = batch_mesh_get_mesh(state.batch);
	gpu_mesh_update(state.gpu_mesh_batch, batch_mesh);

	graphics_draw(&(struct Render_Pass){
		.target = state.gpu_target,
		//
		.clear_mask = TEXTURE_TYPE_COLOR | TEXTURE_TYPE_DEPTH,
		.clear_rgba = 0x303030ff,
		//
		.blend_mode = {.mask = COLOR_CHANNEL_FULL},
		.material = &state.material,
		.mesh = state.gpu_mesh_batch,
		//
		.camera_id = uniforms.camera,
		.transform_id = uniforms.transform,
		.camera = mat4_set_projection((struct vec2){1, (float)size_x / (float)size_y}, 0, 1, 1),
		.transform = mat4_identity,
	});

	//
	graphics_draw(&(struct Render_Pass){
		.target = state.gpu_target,
		//
		.clear_mask = TEXTURE_TYPE_COLOR | TEXTURE_TYPE_DEPTH,
		.clear_rgba = 0x303030ff,
		//
		.blend_mode = {.mask = COLOR_CHANNEL_FULL},
		.material = &state.material,
		.mesh = content.gpu_mesh,
		//
		.camera_id = uniforms.camera,
		.transform_id = uniforms.transform,
		.camera = mat4_mul_mat(
			mat4_set_projection((struct vec2){1, (float)size_x / (float)size_y}, 0.1f, FLOAT_POS_INFINITY, 0),
			mat4_set_inverse_transformation(state.camera.position, state.camera.scale, state.camera.rotation)
		),
		.transform = mat4_set_transformation(state.object.position, state.object.scale, state.object.rotation),
	});

	//
	graphics_draw(&(struct Render_Pass){
		.size_x = size_x, .size_y = size_y,
		//
		.clear_mask = TEXTURE_TYPE_COLOR | TEXTURE_TYPE_DEPTH,
		.clear_rgba = 0x303030ff,
		//
		.blend_mode = {.mask = COLOR_CHANNEL_FULL},
		.material = &state.target_material,
		.mesh = content.target_mesh,
		//
		.camera_id = uniforms.camera,
		.transform_id = uniforms.transform,
		.camera = mat4_identity,
		.transform = mat4_identity,
	});
}

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
	uint32_t const sizes[] = {3, 2};
	uint32_t const locations[] = {0, 1};
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
			.count = sizeof(locations) / sizeof(*locations),
		},
		[1] = {
			.type = DATA_TYPE_U32,
			.frequency = MESH_FREQUENCY_STATIC,
			.access = MESH_ACCESS_DRAW,
			.is_index = true,
		},
	};
	memcpy(settings[0].sizes, sizes, sizeof(sizes));
	memcpy(settings[0].locations, locations, sizeof(locations));

	*asset_mesh = (struct Asset_Mesh){
		.count = 2,
		.buffers = buffers,
		.settings = settings,
	};
}
