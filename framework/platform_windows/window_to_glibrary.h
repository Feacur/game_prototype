#if !defined(GAME_PLATFORM_WINDOW_TO_GLIBRARY)
#define GAME_PLATFORM_WINDOW_TO_GLIBRARY

// interface from `window.c` to `glibrary_*.c`
// - rendering context initialization

#include <Windows.h>

struct Window;

HDC window_to_glibrary_get_private_device(struct Window * window);

#endif
