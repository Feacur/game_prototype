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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
#include "application.h"

static struct {
	struct Application_Config * config;
	struct Window * window;

	struct {
		uint64_t frame_start, per_second, elapsed;
	} ticks;
} game_application;

static void application_init(void) {
	platform_system_init();

	game_application.window = platform_window_init();
	platform_window_set_vsync(game_application.window, game_application.config->vsync);

	game_application.ticks.frame_start = platform_timer_get_ticks();
	game_application.ticks.per_second = platform_timer_get_ticks_per_second();

	if (game_application.config->vsync != 0) {
		uint32_t target_refresh_rate = platform_window_get_refresh_rate(game_application.window, game_application.config->maximum_refresh_rate);
		if (game_application.config->vsync > 0) {
			game_application.ticks.elapsed = game_application.ticks.per_second * (uint32_t)game_application.config->vsync / target_refresh_rate;
		}
		else {
			game_application.ticks.elapsed = game_application.ticks.per_second / target_refresh_rate;
		}
	}

	game_application.config->callbacks.init();
}

static void application_free(void) {
	game_application.config->callbacks.free();

	if (game_application.window != NULL) {
		platform_window_free(game_application.window);
	}
	platform_system_free();

	memset(&game_application, 0, sizeof(game_application));
}

static void application_update(void) {
	if (platform_window_get_vsync(game_application.window) == 0) {
		uint32_t target_refresh_rate = platform_window_get_refresh_rate(game_application.window, game_application.config->maximum_refresh_rate);
		if (target_refresh_rate > game_application.config->maximum_refresh_rate) { target_refresh_rate = game_application.config->maximum_refresh_rate; }

		uint64_t frame_end_ticks = game_application.ticks.frame_start + game_application.ticks.per_second / target_refresh_rate;
		while (platform_timer_get_ticks() < frame_end_ticks) {
			platform_system_sleep(0);
		}
	}

	game_application.ticks.elapsed = platform_timer_get_ticks() - game_application.ticks.frame_start;
	game_application.ticks.frame_start = platform_timer_get_ticks();

	game_application.config->callbacks.update(
		game_application.window,
		game_application.ticks.elapsed,
		game_application.ticks.per_second
	);
}

static void application_render(void) {
	game_application.config->callbacks.render(game_application.window);
	platform_window_display(game_application.window);
}

void application_run(struct Application_Config * config) {
	game_application.config = config;

	application_init();

	while (game_application.window != NULL && platform_window_is_running()) {
		platform_window_update(game_application.window);
		platform_system_update();

		if (platform_window_exists(game_application.window)) {
			application_update();
			application_render();
		}
		else { break; }
	}

	application_free();
}
