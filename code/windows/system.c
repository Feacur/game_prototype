#include <Windows.h>

#include "code/common.h"
#include "code/platform_system.h"

#include "internal_system_window.h"

void platform_system_init(void) {
	internal_system_window_init();
}

void platform_system_free(void) {
	internal_system_window_free();
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
