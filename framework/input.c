#include "framework/logger.h"
#include "framework/containers/array_u32.h"

// @idea: elaborate raw input
// @idea: expose Caps Lock and Num Lock toggle states?
// @idea: use per-frame input states (as it is now; 2021, march 23)
//        OR rely on the backend sending correct ammount of pushes and releases?

//
#include "input.h"

#define KEYBOARD_KEYS_MAX UINT8_MAX + 1
#define MOUSE_KEYS_MAX 8

struct Keyboard_State {
	uint8_t keys[KEYBOARD_KEYS_MAX];
	// bool caps_lock, num_lock;
};

struct Mouse_State {
	uint8_t keys[MOUSE_KEYS_MAX];

	uint32_t display_x, display_y;
	uint32_t window_x, window_y;

	int32_t delta_x, delta_y;
	float wheel_x, wheel_y;
};

static struct Input_State {
	struct Keyboard_State keyboard, keyboard_prev;
	struct Mouse_State mouse, mouse_prev;
	struct Array_U32 codepoints;
	bool track_codepoints;
} gs_input_state;

bool input_key(enum Key_Code key) {
	return gs_input_state.keyboard.keys[key];
}

bool input_key_transition(enum Key_Code key, bool state) {
	uint8_t now = gs_input_state.keyboard.keys[key];
	uint8_t was = gs_input_state.keyboard_prev.keys[key];
	return (now != was) && (now == state);
}

void input_track_codepoints(bool state) {
	if (gs_input_state.track_codepoints != state) { gs_input_state.codepoints.count = 0; }
	gs_input_state.track_codepoints = state;
}

struct Array_U32 const * input_get_codepoints(void) {
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

bool input_mouse(enum Mouse_Code key) {
	return gs_input_state.mouse.keys[key];
}

bool input_mouse_transition(enum Mouse_Code key, bool state) {
	uint8_t now = gs_input_state.mouse.keys[key];
	uint8_t was = gs_input_state.mouse_prev.keys[key];
	return (now != was) && (now == state);
}

//
#include "framework/internal/input_to_system.h"

void input_to_system_init(void) {
	array_u32_init(&gs_input_state.codepoints);
}

void input_to_system_free(void) {
	array_u32_free(&gs_input_state.codepoints);
	common_memset(&gs_input_state, 0, sizeof(gs_input_state));
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
	// remap keyboard input ASCII characters
	static char const c_remap_src[] = ",./"    ";'"     "[]\\"    "`1234567890-=";
	static char const c_remap_dst[] = "<?>"    ":\""    "{}|"     "~!@#$%^&*()_+";

	for (uint8_t i = 0; i < sizeof(c_remap_src) / sizeof(*c_remap_src); i++) {
		gs_input_state.keyboard.keys[(uint8_t)c_remap_dst[i]] = gs_input_state.keyboard.keys[(uint8_t)c_remap_src[i]];
	}

	common_memcpy(
		gs_input_state.keyboard.keys + (uint8_t)'A',
		gs_input_state.keyboard.keys + (uint8_t)'a',
		sizeof(*gs_input_state.keyboard.keys) * (1 + 'Z' - 'A')
	);

	//
	gs_input_state.keyboard.keys[KC_SHIFT] = gs_input_state.keyboard.keys[KC_LSHIFT] || gs_input_state.keyboard.keys[KC_RSHIFT];
	gs_input_state.keyboard.keys[KC_CTRL] = gs_input_state.keyboard.keys[KC_LCTRL] || gs_input_state.keyboard.keys[KC_RCTRL];
	gs_input_state.keyboard.keys[KC_ALT] = gs_input_state.keyboard.keys[KC_LALT] || gs_input_state.keyboard.keys[KC_RALT];

	//
	if (gs_input_state.mouse.delta_x == 0 && gs_input_state.mouse.delta_y == 0) {
		gs_input_state.mouse.delta_x = (int32_t)gs_input_state.mouse.window_x - (int32_t)gs_input_state.mouse_prev.window_x;
		gs_input_state.mouse.delta_y = (int32_t)gs_input_state.mouse.window_y - (int32_t)gs_input_state.mouse_prev.window_y;
	}
}

//
#include "framework/internal/input_to_window.h"

void input_to_platform_reset(void) {
	common_memset(&gs_input_state.keyboard, 0, sizeof(gs_input_state.keyboard));
	common_memset(&gs_input_state.keyboard_prev, 0, sizeof(gs_input_state.keyboard_prev));
	common_memset(&gs_input_state.mouse, 0, sizeof(gs_input_state.mouse));
	common_memset(&gs_input_state.mouse_prev, 0, sizeof(gs_input_state.mouse_prev));
}

void input_to_platform_on_key(enum Key_Code key, bool is_down) {
	gs_input_state.keyboard.keys[key] = is_down;
}

void input_to_platform_on_codepoint(uint32_t codepoint) {
	if (!gs_input_state.track_codepoints) { return; }
	array_u32_push(&gs_input_state.codepoints, codepoint);
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
