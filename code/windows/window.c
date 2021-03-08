#include "code/memory.h"

#include "system_to_internal.h"
#include "graphics_library.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Windows.h>
#include <hidusage.h>

#define KEYBOARD_KEYS_MAX UINT8_MAX + 1
#define MOUSE_KEYS_MAX 8

struct Window {
	HWND handle;
	HDC private_context;
	uint32_t size_x, size_y;

	struct {
		uint8_t keys[KEYBOARD_KEYS_MAX];
		uint8_t prev[KEYBOARD_KEYS_MAX];
		// bool caps_lock, num_lock;
	} keyboard;

	struct {
		uint8_t keys[MOUSE_KEYS_MAX];
		uint8_t prev[MOUSE_KEYS_MAX];

		uint32_t display_x, display_y;
		uint32_t window_x, window_y;
		int32_t delta_x, delta_y;

		float wheel_x, wheel_y;
	} mouse;

	struct Graphics * graphics;
};

//
#include "code/platform_window.h"

static void platform_window_toggle_raw_input(struct Window * window, bool state);
struct Window * platform_window_init(void) {
	struct Window * window = MEMORY_ALLOCATE(struct Window);

	window->handle = CreateWindowExA(
		WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR,
		APPLICATION_CLASS_NAME, "game prototype",
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		HWND_DESKTOP, NULL, system_to_internal_get_module(), NULL
	);
	if (window->handle == NULL) { fprintf(stderr, "'CreateWindow' failed\n"); DEBUG_BREAK(); exit(1); }

	window->private_context = GetDC(window->handle);
	if (window->private_context == NULL) { fprintf(stderr, "'GetDC' failed\n"); DEBUG_BREAK(); exit(1); }

	BOOL prop_is_set = SetPropA(window->handle, APPLICATION_CLASS_NAME, window);
	if (!prop_is_set) { fprintf(stderr, "'SetProp' failed\n"); DEBUG_BREAK(); exit(1); }

	RECT rect;
	GetClientRect(window->handle, &rect);
	window->size_x = (uint32_t)(rect.right - rect.left);
	window->size_y = (uint32_t)(rect.bottom - rect.top);

	memset(&window->keyboard, 0, sizeof(window->keyboard));
	memset(&window->mouse, 0, sizeof(window->mouse));

	// uint8_t keys[256];
	// if (GetKeyboardState(keys)) {
	// 	window->keyboard.caps_lock = keys[VK_CAPITAL] & 0x01;
	// 	window->keyboard.num_lock = keys[VK_NUMLOCK] & 0x01;
	// }

	window->graphics = graphics_init(window);

	// platform_window_toggle_raw_input(window, true);
	(void)platform_window_toggle_raw_input;

	return window;
}

void platform_window_free(struct Window * window) {
	if (window->handle != NULL) {
		DestroyWindow(window->handle);
		// delegate all the work to WM_DESTROY
	}
	else {
		MEMORY_FREE(window);
		// WM_CLOSE has been processed; now, just free the application window
	}
}

bool platform_window_exists(struct Window * window) {
	return window->handle != NULL;
}

void platform_window_update(struct Window * window) {
	// remap keyboard input ASCII characters
	static char const remap_src[] = ",./"    ";'"     "[]\\"    "`1234567890-=";
	static char const remap_dst[] = "<?>"    ":\""    "{}|"     "~!@#$%^&*()_+";

	for (uint8_t i = 0; i < sizeof(remap_src) / sizeof(*remap_src); i++) {
		window->keyboard.keys[(uint8_t)remap_dst[i]] = window->keyboard.keys[(uint8_t)remap_src[i]];
	}

	memcpy(window->keyboard.keys + (uint8_t)'A', window->keyboard.keys + (uint8_t)'a', sizeof(*window->keyboard.keys) * (1 + 'Z' - 'A'));

	// track input transitions
	memcpy(window->keyboard.prev, window->keyboard.keys, sizeof(window->keyboard.keys));
	memcpy(window->mouse.prev, window->mouse.keys, sizeof(window->mouse.keys));

	// reset per-frame data
	window->mouse.delta_x = 0;
	window->mouse.delta_y = 0;
	window->mouse.wheel_x = 0;
	window->mouse.wheel_y = 0;
}

