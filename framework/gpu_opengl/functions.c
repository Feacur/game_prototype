#include "framework/logger.h"

#include <stdbool.h>

//
#include "functions.h"

uint32_t ogl_version = 0;

void glibrary_functions_init(void * (* get)(struct CString name)) {
	#define XMACRO_INIT() \
		do { \
			GLint version_major; \
			GLint version_minor; \
			glGetIntegerv(GL_MAJOR_VERSION, &version_major); \
			glGetIntegerv(GL_MINOR_VERSION, &version_minor); \
			ogl_version = (uint32_t)(version_major * 10 + version_minor); \
			\
			logger_to_console( \
				"\n" \
				"> OpenGL info:\n" \
				"  vendor ..... %s\n" \
				"  renderer: .. %s\n" \
				"  version: ... %s\n" \
				"  shaders: ... %s\n" \
				"", \
				glGetString(GL_VENDOR), \
				glGetString(GL_RENDERER), \
				glGetString(GL_VERSION), \
				glGetString(GL_SHADING_LANGUAGE_VERSION) \
			); \
		} while (false); \

	#define XMACRO(type, name) gl ## name = (type)get(S_("gl" #name));
	#include "xmacro.h"
}

void glibrary_functions_free(void) {
	#define XMACRO(type, name) gl ## name = NULL;
	#include "xmacro.h"

	ogl_version = 0;
}

#define XMACRO(type, name) type gl ## name;
#include "xmacro.h"
