#include "code/memory.h"
#include "code/array_byte.h"

#include "functions.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char * ogl_extensions = NULL;

//
#include "implementation.h"

// -- GPU program part
struct Gpu_Program {
	GLuint id;
};

static void verify_shader(GLuint id, GLenum parameter);
static void verify_program(GLuint id, GLenum parameter);
struct Gpu_Program * gpu_program_init(char const * text) {
#define ADD_SECTION_HEADER(shader_type, version) do {\
		if (strstr(text, #shader_type)) {\
			if (ogl_version < (version)) { fprintf(stderr, "'" #shader_type "' is unavailable\n"); DEBUG_BREAK(); exit(1); }\
			static char const header_text[] = "#define " #shader_type "\n";\
			headers[headers_count++] = (struct Section_Header){\
				.type = GL_ ## shader_type,\
				.length = (sizeof(header_text) / sizeof(*header_text)) - 1,\
				.data = header_text,\
			};\
		}\
	} while (false)\

	GLint text_length = (GLint)strlen(text);

	// a mandatory version header
	static GLchar glsl_version[20];
	GLint glsl_version_length = sprintf(
		glsl_version, "#version %d core\n",
		(ogl_version > 33) ? ogl_version * 10 : 330
	);

	// section headers, per shader type
	struct Section_Header {
		GLenum type;
		GLchar const * data;
		GLint length;
	};

	uint32_t headers_count = 0;
	struct Section_Header headers[4];
	ADD_SECTION_HEADER(VERTEX_SHADER, 20);
	ADD_SECTION_HEADER(FRAGMENT_SHADER, 20);
	ADD_SECTION_HEADER(GEOMETRY_SHADER, 32);
	ADD_SECTION_HEADER(COMPUTE_SHADER, 43);

	// compile shader objects
	GLuint shader_ids[4];
	for (uint32_t i = 0; i < headers_count; i++) {
		GLchar const * code[]   = {glsl_version,        headers[i].data,   text};
		GLint          length[] = {glsl_version_length, headers[i].length, text_length};

		GLuint shader_id = glCreateShader(headers[i].type);
		glShaderSource(shader_id, sizeof(code) / sizeof(*code), code, length);
		glCompileShader(shader_id);
		verify_shader(shader_id, GL_COMPILE_STATUS);

		shader_ids[i] = shader_id;
	}

	// link shader objects into a program
	GLuint program_id = glCreateProgram();
	for (uint32_t i = 0; i < headers_count; i++) {
		glAttachShader(program_id, shader_ids[i]);
	}

	glLinkProgram(program_id);
	verify_program(program_id, GL_LINK_STATUS);

	// free redundant resources
	for (uint32_t i = 0; i < headers_count; i++) {
		glDetachShader(program_id, shader_ids[i]);
		glDeleteShader(shader_ids[i]);
	}

	//
	struct Gpu_Program * gpu_program = MEMORY_ALLOCATE(struct Gpu_Program);
	gpu_program->id = program_id;
	return gpu_program;

#undef ADD_HEADER
}

void gpu_program_free(struct Gpu_Program * gpu_program) {
	glDeleteProgram(gpu_program->id);
	MEMORY_FREE(gpu_program);
}

// -- GPU texture part
struct Gpu_Texture {
	GLuint id;
};

struct Gpu_Texture * gpu_texture_init(void) {
	struct Gpu_Texture * gpu_texture = MEMORY_ALLOCATE(struct Gpu_Texture);

	glCreateTextures(GL_TEXTURE_2D, 1, &gpu_texture->id);

	return gpu_texture;
}

void gpu_texture_free(struct Gpu_Texture * gpu_texture) {
	glDeleteTextures(1, &gpu_texture->id);
	MEMORY_FREE(gpu_texture);
}

// -- GPU mesh part
struct Gpu_Mesh {
	GLuint id;
};

struct Gpu_Mesh * gpu_mesh_init(void) {
	struct Gpu_Mesh * gpu_mesh = MEMORY_ALLOCATE(struct Gpu_Mesh);

	glCreateVertexArrays(1, &gpu_mesh->id);

	return gpu_mesh;
}

void gpu_mesh_free(struct Gpu_Mesh * gpu_mesh) {
	glDeleteVertexArrays(1, &gpu_mesh->id);
	MEMORY_FREE(gpu_mesh);
}

//
#include "graphics_to_graphics_library.h"

static void __stdcall opengl_debug_message_callback(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar *message,
	const void *userParam
);

static char * allocate_extensions_string(void);
void graphics_to_graphics_library_init(void) {
	// setup debug
	if (glDebugMessageCallback != NULL) {
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(&opengl_debug_message_callback, NULL);
		if (glDebugMessageControl != NULL) {
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
		}
	}

	// fetch extensions
	ogl_extensions = allocate_extensions_string();
}

void graphics_to_graphics_library_free(void) {
	ogl_version = 0;
	MEMORY_FREE(ogl_extensions);
}

//

static char * allocate_extensions_string(void) {
	struct Array_Byte string;
	array_byte_init(&string);

	GLint extensions_count = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &extensions_count);

	array_byte_resize(&string, (uint32_t)(extensions_count * 26));
	for(GLint i = 0; i < extensions_count; i++) {
		GLubyte const * value = glGetStringi(GL_EXTENSIONS, (GLuint)i);
		array_byte_write_many(&string, value, (uint32_t)strlen((char const *)value));
		array_byte_write(&string, ' ');
	}
	array_byte_write(&string, '\0');

	return (char *)string.data;
}

//

static void verify_shader(GLuint id, GLenum parameter) {
	GLint status;
	glGetShaderiv(id, parameter, &status);
	if (status) { return; }

	GLint max_length;
	glGetShaderiv(id, GL_INFO_LOG_LENGTH, &max_length);

	if (max_length > 0) {
		struct Array_Byte message;
		array_byte_init(&message);
		array_byte_resize(&message, (uint32_t)max_length);

		glGetShaderInfoLog(id, max_length, &max_length, (char *)message.data);
		fprintf(stderr, "%s\n", (char *)message.data);

		array_byte_free(&message);
	}

	DEBUG_BREAK();
}

static void verify_program(GLuint id, GLenum parameter) {
	GLint status;
	glGetProgramiv(id, parameter, &status);
	if (status) { return; }

	GLint max_length;
	glGetProgramiv(id, GL_INFO_LOG_LENGTH, &max_length);

	if (max_length > 0) {
		struct Array_Byte message;
		array_byte_init(&message);
		array_byte_resize(&message, (uint32_t)max_length);

		glGetProgramInfoLog(id, max_length, &max_length, (char *)message.data);
		fprintf(stderr, "%s\n", (char *)message.data);

		array_byte_free(&message);
	}

	DEBUG_BREAK();
}

//

static void __stdcall opengl_debug_message_callback(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar *message,
	const void *userParam
) {
	(void)length; (void)userParam;

	char const * source_string = NULL;
	switch (source) {
		case GL_DEBUG_SOURCE_API:             source_string = "API";             break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   source_string = "window system";   break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: source_string = "shader compiler"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:     source_string = "thirdparty";      break;
		case GL_DEBUG_SOURCE_APPLICATION:     source_string = "application";     break;
		case GL_DEBUG_SOURCE_OTHER:           source_string = "other";           break;
		default:                              source_string = "unknown";         break;
	}

	char const * type_string = NULL;
	switch (type) {
		case GL_DEBUG_TYPE_ERROR:               type_string = "error";               break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: type_string = "deprecated behavior"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  type_string = "undefined behavior";  break;
		case GL_DEBUG_TYPE_PORTABILITY:         type_string = "portability";         break;
		case GL_DEBUG_TYPE_PERFORMANCE:         type_string = "performance";         break;
		case GL_DEBUG_TYPE_MARKER:              type_string = "marker";              break;
		case GL_DEBUG_TYPE_PUSH_GROUP:          type_string = "push group";          break;
		case GL_DEBUG_TYPE_POP_GROUP:           type_string = "pop group";           break;
		case GL_DEBUG_TYPE_OTHER:               type_string = "other";               break;
		default:                                type_string = "unknown";             break;
	}

	char const * severity_string = NULL;
	switch (severity) {
		case GL_DEBUG_SEVERITY_HIGH:         severity_string = "[crt]"; break; // All OpenGL Errors, shader compilation/linking errors, or highly-dangerous undefined behavior
		case GL_DEBUG_SEVERITY_MEDIUM:       severity_string = "[err]"; break; // Major performance warnings, shader compilation/linking warnings, or the use of deprecated functionality
		case GL_DEBUG_SEVERITY_LOW:          severity_string = "[wrn]"; break; // Redundant state change performance warning, or unimportant undefined behavior
		case GL_DEBUG_SEVERITY_NOTIFICATION: severity_string = "[trc]"; break; // Anything that isn't an error or performance issue.
		default:                             severity_string = "[???]"; break; // ?
	}

	fprintf(
		stderr,
		"OpenGL message '0x%x'"
		"\n  - message:  %s"
		"\n  - severity: %s"
		"\n  - type:     %s"
		"\n  - source:   %s"
		"\n",
		id,
		message,
		severity_string,
		type_string,
		source_string
	);

	DEBUG_BREAK();
}
