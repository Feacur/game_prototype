#include "framework/formatter.h"


//
#include "functions.h"

struct OGL gl;

//
#include "internal/functions_to_gpu_library.h"

void functions_to_gpu_library_init(Proc_Getter * get_proc) {
	TRC("loading OGL");
	gl = (struct OGL){
		#define XMACRO(type, name) .name = (PFNGL##type##PROC)get_proc(S_("gl" #name)),
		#include "internal/functions_xmacro.h"
	};

	GLint version_major;
	GLint version_minor;
	gl.GetIntegerv(GL_MAJOR_VERSION, &version_major);
	gl.GetIntegerv(GL_MINOR_VERSION, &version_minor);
	gl.version = (uint32_t)(version_major * 10 + version_minor);

	switch (gl.version) {
		case 20: gl.glsl = 110; break;
		case 21: gl.glsl = 120; break;
		case 30: gl.glsl = 130; break;
		case 31: gl.glsl = 140; break;
		case 32: gl.glsl = 150; break;
		default: gl.glsl = gl.version * 10; break;
	}

	LOG(
		"> OpenGL info:\n"
		"  vendor ..... %s\n"
		"  renderer: .. %s\n"
		"  version: ... %s\n"
		"  shaders: ... %s\n"
		""
		, gl.GetString(GL_VENDOR)
		, gl.GetString(GL_RENDERER)
		, gl.GetString(GL_VERSION)
		, gl.GetString(GL_SHADING_LANGUAGE_VERSION)
	);
}

void functions_to_gpu_library_free(void) {
	common_memset(&gl, 0, sizeof(gl));
	TRC("unloaded OGL");
}
