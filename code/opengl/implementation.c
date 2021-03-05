#include "code/memory.h"
#include "code/array_byte.h"
#include "code/asset_mesh.h"
#include "code/asset_image.h"

#include "functions.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
#include "implementation.h"

// -- GPU types part
struct Gpu_Program_Field {
	uint32_t id;
	GLint location;
	GLenum type;
};

struct Gpu_Program {
	GLuint id;
	struct Gpu_Program_Field uniforms[10];
	uint32_t uniforms_count;
};

struct Gpu_Texture {
	GLuint id;
};

struct Gpu_Mesh {
	GLuint id;
	GLuint vertices_buffer_id;
	GLuint indices_buffer_id;
	GLsizei indices_count;
};

struct Gpu_Unit {
	struct Gpu_Texture * gpu_texture;
};

// -- graphics library part
static struct {
	char * uniforms[100];
	size_t lengths[100];
	uint32_t uniforms_count;

	char * extensions;

	struct Gpu_Program * active_program;
	struct Gpu_Mesh * active_mesh;

	struct Gpu_Unit units[100];
} glibrary;

uint32_t glibrary_get_uniform_id(char const * name) {
	size_t name_length = strlen(name);
	for (uint32_t i = 1; i < glibrary.uniforms_count; i++) {
		if (name_length != glibrary.lengths[i]) { continue; }
		if (strncmp(name, glibrary.uniforms[i], name_length) == 0) { return i; }
	}
	return 0;
}

void glibrary_clear(void) {
	glClear(GL_COLOR_BUFFER_BIT);
}

void glibrary_draw(struct Gpu_Program * gpu_program, struct Gpu_Mesh * gpu_mesh) {
	gpu_program_select(gpu_program);
	gpu_mesh_select(gpu_mesh);
	glDrawElements(GL_TRIANGLES, gpu_mesh->indices_count, GL_UNSIGNED_INT, NULL);
}

static uint32_t glibrary_add_uniform(char const * name, size_t name_length) {
	for (uint32_t i = 1; i < glibrary.uniforms_count; i++) {
		if (name_length != glibrary.lengths[i]) { continue; }
		if (strncmp(name, glibrary.uniforms[i], name_length) == 0) { return i; }
	}

	char * copy = MEMORY_ALLOCATE_ARRAY(char, name_length + 1);
	memcpy(copy, name, name_length);
	copy[name_length] = '\0';

	glibrary.uniforms[glibrary.uniforms_count] = copy;
	glibrary.lengths[glibrary.uniforms_count] = name_length;
	glibrary.uniforms_count++;

	return glibrary.uniforms_count - 1;
}

static uint32_t glibrary_find_unit(struct Gpu_Texture * gpu_texture) {
	for (uint32_t i = 0; i < sizeof(glibrary.units) / sizeof(*glibrary.units); i++) {
		if (glibrary.units[i].gpu_texture == gpu_texture) { return i; }
	}
	return UINT32_MAX;
}

