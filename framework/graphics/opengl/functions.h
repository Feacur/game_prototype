#if !defined(FRAMEWORK_GRAPHICS_OPENGL_FUNCTIONS)
#define FRAMEWORK_GRAPHICS_OPENGL_FUNCTIONS

#include "framework/common.h"

#include "framework/__warnings_push.h"
	#include <KHR/khrplatform.h>
	#include <GL/glcorearb.h>
#include "framework/__warnings_pop.h"

extern struct OGL {
	uint32_t version, glsl;
	#define XMACRO(type, name) PFNGL##type##PROC name;
	#include "internal/functions_xmacro.h"
} gl;

#endif
