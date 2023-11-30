#include "framework/memory.h"
#include "framework/formatter.h"
#include "framework/gpu_context.h"
#include "framework/internal/input_to_window.h"

#include "internal/system.h"

#include <initguid.h> // `DEFINE_GUID`
#include <Windows.h>
#include <hidusage.h>
#include <malloc.h> // alloca

#define HANDLE_PROP_WINDOW_NAME "prop_window"

//
#include "framework/platform/window.h"

struct Window {
	struct Window_Config config;
	struct Window_Callbacks callbacks;
	//
	HWND handle;
	bool raw_input;
	// fullscreen
	WINDOWPLACEMENT pre_fullscreen_position;
	LONG_PTR pre_fullscreen_style;
	// transient
	HDC frame_cached_device;
	bool ignore_once_WM_KILLFOCUS;
	// command
	enum Window_Command {
		WINDOW_COMMAND_NONE,
		WINDOW_COMMAND_SIZE_LEFT,
		WINDOW_COMMAND_SIZE_RIGHT,
		WINDOW_COMMAND_SIZE_TOP,
		WINDOW_COMMAND_SIZE_TOPLEFT,
		WINDOW_COMMAND_SIZE_TOPRIGHT,
		WINDOW_COMMAND_SIZE_BOTTOM,
		WINDOW_COMMAND_SIZE_BOTTOMLEFT,
		WINDOW_COMMAND_SIZE_BOTTOMRIGHT,
		WINDOW_COMMAND_MOVE,
	} command;
	LONG command_offset_x, command_offset_y;
	LONG command_min_x, command_min_y;
};

static void platform_window_internal_set_raw_input(struct Window * window, bool state);
struct Window * platform_window_init(struct Window_Config config, struct Window_Callbacks callbacks) {
	// @note: initial styles are bound to have some implicit flags
	//        thus we strip those off using `SetWindowLongPtr` after the `CreateWindowEx`
	//        [!] initialize invisible, without `WS_VISIBLE` being set
	//        [!] initialize windowed with a title bar, i.e. `WS_CAPTION`
	DWORD target_style = WS_CLIPSIBLINGS | WS_CAPTION;
	DWORD const target_ex_style = WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR;

	if (config.settings & WINDOW_SETTINGS_MINIMIZE)  { target_style |= WS_MINIMIZEBOX; }
	if (config.settings & WINDOW_SETTINGS_MAXIMIZE)  { target_style |= WS_MAXIMIZEBOX; }
	if (config.settings & WINDOW_SETTINGS_RESIZABLE) { target_style |= WS_SIZEBOX; }

	if (config.settings & (WINDOW_SETTINGS_MINIMIZE | WINDOW_SETTINGS_MAXIMIZE | WINDOW_SETTINGS_RESIZABLE)) {
		target_style |= WS_SYSMENU;
	}

	RECT target_rect = {.right = (LONG)config.size_x, .bottom = (LONG)config.size_y};
	AdjustWindowRectExForDpi(
		&target_rect, target_style, FALSE, target_ex_style,
		GetDpiForSystem()
	);

	HWND const handle = CreateWindowEx(
		target_ex_style,
		TEXT(APPLICATION_CLASS_NAME), TEXT("game prototype"),
		target_style,
		CW_USEDEFAULT, CW_USEDEFAULT,
		target_rect.right - target_rect.left, target_rect.bottom - target_rect.top,
		HWND_DESKTOP, NULL, system_to_internal_get_module(), NULL
	);
	if (handle == NULL) { goto fail_handle; }

	// @note: supress OS-defaults, enforce external settings
	SetWindowLongPtr(handle, GWL_STYLE, target_style);
	SetWindowPos(
		handle, HWND_TOP,
		0, 0, 0, 0,
		SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE
	);

	struct Window * const window = MEMORY_ALLOCATE(struct Window);
	if (window == NULL) { goto fail_window; }
	if (!SetProp(handle, TEXT(HANDLE_PROP_WINDOW_NAME), window)) { goto fail_window; }

	*window = (struct Window){
		.config = config,
		.callbacks = callbacks,
		.handle = handle,
	};

	platform_window_internal_set_raw_input(window, true);
	ShowWindow(handle, SW_SHOW);
	return window;

	// process errors
	fail_window:
	if (window != NULL) { MEMORY_FREE(window); }
	else { ERR("failed to initialize application window"); }
	REPORT_CALLSTACK(); DEBUG_BREAK();

