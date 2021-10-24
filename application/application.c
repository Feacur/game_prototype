#include "framework/platform_timer.h"
#include "framework/platform_file.h"
#include "framework/platform_system.h"
#include "framework/platform_window.h"
#include "framework/input.h"
#include "framework/logger.h"

//
#include "application.h"

static struct Application {
	struct Application_Callbacks callbacks;
	struct Application_Config config;
	struct Window * window;

	struct {
		uint64_t per_second, frame_start;
		uint64_t fixed_accumulator;
	} ticks;
} app; // @note: global state

static uint64_t get_target_ticks(int32_t vsync_mode) {
	uint32_t const vsync_factor = (vsync_mode > 0) ? (uint32_t)vsync_mode : 1;
	uint32_t refresh_rate = (vsync_mode != 0) || (app.config.target_refresh_rate == 0)
		? platform_window_get_refresh_rate(app.window, app.config.target_refresh_rate)
		: app.config.target_refresh_rate;
	return app.ticks.per_second * vsync_factor / refresh_rate;
}

static void application_init(void) {
	platform_system_init();

	logger_to_console(
		"\n"
		"> system status:\n"
		"  power .. %s\n"
		"",
		platform_system_is_powered() ? "on" : "off"
	);

	logger_to_console(
		"\n"
		"> application settings:\n"
		"  size ......... %u x %u\n"
		"  vsync ........ %d\n"
		"  target rate .. %u\n"
		"  fixed rate ... %u\n"
		"  slow frames .. %u\n"
		"",
		app.config.size_x, app.config.size_y,
		app.config.vsync,
		app.config.target_refresh_rate,
		app.config.fixed_refresh_rate,
		app.config.slow_frames_limit
	);

	// setup window
	enum Window_Settings window_settings = WINDOW_SETTINGS_MINIMIZE;
	if (app.config.flexible) { window_settings |= WINDOW_SETTINGS_FLEXIBLE; }

	app.window = platform_window_init(app.config.size_x, app.config.size_y, window_settings);
	if (app.window == NULL) {
		logger_to_console("failed to create application window"); DEBUG_BREAK();
		common_exit_failure();
	}

	platform_window_set_vsync(app.window, app.config.vsync);

	// setup timer, rewind it one frame
	app.ticks.per_second  = platform_timer_get_ticks_per_second();
	app.ticks.frame_start = platform_timer_get_ticks() - get_target_ticks(platform_window_get_vsync(app.window));

	//
	if (app.callbacks.init != NULL) {
		app.callbacks.init();
	}
}

static void application_free(void) {
	if (app.callbacks.free != NULL) {
		app.callbacks.free();
	}

	if (app.window != NULL) {
		platform_window_free(app.window);
	}
	platform_system_free();

	common_memset(&app, 0, sizeof(app));
}

static bool application_update(void) {
	// application and platform are ready
	if (app.window == NULL) { return false; }
	if (!platform_system_is_running()) { return false; }

	// reset per-frame data / poll platform events
	platform_system_update();

	// window might be closed by platform
	if (!platform_window_exists(app.window)) { return false; }
	platform_window_update(app.window);

	// process application-side input
	if (input_key(KC_ALT)) {
		if (input_key_transition(KC_F4, true)) { return false; }
		if (input_key_transition(KC_ENTER, true)) {
			platform_window_toggle_borderless_fullscreen(app.window);
		}
	}

	// track frame time, limit in case of heavy stutters or debug steps
	uint64_t const timer_ticks  = platform_timer_get_ticks();
	int32_t  const vsync_mode   = platform_window_get_vsync(app.window);
	uint64_t const target_ticks = get_target_ticks(vsync_mode);
	uint64_t const fixed_ticks  = app.ticks.per_second / app.config.fixed_refresh_rate;

	uint64_t frame_ticks = timer_ticks - app.ticks.frame_start;
	app.ticks.frame_start = timer_ticks;

	if (app.config.slow_frames_limit > 0 && frame_ticks > target_ticks * app.config.slow_frames_limit) {
		frame_ticks = target_ticks * app.config.slow_frames_limit;
	}

	app.ticks.fixed_accumulator += frame_ticks;

	// fixed update / frame update / render
	while (app.ticks.fixed_accumulator > fixed_ticks) {
		app.ticks.fixed_accumulator -= fixed_ticks;
		if (app.callbacks.fixed_update != NULL) {
			app.callbacks.fixed_update(
				fixed_ticks,
				app.ticks.per_second
			);
		}
	}

	if (app.callbacks.frame_update != NULL) {
		app.callbacks.frame_update(
			frame_ticks,
			app.ticks.per_second
		);
	}

	if (app.callbacks.draw_update != NULL) {
		app.callbacks.draw_update(
			frame_ticks,
			app.ticks.per_second
		);
	}

	// swap buffers / display buffer / might vsync
	platform_window_display(app.window);

	if (vsync_mode == 0) {
		uint64_t const frame_end_ticks = app.ticks.frame_start + target_ticks;
		while (platform_timer_get_ticks() < frame_end_ticks) {
			platform_system_sleep(0);
		}
	}

	return true;
}

void application_run(struct Application_Config config, struct Application_Callbacks callbacks) {
	app.config = config;
	app.callbacks = callbacks;
	application_init();
	while (application_update()) { }
	application_free();
}

void application_get_screen_size(uint32_t * size_x, uint32_t * size_y) {
	platform_window_get_size(app.window, size_x, size_y);
}
