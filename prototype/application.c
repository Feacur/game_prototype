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
		uint64_t fixed_accumulator;
	} ticks;
} app;

static uint32_t get_target_refresh_rate(void) {
	return app.config->target_refresh_rate == 0
		? platform_window_get_refresh_rate(app.window, 60)
		: app.config->target_refresh_rate;
}

static void application_init(void) {
	platform_system_init();

	app.window = platform_window_init();
	platform_window_set_vsync(app.window, app.config->vsync);

	app.ticks.frame_start = platform_timer_get_ticks();
	app.ticks.per_second = platform_timer_get_ticks_per_second();

	if (app.config->vsync != 0) {
		uint32_t const target_refresh_rate = get_target_refresh_rate();
		uint32_t const vsync_factor = (app.config->vsync > 0) ? (uint32_t)app.config->vsync : 1;
		app.ticks.elapsed = app.ticks.per_second * vsync_factor / target_refresh_rate;
	}

	app.config->callbacks.init();
}

static void application_free(void) {
	app.config->callbacks.free();

	if (app.window != NULL) {
		platform_window_free(app.window);
	}
	platform_system_free();

	memset(&app, 0, sizeof(app));
}

static void application_update(void) {
	if (platform_window_get_vsync(app.window) == 0) {
		uint32_t const target_refresh_rate = get_target_refresh_rate();
		uint64_t const frame_end_ticks = app.ticks.frame_start + app.ticks.per_second / target_refresh_rate;
		while (platform_timer_get_ticks() < frame_end_ticks) {
			platform_system_sleep(0);
		}
	}

	app.ticks.elapsed = platform_timer_get_ticks() - app.ticks.frame_start;
	app.ticks.frame_start = platform_timer_get_ticks();

	app.config->callbacks.update(
		app.window,
		app.ticks.elapsed,
		app.ticks.per_second
	);

	app.config->callbacks.render(app.window);
	platform_window_display(app.window);
}

void application_run(struct Application_Config * config) {
	app.config = config;

	application_init();

	while (app.window != NULL && platform_window_is_running()) {
		platform_window_update(app.window);
		platform_system_update();

		if (platform_window_exists(app.window)) {
			application_update();
		}
		else { break; }
	}

	application_free();
}