int32_t platform_window_get_vsync(struct Window * window) {
	return graphics_get_vsync(window->graphics);
}

void platform_window_set_vsync(struct Window * window, int32_t value) {
	graphics_set_vsync(window->graphics, value);
}

void platform_window_display(struct Window * window) {
	graphics_display(window->graphics);
}

bool platform_window_key(struct Window * window, enum Key_Code key) {
	return window->keyboard.keys[key];
}

bool platform_window_key_transition(struct Window * window, enum Key_Code key, bool state) {
	uint8_t now = window->keyboard.keys[key];
	uint8_t was = window->keyboard.prev[key];
	return (now != was) && (now == state);
}

void platform_window_mouse_position_window(struct Window * window, uint32_t * x, uint32_t * y) {
	*x = window->mouse.window_x;
	*y = window->mouse.window_y;
}

void platform_window_mouse_position_display(struct Window * window, uint32_t * x, uint32_t * y) {
	*x = window->mouse.display_x;
	*y = window->mouse.display_y;
}

void platform_window_mouse_delta(struct Window * window, int32_t * x, int32_t * y) {
	*x = window->mouse.delta_x;
	*y = window->mouse.delta_y;
}

bool platform_window_mouse(struct Window * window, enum Mouse_Code key) {
	return window->mouse.keys[key];
}

bool platform_window_mouse_transition(struct Window * window, enum Mouse_Code key, bool state) {
	uint8_t now = window->mouse.keys[key];
	uint8_t was = window->mouse.prev[key];
	return (now != was) && (now == state);
}

void platform_window_get_size(struct Window * window, uint32_t * size_x, uint32_t * size_y) {
	*size_x = window->size_x;
	*size_y = window->size_y;
}

uint32_t platform_window_get_refresh_rate(struct Window * window, uint32_t default_value) {
	int value = GetDeviceCaps(window->private_context, VREFRESH);
	return value > 1 ? (uint32_t)value : default_value;
}

//
#include "window_to_system.h"

static LRESULT CALLBACK window_procedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

void window_to_system_init(void) {
	ATOM atom = RegisterClassExA(&(WNDCLASSEXA){
		.cbSize = sizeof(WNDCLASSEXA),
		.lpszClassName = APPLICATION_CLASS_NAME,
		.hInstance = system_to_internal_get_module(),
		.lpfnWndProc = window_procedure,
		.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW,
		.hCursor = LoadCursorA(0, IDC_ARROW),
	});
	if (atom == 0) { fprintf(stderr, "'RegisterClassExA' failed\n"); DEBUG_BREAK(); exit(1); }
	// https://docs.microsoft.com/en-us/windows/win32/winmsg/about-window-classes
	// https://docs.microsoft.com/en-us/windows/win32/gdi/private-display-device-contexts
}

void window_to_system_free(void) {
	UnregisterClassA(APPLICATION_CLASS_NAME, system_to_internal_get_module());
}

//
#include "window_to_glibrary.h"

HDC window_to_glibrary_get_private_device(struct Window * window) {
	return window->private_context;
}

//