	fail_handle:
	if (handle != NULL) { DestroyWindow(handle); }
	else { ERR("failed to create platform window"); }
	REPORT_CALLSTACK(); DEBUG_BREAK();

	return NULL;
}

void platform_window_free(struct Window * window) {
	if (window->handle != NULL) {
		platform_window_internal_set_raw_input(window, false);
		DestroyWindow(window->handle);
		// delegate all the work to WM_DESTROY
	}
	else {
		MEMORY_FREE(window);
		// WM_CLOSE has been processed; now, just free the application window
	}
}

void platform_window_focus(struct Window const * window) {
	SetForegroundWindow(window->handle);
}

bool platform_window_exists(struct Window const * window) {
	return window->handle != NULL;
}

static void platform_window_internal_handle_moving(struct Window *window);
static void platform_window_internal_handle_sizing(struct Window *window);
void platform_window_update(struct Window * window) {
	platform_window_internal_handle_moving(window);
	platform_window_internal_handle_sizing(window);
}

void platform_window_start_frame(struct Window * window) {
	if (window->frame_cached_device != NULL) {
		REPORT_CALLSTACK(); DEBUG_BREAK(); return;
	}
	window->frame_cached_device = GetDC(window->handle);
}

void platform_window_draw_frame(struct Window * window) {
	(void)window;
}

void platform_window_end_frame(struct Window * window) {
	ReleaseDC(window->handle, window->frame_cached_device);
	window->frame_cached_device = NULL;
}

void * platform_window_get_cached_device(struct Window * window) {
	return window->frame_cached_device;
}

void platform_window_get_size(struct Window const * window, uint32_t * size_x, uint32_t * size_y) {
	RECT client_rect; GetClientRect(window->handle, &client_rect);
	*size_x = (uint32_t)(client_rect.right - client_rect.left);
	*size_y = (uint32_t)(client_rect.bottom - client_rect.top);
}

uint32_t platform_window_get_refresh_rate(struct Window const * window, uint32_t default_value) {
	if (window->frame_cached_device == NULL) {
		REPORT_CALLSTACK(); DEBUG_BREAK(); return 0;
	}

	int value = GetDeviceCaps(window->frame_cached_device, VREFRESH);
	return value > 1 ? (uint32_t)value : default_value;
}

static void platform_window_internal_toggle_borderless_fullscreen(struct Window * window);
void platform_window_toggle_borderless_fullscreen(struct Window * window) {
	window->ignore_once_WM_KILLFOCUS = true;
	ShowWindow(window->handle, SW_HIDE);
	platform_window_internal_toggle_borderless_fullscreen(window);
	ShowWindow(window->handle, SW_SHOW);
}

//
#include "internal/window_to_system.h"

static LRESULT CALLBACK window_procedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

bool window_to_system_init(void) {
	HINSTANCE const module = GetModuleHandle(NULL);
	return RegisterClassEx(&(WNDCLASSEX){
		.cbSize = sizeof(WNDCLASSEX),
		.lpszClassName = TEXT(APPLICATION_CLASS_NAME),
		.hInstance = system_to_internal_get_module(),
		.lpfnWndProc = window_procedure,
		.style = CS_HREDRAW | CS_VREDRAW,
		.hCursor = LoadCursor(0, IDC_ARROW),
		.hIcon = LoadIconA(module, "ico"),
		.hIconSm = LoadIconA(module, "ico"),
	}) != 0;

	// https://learn.microsoft.com/windows/win32/winmsg/about-window-classes
	// https://learn.microsoft.com/windows/win32/gdi/private-display-device-contexts
}

void window_to_system_free(void) {
	UnregisterClass(TEXT(APPLICATION_CLASS_NAME), system_to_internal_get_module());
}

//

static void platform_window_internal_syscommand_move(struct Window * window, uint8_t mode) {
	if (window->command != WINDOW_COMMAND_NONE) { return; }
	if (mode != HTCAPTION) { return; }
	window->command = WINDOW_COMMAND_MOVE;

	POINT pos; GetCursorPos(&pos);
	RECT rect; GetWindowRect(window->handle, &rect);

	window->command_offset_x = (rect.left - pos.x);
	window->command_offset_y = (rect.top - pos.y);
}

