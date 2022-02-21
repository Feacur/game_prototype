#include "framework/platform_timer.h"
#include "framework/platform_file.h"
#include "framework/platform_system.h"
#include "framework/platform_window.h"
#include "framework/gpu_context.h"
#include "framework/input.h"
#include "framework/logger.h"

//
#include "application.h"

static struct Application {
	struct Application_Callbacks callbacks;
	struct Application_Config config;
	struct Window * window;
	struct Gpu_Context * gpu_context;

	struct {
		uint64_t per_second, frame_start;
		uint64_t fixed_accumulator;
	} ticks;
} gs_app;

static uint64_t get_target_ticks(int32_t vsync_mode) {
	uint32_t const vsync_factor = (vsync_mode > 0) ? (uint32_t)vsync_mode : 1;
	uint32_t refresh_rate = (vsync_mode != 0) || (gs_app.config.target_refresh_rate == 0)
		? platform_window_get_refresh_rate(gs_app.window, gs_app.config.target_refresh_rate)
		: gs_app.config.target_refresh_rate;
	return gs_app.ticks.per_second * vsync_factor / refresh_rate;
}

static void application_init(void) {
	platform_system_init();

	logger_to_console("\n"
		"> system status:\n"
		"  power .. %s\n"
		"",
		platform_system_is_powered() ? "on" : "off"
	);

	logger_to_console("\n"
		"> application settings:\n"
		"  size ......... %u x %u\n"
		"  vsync ........ %d\n"
		"  target rate .. %u\n"
		"  fixed rate ... %u\n"
		"  slow frames .. %u\n"
		"",
		gs_app.config.size_x, gs_app.config.size_y,
		gs_app.config.vsync,
		gs_app.config.target_refresh_rate,
		gs_app.config.fixed_refresh_rate,
		gs_app.config.slow_frames_limit
	);

	// setup window
	enum Window_Settings window_settings = WINDOW_SETTINGS_MINIMIZE;
	if (gs_app.config.flexible) { window_settings |= WINDOW_SETTINGS_FLEXIBLE; }

	gs_app.window = platform_window_init(gs_app.config.size_x, gs_app.config.size_y, window_settings);
	if (gs_app.window == NULL) {
		logger_to_console("failed to create application window"); DEBUG_BREAK();
		common_exit_failure();
	}

	platform_window_start_frame(gs_app.window);
	gs_app.gpu_context = gpu_context_init(platform_window_get_cached_device(gs_app.window));
	gpu_context_start_frame(gs_app.gpu_context, platform_window_get_cached_device(gs_app.window));

	// setup timer, rewind it one frame
	int32_t const vsync_mode = gpu_context_get_vsync(gs_app.gpu_context);
	gs_app.ticks.per_second  = platform_timer_get_ticks_per_second();
	gs_app.ticks.frame_start = platform_timer_get_ticks() - get_target_ticks(vsync_mode);

	gpu_context_set_vsync(gs_app.gpu_context, gs_app.config.vsync);
	if (gs_app.callbacks.init != NULL) {
		gs_app.callbacks.init();
	}

	platform_window_end_frame(gs_app.window);
	gpu_context_end_frame();
}

static void application_free(void) {
	if (gs_app.callbacks.free != NULL) {
		gs_app.callbacks.free();
	}

	if (gs_app.gpu_context != NULL) {
		gpu_context_free(gs_app.gpu_context);
	}

	if (gs_app.window != NULL) {
		platform_window_free(gs_app.window);
	}
	platform_system_free();

	common_memset(&gs_app, 0, sizeof(gs_app));
}

static bool application_update(void) {
	// application and platform are ready
	if (gs_app.window == NULL) { return false; }
	if (!platform_system_is_running()) { return false; }

	// reset per-frame data / poll platform events
	platform_system_update();

	// window might be closed by platform
	if (!platform_window_exists(gs_app.window)) { return false; }

	// process application-side input
	if (input_key(KC_ALT)) {
		if (input_key_transition(KC_F4, true)) { return false; }
		if (input_key_transition(KC_ENTER, true)) {
			platform_window_toggle_borderless_fullscreen(gs_app.window);
		}
	}

	platform_window_start_frame(gs_app.window);
	gpu_context_start_frame(gs_app.gpu_context, platform_window_get_cached_device(gs_app.window));

	uint64_t const timer_ticks  = platform_timer_get_ticks();
	int32_t  const vsync_mode   = gpu_context_get_vsync(gs_app.gpu_context);
	uint64_t const target_ticks = get_target_ticks(vsync_mode);
	uint64_t const fixed_ticks  = gs_app.ticks.per_second / gs_app.config.fixed_refresh_rate;

	uint64_t frame_ticks = timer_ticks - gs_app.ticks.frame_start;
	gs_app.ticks.frame_start = timer_ticks;

	if (gs_app.config.slow_frames_limit > 0 && frame_ticks > target_ticks * gs_app.config.slow_frames_limit) {
		frame_ticks = target_ticks * gs_app.config.slow_frames_limit;
	}

	gs_app.ticks.fixed_accumulator += frame_ticks;

	while (gs_app.ticks.fixed_accumulator > fixed_ticks) {
		gs_app.ticks.fixed_accumulator -= fixed_ticks;
		if (gs_app.callbacks.fixed_update != NULL) {
			gs_app.callbacks.fixed_update(
				fixed_ticks,
				gs_app.ticks.per_second
			);
		}
	}

	if (gs_app.callbacks.frame_update != NULL) {
		gs_app.callbacks.frame_update(
			frame_ticks,
			gs_app.ticks.per_second
		);
	}

	if (gs_app.callbacks.draw_update != NULL) {
		gs_app.callbacks.draw_update(
			frame_ticks,
			gs_app.ticks.per_second
		);
	}

	platform_window_draw_frame(gs_app.window);
	gpu_context_draw_frame(gs_app.gpu_context);
	
	platform_window_end_frame(gs_app.window);
	gpu_context_end_frame();

	if (vsync_mode == 0) {
		uint64_t const frame_end_ticks = gs_app.ticks.frame_start + target_ticks;
		while (platform_timer_get_ticks() < frame_end_ticks) {
			platform_system_sleep(0);
		}
	}

	return true;
}

void application_run(struct Application_Config config, struct Application_Callbacks callbacks) {
	gs_app.config = config;
	gs_app.callbacks = callbacks;
	application_init();
	while (application_update()) { }
	application_free();
}

void application_get_screen_size(uint32_t * size_x, uint32_t * size_y) {
	platform_window_get_size(gs_app.window, size_x, size_y);
}
