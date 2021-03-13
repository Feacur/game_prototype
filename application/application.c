#include "framework/platform_timer.h"
#include "framework/platform_file.h"
#include "framework/platform_system.h"
#include "framework/platform_window.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
#include "application.h"

static struct {
	struct Application_Config * config;
	struct Window * window;

	struct {
		uint64_t frame_start, per_second;
		uint64_t fixed_accumulator;
	} ticks;
} app;

static uint64_t get_target_ticks(int32_t vsync) {
	uint32_t const vsync_factor = (vsync > 0) ? (uint32_t)vsync : 1;
	uint32_t refresh_rate = (vsync != 0) || (app.config->target_refresh_rate == 0)
		? platform_window_get_refresh_rate(app.window, app.config->target_refresh_rate)
		: app.config->target_refresh_rate;
	return app.ticks.per_second * vsync_factor / refresh_rate;
}

static void application_init(void) {
	platform_system_init();

	app.window = platform_window_init();
	platform_window_set_vsync(app.window, app.config->vsync);

	app.ticks.frame_start = platform_timer_get_ticks();
	app.ticks.per_second = platform_timer_get_ticks_per_second();

	if (app.config->vsync != 0) { 
		// prepare an adequate default frame time
		app.ticks.frame_start -= get_target_ticks(app.config->vsync);
	}

	if (app.config->callbacks.init != NULL) {
		app.config->callbacks.init();
	}
}

static void application_free(void) {
	if (app.config->callbacks.free != NULL) {
		app.config->callbacks.free();
	}

	if (app.window != NULL) {
		platform_window_free(app.window);
	}
	platform_system_free();

	memset(&app, 0, sizeof(app));
}

static bool application_update(void) {
	// application and platform are ready
	if (app.window == NULL) { return false; }
	if (!platform_window_is_running()) { return false; }

	// reset per-frame data / poll platform events
	platform_system_update();

	// window might be closed by platform
	if (!platform_window_exists(app.window)) { return false; }

	// track frame time
	int32_t const vsync = platform_window_get_vsync(app.window);
	uint64_t const target_ticks = get_target_ticks(vsync);
	uint64_t const fixed_ticks = app.ticks.per_second / app.config->fixed_refresh_rate;

	if (vsync == 0) {
		uint64_t const frame_end_ticks = app.ticks.frame_start + target_ticks;
		while (platform_timer_get_ticks() < frame_end_ticks) {
			platform_system_sleep(0);
		}
	}

	uint64_t frame_ticks = platform_timer_get_ticks() - app.ticks.frame_start;
	app.ticks.frame_start = platform_timer_get_ticks();

	// limit elapsed time in case of stutters or debug steps
	if (app.config->slow_frames_limit > 0 && frame_ticks > target_ticks * app.config->slow_frames_limit) {
		frame_ticks = target_ticks * app.config->slow_frames_limit;
	}

	// fixed update / update / render
	if (app.config->callbacks.fixed_update != NULL) {
		app.ticks.fixed_accumulator += frame_ticks;
		while (app.ticks.fixed_accumulator > fixed_ticks) {
			app.ticks.fixed_accumulator -= fixed_ticks;
			app.config->callbacks.fixed_update(
				fixed_ticks,
				app.ticks.per_second
			);
		}
	}

	if (app.config->callbacks.update != NULL) {
		app.config->callbacks.update(
			frame_ticks,
			app.ticks.per_second
		);
	}

	if (app.config->callbacks.render != NULL) {
		uint32_t size_x, size_y;
		platform_window_get_size(app.window, &size_x, &size_y);
		app.config->callbacks.render(size_x, size_y);
	}

	// swap buffers / display buffer / might vsync
	platform_window_display(app.window);

	return true;
}

void application_run(struct Application_Config * config) {
	app.config = config;
	if (config == NULL) { fprintf(stderr, "provide an application config\n"); DEBUG_BREAK(); exit(1); }

	application_init();
	while (application_update()) { }
	application_free();
}