static void platform_window_internal_syscommand_size(struct Window * window, uint8_t mode) {
	if (window->command != WINDOW_COMMAND_NONE) { return; }
	switch (mode) {
		case WMSZ_LEFT:        window->command = WINDOW_COMMAND_SIZE_LEFT; break;
		case WMSZ_RIGHT:       window->command = WINDOW_COMMAND_SIZE_RIGHT; break;
		case WMSZ_TOP:         window->command = WINDOW_COMMAND_SIZE_TOP; break;
		case WMSZ_TOPLEFT:     window->command = WINDOW_COMMAND_SIZE_TOPLEFT; break;
		case WMSZ_TOPRIGHT:    window->command = WINDOW_COMMAND_SIZE_TOPRIGHT; break;
		case WMSZ_BOTTOM:      window->command = WINDOW_COMMAND_SIZE_BOTTOM; break;
		case WMSZ_BOTTOMLEFT:  window->command = WINDOW_COMMAND_SIZE_BOTTOMLEFT; break;
		case WMSZ_BOTTOMRIGHT: window->command = WINDOW_COMMAND_SIZE_BOTTOMRIGHT; break;
		default: return;
	}

	POINT pos; GetCursorPos(&pos);
	RECT rect; GetWindowRect(window->handle, &rect);

	switch (mode) {
		case WMSZ_LEFT:
		case WMSZ_TOPLEFT:
		case WMSZ_BOTTOMLEFT:
			window->command_offset_x = (rect.left - pos.x); break;
		case WMSZ_RIGHT:
		case WMSZ_TOPRIGHT:
		case WMSZ_BOTTOMRIGHT:
			window->command_offset_x = (rect.right - pos.x); break;
	}

	switch (mode) {
		case WMSZ_TOP:
		case WMSZ_TOPLEFT:
		case WMSZ_TOPRIGHT:
			window->command_offset_y = (rect.top - pos.y); break;
		case WMSZ_BOTTOM:
		case WMSZ_BOTTOMLEFT:
		case WMSZ_BOTTOMRIGHT:
			window->command_offset_y = (rect.bottom - pos.y); break;
	}
}

static void platform_window_internal_handle_moving(struct Window * window) {
	if (window->command != WINDOW_COMMAND_MOVE) { return; }
	if (!(GetKeyState(VK_LBUTTON) & 0x80)) { // !input_mouse(MC_LEFT)
		window->command = WINDOW_COMMAND_NONE;
	}

	POINT pos; GetCursorPos(&pos);
	RECT rect; GetWindowRect(window->handle, &rect);

	pos.x += window->command_offset_x;
	pos.y += window->command_offset_y;

	MoveWindow(
		window->handle,
		pos.x, pos.y,
		rect.right - rect.left,
		rect.bottom - rect.top,
		FALSE
	);
}

static void platform_window_internal_handle_sizing(struct Window * window) {
	if (window->command < WINDOW_COMMAND_SIZE_LEFT) { return; }
	if (window->command > WINDOW_COMMAND_SIZE_BOTTOMRIGHT) { return; }
	if (!(GetKeyState(VK_LBUTTON) & 0x80)) { // !input_mouse(MC_LEFT)
		window->command = WINDOW_COMMAND_NONE;
	}

	POINT pos; GetCursorPos(&pos);
	RECT rect; GetWindowRect(window->handle, &rect);

	pos.x += window->command_offset_x;
	pos.y += window->command_offset_y;

	switch (window->command) {
		default: break;
		case WINDOW_COMMAND_SIZE_LEFT:
		case WINDOW_COMMAND_SIZE_TOPLEFT:
		case WINDOW_COMMAND_SIZE_BOTTOMLEFT:
			rect.left = (pos.x + window->command_min_x <= rect.right)
				? pos.x : rect.right - window->command_min_x;
			break;
		case WINDOW_COMMAND_SIZE_RIGHT:
		case WINDOW_COMMAND_SIZE_TOPRIGHT:
		case WINDOW_COMMAND_SIZE_BOTTOMRIGHT:
			rect.right = (pos.x - window->command_min_x >= rect.left)
				? pos.x : rect.left + window->command_min_x;
			break;
	}

	switch (window->command) {
		default: break;
		case WINDOW_COMMAND_SIZE_TOP:
		case WINDOW_COMMAND_SIZE_TOPLEFT:
		case WINDOW_COMMAND_SIZE_TOPRIGHT:
			rect.top = (pos.y + window->command_min_y <= rect.bottom)
				? pos.y : rect.bottom - window->command_min_y;
			break;
		case WINDOW_COMMAND_SIZE_BOTTOM:
		case WINDOW_COMMAND_SIZE_BOTTOMLEFT:
		case WINDOW_COMMAND_SIZE_BOTTOMRIGHT:
			rect.bottom = (pos.y - window->command_min_y >= rect.top)
				? pos.y : rect.top + window->command_min_y;
			break;
	}

	MoveWindow(
		window->handle,
		rect.left, rect.top,
		rect.right - rect.left,
		rect.bottom - rect.top,
		FALSE
	);
}

