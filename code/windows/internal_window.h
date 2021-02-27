#if !defined(GAME_INTERNAL_WINDOW)
#define GAME_INTERNAL_WINDOW

#include <Windows.h>

struct Window;

HDC internal_window_get_private_device(struct Window * window);

#endif
