#if !defined(GAME_GRAPHICS_FUNCTIONS)
#define GAME_GRAPHICS_FUNCTIONS

#include <KHR/khrplatform.h>
#include <GL/glcorearb.h>

extern uint32_t ogl_version;

void graphics_functions_init(void * (* get)(char const * name));
void graphics_functions_free(void);

#define XMACRO(type, name) extern type gl ## name;
#include "xmacro.h"

#endif
