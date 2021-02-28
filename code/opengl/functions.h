#if !defined(GAME_GRAPHICS_FUNCTIONS)
#define GAME_GRAPHICS_FUNCTIONS

#include <KHR/khrplatform.h>
#include <GL/glcorearb.h>

#define XMACRO(type, name) extern type gl ## name;
#include "xmacro.h"

#endif
