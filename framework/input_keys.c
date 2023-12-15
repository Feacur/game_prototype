#include "common.h"


//
#include "input_keys.h"

enum Key_Code scan_to_key(enum Scan_Code scan) {
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
