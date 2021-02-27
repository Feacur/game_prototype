#include <stdio.h>

#include "platform_system.h"
#include "platform_window.h"

int main (int argc, char * argv[]) {
	(void)argc; (void)argv;
	platform_system_init();

	struct Window * window = platform_window_init();

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

	if (window != NULL) {
		platform_window_free(window);
		window = NULL;
	}

	platform_system_free();
	return 0;
}
