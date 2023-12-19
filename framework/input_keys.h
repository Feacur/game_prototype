#if !defined(FRAMEWORK_INPUT_KEYS)
#define FRAMEWORK_INPUT_KEYS

enum Scan_Code {
	SC_NONE,
	// letters
	SC_A                = 0x1e, // A
	SC_B                = 0x30, // B
	SC_C                = 0x2e, // C
	SC_D                = 0x20, // D
	SC_E                = 0x12, // E
	SC_F                = 0x21, // F
	SC_G                = 0x22, // G
	SC_H                = 0x23, // H
	SC_I                = 0x17, // I
	SC_J                = 0x24, // J
	SC_K                = 0x25, // K
	SC_L                = 0x26, // L
	SC_M                = 0x32, // M
	SC_N                = 0x31, // N
	SC_O                = 0x18, // O
	SC_P                = 0x19, // P
	SC_Q                = 0x10, // Q
	SC_R                = 0x13, // R
	SC_S                = 0x1f, // S
	SC_T                = 0x14, // T
	SC_U                = 0x16, // U
	SC_V                = 0x2f, // V
	SC_W                = 0x11, // W
	SC_X                = 0x2d, // X
	SC_Y                = 0x15, // Y
	SC_Z                = 0x2c, // Z
	// number and backslash
	SC_D1               = 0x02, // 1 !
	SC_D2               = 0x03, // 2 @
	SC_D3               = 0x04, // 3 #
	SC_D4               = 0x05, // 4 $
	SC_D5               = 0x06, // 5 %
	SC_D6               = 0x07, // 6 ^
	SC_D7               = 0x08, // 7 &
	SC_D8               = 0x09, // 8 *
	SC_D9               = 0x0a, // 9 (
	SC_D0               = 0x0b, // 0 )
	SC_DASH             = 0x0c, // - _
	SC_EQUALS           = 0x0d, // = +
	SC_BACK             = 0x0e, // \b
	// near letters
	SC_RETURN           = 0x1c, // \r
	SC_ESCAPE           = 0x01,
	SC_TAB              = 0x0f, // \t
	SC_SPACEBAR         = 0x39,
	SC_LBRACE           = 0x1a, // [ {
	SC_RBRACE           = 0x1b, // ] }
	SC_BSLASH           = 0x2b, // \ |
	SC_SCOLON           = 0x27, // ; :
	SC_QUOTE            = 0x28, // ' "
	SC_GRAVE            = 0x29, // ` ~
	SC_COMMA            = 0x33, // , <
	SC_PERIOD           = 0x34, // . >
	SC_SLASH            = 0x35, // / ?
	SC_DELETE           = 0x53 | 0xe000,
	// modificators
	SC_CAPS             = 0x3a,
	SC_LCTRL            = 0x1d,
	SC_RCTRL            = 0x1d | 0xe000,
	SC_LSHIFT           = 0x2a,
	SC_RSHIFT           = 0x36,
	SC_LALT             = 0x38,
	SC_RALT             = 0x38 | 0xe000,
	SC_LGUI             = 0x5b | 0xe000,
	SC_RGUI             = 0x5c | 0xe000,
	// navigation block
	SC_HOME             = 0x47 | 0xe000,
	SC_PAGE_UP          = 0x49 | 0xe000,
	SC_END              = 0x4f | 0xe000,
	SC_PAGE_DOWN        = 0x51 | 0xe000,
	SC_INSERT           = 0x52 | 0xe000,
	SC_PSCREEN          = 0x37 | 0xe000,
	// arrows
	SC_ARROW_RIGHT      = 0x4d | 0xe000,
	SC_ARROW_LEFT       = 0x4b | 0xe000,
	SC_ARROW_DOWN       = 0x50 | 0xe000,
	SC_ARROW_UP         = 0x48 | 0xe000,
	// functional
	SC_F1               = 0x3b,
	SC_F2               = 0x3c,
	SC_F3               = 0x3d,
	SC_F4               = 0x3e,
	SC_F5               = 0x3f,
	SC_F6               = 0x40,
	SC_F7               = 0x41,
	SC_F8               = 0x42,
	SC_F9               = 0x43,
	SC_F10              = 0x44,
	SC_F11              = 0x57,
	SC_F12              = 0x58,
	SC_F13              = 0x64,
	SC_F14              = 0x65,
	SC_F15              = 0x66,
	SC_F16              = 0x67,
	SC_F17              = 0x68,
	SC_F18              = 0x69,
	SC_F19              = 0x6a,
	SC_F20              = 0x6b,
	SC_F21              = 0x6c,
	SC_F22              = 0x6d,
	SC_F23              = 0x6e,
	SC_F24              = 0x76,
	// numpad
	SC_NUM_LOCK         = 0x45,
	SC_NUM_SLASH        = 0x35 | 0xe000,
	SC_NUM_STAR         = 0x37,
	SC_NUM_DASH         = 0x4a,
	SC_NUM_PLUS         = 0x4e,
	SC_NUM_ENTER        = 0x1c | 0xe000,
	SC_NUM1             = 0x4f, // end
	SC_NUM2             = 0x50, // arrow down
	SC_NUM3             = 0x51, // page down
	SC_NUM4             = 0x4b, // arrow left
	SC_NUM5             = 0x4c,
	SC_NUM6             = 0x4d, // right
	SC_NUM7             = 0x47, // home
	SC_NUM8             = 0x48, // arrow up
	SC_NUM9             = 0x49, // page up
	SC_NUM0             = 0x52, // insert
	SC_NUM_PERIOD       = 0x53,
};

