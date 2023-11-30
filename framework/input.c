#include "framework/formatter.h"
#include "framework/containers/array.h"

// @idea: elaborate raw input
// @idea: expose Caps Lock and Num Lock toggle states?
// @idea: use per-frame input states (as it is now; 2021, march 23)
//        OR rely on the backend sending correct ammount of pushes and releases?

//
#include "input.h"

#define KEYBOARD_KEYS_MAX KC_ERROR + 1
#define MOUSE_KEYS_MAX 8

struct Keyboard_State {
	bool keys[KEYBOARD_KEYS_MAX];
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
	if (gs_input_state.mouse.delta_x == 0 && gs_input_state.mouse.delta_y == 0) {
		gs_input_state.mouse.delta_x = (int32_t)gs_input_state.mouse.window_x - (int32_t)gs_input_state.mouse_prev.window_x;
		gs_input_state.mouse.delta_y = (int32_t)gs_input_state.mouse.window_y - (int32_t)gs_input_state.mouse_prev.window_y;
	}
}

//
#include "framework/internal/input_to_window.h"


static enum Key_Code translate_scan(uint32_t scan) {
	static enum Key_Code const LUT_normal[] = {
		// ASCII, numbers
		[0x0B] = KC_D0,
		[0x02] = KC_D1,
		[0x03] = KC_D2,
		[0x04] = KC_D3,
		[0x05] = KC_D4,
		[0x06] = KC_D5,
		[0x07] = KC_D6,
		[0x08] = KC_D7,
		[0x09] = KC_D8,
		[0x0A] = KC_D9,
		// ASCII, letters
		[0x1E] = KC_A,
		[0x30] = KC_B,
		[0x2E] = KC_C,
		[0x20] = KC_D,
		[0x12] = KC_E,
		[0x21] = KC_F,
		[0x22] = KC_G,
		[0x23] = KC_H,
		[0x17] = KC_I,
		[0x24] = KC_J,
		[0x25] = KC_K,
		[0x26] = KC_L,
		[0x32] = KC_M,
		[0x31] = KC_N,
		[0x18] = KC_O,
		[0x19] = KC_P,
		[0x10] = KC_Q,
		[0x13] = KC_R,
		[0x1F] = KC_S,
		[0x14] = KC_T,
		[0x16] = KC_U,
		[0x2F] = KC_V,
		[0x11] = KC_W,
		[0x2D] = KC_X,
		[0x15] = KC_Y,
		[0x2C] = KC_Z,
		// ASCII, control characters
		[0x0E] = '\b',
		[0x0F] = '\t',
		[0x1C] = '\r',
		[0x01] = KC_ESC,
		// ASCII, printable characters
		[0x39] = ' ',
		[0x33] = ',',
		[0x34] = '.',
		[0x0C] = '-',
		[0x0D] = '=',
		[0x27] = ';',
		[0x35] = '/',
		[0x29] = '`',
		[0x1A] = '[',
		[0x2B] = '\\',
		[0x28] = '\'',
		[0x1B] = ']',
		// non-ASCII, common
		[0x2A] = KC_LSHIFT,
		[0x36] = KC_RSHIFT,
		[0x1D] = KC_LCTRL,
		[0x38] = KC_LALT,
		// non-ASCII, common
		[0x3A] = KC_CAPS_LOCK,
		// non-ASCII, numeric
		[0x45] = KC_NUM_LOCK,
		[0x4E] = KC_NUM_ADD,
		[0x4A] = KC_NUM_SUB,
		[0x37] = KC_NUM_MUL,
		[0x53] = KC_NUM_DEC,
		[0x52] = KC_NUM0,
		[0x4F] = KC_NUM1,
		[0x50] = KC_NUM2,
		[0x51] = KC_NUM3,
		[0x4B] = KC_NUM4,
		[0x4C] = KC_NUM5,
		[0x4D] = KC_NUM6,
		[0x47] = KC_NUM7,
		[0x48] = KC_NUM8,
		[0x49] = KC_NUM9,
		// non-ASCII, functional
		[0x3B] = KC_F1,
		[0x3C] = KC_F2,
		[0x3D] = KC_F3,
		[0x3E] = KC_F4,
		[0x3F] = KC_F5,
		[0x40] = KC_F6,
		[0x41] = KC_F7,
		[0x42] = KC_F8,
		[0x43] = KC_F9,
		[0x44] = KC_F10,
		[0x57] = KC_F11,
		[0x58] = KC_F12,
		[0x64] = KC_F13,
		[0x65] = KC_F14,
		[0x66] = KC_F15,
		[0x67] = KC_F16,
		[0x68] = KC_F17,
		[0x69] = KC_F18,
		[0x6A] = KC_F19,
		[0x6B] = KC_F20,
		[0x6C] = KC_F21,
		[0x6D] = KC_F22,
		[0x6E] = KC_F23,
		[0x76] = KC_F24,
		//
		[0xff] = 0,
	};

	static enum Key_Code const LUT_extended[] = {
		//
		[0x1C] = KC_NUM_ENTER,
		[0x35] = KC_NUM_DIV,
		//
		[0x37] = KC_PSCREEN,
		[0x1D] = KC_RCTRL,
		[0x38] = KC_RALT,
		[0x53] = KC_DEL,
		//
		[0x4B] = KC_ARROW_LEFT,
		[0x4D] = KC_ARROW_RIGHT,
		[0x50] = KC_ARROW_DOWN,
		[0x48] = KC_ARROW_UP,
		//
		[0x52] = KC_INSERT,
		[0x51] = KC_PAGE_UP,
		[0x49] = KC_PAGE_DOWN,
		[0x47] = KC_HOME,
		[0x4F] = KC_END,
		//
		[0xff] = 0,
	};

	uint8_t const index = scan & 0xff;
	return (scan & 0xff00)
		? LUT_extended[index]
		: LUT_normal[index];
}

void input_to_platform_reset(void) {
	common_memset(&gs_input_state.keyboard, 0, sizeof(gs_input_state.keyboard));
	common_memset(&gs_input_state.keyboard_prev, 0, sizeof(gs_input_state.keyboard_prev));
	common_memset(&gs_input_state.mouse, 0, sizeof(gs_input_state.mouse));
	common_memset(&gs_input_state.mouse_prev, 0, sizeof(gs_input_state.mouse_prev));
}

void input_to_platform_on_key(enum Key_Code key, uint32_t scan, bool is_down) {
	// @note: override translated key with a universal HID one
	key = translate_scan(scan);
	gs_input_state.keyboard.keys[key] = is_down;
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
