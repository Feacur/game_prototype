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
} input_state; // @note: global state

bool input_key(enum Key_Code key) {
	return input_state.keyboard.keys[key];
}

bool input_key_transition(enum Key_Code key, bool state) {
	uint8_t now = input_state.keyboard.keys[key];
	uint8_t was = input_state.keyboard_prev.keys[key];
	return (now != was) && (now == state);
}

void input_track_codepoints(bool state) {
	if (input_state.track_codepoints != state) { input_state.codepoints.count = 0; }
	input_state.track_codepoints = state;
}

struct Array_U32 const * input_get_codepoints(void) {
	return &input_state.codepoints;
}

void input_mouse_position_window(uint32_t * x, uint32_t * y) {
	*x = input_state.mouse.window_x;
	*y = input_state.mouse.window_y;
}

void input_mouse_position_display(uint32_t * x, uint32_t * y) {
	*x = input_state.mouse.display_x;
	*y = input_state.mouse.display_y;
}

void input_mouse_delta(int32_t * x, int32_t * y) {
	*x = input_state.mouse.delta_x;
	*y = input_state.mouse.delta_y;
}

bool input_mouse(enum Mouse_Code key) {
	return input_state.mouse.keys[key];
}

bool input_mouse_transition(enum Mouse_Code key, bool state) {
	uint8_t now = input_state.mouse.keys[key];
	uint8_t was = input_state.mouse_prev.keys[key];
	return (now != was) && (now == state);
}

//
#include "framework/internal/input_to_system.h"

void input_to_system_init(void) {
	array_u32_init(&input_state.codepoints);
}

void input_to_system_free(void) {
	array_u32_free(&input_state.codepoints);
	common_memset(&input_state, 0, sizeof(input_state));
}

void input_to_platform_before_update(void) {
	// track input transitions
	common_memcpy(&input_state.keyboard_prev, &input_state.keyboard, sizeof(input_state.keyboard));
	common_memcpy(&input_state.mouse_prev, &input_state.mouse, sizeof(input_state.mouse));

	// reset per-frame data
	input_state.mouse.delta_x = 0;
	input_state.mouse.delta_y = 0;
	input_state.mouse.wheel_x = 0;
	input_state.mouse.wheel_y = 0;
}

void input_to_platform_after_update(void) {
	// remap keyboard input ASCII characters
	static char const remap_src[] = ",./"    ";'"     "[]\\"    "`1234567890-=";
	static char const remap_dst[] = "<?>"    ":\""    "{}|"     "~!@#$%^&*()_+";

	for (uint8_t i = 0; i < sizeof(remap_src) / sizeof(*remap_src); i++) {
		input_state.keyboard.keys[(uint8_t)remap_dst[i]] = input_state.keyboard.keys[(uint8_t)remap_src[i]];
	}

	common_memcpy(
		input_state.keyboard.keys + (uint8_t)'A',
		input_state.keyboard.keys + (uint8_t)'a',
		sizeof(*input_state.keyboard.keys) * (1 + 'Z' - 'A')
	);

	//
	input_state.keyboard.keys[KC_SHIFT] = input_state.keyboard.keys[KC_LSHIFT] || input_state.keyboard.keys[KC_RSHIFT];
	input_state.keyboard.keys[KC_CTRL] = input_state.keyboard.keys[KC_LCTRL] || input_state.keyboard.keys[KC_RCTRL];
	input_state.keyboard.keys[KC_ALT] = input_state.keyboard.keys[KC_LALT] || input_state.keyboard.keys[KC_RALT];

	//
	if (input_state.mouse.delta_x == 0 && input_state.mouse.delta_y == 0) {
		input_state.mouse.delta_x = (int32_t)input_state.mouse.window_x - (int32_t)input_state.mouse_prev.window_x;
		input_state.mouse.delta_y = (int32_t)input_state.mouse.window_y - (int32_t)input_state.mouse_prev.window_y;
	}
}

//
#include "framework/internal/input_to_window.h"

void input_to_platform_reset(void) {
	common_memset(&input_state.keyboard, 0, sizeof(input_state.keyboard));
	common_memset(&input_state.keyboard_prev, 0, sizeof(input_state.keyboard_prev));
	common_memset(&input_state.mouse, 0, sizeof(input_state.mouse));
	common_memset(&input_state.mouse_prev, 0, sizeof(input_state.mouse_prev));
}

void input_to_platform_on_key(enum Key_Code key, bool is_down) {
	input_state.keyboard.keys[key] = is_down;
}

void input_to_platform_on_codepoint(uint32_t codepoint) {
	if (!input_state.track_codepoints) { return; }
	array_u32_push(&input_state.codepoints, codepoint);
}

void input_to_platform_on_mouse_move(uint32_t x, uint32_t y) {
	input_state.mouse.display_x = x;
	input_state.mouse.display_y = y;
}

void input_to_platform_on_mouse_move_window(uint32_t x, uint32_t y) {
	input_state.mouse.window_x = x;
	input_state.mouse.window_y = y;
}

void input_to_platform_on_mouse_delta(int32_t x, int32_t y) {
	input_state.mouse.delta_x += x;
	input_state.mouse.delta_y += y;
}

void input_to_platform_on_mouse_wheel(float x, float y) {
	input_state.mouse.wheel_x += x;
	input_state.mouse.wheel_y += y;
}

void input_to_platform_on_mouse(enum Mouse_Code key, bool is_down) {
	input_state.mouse.keys[key] = is_down;
}
