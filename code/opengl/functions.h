#if !defined(GAME_GRAPHICS_BACKEND)
#define GAME_GRAPHICS_BACKEND

#include <KHR/khrplatform.h>
#include <GL/glcorearb.h>

#define XMACRO(type, name) extern type gl ## name;
#include "xmacro.h"

#endif