static void platform_window_internal_toggle_borderless_fullscreen(struct Window * window) {
	LONG_PTR const window_style = GetWindowLongPtr(window->handle, GWL_STYLE);

	if (window_style & WS_CAPTION) {
		MONITORINFO monitor_info = {.cbSize = sizeof(MONITORINFO)};
		if (!GetMonitorInfo(MonitorFromWindow(window->handle, MONITOR_DEFAULTTOPRIMARY), &monitor_info)) { return; }

		if (!GetWindowPlacement(window->handle, &window->pre_fullscreen_position)) { return; }
		window->pre_fullscreen_style = window_style;

		// set borderless fullscreen mode
		SetWindowLongPtr(window->handle, GWL_STYLE, WS_CLIPSIBLINGS | (window_style & WS_VISIBLE));
		SetWindowPos(
			window->handle, HWND_TOP,
			monitor_info.rcMonitor.left, monitor_info.rcMonitor.top,
			monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
			monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
			SWP_FRAMECHANGED | SWP_NOZORDER
		);
		return;
	}

	// restore windowed mode
	SetWindowLongPtr(window->handle, GWL_STYLE, window->pre_fullscreen_style);
	SetWindowPos(
		window->handle, HWND_TOP,
		0, 0, 0, 0,
		SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE
	);
	SetWindowPlacement(window->handle, &window->pre_fullscreen_position);
}

static uint8_t fix_virtual_key(uint8_t key, enum Scan_Code scan) {
	return (key == VK_SHIFT)
		? (uint8_t)MapVirtualKey(scan, MAPVK_VSC_TO_VK_EX)
		: key;
}

