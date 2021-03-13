#include "framework/memory.h"
#include "framework/platform_timer.h"
#include "framework/platform_file.h"
#include "framework/platform_system.h"

#include "framework/maths.h"
#include "framework/input.h"
#include "framework/graphics_types.h"
#include "framework/gpu_objects.h"
#include "framework/material.h"

#include "framework/containers/array_byte.h"
#include "framework/assets/asset_mesh.h"
#include "framework/assets/asset_image.h"

#include "framework/opengl/functions.h"
#include "framework/opengl/graphics.h"

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
	struct Gpu_Texture * gpu_texture;
	struct Gpu_Mesh * gpu_mesh;
} content;

static struct Game_State {
	struct Transform camera;
	struct Transform object;
	struct Material material;
} state;

static void game_init(void) {
	// setup GL
	glEnable(GL_DEPTH_TEST);
	glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
	glDepthRangef(0, 1);
	glDepthFunc(GL_LESS);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	// init uniforms ids
	uniforms.color = graphics_add_uniform("u_Color");
	uniforms.texture = graphics_add_uniform("u_Texture");
	uniforms.camera = graphics_add_uniform("u_Camera");
	uniforms.transform = graphics_add_uniform("u_Transform");

	// load content
	struct Array_Byte asset_shader;
	platform_file_init(&asset_shader, "assets/test.glsl");

	struct Asset_Image asset_image;
	asset_image_init(&asset_image, "assets/test.png");

	struct Asset_Mesh asset_mesh;
	asset_mesh_init(&asset_mesh, "assets/cube.obj");

	content.gpu_program = gpu_program_init(&asset_shader);
	content.gpu_texture = gpu_texture_init(&asset_image);
	content.gpu_mesh = gpu_mesh_init(&asset_mesh);

	platform_file_free(&asset_shader);
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

	material_init(&state.material, content.gpu_program);
	material_set_texture(&state.material, uniforms.texture, 1, &content.gpu_texture);
	material_set_float(&state.material, uniforms.color, 4, &(struct vec4){0.2f, 0.6f, 1, 1}.x);
}

static void game_free(void) {
	gpu_program_free(content.gpu_program);
	gpu_texture_free(content.gpu_texture);
	gpu_mesh_free(content.gpu_mesh);

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
	graphics_viewport(0, 0, size_x, size_y);
	graphics_clear();

	//
	struct mat4 const matrix_camera = mat4_mul_mat(
		mat4_set_projection((struct vec2){1, (float)size_x / (float)size_y}, 0.1f, 1000.0f, 0),
		mat4_set_inverse_transformation(state.camera.position, state.camera.scale, state.camera.rotation)
	);
	struct mat4 const matrix_object = mat4_set_transformation(state.object.position, state.object.scale, state.object.rotation);

	material_set_float(&state.material, uniforms.camera, 4*4, &matrix_camera.x.x);
	material_set_float(&state.material, uniforms.transform, 4*4, &matrix_object.x.x);

	graphics_draw(&state.material, content.gpu_mesh);
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

/*
static void asset_mesh_init_1(struct Asset_Mesh * asset_mesh) {
#define CONSTRUCT(type, array) (type){ .data = array, .count = sizeof(array) / sizeof(*array) }

	static float vertices[] = {
		-0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
		 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
		-0.5f,  0.5f, 0.0f, 0.0f, 1.0f,
		 0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
	};
	static uint32_t sizes[] = {3, 2};
	static uint32_t locations[] = {0, 1};
	static uint32_t indices[] = {0, 1, 2, 3, 2, 1};

	*asset_mesh = (struct Asset_Mesh){
		.vertices = CONSTRUCT(struct Array_Float, vertices),
		.sizes = CONSTRUCT(struct Array_U32, sizes),
		.locations = CONSTRUCT(struct Array_U32, locations),
		.indices = CONSTRUCT(struct Array_U32, indices),
	};

#undef CONSTRUCT
}
*/

/*
> forward Z
	glEnable(GL_DEPTH_TEST);
	glDepthRangef(0, 1);
	glDepthFunc(GL_LESS);
	glClearDepthf(1);

> reverse Z
	glEnable(GL_DEPTH_TEST);
	glDepthRangef(1, 0);
	glDepthFunc(GL_GREATER);
	glClearDepthf(0);
	+
	glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
*/
