#include "framework/maths.h"
#include "framework/input.h"
#include "framework/formatter.h"

#include "framework/platform/timer.h"
#include "framework/platform/file.h"
#include "framework/platform/system.h"
#include "framework/platform/window.h"
#include "framework/platform/gpu_context.h"
#include "framework/systems/memory.h"
#include "framework/systems/defer.h"

#include "framework/graphics/misc.h"


//
#include "application.h"

static struct Application {
	struct Application_Config config;
	struct Application_Callbacks callbacks;

	bool should_exit;
	struct Window * window;
	struct GPU_Context * gpu_context;

	struct Application_Ticks {
		uint64_t elapsed;
		uint64_t fixed_accumulator;
	} ticks;
} gs_app;

static uint64_t get_target_ticks(void) {
	int32_t  const vsync_mode   = gpu_context_get_vsync(gs_app.gpu_context);
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
	void * surface = platform_window_get_surface(gs_app.window);
	gs_app.gpu_context = gpu_context_init(surface);
	gpu_context_start_frame(gs_app.gpu_context, surface);

	gpu_context_set_vsync(gs_app.gpu_context, gs_app.config.vsync);
	gs_app.ticks.elapsed = get_target_ticks();
	gs_app.ticks.fixed_accumulator = 0;

	if (gs_app.callbacks.init != NULL) {
		gs_app.callbacks.init();
	}

	gpu_context_end_frame(gs_app.gpu_context);
	platform_window_let_surface(gs_app.window, surface);
}

static void application_free(void) {
	if (gs_app.callbacks.free != NULL) { gs_app.callbacks.free(); }
	gpu_context_free(gs_app.gpu_context);
	platform_window_free(gs_app.window);
	cbuffer_clear(CBM_(gs_app));
}

void application_update(void) {
	// application and platform are ready
	if (platform_system_is_error()) { goto exit; }
	uint64_t const ticks_before = platform_timer_get_ticks();

	// reset per-frame data / poll platform events
	system_memory_arena_clear();
	platform_system_update();

	// window might be closed by platform
	if (!platform_window_exists(gs_app.window)) { goto exit; }
	if (!gpu_context_exists(gs_app.gpu_context)) { goto exit; }
	platform_window_update(gs_app.window);

	// process application-side input
	if (input_scan(SC_F11, IT_DOWN_TRNS)) {
		platform_window_toggle_borderless_fullscreen(gs_app.window);
	}

	// process logic and rendering
	void * surface = platform_window_get_surface(gs_app.window);
	gpu_context_start_frame(gs_app.gpu_context, surface);

	uint64_t const target_ticks = get_target_ticks();
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
	}

	// finalize defered actions and the whole frame
	system_defer_invoke();
	gpu_context_end_frame(gs_app.gpu_context);
	platform_window_let_surface(gs_app.window, surface);

	// maintain framerate
	uint64_t const ticks_until = ticks_before + target_ticks;
	while (platform_timer_get_ticks() < ticks_until) {
		platform_system_sleep(0);
	}

	gs_app.ticks.elapsed = clamp_u32(
		(uint32_t)(platform_timer_get_ticks() - ticks_before),
		(uint32_t)(platform_timer_get_ticks_per_second() / 10000), // max FPS
		(uint32_t)(platform_timer_get_ticks_per_second() /    10)  // min FPS
	);

	// TRC("%5llu micros", mul_div_u64(
	// 	platform_timer_get_ticks() - ticks_before
	// 	, 1000000
	// 	, platform_timer_get_ticks_per_second()
	// ));

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