static enum Key_Code translate_virtual_key(uint8_t key, bool is_extended) {
	static enum Key_Code const LUT_normal[] = {
		// letters
		['A']                 = 'A',
		['B']                 = 'B',
		['C']                 = 'C',
		['D']                 = 'D',
		['E']                 = 'E',
		['F']                 = 'F',
		['G']                 = 'G',
		['H']                 = 'H',
		['I']                 = 'I',
		['J']                 = 'J',
		['K']                 = 'K',
		['L']                 = 'L',
		['M']                 = 'M',
		['N']                 = 'N',
		['O']                 = 'O',
		['P']                 = 'P',
		['Q']                 = 'Q',
		['R']                 = 'R',
		['S']                 = 'S',
		['T']                 = 'T',
		['U']                 = 'U',
		['V']                 = 'V',
		['W']                 = 'W',
		['X']                 = 'X',
		['Y']                 = 'Y',
		['Z']                 = 'Z',
		// number and backslash
		['1']                 = '1',
		['2']                 = '2',
		['3']                 = '3',
		['4']                 = '4',
		['5']                 = '5',
		['6']                 = '6',
		['7']                 = '7',
		['8']                 = '8',
		['9']                 = '9',
		['0']                 = '0',
		[VK_OEM_MINUS]        = '-',
		[VK_OEM_PLUS]         = '=',
		['\b']                = '\b',
		// near letters
		['\r']                = '\r',
		[VK_ESCAPE]           = KC_ESCAPE,
		[VK_TAB]              = '\t',
		[' ']                 = ' ',
		[VK_OEM_4]            = '[',
		[VK_OEM_6]            = ']',
		[VK_OEM_5]            = '\\',
		[VK_OEM_1]            = ';',
		[VK_OEM_7]            = '\'',
		[VK_OEM_3]            = '`',
		[VK_OEM_COMMA]        = ',',
		[VK_OEM_PERIOD]       = '.',
		[VK_OEM_2]            = '/',
		// modificators
		[VK_CAPITAL]          = KC_CAPS,
		[VK_CONTROL]          = KC_LCTRL,
		[VK_LSHIFT]           = KC_LSHIFT,
		[VK_RSHIFT]           = KC_RSHIFT,
		[VK_MENU]             = KC_LALT,
		// functional
		[VK_F1]               = KC_F1,
		[VK_F2]               = KC_F2,
		[VK_F3]               = KC_F3,
		[VK_F4]               = KC_F4,
		[VK_F5]               = KC_F5,
		[VK_F6]               = KC_F6,
		[VK_F7]               = KC_F7,
		[VK_F8]               = KC_F8,
		[VK_F9]               = KC_F9,
		[VK_F10]              = KC_F10,
		[VK_F11]              = KC_F11,
		[VK_F12]              = KC_F12,
		[VK_F13]              = KC_F13,
		[VK_F14]              = KC_F14,
		[VK_F15]              = KC_F15,
		[VK_F16]              = KC_F16,
		[VK_F17]              = KC_F17,
		[VK_F18]              = KC_F18,
		[VK_F19]              = KC_F19,
		[VK_F20]              = KC_F20,
		[VK_F21]              = KC_F21,
		[VK_F22]              = KC_F22,
		[VK_F23]              = KC_F23,
		[VK_F24]              = KC_F24,
		// numpad
		[VK_NUMLOCK]          = KC_NUM_LOCK,
		[VK_MULTIPLY]         = KC_NUM_STAR,
		[VK_SUBTRACT]         = KC_NUM_DASH,
		[VK_ADD]              = KC_NUM_PLUS,
		[VK_NUMPAD1]          = KC_NUM1,
		[VK_NUMPAD2]          = KC_NUM2,
		[VK_NUMPAD3]          = KC_NUM3,
		[VK_NUMPAD4]          = KC_NUM4,
		[VK_NUMPAD5]          = KC_NUM5,
		[VK_NUMPAD6]          = KC_NUM6,
		[VK_NUMPAD7]          = KC_NUM7,
		[VK_NUMPAD8]          = KC_NUM8,
		[VK_NUMPAD9]          = KC_NUM9,
		[VK_NUMPAD0]          = KC_NUM0,
		[VK_DECIMAL]          = KC_NUM_PERIOD,
		//
		[0xff] = 0,
	};

	static enum Key_Code const LUT_extended[] = {
		// near letters
		[VK_DELETE]           = KC_DELETE,
		// modificators
		[VK_CONTROL]          = KC_RCTRL,
		[VK_MENU]             = KC_RALT,
		[VK_LWIN]             = KC_LGUI,
		[VK_RWIN]             = KC_RGUI,
		// navigation block
		[VK_INSERT]           = KC_INSERT,
		[VK_HOME]             = KC_HOME,
		[VK_PRIOR]            = KC_PAGE_UP,
		[VK_END]              = KC_END,
		[VK_NEXT]             = KC_PAGE_DOWN,
		// arrows
		[VK_RIGHT]            = KC_ARROW_RIGHT,
		[VK_LEFT]             = KC_ARROW_LEFT,
		[VK_DOWN]             = KC_ARROW_DOWN,
		[VK_UP]               = KC_PAGE_UP,
		// numpad
		[VK_DIVIDE]           = KC_NUM_SLASH,
		[VK_RETURN]           = KC_NUM_ENTER,
		//
		[0xff] = 0,
	};

	return is_extended
		? LUT_extended[key]
		: LUT_normal[key];
}

static bool platform_window_internal_has_raw_input(struct Window * window) {
	UINT count;
	GetRegisteredRawInputDevices(NULL, &count, sizeof(RAWINPUTDEVICE));

	// @todo: arena/stack allocator (?)
	RAWINPUTDEVICE * devices = alloca(sizeof(RAWINPUTDEVICE) * count);
	if (GetRegisteredRawInputDevices(devices, &count, sizeof(RAWINPUTDEVICE)) != (UINT)-1) {
		for (uint32_t i = 0; i < count; i++) {
			if (devices[i].hwndTarget == window->handle) {
				return true;
			}
		}
	}

	return false;
}

