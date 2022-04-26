#if !defined(GAME_GPU_FUNCTIONS)
#define GAME_GPU_FUNCTIONS

// interface from `functions.c` to `gpu_library_opengl.c`
// - opengl functions initialization

#include "framework/common.h"

#include <KHR/khrplatform.h>
#include <GL/glcorearb.h>

void gpu_library_functions_init(void * (* get)(struct CString name));
void gpu_library_functions_free(void);

#define XMACRO(type, name) extern PFNGL##type##PROC gl##name;
#include "functions_xmacro.h"

//

extern uint32_t gs_ogl_version;

#endif
