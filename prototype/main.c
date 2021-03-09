#include "framework/memory.h"
#include "framework/platform_timer.h"
#include "framework/platform_file.h"
#include "framework/platform_system.h"
#include "framework/platform_window.h"

#include "framework/maths.h"

#include "framework/containers/array_byte.h"
#include "framework/assets/asset_mesh.h"
#include "framework/assets/asset_image.h"

#include "framework/opengl/functions.h"
#include "framework/opengl/implementation.h"

#include "application.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Transform {
	struct vec3 position;
	struct vec3 scale;
	struct vec4 rotation;
};

static struct {
	struct Gpu_Program * gpu_program;
	struct Gpu_Texture * gpu_texture;
	struct Gpu_Mesh * gpu_mesh;

	struct {
		uint32_t color;
		uint32_t texture;
		uint32_t camera;
		uint32_t transform;
	} uniforms;

	struct Transform camera;
	struct Transform object;
} game_state;

static void game_init(void) {
	glEnable(GL_DEPTH_TEST);
	glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
	glDepthRangef(0, 1);
	glDepthFunc(GL_LESS);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

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

	//
	struct Array_Byte asset_shader;
	platform_file_init(&asset_shader, "assets/test.glsl");

	struct Asset_Image asset_image;
	asset_image_init(&asset_image, "assets/test.png");

	struct Asset_Mesh asset_mesh;
	asset_mesh_init(&asset_mesh, "assets/cube.obj");

	game_state.gpu_program = gpu_program_init(&asset_shader);
	game_state.gpu_texture = gpu_texture_init(&asset_image);
	game_state.gpu_mesh = gpu_mesh_init(&asset_mesh);

	platform_file_free(&asset_shader);
	asset_image_free(&asset_image);
	asset_mesh_free(&asset_mesh);

	//
	game_state.uniforms.color = glibrary_find_uniform("u_Color");
	game_state.uniforms.texture = glibrary_find_uniform("u_Texture");
	game_state.uniforms.camera = glibrary_find_uniform("u_Camera");
	game_state.uniforms.transform = glibrary_find_uniform("u_Transform");

	//
	game_state.camera = (struct Transform){
		.position = (struct vec3){0, 3, -5},
		.scale = (struct vec3){1, 1, 1},
		.rotation = quat_set_radians((struct vec3){MATHS_TAU / 16, 0, 0}),
	};

	game_state.object = (struct Transform){
		.position = (struct vec3){0, 0, 0},
		.scale = (struct vec3){1, 1, 1},
		.rotation = (struct vec4){0, 0, 0, 1},
	};
}

static void game_free(void) {
	gpu_program_free(game_state.gpu_program);
	gpu_texture_free(game_state.gpu_texture);
	gpu_mesh_free(game_state.gpu_mesh);

	memset(&game_state, 0, sizeof(game_state));
}

static void game_fixed_update(struct Window * window, uint64_t elapsed, uint64_t per_second) {
	float const delta_time = (float)((double)elapsed / (double)per_second);
	(void)window; (void)delta_time;
}

static void game_update(struct Window * window, uint64_t elapsed, uint64_t per_second) {
	float const delta_time = (float)((double)elapsed / (double)per_second);

	if (platform_window_key_transition(window, KC_A, true)) {
		printf("pressed A\n");
	}

	if (platform_window_key_transition(window, KC_W, false)) {
		printf("released W\n");
	}

	if (platform_window_mouse_transition(window, MC_LEFT, true)) {
		uint32_t pos_x, pos_y;
		platform_window_mouse_position_window(window, &pos_x, &pos_y);
		printf("pressed mouse left at %d %d\n", pos_x, pos_y);
	}

	if (platform_window_mouse_transition(window, MC_RIGHT, false)) {
		uint32_t pos_x, pos_y;
		platform_window_mouse_position_display(window, &pos_x, &pos_y);
		printf("released mouse right at %d %d\n", pos_x, pos_y);
	}

	game_state.object.rotation = quat_mul(game_state.object.rotation, quat_set_radians(
		(struct vec3){0 * delta_time, 1 * delta_time, 0 * delta_time}
	));
}

static void game_render(struct Window * window) {
	uint32_t size_x, size_y;
	platform_window_get_size(window, &size_x, &size_y);
	glibrary_viewport(0, 0, size_x, size_y);

	gpu_program_set_uniform(game_state.gpu_program, game_state.uniforms.color, &(struct vec4){0.2f, 0.6f, 1, 1});
	gpu_program_set_uniform(game_state.gpu_program, game_state.uniforms.texture, &game_state.gpu_texture);

	struct mat4 matrix_camera = mat4_mul_mat(
		mat4_set_projection((struct vec2){1, (float)size_x / (float)size_y}, 0.1f, 1000.0f, 0),
		mat4_set_inverse_transformation(game_state.camera.position, game_state.camera.scale, game_state.camera.rotation)
	);

	struct mat4 matrix_object = mat4_set_transformation(game_state.object.position, game_state.object.scale, game_state.object.rotation);

	gpu_program_set_uniform(game_state.gpu_program, game_state.uniforms.camera, &matrix_camera.x.x);
	gpu_program_set_uniform(game_state.gpu_program, game_state.uniforms.transform, &matrix_object.x.x);

	// draw
	glibrary_clear();
	glibrary_draw(game_state.gpu_program, game_state.gpu_mesh);
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
		.slow_frames_factor = 5,
	});
	return 0;
}

//

// static void asset_mesh_init_1(struct Asset_Mesh * asset_mesh) {
// #define CONSTRUCT(type, array) (type){ .data = array, .count = sizeof(array) / sizeof(*array) }
// 
// 	static float vertices[] = {
// 		/*position*/ -0.5f, -0.5f, 0.0f, /*texcoord*/ 0.0f, 0.0f,
// 		/*position*/  0.5f, -0.5f, 0.0f, /*texcoord*/ 1.0f, 0.0f,
// 		/*position*/ -0.5f,  0.5f, 0.0f, /*texcoord*/ 0.0f, 1.0f,
// 		/*position*/  0.5f,  0.5f, 0.0f, /*texcoord*/ 1.0f, 1.0f,
// 	};
// 	static uint32_t sizes[] = {3, 2};
// 	static uint32_t locations[] = {0, 1};
// 	static uint32_t indices[] = {0, 1, 2, 3, 2, 1};
// 
// 	*asset_mesh = (struct Asset_Mesh){
// 		.vertices = CONSTRUCT(struct Array_Float, vertices),
// 		.sizes = CONSTRUCT(struct Array_U32, sizes),
// 		.locations = CONSTRUCT(struct Array_U32, locations),
// 		.indices = CONSTRUCT(struct Array_U32, indices),
// 	};
// 
// #undef CONSTRUCT
// }