static void platform_window_internal_set_raw_input(struct Window * window, bool state) {
	if (platform_window_internal_has_raw_input(window) == state) { return; }

	// `RIDEV_NOLEGACY` is tiresome:
	// - for keyboard it removes crucial `WM_CHAR`
	// - for mouse it removes windowed interactions

	USHORT const flags = state ? /*RIDEV_INPUTSINK*/ 0 : RIDEV_REMOVE;
	HWND const target = state ? window->handle : NULL;

	RAWINPUTDEVICE const devices[] = {
		{.usUsagePage = HID_USAGE_PAGE_GENERIC, .usUsage = HID_USAGE_GENERIC_KEYBOARD, .dwFlags = flags, .hwndTarget = target},
		{.usUsagePage = HID_USAGE_PAGE_GENERIC, .usUsage = HID_USAGE_GENERIC_MOUSE,    .dwFlags = flags, .hwndTarget = target},
		{.usUsagePage = HID_USAGE_PAGE_GENERIC, .usUsage = HID_USAGE_GENERIC_GAMEPAD,  .dwFlags = flags, .hwndTarget = target},
	};

	if (!RegisterRawInputDevices(devices, SIZE_OF_ARRAY(devices), sizeof(RAWINPUTDEVICE))) {
		ERR("'RegisterRawInputDevices' failed");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return;
	}

	window->raw_input = state;
	// https://learn.microsoft.com/windows-hardware/drivers/hid/
}

static void handle_input_keyboard_raw(struct Window * window, RAWKEYBOARD * data) {
	if (!window->raw_input) { return; }

	bool const is_extended = (data->Flags & RI_KEY_E0) == RI_KEY_E0;

	enum Scan_Code const scan = (data->Flags & RI_KEY_E1)
		? (enum Scan_Code)MapVirtualKey(data->VKey, MAPVK_VK_TO_VSC)
		: data->MakeCode | (is_extended ? 0xe000 : 0x0000);

	uint8_t const key = fix_virtual_key((uint8_t)data->VKey, scan);
	if (key == 0x00) { REPORT_CALLSTACK(); DEBUG_BREAK(); return; }
	if (key == 0xff) { REPORT_CALLSTACK(); DEBUG_BREAK(); return; }

	input_to_platform_on_key(
		translate_virtual_key(key, is_extended),
		scan,
		(data->Flags & RI_KEY_BREAK) == 0
	);

	// https://learn.microsoft.com/windows/win32/inputdev/about-keyboard-input
	// https://learn.microsoft.com/windows/win32/api/winuser/ns-winuser-rawkeyboard
	// https://blog.molecular-matters.com/2011/09/05/properly-handling-keyboard-input/

/*
	char key_name[32];
	GetKeyNameText((LONG)((scan << 16) | (is_extended << 24)), key_name, sizeof(key_name));
	TRC("%s", key_name);
*/
}