// -- GPU program part
static void verify_shader(GLuint id, GLenum parameter);
static void verify_program(GLuint id, GLenum parameter);
struct Gpu_Program * gpu_program_init(struct Array_Byte * asset) {
#define ADD_SECTION_HEADER(shader_type, version) \
	do { \
		if (strstr((char const *)asset->data, #shader_type)) {\
			if (ogl_version < (version)) { fprintf(stderr, "'" #shader_type "' is unavailable\n"); DEBUG_BREAK(); exit(1); } \
			static char const header_text[] = "#define " #shader_type "\n"; \
			headers[headers_count++] = (struct Section_Header){ \
				.type = GL_ ## shader_type, \
				.length = (sizeof(header_text) / sizeof(*header_text)) - 1, \
				.data = header_text, \
			}; \
		} \
	} while (false) \

	// array_byte_write(asset, '\0');

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
		GLchar const * code[]   = {glsl_version,        headers[i].data,   (GLchar *)asset->data};
		GLint          length[] = {glsl_version_length, headers[i].length, (GLint)asset->count};

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

	// introspect the program
	GLint uniforms_count;
	glGetProgramInterfaceiv(program_id, GL_UNIFORM, GL_ACTIVE_RESOURCES, &uniforms_count);
	
	GLint uniform_name_buffer_length; // includes zero-terminator
	glGetProgramInterfaceiv(program_id, GL_UNIFORM, GL_MAX_NAME_LENGTH, &uniform_name_buffer_length);
	GLchar * uniform_name_buffer = MEMORY_ALLOCATE_ARRAY(GLchar, uniform_name_buffer_length);

	struct Gpu_Program_Field uniforms[10];
	for (GLint i = 0; i < uniforms_count; i++) {
		// GL_NAME_LENGTH // includes zero-terminator
		GLenum const props[] = {GL_TYPE, GL_LOCATION};
		GLint params[sizeof(props) / sizeof(*props)];
		glGetProgramResourceiv(program_id, GL_UNIFORM, (GLuint)i, sizeof(props) / sizeof(*props), props, sizeof(params) / sizeof(*params), NULL, params);

		GLsizei name_length;
		glGetProgramResourceName(program_id, GL_UNIFORM, (GLuint)i, uniform_name_buffer_length, &name_length, uniform_name_buffer);

		uniforms[i] = (struct Gpu_Program_Field){
			.id = glibrary_add_uniform(uniform_name_buffer, (size_t)name_length),
			.location = params[1],
			.type = (GLenum)params[0]
		};
	}

	MEMORY_FREE(uniform_name_buffer);

	//
	struct Gpu_Program * gpu_program = MEMORY_ALLOCATE(struct Gpu_Program);
	*gpu_program = (struct Gpu_Program){
		.id = program_id,
		.uniforms_count = (uint32_t)uniforms_count,
	};
	memcpy(gpu_program->uniforms, uniforms, sizeof(*uniforms) * (size_t)uniforms_count);
	return gpu_program;
	// https://www.khronos.org/opengl/wiki/Program_Introspection

#undef ADD_HEADER
}

void gpu_program_free(struct Gpu_Program * gpu_program) {
	if (ogl_version > 0) {
		if (glibrary.active_program == gpu_program) { glibrary.active_program = NULL; }
		glDeleteProgram(gpu_program->id);
	}
	MEMORY_FREE(gpu_program);
}

void gpu_program_select(struct Gpu_Program * gpu_program) {
	if (glibrary.active_program == gpu_program) { return; }
	glibrary.active_program = gpu_program;
	glUseProgram(gpu_program->id);
}

static GLint gpu_program_find_uniform_location(struct Gpu_Program * gpu_program, uint32_t uniform_id) {
	for (uint32_t i = 0; i < gpu_program->uniforms_count; i++) {
		if (gpu_program->uniforms[i].id == uniform_id) {
			return gpu_program->uniforms[i].location;
		}
	}
	fprintf(stderr, "'gpu_program_find_uniform_location' failed\n"); DEBUG_BREAK();
	return -2; // -1 for silent mode
}

void gpu_program_set_texture(struct Gpu_Program * gpu_program, uint32_t uniform_id, struct Gpu_Texture * gpu_texture) {
	uint32_t unit = glibrary_find_unit(gpu_texture);
	if (unit == UINT32_MAX) { fprintf(stderr, "'glibrary_find_unit' failed\n"); DEBUG_BREAK(); exit(1); }

	GLint data = (GLint)unit;
	GLint location = gpu_program_find_uniform_location(gpu_program, uniform_id);
	glProgramUniform1iv(gpu_program->id, location, 1, &data);
}

// -- GPU texture part
struct Gpu_Texture * gpu_texture_init(struct Asset_Image * asset) {
	GLuint texture_id;
	glCreateTextures(GL_TEXTURE_2D, 1, &texture_id);

	// allocate buffer
	GLsizei levels = 1;
	glTextureStorage2D(texture_id, levels, GL_RGB8, (GLsizei)asset->size_x, (GLsizei)asset->size_y);

	// chart buffer
	glTextureParameteri(texture_id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(texture_id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(texture_id, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTextureParameteri(texture_id, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// load buffer
	GLint level = 0;
	glTextureSubImage2D(texture_id, level, 0, 0, (GLsizei)asset->size_x, (GLsizei)asset->size_y, GL_RGB, GL_UNSIGNED_BYTE, asset->data);

	//
	struct Gpu_Texture * gpu_texture = MEMORY_ALLOCATE(struct Gpu_Texture);
	*gpu_texture = (struct Gpu_Texture){
		.id = texture_id,
	};
	return gpu_texture;
}

void gpu_texture_free(struct Gpu_Texture * gpu_texture) {
	if (ogl_version > 0) {
		for (uint32_t i = 0; i < sizeof(glibrary.units) / sizeof(*glibrary.units); i++) {
			if (glibrary.units[i].gpu_texture == gpu_texture) {
				glibrary.units[i].gpu_texture = NULL;
				// glBindTextureUnit((GLuint)i, 0);
			}
		}
		glDeleteTextures(1, &gpu_texture->id);
	}
	MEMORY_FREE(gpu_texture);
}

// -- GPU mesh part
struct Gpu_Mesh * gpu_mesh_init(struct Asset_Mesh * asset) {
	GLuint mesh_id;
	glCreateVertexArrays(1, &mesh_id);

	// allocate buffer: vertices
	GLuint vertices_buffer_id;
	glCreateBuffers(1, &vertices_buffer_id);
	glNamedBufferData(
		vertices_buffer_id,
		asset->vertices.count * sizeof(*asset->vertices.data),
		NULL, GL_STATIC_DRAW
	);

	// allocate buffer: indices
	GLuint indices_buffer_id;
	glCreateBuffers(1, &indices_buffer_id);
	glNamedBufferData(
		indices_buffer_id,
		asset->indices.count * sizeof(*asset->indices.data),
		NULL, GL_STATIC_DRAW
	);

	// chart buffer: vertices
	GLsizei all_attributes_size = 0;
	for (uint32_t i = 0; i < asset->sizes.count; i++) {
		all_attributes_size += (GLsizei)asset->sizes.data[i] * (GLsizei)sizeof(*asset->vertices.data);
	}

	GLuint buffer_index = 0;
	GLintptr buffer_start = 0;
	glVertexArrayVertexBuffer(mesh_id, buffer_index, vertices_buffer_id, buffer_start, all_attributes_size);

	GLuint attribute_offset = 0;
	for (uint32_t i = 0; i < asset->locations.count; i++) {
		GLuint location = (GLuint)asset->locations.data[i];
		uint32_t size = asset->sizes.data[i];
		glEnableVertexArrayAttrib(mesh_id, location);
		glVertexArrayAttribBinding(mesh_id, location, buffer_index);
		glVertexArrayAttribFormat(mesh_id, location, (GLint)size, GL_FLOAT, GL_FALSE, attribute_offset);
		attribute_offset += size * (GLuint)sizeof(*asset->vertices.data);
	}

	// chart buffer: indices
	glVertexArrayElementBuffer(mesh_id, indices_buffer_id);

	// load buffer: vertices
	glNamedBufferSubData(vertices_buffer_id, 0, asset->vertices.count * sizeof(*asset->vertices.data), asset->vertices.data);

	// load buffer: indices
	glNamedBufferSubData(indices_buffer_id, 0, asset->indices.count * sizeof(*asset->indices.data), asset->indices.data);

	//
	struct Gpu_Mesh * gpu_mesh = MEMORY_ALLOCATE(struct Gpu_Mesh);
	*gpu_mesh = (struct Gpu_Mesh){
		.id = mesh_id,
		.vertices_buffer_id = vertices_buffer_id,
		.indices_buffer_id = indices_buffer_id,
		.indices_count = (GLsizei)asset->indices.count,
	};
	return gpu_mesh;
}

void gpu_mesh_free(struct Gpu_Mesh * gpu_mesh) {
	if (ogl_version > 0) {
		if (glibrary.active_mesh == gpu_mesh) { glibrary.active_mesh = NULL; }
		glDeleteBuffers(1, &gpu_mesh->vertices_buffer_id);
		glDeleteBuffers(1, &gpu_mesh->indices_buffer_id);
		glDeleteVertexArrays(1, &gpu_mesh->id);
	}
	MEMORY_FREE(gpu_mesh);
}

void gpu_mesh_select(struct Gpu_Mesh * gpu_mesh) {
	if (glibrary.active_mesh == gpu_mesh) { return; }
	glibrary.active_mesh = gpu_mesh;
	glBindVertexArray(gpu_mesh->id);
}

// -- GPU unit part
void gpu_unit_init(struct Gpu_Texture * gpu_texture) {
	uint32_t unit = glibrary_find_unit(gpu_texture);
	if (unit != UINT32_MAX) { return; }

	unit = glibrary_find_unit(NULL);
	if (unit == UINT32_MAX) { fprintf(stderr, "'glibrary_find_unit' failed\n"); DEBUG_BREAK(); exit(1); }

	glibrary.units[unit] = (struct Gpu_Unit){
		.gpu_texture = gpu_texture,
	};

	glBindTextureUnit((GLuint)unit, gpu_texture->id);
}

void gpu_unit_free(struct Gpu_Texture * gpu_texture) {
	if (ogl_version > 0) {
		uint32_t unit = glibrary_find_unit(gpu_texture);
		if (unit == UINT32_MAX) { fprintf(stderr, "'glibrary_find_unit' failed\n"); DEBUG_BREAK(); exit(1); }

		glibrary.units[unit] = (struct Gpu_Unit){
			.gpu_texture = NULL,
		};

		glBindTextureUnit((GLuint)unit, 0);
	}
}

//
#include "graphics_to_glibrary.h"

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
void graphics_to_glibrary_init(void) {
	// setup debug
	if (glDebugMessageCallback != NULL) {
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(&opengl_debug_message_callback, NULL);
		if (glDebugMessageControl != NULL) {
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
		}
	}

	//
	memset(&glibrary, 0, sizeof(glibrary));
	glibrary.extensions = allocate_extensions_string();
	glibrary_add_uniform("", 0);
}

void graphics_to_glibrary_free(void) {
	for (uint32_t i = 0; i < glibrary.uniforms_count; i++) {
		MEMORY_FREE(glibrary.uniforms[i]);
	}

	MEMORY_FREE(glibrary.extensions);
	memset(&glibrary, 0, sizeof(glibrary));
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
