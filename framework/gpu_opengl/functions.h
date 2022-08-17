#if !defined(FRAMEWORK_GPU_OPENGL_FUNCTIONS)
#define FRAMEWORK_GPU_OPENGL_FUNCTIONS

#include "framework/common.h"

#include <KHR/khrplatform.h>
#include <GL/glcorearb.h>

#define XMACRO(type, name) extern PFNGL##type##PROC gl##name;
#include "functions_xmacro.h"

//

extern uint32_t gs_ogl_version;

#endif
