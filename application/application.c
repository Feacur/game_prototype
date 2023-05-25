#include "framework/graphics/gpu_misc.h"

#include "framework/platform_timer.h"
#include "framework/platform_file.h"
#include "framework/platform_system.h"
#include "framework/platform_window.h"
#include "framework/gpu_context.h"
#include "framework/maths.h"
#include "framework/input.h"
#include "framework/logger.h"

//
#include "application.h"

static struct Application {
	struct Application_Config config;
	struct Application_Callbacks callbacks;

	bool is_inited, should_exit;
	struct Window * window;
	struct Gpu_Context * gpu_context;

	struct Application_Ticks {
		uint64_t elapsed, per_second;
		uint64_t fixed_accumulator;
		uint64_t frame_start;
	} ticks;
} gs_app;

static uint64_t get_target_ticks(int32_t vsync_mode) {
	uint32_t const vsync_factor = (vsync_mode > 0) ? (uint32_t)vsync_mode : 1;
	uint32_t const refresh_rate = ((vsync_mode != 0) || (gs_app.config.target_refresh_rate == 0))
		? platform_window_get_refresh_rate(gs_app.window, gs_app.config.target_refresh_rate)
		: gs_app.config.target_refresh_rate;
	return gs_app.ticks.per_second * vsync_factor / refresh_rate;
}

static uint64_t get_fixed_ticks(uint64_t target_ticks) {
	return (gs_app.config.fixed_refresh_rate > 0)
		? gs_app.ticks.per_second / gs_app.config.fixed_refresh_rate
		: target_ticks;
}

static bool application_init(void) {
	logger_to_console(
		"> system status:\n"
		"  power .. %s\n"
		,
		platform_system_is_powered() ? "on" : "off"
	);

	logger_to_console(
		"> application settings:\n"
		"  size ......... %u x %u\n"
		"  vsync ........ %d\n"
		"  target rate .. %u\n"
		"  fixed rate ... %u\n"
		"  slow frames .. %u\n"
		,
		gs_app.config.size.x, gs_app.config.size.y,
		gs_app.config.vsync,
		gs_app.config.target_refresh_rate,
		gs_app.config.fixed_refresh_rate,
		gs_app.config.slow_frames_limit
	);

	// setup window
	struct Window_Config window_config = {
		.size_x = gs_app.config.size.x,
		.size_y = gs_app.config.size.y,
		.settings = WINDOW_SETTINGS_MINIMIZE,
	};
	if (gs_app.config.resizable) { window_config.settings |= WINDOW_SETTINGS_RESIZABLE; }

	gs_app.window = platform_window_init(window_config, gs_app.callbacks.window_callbacks);
	if (gs_app.window == NULL) { goto fail_window; }
	platform_window_focus(gs_app.window);

	platform_window_start_frame(gs_app.window);
	gs_app.gpu_context = gpu_context_init(platform_window_get_cached_device(gs_app.window));
	if (gs_app.gpu_context == NULL) { goto fail_gpu_context; }

	gpu_context_start_frame(gs_app.gpu_context, platform_window_get_cached_device(gs_app.window));

	// setup timer, rewind it one frame
	gs_app.ticks.per_second = platform_timer_get_ticks_per_second();
	gpu_context_set_vsync(gs_app.gpu_context, gs_app.config.vsync);
	int32_t const vsync_mode = gpu_context_get_vsync(gs_app.gpu_context);
	gs_app.ticks.elapsed     = get_target_ticks(vsync_mode);
	gs_app.ticks.frame_start = platform_timer_get_ticks() - gs_app.ticks.elapsed;

	if (gs_app.callbacks.init != NULL) {
		gs_app.callbacks.init();
	}

	gpu_context_end_frame();
	platform_window_end_frame(gs_app.window);

	gs_app.is_inited = true;
	return true;

	// process errors
	fail_gpu_context:
	platform_window_end_frame(gs_app.window);

	fail_window:
	return false;
}

