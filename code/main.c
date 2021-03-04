#include "memory.h"
#include "platform_timer.h"
#include "platform_file.h"
#include "platform_system.h"
#include "platform_window.h"

#include "array_byte.h"
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

	float vertices[] = {
		/*position*/ -0.5f, -0.5f, 0.0f, /*texture*/ 0.0f, 0.0f,
		/*position*/  0.5f, -0.5f, 0.0f, /*texture*/ 1.0f, 0.0f,
		/*position*/ -0.5f,  0.5f, 0.0f, /*texture*/ 0.0f, 1.0f,
		/*position*/  0.5f,  0.5f, 0.0f, /*texture*/ 1.0f, 1.0f,
	};
	uint32_t attributes[] = {3, 2};
	uint32_t indices[] = {0, 1, 2, 3, 2, 1};

	struct Gpu_Program * gpu_program = gpu_program_init(&asset_shader);
	struct Gpu_Texture * gpu_texture = gpu_texture_init(&asset_image);
	struct Gpu_Mesh * gpu_mesh = gpu_mesh_init(
		vertices, sizeof(vertices) / sizeof(*vertices),
		attributes, sizeof(attributes) / sizeof(*attributes),
		indices, sizeof(indices) / sizeof(*indices)
	);

	array_byte_free(&asset_shader);
	asset_image_free(&asset_image);

	gpu_unit_init(gpu_texture);
	gpu_program_select(gpu_program);
	gpu_mesh_select(gpu_mesh);

	uint32_t uniform_id = glibrary_get_uniform_id("u_Texture");
	gpu_program_set_texture(gpu_program, uniform_id, gpu_texture);
	
	uint64_t frame_start_ticks = platform_timer_get_ticks();
	uint64_t const timer_ticks_per_second = platform_timer_get_ticks_per_second();

	// update
	while (window != NULL) {
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

		int32_t size_x, size_y;
		platform_window_get_size(window, &size_x, &size_y);
		if (size_x > 0 && size_y > 0) { glViewport(0, 0, (GLsizei)size_x, (GLsizei)size_y); }
		// else { glViewport(0, 0, 1, 1); }

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
			printf("pressed mouse left\n");
		}

		if (platform_window_mouse_transition(window, MC_RIGHT, false)) {
			printf("released mouse right\n");
		}

		// draw
		glibrary_clear();
		glibrary_draw(gpu_program, gpu_mesh);
		platform_window_display(window);
	}

	// cleanup
	gpu_unit_free(gpu_texture);
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