static LRESULT handle_input_keyboard_virtual(struct Window * window, uint8_t key, bool is_down) {
	if ('A'        <= key && key <= 'Z')        { window->keyboard.keys['a'     + key - 'A']        = is_down; return 0; }
	if ('0'        <= key && key <= '9')        { window->keyboard.keys['0'     + key - '0']        = is_down; return 0; }
	if (VK_NUMPAD0 <= key && key <= VK_NUMPAD9) { window->keyboard.keys[KC_NUM0 + key - VK_NUMPAD0] = is_down; return 0; }
	if (VK_F1      <= key && key <= VK_F24)     { window->keyboard.keys[KC_F1   + key - VK_F1]      = is_down; return 0; }

	switch (key) {
		// ASCII, control characters
		case VK_BACK:   window->keyboard.keys['\b'] = is_down; break;
		case VK_TAB:    window->keyboard.keys['\t'] = is_down; break;
		case VK_RETURN: window->keyboard.keys['\r'] = is_down; break;
		case VK_ESCAPE: window->keyboard.keys[0x1b] = is_down; break;
		case VK_DELETE: window->keyboard.keys[0x7f] = is_down; break;
		// ASCII, printable characters
		case VK_SPACE:      window->keyboard.keys[' ']  = is_down; break;
		case VK_OEM_COMMA:  window->keyboard.keys[',']  = is_down; break;
		case VK_OEM_PERIOD: window->keyboard.keys['.']  = is_down; break;
		case VK_OEM_2:      window->keyboard.keys['/']  = is_down; break;
		case VK_OEM_1:      window->keyboard.keys[';']  = is_down; break;
		case VK_OEM_7:      window->keyboard.keys['\''] = is_down; break;
		case VK_OEM_4:      window->keyboard.keys['[']  = is_down; break;
		case VK_OEM_6:      window->keyboard.keys[']']  = is_down; break;
		case VK_OEM_5:      window->keyboard.keys['\\'] = is_down; break;
		case VK_OEM_3:      window->keyboard.keys['`']  = is_down; break;
		case VK_OEM_MINUS:  window->keyboard.keys['-']  = is_down; break;
		case VK_OEM_PLUS:   window->keyboard.keys['=']  = is_down; break;
		// non-ASCII, common
		case VK_CAPITAL:  window->keyboard.keys[KC_CAPS]        = is_down; break;
		case VK_SHIFT:    window->keyboard.keys[KC_SHIFT]       = is_down; break;
		case VK_CONTROL:  window->keyboard.keys[KC_CTRL]        = is_down; break;
		case VK_MENU:     window->keyboard.keys[KC_ALT]         = is_down; break;
		case VK_LEFT:     window->keyboard.keys[KC_ARROW_LEFT]  = is_down; break;
		case VK_RIGHT:    window->keyboard.keys[KC_ARROW_RIGHT] = is_down; break;
		case VK_DOWN:     window->keyboard.keys[KC_ARROW_DOWN]  = is_down; break;
		case VK_UP:       window->keyboard.keys[KC_ARROW_UP]    = is_down; break;
		case VK_INSERT:   window->keyboard.keys[KC_INSERT]      = is_down; break;
		case VK_SNAPSHOT: window->keyboard.keys[KC_PSCREEN]     = is_down; break;
		case VK_PRIOR:    window->keyboard.keys[KC_PAGE_UP]     = is_down; break;
		case VK_NEXT:     window->keyboard.keys[KC_PAGE_DOWN]   = is_down; break;
		case VK_HOME:     window->keyboard.keys[KC_HOME]        = is_down; break;
		case VK_END:      window->keyboard.keys[KC_END]         = is_down; break;
		// non-ASCII, numeric
		case VK_NUMLOCK:  window->keyboard.keys[KC_NUM_LOCK] = is_down; break;
		case VK_ADD:      window->keyboard.keys[KC_NUM_ADD]  = is_down; break;
		case VK_SUBTRACT: window->keyboard.keys[KC_NUM_SUB]  = is_down; break;
		case VK_MULTIPLY: window->keyboard.keys[KC_NUM_MUL]  = is_down; break;
		case VK_DIVIDE:   window->keyboard.keys[KC_NUM_DIV]  = is_down; break;
		case VK_DECIMAL:  window->keyboard.keys[KC_NUM_DEC]  = is_down; break;
	}

	// switch (key) {
	// 	// non-ASCII, common
	// 	case VK_CAPITAL: {
	// 		window->keyboard.caps_lock = GetKeyState(key) & 0x0001;
	// 		break;
	// 	}
	// 	// non-ASCII, numeric
	// 	case VK_NUMLOCK: {
	// 		window->keyboard.num_lock = GetKeyState(key) & 0x0001;
	// 		break;
	// 	}
	// }

	return 0;
}

