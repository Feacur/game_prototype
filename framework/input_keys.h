#if !defined(FRAMEWORK_INPUT_KEYS)
#define FRAMEWORK_INPUT_KEYS

enum Key_Code {
	// ASCII, control characters
	// KC_NULL   = '\0', // ctrl + @
	// KC_SOH    = 0x01, // ctrl + a
	// KC_STX    = 0x02, // ctrl + b
	// KC_ETX    = 0x03, // ctrl + c
	// KC_EOT    = 0x04, // ctrl + d
	// KC_ENQ    = 0x05, // ctrl + e
	// KC_ACK    = 0x06, // ctrl + f
	// KC_BELL   = '\a', // ctrl + g
	// KC_BSPACE = '\b', // ctrl + h
	KC_TAB    = '\t', // ctrl + i, tab
	// KC_LINE   = '\n', // ctrl + j, ctrl + return
	// KC_VTAB   = '\v', // ctrl + k
	// KC_FORM   = '\f', // ctrl + l
	KC_RET    = '\r', // ctrl + m, return
	// KC_SO     = 0x0e, // ctrl + n
	// KC_SI     = 0x0f, // ctrl + o
	// KC_DLE    = 0x10, // ctrl + p
	// KC_DC1    = 0x11, // ctrl + q
	// KC_DC2    = 0x12, // ctrl + r
	// KC_DC3    = 0x13, // ctrl + s
	// KC_DC4    = 0x14, // ctrl + t
	// KC_NAK    = 0x15, // ctrl + u
	// KC_SYN    = 0x16, // ctrl + v
	// KC_ETB    = 0x17, // ctrl + w
	// KC_EM     = 0x19, // ctrl + y
	// KC_SUB    = 0x1a, // ctrl + z
	KC_ESC    = 0x1b, // ctrl + [, escape
	// KC_FS     = 0x1c, // ctrl + \, [prevent escaping \n]
	// KC_GS     = 0x1d, // ctrl + ]
	// KC_RS     = 0x1e, // ctrl + ^
	// KC_US     = 0x1f, // ctrl + _
	KC_DEL    = 0x7f, // ctrl + backspace
	// ASCII, printable characters
	KC_SPACE = ' ',
	KC_COMMA   = ',',  // '<',
	KC_DOT     = '.',  // '>',
	KC_SLASH   = '/',  // '?',
	KC_SCOLON  = ';',  // ':',
	KC_SQUOTE  = '\'', // '"',
	KC_LSQUARE = '[',  // '{',
	KC_RSQUARE = ']',  // '}',
	KC_BSLASH  = '\\', // '|',
	KC_A = 'A', // a
	KC_B = 'B', // b
	KC_C = 'C', // c
	KC_D = 'D', // d
	KC_E = 'E', // e
	KC_F = 'F', // f
	KC_G = 'G', // g
	KC_H = 'H', // h
	KC_I = 'I', // i
	KC_J = 'J', // j
	KC_K = 'K', // k
	KC_L = 'L', // l
	KC_M = 'M', // m
	KC_N = 'N', // n
	KC_O = 'O', // o
	KC_P = 'P', // p
	KC_Q = 'Q', // q
	KC_R = 'R', // r
	KC_S = 'S', // s
	KC_T = 'T', // t
	KC_U = 'U', // u
	KC_V = 'V', // v
	KC_W = 'W', // w
	KC_X = 'X', // x
	KC_Y = 'Y', // y
	KC_Z = 'Z', // z
	KC_BTICK = '`', // '~',
	KC_D0    = '0', // ')',
	KC_D1    = '1', // '!',
	KC_D2    = '2', // '@',
	KC_D3    = '3', // '#',
	KC_D4    = '4', // '$',
	KC_D5    = '5', // '%',
	KC_D6    = '6', // '^',
	KC_D7    = '7', // '&',
	KC_D8    = '8', // '*',
	KC_D9    = '9', // '(',
	KC_MINUS = '-', // '_',
	KC_EQUAL = '=', // '+',
	// non-ASCII, common
	KC_CAPS_LOCK = 0x80,
	KC_LSHIFT, KC_LCTRL, KC_LALT,
	KC_RSHIFT, KC_RCTRL, KC_RALT,
	KC_ARROW_LEFT, KC_ARROW_RIGHT, KC_ARROW_DOWN, KC_ARROW_UP,
	KC_INSERT, KC_PSCREEN,
	KC_PAGE_UP, KC_PAGE_DOWN,
	KC_HOME, KC_END,
	// non-ASCII, functional
	KC_F1,  KC_F2,  KC_F3,  KC_F4,  KC_F5,  KC_F6,  KC_F7,  KC_F8,  KC_F9,  KC_F10, KC_F11, KC_F12,
	KC_F13, KC_F14, KC_F15, KC_F16, KC_F17, KC_F18, KC_F19, KC_F20, KC_F21, KC_F22, KC_F23, KC_F24,
	// non-ASCII, numeric
	KC_NUM_LOCK,
	KC_NUM_ADD, KC_NUM_SUB,
	KC_NUM_MUL, KC_NUM_DIV,
	KC_NUM_DEC,
	KC_NUM0, KC_NUM1, KC_NUM2, KC_NUM3, KC_NUM4, KC_NUM5, KC_NUM6, KC_NUM7, KC_NUM8, KC_NUM9,
	KC_NUM_ENTER,
	//
	KC_ERROR,
};

enum Mouse_Code {
	MC_LEFT,
	MC_RIGHT,
	MC_MIDDLE,
	MC_X1,
	MC_X2,
};

enum Input_Type {
	IT_NONE = 0,
	IT_DOWN = (1 << 0),
	IT_TRNS = (1 << 1),
	// shorthands
	IT_DOWN_TRNS = IT_DOWN | IT_TRNS,
};

#endif
