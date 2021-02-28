#include "memory.h"
#include "platform_system.h"
#include "platform_window.h"

#include "opengl/implementation.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char * read_file(char const * path) {
	FILE * file = fopen(path, "rb");
	if (file == NULL) { fprintf(stderr, "'fopen' failed\n"); DEBUG_BREAK(); exit(1); }

	fseek(file, 0L, SEEK_END);
	size_t file_size = (size_t)ftell(file);
	rewind(file);

	char * buffer = (char *)malloc(file_size + 1);
	if (buffer == NULL) { fprintf(stderr, "'malloc' failed\n"); DEBUG_BREAK(); exit(1); }

	size_t bytes_read = (size_t)fread(buffer, sizeof(char), file_size, file);
	if (bytes_read < file_size) { fprintf(stderr, "'fread' failed\n"); DEBUG_BREAK(); exit(1); }

	buffer[bytes_read] = '\0';

	fclose(file);
	return buffer;
}

int main (int argc, char * argv[]) {
	(void)argc; (void)argv;

	// start up
	platform_system_init();
	struct Window * window = platform_window_init();

	char * gpu_program_text = read_file("assets/test.glsl");

	struct Gpu_Program * gpu_program = gpu_program_init(gpu_program_text);
	struct Gpu_Texture * gpu_texture = gpu_texture_init();
	struct Gpu_Mesh * gpu_mesh = gpu_mesh_init();

	free(gpu_program_text);

	// update
	while (window != NULL && platform_window_exists(window)) {
		platform_window_update(window);
		platform_system_update();

		platform_system_sleep(16);

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
