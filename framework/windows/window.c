#include "framework/memory.h"
#include "framework/internal/input_to_platform.h"

#include "system_to_internal.h"
#include "graphics_library.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Windows.h>
#include <hidusage.h>

struct Window {
	HWND handle;
	HDC private_context;
	uint32_t size_x, size_y;

	struct Graphics * graphics;
};

//
#include "framework/platform_window.h"

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

	input_to_platform_reset();

/*
	uint8_t keys[256];
	if (GetKeyboardState(keys)) {
		window->keyboard.caps_lock = keys[VK_CAPITAL] & 0x01;
		window->keyboard.num_lock = keys[VK_NUMLOCK] & 0x01;
	}
*/

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

int32_t platform_window_get_vsync(struct Window * window) {
	return graphics_get_vsync(window->graphics);
}

void platform_window_set_vsync(struct Window * window, int32_t value) {
	graphics_set_vsync(window->graphics, value);
}

void platform_window_display(struct Window * window) {
	graphics_display(window->graphics);
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
static enum Key_Code translate_virtual_key_to_application(uint8_t key) {
	if ('A'        <= key && key <= 'Z')        { return 'a'     + key - 'A'; }
	if ('0'        <= key && key <= '9')        { return '0'     + key - '0'; }
	if (VK_NUMPAD0 <= key && key <= VK_NUMPAD9) { return KC_NUM0 + key - VK_NUMPAD0; }
	if (VK_F1      <= key && key <= VK_F24)     { return KC_F1   + key - VK_F1; }

	switch (key) {
		// ASCII, control characters
		case VK_BACK:   return '\b';
		case VK_TAB:    return '\t';
		case VK_RETURN: return '\r';
		case VK_ESCAPE: return 0x1b;
		case VK_DELETE: return 0x7f;
		// ASCII, printable characters
		case VK_SPACE:      return ' ';
		case VK_OEM_COMMA:  return ',';
		case VK_OEM_PERIOD: return '.';
		case VK_OEM_2:      return '/';
		case VK_OEM_1:      return ';';
		case VK_OEM_7:      return '\'';
		case VK_OEM_4:      return '[';
		case VK_OEM_6:      return ']';
		case VK_OEM_5:      return '\\';
		case VK_OEM_3:      return '`';
		case VK_OEM_MINUS:  return '-';
		case VK_OEM_PLUS:   return '=';
		// non-ASCII, common
		case VK_CAPITAL:  return KC_CAPS;
		case VK_SHIFT:    return KC_SHIFT;
		case VK_LSHIFT:   return KC_LSHIFT;
		case VK_RSHIFT:   return KC_RSHIFT;
		case VK_CONTROL:  return KC_CTRL;
		case VK_LCONTROL: return KC_LCTRL;
		case VK_RCONTROL: return KC_RCTRL;
		case VK_MENU:     return KC_ALT;
		case VK_LMENU:    return KC_LALT;
		case VK_RMENU:    return KC_RALT;
		case VK_LEFT:     return KC_ARROW_LEFT;
		case VK_RIGHT:    return KC_ARROW_RIGHT;
		case VK_DOWN:     return KC_ARROW_DOWN;
		case VK_UP:       return KC_ARROW_UP;
		case VK_INSERT:   return KC_INSERT;
		case VK_SNAPSHOT: return KC_PSCREEN;
		case VK_PRIOR:    return KC_PAGE_UP;
		case VK_NEXT:     return KC_PAGE_DOWN;
		case VK_HOME:     return KC_HOME;
		case VK_END:      return KC_END;
		// non-ASCII, numeric
		case VK_NUMLOCK:  return KC_NUM_LOCK;
		case VK_ADD:      return KC_NUM_ADD;
		case VK_SUBTRACT: return KC_NUM_SUB;
		case VK_MULTIPLY: return KC_NUM_MUL;
		case VK_DIVIDE:   return KC_NUM_DIV;
		case VK_DECIMAL:  return KC_NUM_DEC;
	}

	return KC_ERROR;

/*
	switch (key) {
		// non-ASCII, common
		case VK_CAPITAL: {
			window->keyboard.caps_lock = GetKeyState(key) & 0x0001;
			break;
		}
		// non-ASCII, numeric
		case VK_NUMLOCK: {
			window->keyboard.num_lock = GetKeyState(key) & 0x0001;
			break;
		}
	}
*/
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
	// bool is_e0 = (data->Flags & RI_KEY_E0) == RI_KEY_E0;
	// bool is_e1 = (data->Flags & RI_KEY_E1) == RI_KEY_E1;
	input_to_platform_on_key_down(
		translate_virtual_key_to_application((uint8_t)data->VKey),
		(data->Flags & RI_KEY_BREAK) != RI_KEY_BREAK
	);

	// https://docs.microsoft.com/windows/win32/api/winuser/ns-winuser-rawkeyboard
}

static void handle_input_mouse_raw(struct Window * window, RAWMOUSE * data) {
	if (raw_input_window != window) { return; }

	bool const is_virtual_desktop = (data->usFlags & MOUSE_VIRTUAL_DESKTOP) == MOUSE_VIRTUAL_DESKTOP;
	int const display_height = GetSystemMetrics(is_virtual_desktop ? SM_CYVIRTUALSCREEN : SM_CYSCREEN);
	int const display_width  = GetSystemMetrics(is_virtual_desktop ? SM_CXVIRTUALSCREEN : SM_CXSCREEN);

	POINT screen;
	if ((data->usFlags & MOUSE_MOVE_ABSOLUTE) == MOUSE_MOVE_ABSOLUTE) {
		screen = (POINT){
			.x = data->lLastX * display_width  / UINT16_MAX,
			.y = data->lLastY * display_height / UINT16_MAX,
		};
	}
	else {
		GetCursorPos(&screen);
		input_to_platform_on_mouse_delta(data->lLastX, -data->lLastY);
	}

	POINT client = screen;
	ScreenToClient(window->handle, &client);

	//
	input_to_platform_on_mouse_move((uint32_t)screen.x, (uint32_t)(display_height - screen.y - 1));
	input_to_platform_on_mouse_move_window((uint32_t)client.x, window->size_y - (uint32_t)client.y - 1);

	//
	if ((data->usButtonFlags & RI_MOUSE_HWHEEL) == RI_MOUSE_HWHEEL) {
		input_to_platform_on_mouse_wheel((float)(short)data->usButtonData / WHEEL_DELTA, 0);
	}

	if ((data->usButtonFlags & RI_MOUSE_WHEEL) == RI_MOUSE_WHEEL) {
		input_to_platform_on_mouse_wheel(0, (float)(short)data->usButtonData / WHEEL_DELTA);
	}

	//
	// static USHORT const keys_down[MOUSE_KEYS_MAX] = {
	// 	RI_MOUSE_LEFT_BUTTON_DOWN,
	// 	RI_MOUSE_RIGHT_BUTTON_DOWN,
	// 	RI_MOUSE_MIDDLE_BUTTON_DOWN,
	// 	RI_MOUSE_BUTTON_4_DOWN,
	// 	RI_MOUSE_BUTTON_5_DOWN,
	// };

	static USHORT const keys_up[] = {
		RI_MOUSE_LEFT_BUTTON_UP,
		RI_MOUSE_RIGHT_BUTTON_UP,
		RI_MOUSE_MIDDLE_BUTTON_UP,
		RI_MOUSE_BUTTON_4_UP,
		RI_MOUSE_BUTTON_5_UP,
	};

	for (uint8_t i = 0; i < sizeof(keys_up) / sizeof(*keys_up); i++) {
		// bool is_down = (data->usButtonFlags & keys_down[i]) == keys_down[i];
		input_to_platform_on_mouse_down(i, (data->usButtonFlags & keys_up[i]) != keys_up[i]);
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

	// DWORD repeat_count = LOWORD(lParam);
	DWORD flags = HIWORD(lParam);

	switch (wParam) {
		case VK_SHIFT:   wParam = MapVirtualKeyA(LOBYTE(flags), MAPVK_VSC_TO_VK_EX); break;
		case VK_CONTROL: wParam = ((flags & KF_EXTENDED) == KF_EXTENDED) ? VK_RCONTROL : VK_LCONTROL; break;
		case VK_MENU:    wParam = ((flags & KF_EXTENDED) == KF_EXTENDED) ? VK_RMENU : VK_LMENU; break;
	}

	// bool alt_down = (flags & KF_ALTDOWN) == KF_ALTDOWN;
	// bool was_down = (flags & KF_REPEAT) == KF_REPEAT;

	input_to_platform_on_key_down(
		translate_virtual_key_to_application((uint8_t)wParam),
		(flags & KF_UP) != KF_UP
	);

	return 0;
	// https://docs.microsoft.com/en-us/windows/win32/inputdev/keyboard-input
}

static LRESULT handle_message_input_mouse(struct Window * window, WPARAM wParam, LPARAM lParam, bool client_space, float wheel_mask_x, float wheel_mask_y) {
	if (raw_input_window == window) { return 0; }

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
	input_to_platform_on_mouse_move((uint32_t)screen.x, (uint32_t)(display_height - screen.y - 1));
	input_to_platform_on_mouse_move_window((uint32_t)client.x, window->size_y - (uint32_t)client.y - 1);

	//
	float const wheel_delta = (float)GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
	input_to_platform_on_mouse_wheel(wheel_mask_x * wheel_delta, wheel_mask_y * wheel_delta);

	//
	static WPARAM const key_masks[] = {
		MK_LBUTTON,
		MK_RBUTTON,
		MK_MBUTTON,
		MK_XBUTTON1,
		MK_XBUTTON2,
	};

	for (uint8_t i = 0; i < sizeof(key_masks) / sizeof(*key_masks); i++) {
		input_to_platform_on_mouse_down(i, (wParam & key_masks[i]) == key_masks[i]);
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
			input_to_platform_reset();
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
			input_to_platform_reset();
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
