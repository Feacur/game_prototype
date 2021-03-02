#if !defined(GAME_PLATFORM_WINDOW_TO_GLIBRARY)
#define GAME_PLATFORM_WINDOW_TO_GLIBRARY

#include <Windows.h>

struct Window;

HDC window_to_glibrary_get_private_device(struct Window * window);

#endif
