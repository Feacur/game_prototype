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

static enum Key_Code scan_to_key(enum Scan_Code scan) {
	static enum Key_Code const LUT_normal[] = {
		// letters
		[SC_A]                = 'A',
		[SC_B]                = 'B',
		[SC_C]                = 'C',
		[SC_D]                = 'D',
		[SC_E]                = 'E',
		[SC_F]                = 'F',
		[SC_G]                = 'G',
		[SC_H]                = 'H',
		[SC_I]                = 'I',
		[SC_J]                = 'J',
		[SC_K]                = 'K',
		[SC_L]                = 'L',
		[SC_M]                = 'M',
		[SC_N]                = 'N',
		[SC_O]                = 'O',
		[SC_P]                = 'P',
		[SC_Q]                = 'Q',
		[SC_R]                = 'R',
		[SC_S]                = 'S',
		[SC_T]                = 'T',
		[SC_U]                = 'U',
		[SC_V]                = 'V',
		[SC_W]                = 'W',
		[SC_X]                = 'X',
		[SC_Y]                = 'Y',
		[SC_Z]                = 'Z',
		// number and backslash
		[SC_D1]               = '1',
		[SC_D2]               = '2',
		[SC_D3]               = '3',
		[SC_D4]               = '4',
		[SC_D5]               = '5',
		[SC_D6]               = '6',
		[SC_D7]               = '7',
		[SC_D8]               = '8',
		[SC_D9]               = '9',
		[SC_D0]               = '0',
		[SC_DASH]             = '-',
		[SC_EQUALS]           = '=',
		[SC_BACK]             = '\b',
		// near letters
		[SC_RETURN]           = '\r',
		[SC_ESCAPE]           = KC_ESCAPE,
		[SC_TAB]              = '\t',
		[SC_SPACEBAR]         = ' ',
		[SC_LBRACE]           = '[',
		[SC_RBRACE]           = ']',
		[SC_BSLASH]           = '\\',
		[SC_SCOLON]           = ';',
		[SC_QUOTE]            = '\'',
		[SC_GRAVE]            = '`',
		[SC_COMMA]            = ',',
		[SC_PERIOD]           = '.',
		[SC_SLASH]            = '/',
		// modificators
		[SC_CAPS]             = KC_CAPS,
		[SC_LCTRL]            = KC_LCTRL,
		[SC_LSHIFT]           = KC_LSHIFT,
		[SC_RSHIFT]           = KC_RSHIFT,
		[SC_LALT]             = KC_LALT,
		// functional
		[SC_F1]               = KC_F1,
		[SC_F2]               = KC_F2,
		[SC_F3]               = KC_F3,
		[SC_F4]               = KC_F4,
		[SC_F5]               = KC_F5,
		[SC_F6]               = KC_F6,
		[SC_F7]               = KC_F7,
		[SC_F8]               = KC_F8,
		[SC_F9]               = KC_F9,
		[SC_F10]              = KC_F10,
		[SC_F11]              = KC_F11,
		[SC_F12]              = KC_F12,
		[SC_F13]              = KC_F13,
		[SC_F14]              = KC_F14,
		[SC_F15]              = KC_F15,
		[SC_F16]              = KC_F16,
		[SC_F17]              = KC_F17,
		[SC_F18]              = KC_F18,
		[SC_F19]              = KC_F19,
		[SC_F20]              = KC_F20,
		[SC_F21]              = KC_F21,
		[SC_F22]              = KC_F22,
		[SC_F23]              = KC_F23,
		[SC_F24]              = KC_F24,
		// numpad
		[SC_NUM_LOCK]         = KC_NUM_LOCK,
		[SC_NUM_STAR]         = KC_NUM_STAR,
		[SC_NUM_DASH]         = KC_NUM_DASH,
		[SC_NUM_PLUS]         = KC_NUM_PLUS,
		[SC_NUM1]             = KC_NUM1,
		[SC_NUM2]             = KC_NUM2,
		[SC_NUM3]             = KC_NUM3,
		[SC_NUM4]             = KC_NUM4,
		[SC_NUM5]             = KC_NUM5,
		[SC_NUM6]             = KC_NUM6,
		[SC_NUM7]             = KC_NUM7,
		[SC_NUM8]             = KC_NUM8,
		[SC_NUM9]             = KC_NUM9,
		[SC_NUM0]             = KC_NUM0,
		[SC_NUM_PERIOD]       = KC_NUM_PERIOD,
		//
		[0xff] = 0,
	};

	static enum Key_Code const LUT_extended[] = {
		// near letters
		[SC_DELETE]           = KC_DELETE,
		// modificators
		[SC_RCTRL]            = KC_RCTRL,
		[SC_RALT]             = KC_RALT,
		[SC_LGUI]             = KC_LGUI,
		[SC_RGUI]             = KC_RGUI,
		// navigation block
		[SC_INSERT]           = KC_INSERT,
		[SC_HOME]             = KC_HOME,
		[SC_PAGE_UP]          = KC_PAGE_UP,
		[SC_END]              = KC_END,
		[SC_PAGE_DOWN]        = KC_PAGE_DOWN,
		// arrows
		[SC_ARROW_RIGHT]      = KC_ARROW_RIGHT,
		[SC_ARROW_LEFT]       = KC_ARROW_LEFT,
		[SC_ARROW_DOWN]       = KC_ARROW_DOWN,
		[SC_ARROW_UP]         = KC_PAGE_UP,
		// numpad
		[SC_NUM_SLASH]        = KC_NUM_SLASH,
		[SC_NUM_ENTER]        = KC_NUM_ENTER,
		//
		[0xff] = 0,
	};

	uint8_t const index = scan & 0x00ff;
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

void input_to_platform_on_key(enum Key_Code key, enum Scan_Code scan, bool is_down) {
	// @note: override translated key with a universal HID one
	key = scan_to_key(scan);
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
