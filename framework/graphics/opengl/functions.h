#if !defined(FRAMEWORK_GRAPHICS_OPENGL_FUNCTIONS)
#define FRAMEWORK_GRAPHICS_OPENGL_FUNCTIONS

#include "framework/common.h"

#include "framework/__warnings_push.h"
	#include <KHR/khrplatform.h>
	#include <GL/glcorearb.h>
#include "framework/__warnings_pop.h"

#define XMACRO(type, name) extern PFNGL##type##PROC gl##name;
#include "internal/functions_xmacro.h"

//

extern uint32_t gs_ogl_version;
extern uint32_t gs_glsl_version;

#endif