static struct Window * raw_input_window = NULL;
static void platform_window_toggle_raw_input(struct Window * window, bool state) {
	static bool previous_state = false;

	if (previous_state == state) { return; }
	previous_state = state;

	USHORT flags = state ? 0 : RIDEV_REMOVE; // RIDEV_NOLEGACY seems to be tiresome
	HWND target = state ? window->handle : NULL;

	RAWINPUTDEVICE const devices[] = {
		(RAWINPUTDEVICE){.usUsagePage = HID_USAGE_PAGE_GENERIC, .usUsage = HID_USAGE_GENERIC_KEYBOARD, .dwFlags = flags, .hwndTarget = target},
		(RAWINPUTDEVICE){.usUsagePage = HID_USAGE_PAGE_GENERIC, .usUsage = HID_USAGE_GENERIC_MOUSE,    .dwFlags = flags, .hwndTarget = target},
		// (RAWINPUTDEVICE){.usUsagePage = HID_USAGE_PAGE_GENERIC, .usUsage = HID_USAGE_GENERIC_GAMEPAD,  .dwFlags = flags, .hwndTarget = target},
	};

	if (RegisterRawInputDevices(devices, sizeof(devices) / sizeof(*devices), sizeof(RAWINPUTDEVICE))) {
		raw_input_window = state ? window : NULL;
		return;
	}

	fprintf(stderr, "'RegisterRawInputDevices' failed\n"); DEBUG_BREAK();
	previous_state = !previous_state;

	// https://docs.microsoft.com/en-us/windows-hardware/drivers/hid/
}

static void handle_input_keyboard_raw(struct Window * window, RAWKEYBOARD * data) {
	if (raw_input_window != window) { return; }
	bool is_down = (data->Flags & RI_KEY_MAKE) == RI_KEY_MAKE;
	bool is_up = (data->Flags & RI_KEY_BREAK) == RI_KEY_BREAK;
	handle_input_keyboard_virtual(window, (uint8_t)data->VKey, is_down && !is_up);

	// https://docs.microsoft.com/windows/win32/api/winuser/ns-winuser-rawkeyboard
}

static void handle_input_mouse_raw(struct Window * window, RAWMOUSE * data) {
	if (raw_input_window != window) { return; }
	uint32_t const prev_display_x = window->mouse.display_x;
	uint32_t const prev_display_y = window->mouse.display_y;

	bool const is_virtual_desktop = (data->usFlags & MOUSE_VIRTUAL_DESKTOP) == MOUSE_VIRTUAL_DESKTOP;
	int const display_height = GetSystemMetrics(is_virtual_desktop ? SM_CYVIRTUALSCREEN : SM_CYSCREEN);
	int const display_width  = GetSystemMetrics(is_virtual_desktop ? SM_CXVIRTUALSCREEN : SM_CXSCREEN);

	POINT screen;
	int32_t delta_x, delta_y;
	if ((data->usFlags & MOUSE_MOVE_ABSOLUTE) == MOUSE_MOVE_ABSOLUTE) {
		screen = (POINT){
			.x = data->lLastX * display_width  / UINT16_MAX,
			.y = data->lLastY * display_height / UINT16_MAX,
		};

		delta_x = screen.x - (int32_t)prev_display_x;
		delta_y = (display_height - screen.y - 1) - (int32_t)prev_display_y;
	}
	else {
		GetCursorPos(&screen);
		delta_x = data->lLastX;
		delta_y = -data->lLastY;
	}

	POINT client = screen;
	ScreenToClient(window->handle, &client);

	//
	window->mouse.display_x = (uint32_t)screen.x;
	window->mouse.display_y = (uint32_t)(display_height - screen.y - 1);

	window->mouse.window_x = (uint32_t)(client.x);
	window->mouse.window_y = (uint32_t)((int32_t)window->size_y - client.y - 1);

	window->mouse.delta_x += (int32_t)window->mouse.display_x - (int32_t)prev_display_x;
	window->mouse.delta_y += (int32_t)window->mouse.display_y - (int32_t)prev_display_y;

	//
	if ((data->usButtonFlags & RI_MOUSE_HWHEEL) == RI_MOUSE_HWHEEL) {
		window->mouse.wheel_x += (float)(short)data->usButtonData / WHEEL_DELTA;
	}

	if ((data->usButtonFlags & RI_MOUSE_WHEEL) == RI_MOUSE_WHEEL) {
		window->mouse.wheel_y += (float)(short)data->usButtonData / WHEEL_DELTA;
	}

	//
	static USHORT const keys_down[MOUSE_KEYS_MAX] = {
		RI_MOUSE_LEFT_BUTTON_DOWN,
		RI_MOUSE_RIGHT_BUTTON_DOWN,
		RI_MOUSE_MIDDLE_BUTTON_DOWN,
		RI_MOUSE_BUTTON_4_DOWN,
		RI_MOUSE_BUTTON_5_DOWN,
	};

	static USHORT const keys_up[MOUSE_KEYS_MAX] = {
		RI_MOUSE_LEFT_BUTTON_UP,
		RI_MOUSE_RIGHT_BUTTON_UP,
		RI_MOUSE_MIDDLE_BUTTON_UP,
		RI_MOUSE_BUTTON_4_UP,
		RI_MOUSE_BUTTON_5_UP,
	};

	for (uint8_t i = 0; i < MOUSE_KEYS_MAX; i++) {
		bool is_down = (data->usButtonFlags & keys_down[i]) == keys_down[i];
		bool is_up   = (data->usButtonFlags & keys_up[i])   == keys_up[i];
		window->mouse.keys[i] = is_down && !is_up;
	}

	// https://docs.microsoft.com/windows/win32/api/winuser/ns-winuser-rawmouse
}