static void handle_input_mouse_raw(struct Window * window, RAWMOUSE * data) {
	if (!window->raw_input) { return; }

	uint32_t size_x, size_y;
	platform_window_get_size(window, &size_x, &size_y);

	bool const is_virtual_desktop = (data->usFlags & MOUSE_VIRTUAL_DESKTOP);
	int const display_height = GetSystemMetrics(is_virtual_desktop ? SM_CYVIRTUALSCREEN : SM_CYSCREEN);
	int const display_width  = GetSystemMetrics(is_virtual_desktop ? SM_CXVIRTUALSCREEN : SM_CXSCREEN);

	POINT screen;
	if ((data->usFlags & MOUSE_MOVE_ABSOLUTE)) {
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
	input_to_platform_on_mouse_move_window((uint32_t)client.x, size_y - (uint32_t)client.y - 1);

	//
	if ((data->usButtonFlags & RI_MOUSE_HWHEEL)) {
		input_to_platform_on_mouse_wheel((float)(short)data->usButtonData / WHEEL_DELTA, 0);
	}

	if ((data->usButtonFlags & RI_MOUSE_WHEEL)) {
		input_to_platform_on_mouse_wheel(0, (float)(short)data->usButtonData / WHEEL_DELTA);
	}

	//
	static USHORT const c_keys_down[] = {
		RI_MOUSE_LEFT_BUTTON_DOWN,
		RI_MOUSE_RIGHT_BUTTON_DOWN,
		RI_MOUSE_MIDDLE_BUTTON_DOWN,
		RI_MOUSE_BUTTON_4_DOWN,
		RI_MOUSE_BUTTON_5_DOWN,
	};

	static USHORT const c_keys_up[] = {
		RI_MOUSE_LEFT_BUTTON_UP,
		RI_MOUSE_RIGHT_BUTTON_UP,
		RI_MOUSE_MIDDLE_BUTTON_UP,
		RI_MOUSE_BUTTON_4_UP,
		RI_MOUSE_BUTTON_5_UP,
	};

	for (uint8_t i = 0; i < SIZE_OF_ARRAY(c_keys_down); i++) {
		if (data->usButtonFlags & c_keys_down[i]) {
			input_to_platform_on_mouse(i, true);
		}
		if (data->usButtonFlags & c_keys_up[i]) {
			input_to_platform_on_mouse(i, false);
		}
	}

	// https://learn.microsoft.com/windows/win32/api/winuser/ns-winuser-rawmouse
}

static void handle_input_hid_raw(struct Window * window, RAWHID * data) {
	if (!window->raw_input) { return; }
	(void)window; (void)data;
	// @todo: ERR("'RAWHID' input is not implemented");
	// REPORT_CALLSTACK(); DEBUG_BREAK();
	// https://learn.microsoft.com/windows/win32/api/winuser/ns-winuser-rawhid
}

static void handle_message_input_raw(struct Window * window, WPARAM wParam, LPARAM lParam) {
	// skip input in background; requires `RIDEV_INPUTSINK` at registration
	if (GET_RAWINPUT_CODE_WPARAM(wParam) == RIM_INPUTSINK) { return; }

	RAWINPUTHEADER header; UINT header_size = sizeof(header);
	if (GetRawInputData((HRAWINPUT)lParam, RID_HEADER, &header, &header_size, sizeof(RAWINPUTHEADER)) != (UINT)-1) {
		// @todo: arena/stack allocator (?)
		RAWINPUT * input = alloca(header.dwSize); UINT input_size = header.dwSize;
		if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, input, &input_size, sizeof(RAWINPUTHEADER)) != (UINT)-1) {
			switch (input->header.dwType) {
				case RIM_TYPEKEYBOARD: handle_input_keyboard_raw(window, &input->data.keyboard); break;
				case RIM_TYPEMOUSE:    handle_input_mouse_raw(window, &input->data.mouse); break;
				case RIM_TYPEHID:      handle_input_hid_raw(window, &input->data.hid); break;
			}
		}
	}

	// https://learn.microsoft.com/windows/win32/inputdev/raw-input
}

static void handle_message_input_keyboard(struct Window * window, WPARAM wParam, LPARAM lParam) {
	if (window->raw_input) { return; }

	WORD const flags = HIWORD(lParam);
	bool const is_extended = (flags & KF_EXTENDED) == KF_EXTENDED;

	enum Scan_Code const scan = (wParam == VK_PAUSE)
		? (enum Scan_Code)MapVirtualKey(wParam, MAPVK_VK_TO_VSC)
		: LOBYTE(flags) | (is_extended ? 0xe000 : 0x0000);

	uint8_t const key = fix_virtual_key((uint8_t)wParam, scan);
	TRC("0x%x", key);
	if (key == 0x00) { REPORT_CALLSTACK(); DEBUG_BREAK(); return; }
	if (key == 0xff) { REPORT_CALLSTACK(); DEBUG_BREAK(); return; }

	input_to_platform_on_key(
		translate_virtual_key(key, is_extended),
		scan,
		(flags & KF_UP) != KF_UP
	);

	// https://learn.microsoft.com/windows/win32/inputdev/keyboard-input
}

static void handle_message_input_mouse(struct Window * window, WPARAM wParam, LPARAM lParam, bool client_space, float wheel_mask_x, float wheel_mask_y) {
	if (window->raw_input) { return; }

	uint32_t size_x, size_y;
	platform_window_get_size(window, &size_x, &size_y);

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
	input_to_platform_on_mouse_move_window((uint32_t)client.x, size_y - (uint32_t)client.y - 1);

	//
	float const wheel_delta = (float)GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
	input_to_platform_on_mouse_wheel(wheel_mask_x * wheel_delta, wheel_mask_y * wheel_delta);

	//
	static WPARAM const c_key_masks[] = {
		MK_LBUTTON,
		MK_RBUTTON,
		MK_MBUTTON,
		MK_XBUTTON1,
		MK_XBUTTON2,
	};

	for (uint8_t i = 0; i < SIZE_OF_ARRAY(c_key_masks); i++) {
		bool const mask = c_key_masks[i];
		bool const is_down = (wParam & mask) == mask;
		input_to_platform_on_mouse(i, is_down);
	}

	// https://learn.microsoft.com/windows/win32/inputdev/mouse-input
}

