#include "functions.h"

#include <stdbool.h>
#include <stdio.h>

uint32_t ogl_version = 0;

void graphics_load_functions(void * (* get)(char const * name)) {
	#define XMACRO_INIT() \
		do { \
			GLint version_major; \
			GLint version_minor; \
			glGetIntegerv(GL_MAJOR_VERSION, &version_major); \
			glGetIntegerv(GL_MINOR_VERSION, &version_minor); \
			ogl_version = (uint32_t)(version_major * 10 + version_minor); \
			\
			printf( \
				"OpenGL info:" \
				"\n  - vendor:   %s" \
				"\n  - renderer: %s" \
				"\n  - version:  %s" \
				"\n  - shaders:  %s" \
				"\n", \
				glGetString(GL_VENDOR), \
				glGetString(GL_RENDERER), \
				glGetString(GL_VERSION), \
				glGetString(GL_SHADING_LANGUAGE_VERSION) \
			); \
		} while (false); \

	#define XMACRO(type, name) gl ## name = (type)get("gl" #name);
	#include "xmacro.h"
}

#define XMACRO(type, name) type gl ## name;
#include "xmacro.h"
