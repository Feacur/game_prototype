#if !defined(GAME_PLATFORM_WINDOW_TO_GRAPHICS_LIBRARY)
#define GAME_PLATFORM_WINDOW_TO_GRAPHICS_LIBRARY

#include <Windows.h>

struct Window;

HDC window_to_graphics_library_get_private_device(struct Window * window);

#endif
