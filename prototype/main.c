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
	//
	struct Gpu_Target * gpu_target;
	struct Gfx_Material target_material;
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

	struct Asset_Mesh asset_target_quad;
	asset_mesh_init__target_quad(&asset_target_quad);

	content.gpu_program = gpu_program_init(&asset_shader);
	content.gpu_program_target = gpu_program_init(&asset_shader_target);
	content.gpu_texture = gpu_texture_init(&asset_image);
	content.gpu_mesh = gpu_mesh_init(&asset_mesh);
	content.target_mesh = gpu_mesh_init(&asset_target_quad);

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
				.readable = true,
			},
			[1] = {
				.texture_type = TEXTURE_TYPE_DEPTH,
				.data_type = DATA_TYPE_U32,
				// .readable = true,
			},
		},
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
	struct mat4 const mat4_identity = (struct mat4){
		.x = (struct vec4){1,0,0,0},
		.y = (struct vec4){0,1,0,0},
		.z = (struct vec4){0,0,1,0},
		.w = (struct vec4){0,0,0,1},
	};

	uint32_t target_size_x, target_size_y;
	gpu_target_get_size(state.gpu_target, &target_size_x, &target_size_y);

	//
	graphics_viewport(0, 0, target_size_x, target_size_y);
	graphics_clear(state.gpu_target, TEXTURE_TYPE_COLOR | TEXTURE_TYPE_DEPTH, 0x303030ff);
	graphics_draw(&(struct Render_Pass){
		.material = &state.material,
		.target = state.gpu_target,
		.mesh = content.gpu_mesh,
		.blend_mode = {
			.rgb = (struct Blend_Func){
				.op = BLEND_OP_ADD,
				.src = BLEND_FACTOR_ONE,
				.dst = BLEND_FACTOR_ZERO,
			},
			.mask = COLOR_CHANNEL_FULL,
			.rgba = 0xffffffff,
		},
		.camera_id = uniforms.camera,
		.transform_id = uniforms.transform,
		.camera = mat4_mul_mat(
			mat4_set_projection((struct vec2){1, (float)size_x / (float)size_y}, 0.1f, 1000.0f, 0),
			mat4_set_inverse_transformation(state.camera.position, state.camera.scale, state.camera.rotation)
		),
		.transform = mat4_set_transformation(state.object.position, state.object.scale, state.object.rotation),
	});

	//
	graphics_viewport(0, 0, size_x, size_y);
	graphics_clear(NULL, TEXTURE_TYPE_COLOR | TEXTURE_TYPE_DEPTH, 0);
	graphics_draw(&(struct Render_Pass){
		.material = &state.target_material,
		.mesh = content.target_mesh,
		.blend_mode = {
			.mask = COLOR_CHANNEL_FULL,
		},
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
#define CONSTRUCT(type, array) (type){ .data = array, .count = sizeof(array) / sizeof(*array) }

	static float vertices[] = {
		-1, -1, 0, 0, 0,
		-1,  1, 0, 0, 1,
		 1, -1, 0, 1, 0,
		 1,  1, 0, 1, 1,
	};
	static uint32_t sizes[] = {3, 2};
	static uint32_t locations[] = {0, 1};
	static uint32_t indices[] = {1, 0, 2, 1, 2, 3};

	*asset_mesh = (struct Asset_Mesh){
		.vertices = CONSTRUCT(struct Array_Float, vertices),
		.sizes = CONSTRUCT(struct Array_U32, sizes),
		.locations = CONSTRUCT(struct Array_U32, locations),
		.indices = CONSTRUCT(struct Array_U32, indices),
	};

#undef CONSTRUCT
}
