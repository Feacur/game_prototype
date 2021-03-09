#include "memory.h"
#include "platform_timer.h"
#include "platform_file.h"
#include "platform_system.h"
#include "platform_window.h"

#include "vectors.h"

#include "array_byte.h"
#include "asset_mesh.h"
#include "asset_image.h"

#include "opengl/functions.h"
#include "opengl/implementation.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main (int argc, char * argv[]) {
	(void)argc; (void)argv;

	// start up
	platform_system_init();
	struct Window * window = platform_window_init();
	platform_window_set_vsync(window, 1);

	struct Array_Byte asset_shader;
	platform_file_init(&asset_shader, "assets/test.glsl");

	struct Asset_Image asset_image;
	asset_image_init(&asset_image, "assets/test.png");

	struct Asset_Mesh asset_mesh;
	asset_mesh_init(&asset_mesh, "assets/test.obj");

	struct Gpu_Program * gpu_program = gpu_program_init(&asset_shader);
	struct Gpu_Texture * gpu_texture = gpu_texture_init(&asset_image);
	struct Gpu_Mesh * gpu_mesh = gpu_mesh_init(&asset_mesh);

	platform_file_free(&asset_shader);
	asset_image_free(&asset_image);
	asset_mesh_free(&asset_mesh);

	uint32_t color_uniform_id = glibrary_find_uniform("u_Color");
	gpu_program_set_uniform(gpu_program, color_uniform_id, &(struct vec4){0.2f, 0.6f, 1, 1});

	uint32_t texture_uniform_id = glibrary_find_uniform("u_Texture");
	gpu_program_set_uniform(gpu_program, texture_uniform_id, &gpu_texture);
	
	uint64_t frame_start_ticks = platform_timer_get_ticks();
	uint64_t const timer_ticks_per_second = platform_timer_get_ticks_per_second();

	// update
	while (window != NULL && platform_window_is_running()) {
		// track time
		if (platform_window_get_vsync(window) == 0) {
			uint32_t const maximum_refresh_rate = 72;
			uint32_t target_refresh_rate = platform_window_get_refresh_rate(window, maximum_refresh_rate);
			if (target_refresh_rate > maximum_refresh_rate) { target_refresh_rate = maximum_refresh_rate; }
			uint64_t frame_end_ticks = frame_start_ticks + timer_ticks_per_second / target_refresh_rate;
			while (platform_timer_get_ticks() < frame_end_ticks) {
				platform_system_sleep(0);
			}
		}
		uint64_t elapsed = platform_timer_get_ticks() - frame_start_ticks;
		frame_start_ticks = platform_timer_get_ticks();

		// update platform
		platform_window_update(window);
		platform_system_update();
		if (!platform_window_exists(window)) { break; }

		uint32_t size_x, size_y;
		platform_window_get_size(window, &size_x, &size_y);
		glibrary_viewport(0, 0, size_x, size_y);

		// process input
		double delta_time_double = (double)elapsed / (double)timer_ticks_per_second;
		(void)delta_time_double;
		// printf("%f\n", delta_time_double);

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

		// draw
		glibrary_clear();
		glibrary_draw(gpu_program, gpu_mesh);
		platform_window_display(window);
	}

	// cleanup
	gpu_program_free(gpu_program);
	gpu_texture_free(gpu_texture);
	gpu_mesh_free(gpu_mesh);

	if (window != NULL) {
		platform_window_free(window);
		window = NULL;
	}

	platform_system_free();
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
