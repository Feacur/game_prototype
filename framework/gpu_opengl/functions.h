#if !defined(GAME_GPU_FUNCTIONS)
#define GAME_GPU_FUNCTIONS

// interface from `functions.c` to `glibrary_opengl.c`
// - opengl functions initialization

#include <KHR/khrplatform.h>
#include <GL/glcorearb.h>

extern uint32_t ogl_version;

void glibrary_functions_init(void * (* get)(char const * name));
void glibrary_functions_free(void);

#define XMACRO(type, name) extern type gl ## name;
#include "xmacro.h"

#endif
