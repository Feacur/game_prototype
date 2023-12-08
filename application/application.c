#include "framework/graphics/misc.h"

#include "framework/platform/timer.h"
#include "framework/platform/file.h"
#include "framework/platform/system.h"
#include "framework/platform/window.h"
#include "framework/systems/action_system.h"
#include "framework/systems/material_system.h"
#include "framework/systems/asset_system.h"
#include "framework/gpu_context.h"
#include "framework/maths.h"
#include "framework/input.h"
#include "framework/formatter.h"

#include "asset_registry.h"

//
#include "application.h"

static struct Application {
	struct Application_Config config;
	struct Application_Callbacks callbacks;

	bool should_exit;
	struct Window * window;
	struct Gpu_Context * gpu_context;

	struct Application_Ticks {
		uint64_t elapsed;
		uint64_t fixed_accumulator;
	} ticks;
} gs_app;

static uint64_t get_target_ticks(int32_t vsync_mode) {
	uint64_t const vsync_factor = (vsync_mode > 0) ? (uint64_t)vsync_mode : 1;
	uint64_t const refresh_rate = ((vsync_mode != 0) || (gs_app.config.target_refresh_rate == 0))
		? platform_window_get_refresh_rate(gs_app.window, gs_app.config.target_refresh_rate)
		: gs_app.config.target_refresh_rate;
	return platform_timer_get_ticks_per_second() * vsync_factor / refresh_rate;
}

static uint64_t get_fixed_ticks(uint64_t default_value) {
	return (gs_app.config.fixed_refresh_rate > 0)
		? platform_timer_get_ticks_per_second() / gs_app.config.fixed_refresh_rate
		: default_value;
}

static void application_init(void) {
	action_system_init();
	material_system_init();
	asset_system_init();
	asset_types_init();

	LOG(
		"> application settings:\n"
		"  size ......... %u x %u\n"
		"  vsync ........ %d\n"
		"  target rate .. %u\n"
		"  fixed rate ... %u\n"
		""
		, gs_app.config.size.x, gs_app.config.size.y
		, gs_app.config.vsync
		, gs_app.config.target_refresh_rate
		, gs_app.config.fixed_refresh_rate
	);

	// setup window
	struct Window_Config window_config = {
		.size = gs_app.config.size,
		.settings = WINDOW_SETTINGS_MINIMIZE,
	};
	if (gs_app.config.resizable) { window_config.settings |= WINDOW_SETTINGS_RESIZABLE; }

	gs_app.window = platform_window_init(window_config, gs_app.callbacks.window_callbacks);
	if (gs_app.window == NULL) { goto finalize; }
	platform_window_start_frame(gs_app.window);

	gs_app.gpu_context = gpu_context_init(platform_window_get_cached_device(gs_app.window));
	if (gs_app.gpu_context == NULL) { goto finalize; }
	gpu_context_start_frame(gs_app.gpu_context, platform_window_get_cached_device(gs_app.window));

	// setup timer, rewind it one frame
	gpu_context_set_vsync(gs_app.gpu_context, gs_app.config.vsync);
	int32_t const vsync_mode = gpu_context_get_vsync(gs_app.gpu_context);
	gs_app.ticks.elapsed     = get_target_ticks(vsync_mode);
	gs_app.ticks.fixed_accumulator = 0;

	finalize:
	if (gs_app.callbacks.init != NULL) {
		gs_app.callbacks.init();
	}

	if (gs_app.gpu_context != NULL) { gpu_context_end_frame(gs_app.gpu_context); }
	if (gs_app.window != NULL) { platform_window_end_frame(gs_app.window); }
}

static void application_free(void) {
	if (gs_app.callbacks.free != NULL) { gs_app.callbacks.free(); }
	if (gs_app.gpu_context    != NULL) { gpu_context_free(gs_app.gpu_context); }
	if (gs_app.window         != NULL) { platform_window_free(gs_app.window); }

	asset_types_free();
	asset_system_free();
	material_system_free();
	action_system_free();

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
	action_system_update();
	platform_window_end_frame(gs_app.window);
	gpu_context_end_frame(gs_app.gpu_context);
}

void application_update(void) {
	// application and platform are ready
	if (gs_app.window == NULL) { goto exit; }
	if (platform_system_is_error()) { goto exit; }
	uint64_t const ticks_before = platform_timer_get_ticks();

	// reset per-frame data / poll platform events
	platform_system_update();

	// window might be closed by platform
	if (!platform_window_exists(gs_app.window)) { goto exit; }
	platform_window_update(gs_app.window);

	// process application-side input
	if (input_key(KC_F11, IT_DOWN_TRNS)) {
		platform_window_toggle_borderless_fullscreen(gs_app.window);
	}

	// process logic and rendering
	application_begin_frame();
	int32_t  const vsync_mode   = gpu_context_get_vsync(gs_app.gpu_context);
	uint64_t const target_ticks = get_target_ticks(vsync_mode);
	uint64_t const fixed_ticks  = get_fixed_ticks(target_ticks);

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

	action_system_update();

	// maintain framerate
	uint64_t const ticks_until = ticks_before + target_ticks;
	while (platform_timer_get_ticks() < ticks_until) {
		platform_system_sleep(0);
	}

	// TRC("%5llu micros", mul_div_u64(
	// 	platform_timer_get_ticks() - ticks_before
	// 	, 1000000
	// 	, platform_timer_get_ticks_per_second()
	// ));

	gs_app.ticks.elapsed = clamp_u32(
		(uint32_t)(platform_timer_get_ticks() - ticks_before),
		(uint32_t)(platform_timer_get_ticks_per_second() / 10000), // max FPS
		(uint32_t)(platform_timer_get_ticks_per_second() /    10)  // min FPS
	);

	// done
	return;
	exit: gs_app.should_exit = true;
}

void application_run(struct Application_Config config, struct Application_Callbacks callbacks) {
	gs_app.config = config;
	gs_app.callbacks = callbacks;
	gs_app.should_exit = false;

	application_init();
	if (gs_app.window == NULL) { goto finalize; }
	if (gs_app.gpu_context == NULL) { goto finalize; }

	platform_window_focus(gs_app.window);
	while (!gs_app.should_exit) { application_update(); }

	finalize:
	application_free();
}

void application_exit(void) {
	gs_app.should_exit = true;
}

struct uvec2 application_get_screen_size(void) {
	return platform_window_get_size(gs_app.window);
}

double application_get_delta_time(void) {
	return (double)gs_app.ticks.elapsed / (double)platform_timer_get_ticks_per_second();
}