// static void handle_input_hid_raw(struct Window * window, RAWHID * data) {
// 	if (raw_input_window != window) { return; }
// 	(void)window; (void)data;
// 	fprintf(stderr, "'RAWHID' input is not implemented\n"); DEBUG_BREAK();
// 
// 	// https://docs.microsoft.com/windows/win32/api/winuser/ns-winuser-rawhid
// }

static LRESULT handle_message_input_raw(struct Window * window, WPARAM wParam, LPARAM lParam) {
	switch (GET_RAWINPUT_CODE_WPARAM(wParam)) {
		case RIM_INPUT: break;
		case RIM_INPUTSINK: break;
	}

	RAWINPUTHEADER header;
	UINT header_size = sizeof(header);
	if (GetRawInputData((HRAWINPUT)lParam, RID_HEADER, &header, &header_size, sizeof(RAWINPUTHEADER)) == (UINT)-1) { return 0; }

	RAWINPUT * input = MEMORY_ALLOCATE_SIZE(header.dwSize);

	UINT input_size = header.dwSize;
	if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, input, &input_size, sizeof(RAWINPUTHEADER)) == header.dwSize) {
		switch (input->header.dwType) {
			case RIM_TYPEKEYBOARD: handle_input_keyboard_raw(window, &input->data.keyboard); break;
			case RIM_TYPEMOUSE:    handle_input_mouse_raw(window, &input->data.mouse); break;
			// case RIM_TYPEHID:      handle_input_hid_raw(window, &input->data.hid); break;
		}
	}

	MEMORY_FREE(input);

	return 0;
	// https://docs.microsoft.com/en-us/windows/win32/inputdev/raw-input
}

static LRESULT handle_message_input_keyboard(struct Window * window, WPARAM wParam, LPARAM lParam) {
	if (raw_input_window == window) { return 0; }
	WORD flag = HIWORD(lParam);
	bool is_up = (flag & KF_UP) == KF_UP;
	return handle_input_keyboard_virtual(window, (uint8_t)wParam, !is_up);
	// https://docs.microsoft.com/en-us/windows/win32/inputdev/keyboard-input
}

