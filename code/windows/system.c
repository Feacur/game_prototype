#include "window_to_system.h"
#include "graphics_library.h"

#include <Windows.h>

//
#include "code/platform_system.h"

void platform_system_init(void) {
	window_to_system_init();
	graphics_library_init();
}

void platform_system_free(void) {
	graphics_library_free();
	window_to_system_free();
}

void platform_system_update(void) {
	MSG message;
	while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {
		if (message.message == WM_QUIT) { continue; }
		TranslateMessage(&message);
		DispatchMessageA(&message);
	}
}

void platform_system_sleep(uint32_t millis) {
	Sleep(millis);
}