//

static LRESULT CALLBACK window_procedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	struct Window * window = GetProp(hwnd, TEXT(HANDLE_PROP_WINDOW_NAME));
	if (window == NULL) { return DefWindowProc(hwnd, message, wParam, lParam); }

	switch (message) {
		case WM_GETMINMAXINFO: { // sent immediately
			MINMAXINFO const info = *(MINMAXINFO *)lParam;
			window->command_min_x = info.ptMinTrackSize.x;
			window->command_min_y = info.ptMinTrackSize.y;
			return 0;
		}

		case WM_INPUT: // sent immediately
			handle_message_input_raw(window, wParam, lParam);
			return 0;

		case WM_INPUT_DEVICE_CHANGE: switch (wParam) { // sent immediately
			case GIDC_ARRIVAL: return 0;
			case GIDC_REMOVAL: return 0;
		} break;

		case WM_SYSKEYUP:
		case WM_SYSKEYDOWN:
		case WM_KEYUP:
		case WM_KEYDOWN: // posted into queue
			handle_message_input_keyboard(window, wParam, lParam);
			return 0;

		case WM_SYSCHAR:
		case WM_CHAR: { // posted into queue
			static uint32_t s_utf16_high_surrogate = 0;
			uint32_t value = (uint32_t)wParam;
			if (IS_HIGH_SURROGATE(wParam)) { // [0xd800 .. 0xdbff]
				s_utf16_high_surrogate = value;
			}
			else {
				if (IS_LOW_SURROGATE(wParam)) { // [0xdc00 .. 0xdfff]
					value = (((s_utf16_high_surrogate & 0x03ff) << 10) | (value & 0x03ff)) + 0x10000;
				}
				s_utf16_high_surrogate = 0;
				input_to_platform_on_codepoint(value);
			}
			return 0;
		}

		case WM_UNICHAR: { // posted into queue
			if (wParam == UNICODE_NOCHAR) { return TRUE; }
			input_to_platform_on_codepoint((uint32_t)wParam);
			return FALSE;
		}

		case WM_MOUSEMOVE:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP: // posted into queue
			handle_message_input_mouse(window, wParam, lParam, true, 0, 0);
			return 0;

		case WM_MOUSEWHEEL: // sent immediately
			handle_message_input_mouse(window, wParam, lParam, false, 0, 1);
			return 0;

		case WM_MOUSEHWHEEL: // sent immediately
			handle_message_input_mouse(window, wParam, lParam, false, 1, 0);
			return 0;

		case WM_SYSCOMMAND: switch (wParam & 0xfff0) { // sent immediately
			case SC_MOVE: platform_window_internal_syscommand_move(window, wParam & 0x000f); return 0;
			case SC_SIZE: platform_window_internal_syscommand_size(window, wParam & 0x000f); return 0;
		} break;

		// @note: handle WM_MENUCHAR to prevent beeps on [alt+key] chords
		case WM_MENUCHAR: switch (HIWORD(wParam)) { // sent immediately
			case MF_POPUP:
			case MF_SYSMENU:
				return MAKELONG(0, MNC_CLOSE);
		} break;

		case WM_KILLFOCUS: { // sent immediately
			if (!window->ignore_once_WM_KILLFOCUS) {
				input_to_platform_reset();
			}
			window->ignore_once_WM_KILLFOCUS = false;
			return 0;
		}

		case WM_CLOSE: { // sent immediately
			if (window->callbacks.close == NULL || window->callbacks.close()) {
				platform_window_internal_set_raw_input(window, false);
				window->handle = NULL;
				DestroyWindow(hwnd);
			}
			return 0;
			// OS window is being closed through the OS API; we clear the handle reference
			// in order to prevent WM_DESTROY freeing the application window
		}

		case WM_DESTROY: { // sent immediately
			bool should_free = window->handle != NULL;
			RemoveProp(hwnd, TEXT(APPLICATION_CLASS_NAME));
			input_to_platform_reset();
			common_memset(window, 0, sizeof(*window));
			if (should_free) { MEMORY_FREE(window); }
			return 0;
		}
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}
