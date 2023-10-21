#include "framework/formatter.h"

//
#include "functions.h"

uint32_t gs_ogl_version = 0;

#define XMACRO(type, name) PFNGL##type##PROC gl##name;
#include "internal/functions_xmacro.h"

//
#include "internal/functions_to_gpu_library.h"

void functions_to_gpu_library_init(void * (* get)(struct CString name)) {
	#define XMACRO_INIT() \
		do { \
			GLint version_major; \
			GLint version_minor; \
			glGetIntegerv(GL_MAJOR_VERSION, &version_major); \
			glGetIntegerv(GL_MINOR_VERSION, &version_minor); \
			gs_ogl_version = (uint32_t)(version_major * 10 + version_minor); \
			\
			LOG( \
				"> OpenGL info:\n" \
				"  vendor ..... %s\n" \
				"  renderer: .. %s\n" \
				"  version: ... %s\n" \
				"  shaders: ... %s\n" \
				"" \
				, glGetString(GL_VENDOR) \
				, glGetString(GL_RENDERER) \
				, glGetString(GL_VERSION) \
				, glGetString(GL_SHADING_LANGUAGE_VERSION) \
			); \
		} while (false); \

	#define XMACRO(type, name) gl##name = (PFNGL##type##PROC)get(S_("gl" #name));
	#include "internal/functions_xmacro.h"
}

void functions_to_gpu_library_free(void) {
	#define XMACRO(type, name) gl##name = NULL;
	#include "internal/functions_xmacro.h"

	gs_ogl_version = 0;
}