enum Key_Code {
	KC_NONE,
	// letters
	KC_A                = 'A',
	KC_B                = 'B',
	KC_C                = 'C',
	KC_D                = 'D',
	KC_E                = 'E',
	KC_F                = 'F',
	KC_G                = 'G',
	KC_H                = 'H',
	KC_I                = 'I',
	KC_J                = 'J',
	KC_K                = 'K',
	KC_L                = 'L',
	KC_M                = 'M',
	KC_N                = 'N',
	KC_O                = 'O',
	KC_P                = 'P',
	KC_Q                = 'Q',
	KC_R                = 'R',
	KC_S                = 'S',
	KC_T                = 'T',
	KC_U                = 'U',
	KC_V                = 'V',
	KC_W                = 'W',
	KC_X                = 'X',
	KC_Y                = 'Y',
	KC_Z                = 'Z',
	// number and backslash
	KC_D1               = '1', // !
	KC_D2               = '2', // @
	KC_D3               = '3', // #
	KC_D4               = '4', // $
	KC_D5               = '5', // %
	KC_D6               = '6', // ^
	KC_D7               = '7', // &
	KC_D8               = '8', // *
	KC_D9               = '9', // (
	KC_D0               = '0', // )
	KC_DASH             = '-', // _
	KC_EQUALS           = '=', // +
	KC_BACK             = '\b',
	// near letters
	KC_RETURN           = '\r',
	KC_ESCAPE           = 0x1b,
	KC_TAB              = '\t',
	KC_SPACEBAR         = ' ',
	KC_LBRACE           = '[',  // {
	KC_RBRACE           = ']',  // }
	KC_BSLASH           = '\\', // |
	KC_SCOLON           = ';',  // :
	KC_QUOTE            = '\'', // "
	KC_GRAVE            = '`',  // ~
	KC_COMMA            = ',',  // <
	KC_PERIOD           = '.',  // >
	KC_SLASH            = '/',  // ?
	KC_DELETE           = 0x7f,
	// modificators
	KC_CAPS,
	KC_LCTRL,
	KC_RCTRL,
	KC_LSHIFT,
	KC_RSHIFT,
	KC_LALT,
	KC_RALT,
	KC_LGUI,
	KC_RGUI,
	// navigation block
	KC_HOME,
	KC_PAGE_UP,
	KC_END,
	KC_PAGE_DOWN,
	KC_INSERT,
	KC_PSCREEN,
	// arrows
	KC_ARROW_RIGHT,
	KC_ARROW_LEFT,
	KC_ARROW_DOWN,
	KC_ARROW_UP,
	// functional
	KC_F1,
	KC_F2,
	KC_F3,
	KC_F4,
	KC_F5,
	KC_F6,
	KC_F7,
	KC_F8,
	KC_F9,
	KC_F10,
	KC_F11,
	KC_F12,
	KC_F13,
	KC_F14,
	KC_F15,
	KC_F16,
	KC_F17,
	KC_F18,
	KC_F19,
	KC_F20,
	KC_F21,
	KC_F22,
	KC_F23,
	KC_F24,
	// numpad
	KC_NUM_LOCK,
	KC_NUM_SLASH,
	KC_NUM_STAR,
	KC_NUM_DASH,
	KC_NUM_PLUS,
	KC_NUM_ENTER,
	KC_NUM1, // end
	KC_NUM2, // arrow down
	KC_NUM3, // page down
	KC_NUM4, // arrow left
	KC_NUM5,
	KC_NUM6, // right
	KC_NUM7, // home
	KC_NUM8, // arrow up
	KC_NUM9, // page up
	KC_NUM0, // insert
	KC_NUM_PERIOD,
	//
	KC_COUNT = 0xff,
};

enum Mouse_Code {
	MC_NONE,
	MC_LEFT,
	MC_RIGHT,
	MC_MIDDLE,
	MC_X1,
	MC_X2,
	//
	MC_COUNT = 8,
};

enum Input_Type {
	IT_NONE = 0,
	IT_DOWN = (1 << 0),
	IT_TRNS = (1 << 1),
	// shorthands
	IT_DOWN_TRNS = IT_DOWN | IT_TRNS,
};

enum Key_Code scan_to_key(enum Scan_Code scan);

#endif