static LRESULT handle_message_input_mouse(struct Window * window, WPARAM wParam, LPARAM lParam, bool client_space, float wheel_mask_x, float wheel_mask_y) {
	if (raw_input_window == window) { return 0; }
	uint32_t const prev_display_x = window->mouse.display_x;
	uint32_t const prev_display_y = window->mouse.display_y;

	int const display_height = GetSystemMetrics(SM_CYSCREEN);

	//
	POINTS points = MAKEPOINTS(lParam);
	POINT screen, client;
	if (client_space) {
		client = (POINT){.x = points.x, .y = points.y};
		screen = client;
		ClientToScreen(window->handle, &screen);
	}
	else {
		screen = (POINT){.x = points.x, .y = points.y};
		client = screen;
		ScreenToClient(window->handle, &client);
	}

	//
	window->mouse.display_x = (uint32_t)screen.x;
	window->mouse.display_y = (uint32_t)(display_height - screen.y - 1);

	window->mouse.window_x = (uint32_t)(client.x);
	window->mouse.window_y = (uint32_t)((int32_t)window->size_y - client.y - 1);

	window->mouse.delta_x += (int32_t)window->mouse.display_x - (int32_t)prev_display_x;
	window->mouse.delta_y += (int32_t)window->mouse.display_y - (int32_t)prev_display_y;

	//
	float wheel_delta = (float)GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
	window->mouse.wheel_x += wheel_mask_x * wheel_delta;
	window->mouse.wheel_y += wheel_mask_y * wheel_delta;

	//
	static WPARAM const key_masks[MOUSE_KEYS_MAX] = {
		MK_LBUTTON,
		MK_RBUTTON,
		MK_MBUTTON,
		MK_XBUTTON1,
		MK_XBUTTON2,
	};

	for (uint8_t i = 0; i < MOUSE_KEYS_MAX; i++) {
		bool is_down = (wParam & key_masks[i]) == key_masks[i];
		window->mouse.keys[i] = is_down;
	}

	return 0;
	// https://docs.microsoft.com/en-us/windows/win32/inputdev/mouse-input
}

static LRESULT CALLBACK window_procedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	struct Window * window = GetPropA(hwnd, APPLICATION_CLASS_NAME);
	if (window == NULL) { return DefWindowProcA(hwnd, message, wParam, lParam); }

	switch (message) {
		case WM_INPUT:
			return handle_message_input_raw(window, wParam, lParam);

		case WM_SYSKEYUP:
		case WM_SYSKEYDOWN:
		case WM_KEYUP:
		case WM_KEYDOWN:
			return handle_message_input_keyboard(window, wParam, lParam);

		case WM_MOUSEMOVE:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP:
			return handle_message_input_mouse(window, wParam, lParam, true, 0, 0);

		case WM_MOUSEWHEEL:
			return handle_message_input_mouse(window, wParam, lParam, false, 0, 1);

		case WM_MOUSEHWHEEL:
			return handle_message_input_mouse(window, wParam, lParam, false, 1, 0);

		case WM_SIZE: {
			window->size_x = (uint32_t)LOWORD(lParam);
			window->size_y = (uint32_t)HIWORD(lParam);
			// graphics_size(window->graphics, window->size_x, window->size_y);
			return 0;
		}

		case WM_KILLFOCUS: {
			memset(&window->keyboard, 0, sizeof(window->keyboard));
			memset(&window->mouse, 0, sizeof(window->mouse));
			return 0;
		}

		case WM_CLOSE: {
			window->handle = NULL;
			DestroyWindow(hwnd);
			return 0;
			// OS window is being closed through the OS API; we clear the handle reference
			// in order to prevent WM_DESTROY freeing the application window
		}

		case WM_DESTROY: {
			bool should_free = window->handle != NULL;
			RemovePropA(hwnd, APPLICATION_CLASS_NAME);
			graphics_free(window->graphics);
			if (raw_input_window == window) {
				platform_window_toggle_raw_input(window, false);
			}
			memset(window, 0, sizeof(*window));
			if (should_free) { MEMORY_FREE(window); }
			return 0;
		}
	}

	return DefWindowProcA(hwnd, message, wParam, lParam);
}

#undef APPLICATION_CLASS_NAME
