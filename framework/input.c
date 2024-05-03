#include "framework/formatter.h"
#include "framework/containers/array.h"


// @idea: elaborate raw input
// @idea: expose Caps Lock and Num Lock toggle states?
// @idea: use per-frame input states (as it is now; 2021, march 23)
//        OR rely on the backend sending correct ammount of pushes and releases?

//
#include "input.h"

#define KEYBOARD_KEYS_MAX KC_COUNT + 1
#define MOUSE_KEYS_MAX MC_COUNT + 1

struct Keyboard_State {
	bool keys[KEYBOARD_KEYS_MAX];
	bool scans[KEYBOARD_KEYS_MAX];
	// bool caps_lock, num_lock;
};

struct Mouse_State {
	bool keys[MOUSE_KEYS_MAX];

	uint32_t display_x, display_y;
	uint32_t window_x, window_y;

	int32_t delta_x, delta_y;
	float wheel_x, wheel_y;
};

static struct Input_State {
	struct Keyboard_State keyboard, keyboard_prev;
	struct Mouse_State mouse, mouse_prev;
	struct Array codepoints; // uint32_t
	bool track_codepoints;
} gs_input_state;

bool input_key(enum Key_Code key, enum Input_Type state) {
	bool const is_down = gs_input_state.keyboard.keys[key];
	bool const is_trns = gs_input_state.keyboard.keys[key] != gs_input_state.keyboard_prev.keys[key];
	if ((state & IT_DOWN) && !is_down) { return false; }
	if ((state & IT_TRNS) && !is_trns) { return false; }
	return true;
}

bool input_scan(enum Scan_Code scan, enum Input_Type state) {
	enum Key_Code key = scan_to_key(scan);
	bool const is_down = gs_input_state.keyboard.scans[key];
	bool const is_trns = gs_input_state.keyboard.scans[key] != gs_input_state.keyboard_prev.scans[key];
	if ((state & IT_DOWN) && !is_down) { return false; }
	if ((state & IT_TRNS) && !is_trns) { return false; }
	return true;
}

void input_track_codepoints(bool state) {
	if (gs_input_state.track_codepoints != state) { gs_input_state.codepoints.count = 0; }
	gs_input_state.track_codepoints = state;
}

struct Array const * input_get_codepoints(void) {
	return &gs_input_state.codepoints;
}

void input_mouse_position_window(uint32_t * x, uint32_t * y) {
	*x = gs_input_state.mouse.window_x;
	*y = gs_input_state.mouse.window_y;
}

void input_mouse_position_display(uint32_t * x, uint32_t * y) {
	*x = gs_input_state.mouse.display_x;
	*y = gs_input_state.mouse.display_y;
}

void input_mouse_delta(int32_t * x, int32_t * y) {
	*x = gs_input_state.mouse.delta_x;
	*y = gs_input_state.mouse.delta_y;
}

bool input_mouse(enum Mouse_Code key, enum Input_Type state) {
	bool const is_down = gs_input_state.mouse.keys[key];
	bool const is_trns = gs_input_state.mouse.keys[key] != gs_input_state.mouse_prev.keys[key];
	if ((state & IT_DOWN) && !is_down) { return false; }
	if ((state & IT_TRNS) && !is_trns) { return false; }
	return true;
}

//
#include "framework/internal/input_to_system.h"

bool input_to_system_init(void) {
	gs_input_state.codepoints = array_init(sizeof(uint32_t));
	return true;
}

void input_to_system_free(void) {
	array_free(&gs_input_state.codepoints);
	cbuffer_clear(CBM_(gs_input_state));
}

void input_to_platform_before_update(void) {
	// track input transitions
	common_memcpy(&gs_input_state.keyboard_prev, &gs_input_state.keyboard, sizeof(gs_input_state.keyboard));
	common_memcpy(&gs_input_state.mouse_prev, &gs_input_state.mouse, sizeof(gs_input_state.mouse));

	// reset per-frame data
	gs_input_state.mouse.delta_x = 0;
	gs_input_state.mouse.delta_y = 0;
	gs_input_state.mouse.wheel_x = 0;
	gs_input_state.mouse.wheel_y = 0;
}

void input_to_platform_after_update(void) {
	if (gs_input_state.mouse.delta_x == 0 && gs_input_state.mouse.delta_y == 0) {
		gs_input_state.mouse.delta_x = (int32_t)gs_input_state.mouse.window_x - (int32_t)gs_input_state.mouse_prev.window_x;
		gs_input_state.mouse.delta_y = (int32_t)gs_input_state.mouse.window_y - (int32_t)gs_input_state.mouse_prev.window_y;
	}
}

//
#include "framework/internal/input_to_window.h"

void input_to_platform_reset(void) {
	cbuffer_clear(CBM_(gs_input_state.keyboard));
	cbuffer_clear(CBM_(gs_input_state.keyboard_prev));
	cbuffer_clear(CBM_(gs_input_state.mouse));
	cbuffer_clear(CBM_(gs_input_state.mouse_prev));
}

void input_to_platform_on_key(enum Key_Code key, enum Scan_Code scan, bool is_down) {
	enum Key_Code translated = scan_to_key(scan);
	gs_input_state.keyboard.keys[key] = is_down;
	gs_input_state.keyboard.scans[translated] = is_down;
}

void input_to_platform_on_codepoint(uint32_t codepoint) {
	if (!gs_input_state.track_codepoints) { return; }
	array_push_many(&gs_input_state.codepoints, 1, &codepoint);
}

void input_to_platform_on_mouse_move(uint32_t x, uint32_t y) {
	gs_input_state.mouse.display_x = x;
	gs_input_state.mouse.display_y = y;
}

void input_to_platform_on_mouse_move_window(uint32_t x, uint32_t y) {
	gs_input_state.mouse.window_x = x;
	gs_input_state.mouse.window_y = y;
}

void input_to_platform_on_mouse_delta(int32_t x, int32_t y) {
	gs_input_state.mouse.delta_x += x;
	gs_input_state.mouse.delta_y += y;
}

void input_to_platform_on_mouse_wheel(float x, float y) {
	gs_input_state.mouse.wheel_x += x;
	gs_input_state.mouse.wheel_y += y;
}

void input_to_platform_on_mouse(enum Mouse_Code key, bool is_down) {
	gs_input_state.mouse.keys[key] = is_down;
}