static void application_free(void) {
	if (gs_app.callbacks.free != NULL) { gs_app.callbacks.free(); }
	if (gs_app.gpu_context != NULL) { gpu_context_free(gs_app.gpu_context); }
	if (gs_app.window != NULL) { platform_window_free(gs_app.window); }
	common_memset(&gs_app, 0, sizeof(gs_app));
}

static void application_begin_frame(void) {
	platform_window_start_frame(gs_app.window);
	gpu_context_start_frame(gs_app.gpu_context, platform_window_get_cached_device(gs_app.window));
}

static void application_draw_frame(void) {
	platform_window_draw_frame(gs_app.window);
	gpu_context_draw_frame(gs_app.gpu_context);

	graphics_update();
}

static void application_end_frame(void) {
	platform_window_end_frame(gs_app.window);
	gpu_context_end_frame();
}

void application_update(void) {
	// application and platform are ready
	if (gs_app.window == NULL) { goto exit; }
	if (platform_system_is_error()) { goto exit; }

	// reset per-frame data / poll platform events
	platform_system_update();

	// window might be closed by platform
	if (!platform_window_exists(gs_app.window)) { goto exit; }
	platform_window_update(gs_app.window);

	// process application-side input
	if (input_key(KC_ALT) && input_key_transition(KC_ENTER, true)) {
		platform_window_toggle_borderless_fullscreen(gs_app.window);
	}

	application_begin_frame();

	// @note: resulting delta time is extremely inconsistent and stuttery
	uint64_t const timer_ticks  = platform_timer_get_ticks();
	int32_t  const vsync_mode   = gpu_context_get_vsync(gs_app.gpu_context);
	uint64_t const target_ticks = get_target_ticks(vsync_mode);
	uint64_t const fixed_ticks  = get_fixed_ticks(target_ticks);

	gs_app.ticks.elapsed = timer_ticks - gs_app.ticks.frame_start;
	gs_app.ticks.frame_start = timer_ticks;

	if (gs_app.config.slow_frames_limit > 0 && gs_app.ticks.elapsed > target_ticks * gs_app.config.slow_frames_limit) {
		gs_app.ticks.elapsed = target_ticks * gs_app.config.slow_frames_limit;
	}

	// @note: probably should limit this too?
	gs_app.ticks.fixed_accumulator += gs_app.ticks.elapsed;
	while (fixed_ticks > 0 && gs_app.ticks.fixed_accumulator > fixed_ticks) {
		gs_app.ticks.fixed_accumulator -= fixed_ticks;
		if (gs_app.callbacks.fixed_tick != NULL) {
			gs_app.callbacks.fixed_tick();
		}
	}

	if (gs_app.callbacks.frame_tick != NULL) {
		gs_app.callbacks.frame_tick();
		application_draw_frame();
	}

	application_end_frame();

	if (vsync_mode == 0) {
		uint64_t const frame_end_ticks = gs_app.ticks.frame_start + target_ticks;
		while (platform_timer_get_ticks() < frame_end_ticks) {
			platform_system_sleep(0);
		}
	}

	return;
	exit: gs_app.should_exit = true;
}

void application_run(struct Application_Config config, struct Application_Callbacks callbacks) {
	gs_app.config = config;
	gs_app.callbacks = callbacks;
	gs_app.should_exit = false;
	if (application_init()) {
		while (!gs_app.should_exit) { application_update(); }
		application_free();
	}
}

void application_exit(void) {
	gs_app.should_exit = true;
}

struct uvec2 application_get_screen_size(void) {
	struct uvec2 result;
	platform_window_get_size(gs_app.window, &result.x, &result.y);
	return result;
}

double application_get_delta_time(void) {
	return (double)gs_app.ticks.elapsed / (double)gs_app.ticks.per_second;
}

bool application_is_inited(void) { return gs_app.is_inited; }
