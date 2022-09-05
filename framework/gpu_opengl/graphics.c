#include "framework/memory.h"
#include "framework/logger.h"
#include "framework/maths.h"

#include "framework/containers/strings.h"
#include "framework/containers/buffer.h"
#include "framework/containers/array_any.h"
#include "framework/containers/array_u32.h"
#include "framework/containers/hash_table_u32.h"
#include "framework/containers/ref_table.h"

#include "framework/assets/mesh.h"
#include "framework/assets/image.h"

#include "framework/graphics/types.h"
#include "framework/graphics/material.h"
#include "framework/graphics/material_override.h"

#include "functions.h"
#include "types.h"

#include <malloc.h>

// @todo: GPU scissor test
// @todo: expose screen buffer settings, as well as OpenGL's
// @idea: use dedicated samplers instead of texture defaults
// @idea: support older OpenGL versions (pre direct state access, which is 4.5)

struct Gpu_Program_Field_Internal {
	struct Gpu_Program_Field base;
	GLint location;
};

struct Gpu_Program {
	GLuint id;
	struct Hash_Table_U32 uniforms; // uniform string id : `struct Gpu_Program_Field_Internal`
	// @idea: add an optional asset source
};

struct Gpu_Texture {
	GLuint id;
	uint32_t size_x, size_y;
	struct Texture_Parameters parameters;
	struct Texture_Settings settings;
	// @idea: add an optional asset source
};

struct Gpu_Target_Texture {
	struct Ref texture;
	uint32_t drawbuffer;
};

struct Gpu_Target_Buffer {
	GLuint id;
	struct Texture_Parameters parameters;
	uint32_t drawbuffer;
};

struct Gpu_Target {
	GLuint id;
	uint32_t size_x, size_y;
	struct Array_Any textures; // `struct Gpu_Target_Texture`
	struct Array_Any buffers;  // `struct Gpu_Target_Buffer`
	// @idea: add an optional asset source
};

struct Gpu_Mesh_Buffer {
	GLuint id;
	struct Mesh_Parameters parameters;
	uint32_t capacity, count;
};

struct Gpu_Mesh {
	GLuint id;
	struct Array_Any buffers; // `struct Gpu_Mesh_Buffer`
	uint32_t elements_index;
	// @idea: add an optional asset source
};

struct Gpu_Unit {
	struct Ref texture;
};

struct Graphics_Action {
	struct Ref gpu_ref;
	void (* action)(struct Ref gpu_ref);
};

static struct Graphics_State {
	char * extensions;

	struct Strings uniforms;
	struct Array_Any actions; // `struct Graphics_Action`

	struct Ref_Table programs;
	struct Ref_Table targets;
	struct Ref_Table textures;
	struct Ref_Table meshes;

	struct Graphics_State_Active {
		struct Ref program;
		struct Ref target;
		struct Ref mesh;
	} active;

	uint32_t units_capacity;
	struct Gpu_Unit * units;

	struct Gpu_Clip_Space {
		struct vec2 origin;
		float ncp, fcp;
	} clip_space;

	struct Graphics_State_Limits {
		uint32_t max_units_vertex_shader;
		uint32_t max_units_fragment_shader;
		uint32_t max_units_compute_shader;
		uint32_t max_texture_size;
		uint32_t max_renderbuffer_size;
		uint32_t max_elements_vertices;
		uint32_t max_elements_indices;
		uint32_t max_uniform_locations;
	} limits;
} gs_graphics_state;

//
#include "framework/graphics/gpu_objects.h"

// ----- ----- ----- ----- -----
//     GPU program part
// ----- ----- ----- ----- -----

static void verify_shader(GLuint id);
static void verify_program(GLuint id);
struct Ref gpu_program_init(struct Buffer const * asset) {
#define ADD_SECTION_HEADER(shader_type, version) \
	do { \
		if (common_strstr((char const *)asset->data, #shader_type)) {\
			if (gs_ogl_version < (version)) { logger_to_console("'" #shader_type "' is unavailable\n"); DEBUG_BREAK(); break; } \
			headers[headers_count++] = (struct Section_Header){ \
				.type = GL_##shader_type, \
				.text = S_("#define " #shader_type "\n"), \
			}; \
		} \
	} while (false) \

	// a mandatory version header
	GLchar glsl_version[20];
	GLint glsl_version_length = (GLint)logger_to_buffer(
		SIZE_OF_ARRAY(glsl_version), glsl_version, "#version %d core\n",
		(gs_ogl_version > 33) ? gs_ogl_version * 10 : 330
	);

	// section headers, per shader type
	struct Section_Header {
		GLenum type;
		struct CString text;
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
		GLchar const * code[]   = {glsl_version,        headers[i].text.data,          (GLchar *)asset->data};
		GLint const    length[] = {glsl_version_length, (GLint)headers[i].text.length, (GLint)asset->count};

		GLuint shader_id = glCreateShader(headers[i].type);
		glShaderSource(shader_id, SIZE_OF_ARRAY(code), code, length);
		glCompileShader(shader_id);
		verify_shader(shader_id);

		shader_ids[i] = shader_id;
	}

	// link shader objects into a program
	GLuint program_id = glCreateProgram();
	for (uint32_t i = 0; i < headers_count; i++) {
		glAttachShader(program_id, shader_ids[i]);
	}

	glLinkProgram(program_id);
	verify_program(program_id);

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
	GLchar * uniform_name_buffer = alloca(sizeof(GLchar) * (size_t)uniform_name_buffer_length);

	struct Hash_Table_U32 uniforms = hash_table_u32_init(sizeof(struct Gpu_Program_Field_Internal));
	hash_table_u32_resize(&uniforms, (uint32_t)uniforms_count);

	for (GLint i = 0; i < uniforms_count; i++) {
		enum Param {
			PARAM_TYPE,
			PARAM_ARRAY_SIZE,
			PARAM_LOCATION,
			// PARAM_ROW_MAJOR,
			// PARAM_NAME_LENGTH,
		};
		static GLenum const c_props[] = {
			[PARAM_TYPE]        = GL_TYPE,
			[PARAM_ARRAY_SIZE]  = GL_ARRAY_SIZE,
			[PARAM_LOCATION]    = GL_LOCATION,
			// [PARAM_ROW_MAJOR]   = GL_IS_ROW_MAJOR,
			// [PARAM_NAME_LENGTH] = GL_NAME_LENGTH,
		};
		GLint params[SIZE_OF_ARRAY(c_props)];
		glGetProgramResourceiv(program_id, GL_UNIFORM, (GLuint)i, SIZE_OF_ARRAY(c_props), c_props, SIZE_OF_ARRAY(params), NULL, params);

		GLsizei name_length;
		glGetProgramResourceName(program_id, GL_UNIFORM, (GLuint)i, uniform_name_buffer_length, &name_length, uniform_name_buffer);

		struct CString uniform_name = {
			.data = uniform_name_buffer,
			.length = (uint32_t)name_length,
		};

		if (cstring_contains(uniform_name, S_("[0][0]"))) {
			// @todo: provide a convenient API for nested arrays in GLSL
			logger_to_console("nested arrays are not supported\n"); DEBUG_BREAK();
			continue;
		}

		if (cstring_contains(uniform_name, S_("[0]."))) {
			// @todo: provide a convenient API for array of structs in GLSL
			logger_to_console("arrays of structs are not supported\n"); DEBUG_BREAK();
			continue;
		}

		if (params[PARAM_ARRAY_SIZE] > 1) {
			uniform_name.length -= 3; // arrays have suffix `[0]`
		}

		uint32_t const id = strings_add(&gs_graphics_state.uniforms, uniform_name);
		hash_table_u32_set(&uniforms, id, &(struct Gpu_Program_Field_Internal){
			.base = {
				.type = interpret_gl_type(params[PARAM_TYPE]),
				.array_size = (uint32_t)params[PARAM_ARRAY_SIZE],
			},
			.location = params[PARAM_LOCATION],
		});
	}

	//
	return ref_table_aquire(&gs_graphics_state.programs, &(struct Gpu_Program){
		.id = program_id,
		.uniforms = uniforms, // @note: memory ownership transfer
	});
	// https://www.khronos.org/opengl/wiki/Program_Introspection

#undef ADD_HEADER
}

static void gpu_program_free_internal(struct Gpu_Program * gpu_program) {
	hash_table_u32_free(&gpu_program->uniforms);
	glDeleteProgram(gpu_program->id);
}

static void gpu_program_free_immediately(struct Ref gpu_program_ref) {
	if (ref_equals(gs_graphics_state.active.program, gpu_program_ref)) {
		gs_graphics_state.active.program = c_ref_empty;
	}
	struct Gpu_Program * gpu_program = ref_table_get(&gs_graphics_state.programs, gpu_program_ref);
	if (gpu_program != NULL) {
		gpu_program_free_internal(gpu_program);
		ref_table_discard(&gs_graphics_state.programs, gpu_program_ref);
	}
}

void gpu_program_free(struct Ref gpu_program_ref) {
	array_any_push_many(&gs_graphics_state.actions, 1, &(struct Graphics_Action){
		.gpu_ref = gpu_program_ref,
		.action = gpu_program_free_immediately,
	});
}

struct Hash_Table_U32 const * gpu_program_get_uniforms(struct Ref gpu_program_ref) {
	struct Gpu_Program const * gpu_program = ref_table_get(&gs_graphics_state.programs, gpu_program_ref);
	return (gpu_program != NULL) ? &gpu_program->uniforms : NULL;
}

// ----- ----- ----- ----- -----
//     GPU texture part
// ----- ----- ----- ----- -----

static struct Ref gpu_texture_allocate(
	uint32_t size_x, uint32_t size_y,
	struct Texture_Parameters parameters,
	struct Texture_Settings settings,
	void const * data
) {
	if (size_x > gs_graphics_state.limits.max_texture_size) {
		logger_to_console("requested size is too large\n"); DEBUG_BREAK();
		size_x = gs_graphics_state.limits.max_texture_size;
	}

	if (size_y > gs_graphics_state.limits.max_texture_size) {
		logger_to_console("requested size is too large\n"); DEBUG_BREAK();
		size_y = gs_graphics_state.limits.max_texture_size;
	}

	GLuint texture_id;
	glCreateTextures(GL_TEXTURE_2D, 1, &texture_id);

	// allocate buffer
	GLint const level = 0;
	if (data == NULL && !(parameters.flags & (TEXTURE_FLAG_WRITE | TEXTURE_FLAG_READ | TEXTURE_FLAG_INTERNAL))) {
		logger_to_console("non-internal storage should have initial data\n"); DEBUG_BREAK();
	}
	else if (parameters.flags & TEXTURE_FLAG_MUTABLE) {
		glBindTexture(GL_TEXTURE_2D, texture_id);
		glTexImage2D(
			GL_TEXTURE_2D, level,
			(GLint)gpu_sized_internal_format(parameters.texture_type, parameters.data_type),
			(GLsizei)size_x, (GLsizei)size_y, 0,
			gpu_pixel_data_format(parameters.texture_type, parameters.data_type),
			gpu_pixel_data_type(parameters.texture_type, parameters.data_type),
			data
		);
	}
	else if (size_x > 0 && size_y > 0) {
		GLsizei const levels = 1;
		glTextureStorage2D(
			texture_id, levels,
			gpu_sized_internal_format(parameters.texture_type, parameters.data_type),
			(GLsizei)size_x, (GLsizei)size_y
		);
		if (data != NULL) {
			glTextureSubImage2D(
				texture_id, level,
				0, 0, (GLsizei)size_x, (GLsizei)size_y,
				gpu_pixel_data_format(parameters.texture_type, parameters.data_type),
				gpu_pixel_data_type(parameters.texture_type, parameters.data_type),
				data
			);
		}
	}
	else {
		logger_to_console("immutable storage should have non-zero size\n"); DEBUG_BREAK();
	}

	// chart buffer
	glTextureParameteri(texture_id, GL_TEXTURE_MIN_FILTER, gpu_min_filter_mode(settings.mipmap, settings.minification));
	glTextureParameteri(texture_id, GL_TEXTURE_MAG_FILTER, gpu_mag_filter_mode(settings.magnification));
	glTextureParameteri(texture_id, GL_TEXTURE_WRAP_S, gpu_wrap_mode(settings.wrap_x));
	glTextureParameteri(texture_id, GL_TEXTURE_WRAP_T, gpu_wrap_mode(settings.wrap_y));

	glTextureParameteriv(texture_id, GL_TEXTURE_SWIZZLE_RGBA, (GLint[]){
		gpu_swizzle_op(settings.swizzle[0], 0),
		gpu_swizzle_op(settings.swizzle[1], 1),
		gpu_swizzle_op(settings.swizzle[2], 2),
		gpu_swizzle_op(settings.swizzle[3], 3),
	});

	//
	return ref_table_aquire(&gs_graphics_state.textures, &(struct Gpu_Texture){
		.id = texture_id,
		.size_x = size_x,
		.size_y = size_y,
		.parameters = parameters,
		.settings = settings,
	});
}

struct Ref gpu_texture_init(struct Image const * asset) {
	struct Ref const gpu_texture_ref = gpu_texture_allocate(
		asset->size_x, asset->size_y, asset->parameters, asset->settings, asset->data
	);

	gpu_texture_update(gpu_texture_ref, asset);

	return gpu_texture_ref;
}

static void gpu_texture_free_internal(struct Gpu_Texture * gpu_texture) {
	glDeleteTextures(1, &gpu_texture->id);
}

static void gpu_texture_free_immediately(struct Ref gpu_texture_ref) {
	for (uint32_t i = 1; i < gs_graphics_state.units_capacity; i++) {
		struct Gpu_Unit * unit = gs_graphics_state.units + i;
		if (ref_equals(unit->texture, gpu_texture_ref)) {
			unit->texture = c_ref_empty;
		}
	}
	struct Gpu_Texture * gpu_texture = ref_table_get(&gs_graphics_state.textures, gpu_texture_ref);
	if (gpu_texture != NULL) {
		gpu_texture_free_internal(gpu_texture);
		ref_table_discard(&gs_graphics_state.textures, gpu_texture_ref);
	}
}

void gpu_texture_free(struct Ref gpu_texture_ref) {
	array_any_push_many(&gs_graphics_state.actions, 1, &(struct Graphics_Action){
		.gpu_ref = gpu_texture_ref,
		.action = gpu_texture_free_immediately,
	});
}

void gpu_texture_get_size(struct Ref gpu_texture_ref, uint32_t * x, uint32_t * y) {
	struct Gpu_Texture const * gpu_texture = ref_table_get(&gs_graphics_state.textures, gpu_texture_ref);
	if (gpu_texture != NULL) {
		*x = gpu_texture->size_x;
		*y = gpu_texture->size_y;
	}
}

void gpu_texture_update(struct Ref gpu_texture_ref, struct Image const * asset) {
	struct Gpu_Texture * gpu_texture = ref_table_get(&gs_graphics_state.textures, gpu_texture_ref);
	if (gpu_texture == NULL) { return; }

	// @todo: compare texture and asset parameters?

	if (asset->data == NULL) { return; }
	if (asset->size_x == 0) { return; }
	if (asset->size_y == 0) { return; }

	GLint const level = 0;
	if (gpu_texture->size_x >= asset->size_x && gpu_texture->size_y >= asset->size_y) {
		glTextureSubImage2D(
			gpu_texture->id, level,
			0, 0, (GLsizei)asset->size_x, (GLsizei)asset->size_y,
			gpu_pixel_data_format(asset->parameters.texture_type, asset->parameters.data_type),
			gpu_pixel_data_type(asset->parameters.texture_type, asset->parameters.data_type),
			asset->data
		);
	}
	else if (gpu_texture->parameters.flags & TEXTURE_FLAG_MUTABLE) {
		// logger_to_console("WARNING! reallocating a buffer\n");
		gpu_texture->size_x = asset->size_x;
		gpu_texture->size_y = asset->size_y;
		glBindTexture(GL_TEXTURE_2D, gpu_texture->id);
		glTexImage2D(
			GL_TEXTURE_2D, level,
			(GLint)gpu_sized_internal_format(gpu_texture->parameters.texture_type, gpu_texture->parameters.data_type),
			(GLsizei)asset->size_x, (GLsizei)asset->size_y, 0,
			gpu_pixel_data_format(asset->parameters.texture_type, asset->parameters.data_type),
			gpu_pixel_data_type(asset->parameters.texture_type, asset->parameters.data_type),
			asset->data
		);
	}
	else {
		logger_to_console("trying to reallocate an immutable buffer"); DEBUG_BREAK();
		// @todo: completely recreate object instead of using a mutable storage?
	}
}

// ----- ----- ----- ----- -----
//     GPU target part
// ----- ----- ----- ----- -----

struct Ref gpu_target_init(
	uint32_t size_x, uint32_t size_y,
	uint32_t parameters_count,
	struct Texture_Parameters const * parameters
) {
	GLuint target_id;
	glCreateFramebuffers(1, &target_id);

	struct Array_Any textures = array_any_init(sizeof(struct Gpu_Target_Texture));
	struct Array_Any buffers  = array_any_init(sizeof(struct Gpu_Target_Buffer));

	// allocate exact space for the buffers
	uint32_t textures_count = 0;
	for (uint32_t i = 0; i < parameters_count; i++) {
		if (parameters[i].flags & TEXTURE_FLAG_READ) {
			textures_count++;
		}
	}
	uint32_t buffers_count = parameters_count - textures_count;

	array_any_resize(&textures, textures_count);
	array_any_resize(&buffers,  buffers_count);

	// allocate buffers
	for (uint32_t i = 0, color_index = 0; i < parameters_count; i++) {
		bool const is_color = (parameters[i].texture_type == TEXTURE_TYPE_COLOR);
		if (parameters[i].flags & TEXTURE_FLAG_READ) {
			// @idea: expose settings
			struct Ref const gpu_texture_ref = gpu_texture_allocate(size_x, size_y, parameters[i], (struct Texture_Settings){
				.wrap_x = WRAP_MODE_EDGE,
				.wrap_y = WRAP_MODE_EDGE,
			}, NULL);
			array_any_push_many(&textures, 1, &(struct Gpu_Target_Texture){
				.texture = gpu_texture_ref,
				.drawbuffer = is_color ? color_index : 0,
			});
		}
		else {
			GLuint buffer_id;
			glCreateRenderbuffers(1, &buffer_id);
			glNamedRenderbufferStorage(
				buffer_id,
				gpu_sized_internal_format(parameters[i].texture_type, parameters[i].data_type),
				(GLsizei)size_x, (GLsizei)size_y
			);
			array_any_push_many(&buffers, 1, &(struct Gpu_Target_Buffer){
				.id = buffer_id,
				.parameters = parameters[i],
				.drawbuffer = is_color ? color_index : 0,
			});
		}

		if (is_color) { color_index++; }
	}

	// chart buffers
	for (uint32_t i = 0, texture_index = 0, buffer_index = 0, color_index = 0; i < parameters_count; i++) {
		bool const is_color = (parameters[i].texture_type == TEXTURE_TYPE_COLOR);
		if (parameters[i].flags & TEXTURE_FLAG_READ) {
			struct Gpu_Target_Texture const * texture = array_any_at(&textures, texture_index++);
			struct Gpu_Texture const * gpu_texture = ref_table_get(&gs_graphics_state.textures, texture->texture);
			if (gpu_texture != NULL) {
				GLint const level = 0;
				glNamedFramebufferTexture(
					target_id,
					gpu_attachment_point(parameters[i].texture_type, color_index),
					gpu_texture->id,
					level
				);
			}
		}
		else {
			struct Gpu_Target_Buffer const * buffer = array_any_at(&buffers, buffer_index++);
			glNamedFramebufferRenderbuffer(
				target_id,
				gpu_attachment_point(parameters[i].texture_type, color_index),
				GL_RENDERBUFFER,
				buffer->id
			);
		}

		if (is_color) { color_index++; }
	}

	//
	return ref_table_aquire(&gs_graphics_state.targets, &(struct Gpu_Target){
		.id = target_id,
		.size_x = size_x,
		.size_y = size_y,
		.textures = textures, // @note: memory ownership transfer
		.buffers = buffers, // @note: memory ownership transfer
	});
}

static void gpu_target_free_internal(struct Gpu_Target * gpu_target) {
	for (uint32_t i = 0; i < gpu_target->textures.count; i++) {
		struct Gpu_Target_Texture const * entry = array_any_at(&gpu_target->textures, i);
		gpu_texture_free(entry->texture);
	}
	for (uint32_t i = 0; i < gpu_target->buffers.count; i++) {
		struct Gpu_Target_Buffer const * entry = array_any_at(&gpu_target->buffers, i);
		glDeleteRenderbuffers(1, &entry->id);
	}
	array_any_free(&gpu_target->textures);
	array_any_free(&gpu_target->buffers);
	glDeleteFramebuffers(1, &gpu_target->id);
}

static void gpu_target_free_immediately(struct Ref gpu_target_ref) {
	if (ref_equals(gs_graphics_state.active.target, gpu_target_ref)) {
		gs_graphics_state.active.target = c_ref_empty;
	}
	struct Gpu_Target * gpu_target = ref_table_get(&gs_graphics_state.targets, gpu_target_ref);
	if (gpu_target != NULL) {
		gpu_target_free_internal(gpu_target);
		ref_table_discard(&gs_graphics_state.targets, gpu_target_ref);
	}
}

void gpu_target_free(struct Ref gpu_target_ref) {
	array_any_push_many(&gs_graphics_state.actions, 1, &(struct Graphics_Action){
		.gpu_ref = gpu_target_ref,
		.action = gpu_target_free_immediately,
	});
}

void gpu_target_get_size(struct Ref gpu_target_ref, uint32_t * x, uint32_t * y) {
	struct Gpu_Target const * gpu_target = ref_table_get(&gs_graphics_state.targets, gpu_target_ref);
	if (gpu_target != NULL) {
		*x = gpu_target->size_x;
		*y = gpu_target->size_y;
	}
}

struct Ref gpu_target_get_texture_ref(struct Ref gpu_target_ref, enum Texture_Type type, uint32_t index) {
	struct Gpu_Target const * gpu_target = ref_table_get(&gs_graphics_state.targets, gpu_target_ref);
	if (gpu_target == NULL) { return c_ref_empty; }
	for (uint32_t i = 0, color_index = 0; i < gpu_target->textures.count; i++) {
		struct Gpu_Target_Texture const * texture = array_any_at(&gpu_target->textures, i);
		struct Gpu_Texture const * gpu_texture = ref_table_get(&gs_graphics_state.textures, texture->texture);
		if (gpu_texture->parameters.texture_type == type) {
			if (type == TEXTURE_TYPE_COLOR && color_index != index) { color_index++; continue; }
			return texture->texture;
		}
	}

	logger_to_console("failure: target doesn't have requested texture\n"); DEBUG_BREAK();
	return c_ref_empty;
}

// ----- ----- ----- ----- -----
//     GPU mesh part
// ----- ----- ----- ----- -----

static struct Ref gpu_mesh_allocate(
	uint32_t buffers_count,
	struct Buffer * asset_buffers,
	struct Mesh_Parameters const * parameters_set
) {
	GLuint mesh_id;
	glCreateVertexArrays(1, &mesh_id);

	struct Array_Any buffers = array_any_init(sizeof(struct Gpu_Mesh_Buffer));
	array_any_resize(&buffers, buffers_count);

	// allocate buffers
	for (uint32_t i = 0; i < buffers_count; i++) {
		struct Buffer const * asset_buffer = asset_buffers + i;

		struct Mesh_Parameters const parameters = parameters_set[i];
		array_any_push_many(&buffers, 1, &(struct Gpu_Mesh_Buffer){
			.parameters = parameters,
		});

		struct Gpu_Mesh_Buffer * buffer = array_any_at(&buffers, i);
		glCreateBuffers(1, &buffer->id);

		if (asset_buffer->data == NULL && !(parameters.flags & (MESH_FLAG_WRITE | MESH_FLAG_READ | MESH_FLAG_INTERNAL))) {
			logger_to_console("non-internal storage should have initial data\n"); DEBUG_BREAK();
		}
		else if (parameters.flags & MESH_FLAG_MUTABLE) {
			glNamedBufferData(
				buffer->id,
				(GLsizeiptr)asset_buffer->count, asset_buffer->data,
				gpu_mesh_usage_pattern(parameters.flags)
			);
		}
		else if (asset_buffer->count != 0) {
			glNamedBufferStorage(
				buffer->id,
				(GLsizeiptr)asset_buffer->count, asset_buffer->data,
				gpu_mesh_immutable_flag(parameters.flags)
			);
		}
		else {
			logger_to_console("immutable storage should have non-zero size\n"); DEBUG_BREAK();
			continue;
		}

		// @note: deliberately using count instead of capacity, so as to drop junk data
		buffer->capacity = (uint32_t)asset_buffer->count / data_type_get_size(parameters.type);
		buffer->count = (asset_buffer->data != NULL) ? buffer->capacity : 0;
	}

	// chart buffers
	for (uint32_t i = 0; i < buffers_count; i++) {
		struct Gpu_Mesh_Buffer const * buffer = array_any_at(&buffers, i);
		struct Mesh_Parameters const parameters = buffer->parameters;

		// element buffer
		if (parameters.flags & MESH_FLAG_INDEX) {
			glVertexArrayElementBuffer(mesh_id, buffer->id);
			continue;
		}

		// vertex buffer
		uint32_t all_attributes_size = 0;
		for (uint32_t atti = 0; atti < parameters.attributes_count; atti++) {
			all_attributes_size += parameters.attributes[atti * 2 + 1] * data_type_get_size(parameters.type);
		}

		GLuint buffer_index = 0;
		GLintptr buffer_start = 0;
		glVertexArrayVertexBuffer(mesh_id, buffer_index, buffer->id, buffer_start, (GLsizei)all_attributes_size);

		uint32_t attribute_offset = 0;
		for (uint32_t atti = 0; atti < parameters.attributes_count; atti++) {
			GLuint location = (GLuint)parameters.attributes[atti * 2];
			uint32_t size = parameters.attributes[atti * 2 + 1];
			glEnableVertexArrayAttrib(mesh_id, location);
			glVertexArrayAttribBinding(mesh_id, location, buffer_index);
			glVertexArrayAttribFormat(
				mesh_id, location,
				(GLint)size, gpu_data_type(parameters.type),
				GL_FALSE, (GLuint)attribute_offset
			);
			attribute_offset += size * data_type_get_size(parameters.type);
		}
	}

	//
	uint32_t elements_index = INDEX_EMPTY;
	for (uint32_t i = 0; i < buffers_count; i++) {
		struct Gpu_Mesh_Buffer const * buffer = array_any_at(&buffers, i);
		struct Mesh_Parameters const parameters = buffer->parameters;
		if (parameters.flags & MESH_FLAG_INDEX) {
			elements_index = i;
			break;
		}
	}
	if (elements_index == INDEX_EMPTY) {
		logger_to_console("no element buffer\n"); DEBUG_BREAK();
	}

	return ref_table_aquire(&gs_graphics_state.meshes, &(struct Gpu_Mesh){
		.id = mesh_id,
		.buffers = buffers,
		.elements_index = elements_index,
	});
}

struct Ref gpu_mesh_init(struct Mesh const * asset) {
	struct Ref const gpu_mesh_ref = gpu_mesh_allocate(
		asset->count, asset->buffers, asset->parameters
	);

	gpu_mesh_update(gpu_mesh_ref, asset);
	return gpu_mesh_ref;
}

static void gpu_mesh_free_internal(struct Gpu_Mesh * gpu_mesh) {
	for (uint32_t i = 0; i < gpu_mesh->buffers.count; i++) {
		struct Gpu_Mesh_Buffer const * buffer = array_any_at(&gpu_mesh->buffers, i);
		glDeleteBuffers(1, &buffer->id);
	}
	array_any_free(&gpu_mesh->buffers);
	glDeleteVertexArrays(1, &gpu_mesh->id);
}

static void gpu_mesh_free_immediately(struct Ref gpu_mesh_ref) {
	if (ref_equals(gs_graphics_state.active.mesh, gpu_mesh_ref)) {
		gs_graphics_state.active.mesh = c_ref_empty;
	}
	struct Gpu_Mesh * gpu_mesh = ref_table_get(&gs_graphics_state.meshes, gpu_mesh_ref);
	if (gpu_mesh != NULL) {
		gpu_mesh_free_internal(gpu_mesh);
		ref_table_discard(&gs_graphics_state.meshes, gpu_mesh_ref);
	}
}

void gpu_mesh_free(struct Ref gpu_mesh_ref) {
	array_any_push_many(&gs_graphics_state.actions, 1, &(struct Graphics_Action){
		.gpu_ref = gpu_mesh_ref,
		.action = gpu_mesh_free_immediately,
	});
}

void gpu_mesh_update(struct Ref gpu_mesh_ref, struct Mesh const * asset) {
	struct Gpu_Mesh * gpu_mesh = ref_table_get(&gs_graphics_state.meshes, gpu_mesh_ref);
	if (gpu_mesh == NULL) { return; }
	for (uint32_t i = 0; i < gpu_mesh->buffers.count; i++) {
		struct Gpu_Mesh_Buffer * buffer = array_any_at(&gpu_mesh->buffers, i);
		struct Mesh_Parameters const parameters = buffer->parameters;
		// @todo: compare mesh and asset parameters?
		// struct Mesh_Parameters const * asset_parameters = asset->parameters + i;

		if (!(parameters.flags & MESH_FLAG_WRITE)) { continue; }

		struct Buffer const * asset_buffer = asset->buffers + i;
		if (asset_buffer->count == 0) { continue; }
		if (asset_buffer->data == NULL) { continue; }

		uint32_t const data_type_size = data_type_get_size(parameters.type);
		uint32_t const elements_count = (uint32_t)asset_buffer->count / data_type_size;

		if (buffer->capacity >= elements_count) {
			glNamedBufferSubData(
				buffer->id, 0,
				(GLsizeiptr)asset_buffer->count,
				asset_buffer->data
			);
		}
		else if (parameters.flags & MESH_FLAG_MUTABLE) {
			// logger_to_console("WARNING! reallocating a buffer\n");
			buffer->capacity = elements_count;
			glNamedBufferData(
				buffer->id,
				(GLsizeiptr)asset_buffer->count, asset_buffer->data,
				gpu_mesh_usage_pattern(parameters.flags)
			);
		}
		else {
			// @todo: completely recreate object instead of using a mutable storage?
			logger_to_console("trying to reallocate an immutable buffer"); DEBUG_BREAK();
			continue;
		}

		buffer->count = elements_count;
	}
}

//
#include "framework/graphics/gpu_misc.h"

void graphics_update(void) {
	for (uint32_t i = 0; i < gs_graphics_state.actions.count; i++) {
		struct Graphics_Action const * it = array_any_at(&gs_graphics_state.actions, i);
		it->action(it->gpu_ref);
	}
	array_any_clear(&gs_graphics_state.actions);
}

uint32_t graphics_add_uniform_id(struct CString name) {
	return strings_add(&gs_graphics_state.uniforms, name);
}

uint32_t graphics_find_uniform_id(struct CString name) {
	return strings_find(&gs_graphics_state.uniforms, name);
}

struct CString graphics_get_uniform_value(uint32_t value) {
	return strings_get(&gs_graphics_state.uniforms, value);
}

// static void graphics_stencil_test(void) {
// 	enum Comparison_Op comparison_op = COMPARISON_OP_NONE;
// 	uint32_t comparison_ref = 0;
// 	uint32_t comparison_mask = 0;
// 	enum Stencil_Op fail_fail = STENCIL_OP_ZERO;
// 	enum Stencil_Op succ_fail = STENCIL_OP_ZERO;
// 	enum Stencil_Op succ_succ = STENCIL_OP_ZERO;
// 	uint32_t write_mask = 0;
// 
// 	//
// 	if (comparison_op == COMPARISON_OP_NONE) {
// 		glDisable(GL_STENCIL_TEST);
// 	}
// 	else {
// 		glEnable(GL_STENCIL_TEST);
// 
// 		glStencilMask((GLuint)write_mask);
// 
// 		glStencilFunc(gpu_comparison_op(comparison_op), (GLint)comparison_ref, (GLuint)comparison_mask);
// 
// 		glStencilOp(
// 			gpu_stencil_op(fail_fail),
// 			gpu_stencil_op(succ_fail),
// 			gpu_stencil_op(succ_succ)
// 		);
// 	}
// }

static uint32_t graphics_unit_find(struct Ref gpu_texture_ref) {
	// @note: consider `id == 0` empty
	for (uint32_t i = 1; i < gs_graphics_state.units_capacity; i++) {
		struct Gpu_Unit const * unit = gs_graphics_state.units + i;
		if (unit->texture.id != gpu_texture_ref.id) { continue; }
		if (unit->texture.gen != gpu_texture_ref.gen) { continue; }
		return i;
	}
	return 0;
}

static uint32_t gpu_unit_init(struct Ref gpu_texture_ref) {
	uint32_t const existing_unit = graphics_unit_find(gpu_texture_ref);
	if (existing_unit != 0) { return existing_unit; }

	uint32_t const free_unit = graphics_unit_find(c_ref_empty);
	if (free_unit == 0) {
		logger_to_console("failure: no spare texture/sampler units\n"); DEBUG_BREAK();
		return 0;
	}

	struct Gpu_Texture const * gpu_texture = ref_table_get(&gs_graphics_state.textures, gpu_texture_ref);
	if (gpu_texture == NULL) {
		logger_to_console("failure: no spare texture/sampler units\n"); DEBUG_BREAK();
		return 0;
	}

	gs_graphics_state.units[free_unit] = (struct Gpu_Unit){
		.texture = gpu_texture_ref,
	};

	glBindTextureUnit((GLuint)free_unit, gpu_texture->id);
	return free_unit;
}

// static void gpu_unit_free(struct Ref gpu_texture) {
// 	if (gs_ogl_version > 0) {
// 		uint32_t unit = graphics_unit_find(gpu_texture);
// 		if (unit == 0) { return; }
// 
// 		gs_graphics_state.units[unit] = (struct Gpu_Unit){
// 			.gpu_texture_ref = c_ref_empty,
// 		};
// 
// 		glBindTextureUnit((GLuint)unit, 0);
// 	}
// }

static void gpu_select_program(struct Ref gpu_program_ref) {
	if (ref_equals(gs_graphics_state.active.program, gpu_program_ref)) { return; }
	gs_graphics_state.active.program = gpu_program_ref;
	struct Gpu_Program const * gpu_program = ref_table_get(&gs_graphics_state.programs, gpu_program_ref);
	glUseProgram((gpu_program != NULL) ? gpu_program->id : 0);
}

static void gpu_select_target(struct Ref gpu_target_ref) {
	if (ref_equals(gs_graphics_state.active.target, gpu_target_ref)) { return; }
	gs_graphics_state.active.target = gpu_target_ref;
	struct Gpu_Target const * gpu_target = ref_table_get(&gs_graphics_state.targets, gpu_target_ref);
	glBindFramebuffer(GL_FRAMEBUFFER, (gpu_target != NULL) ? gpu_target->id : 0);
}

static void gpu_select_mesh(struct Ref gpu_mesh_ref) {
	if (ref_equals(gs_graphics_state.active.mesh, gpu_mesh_ref)) { return; }
	gs_graphics_state.active.mesh = gpu_mesh_ref;
	struct Gpu_Mesh const * gpu_mesh = ref_table_get(&gs_graphics_state.meshes, gpu_mesh_ref);
	glBindVertexArray((gpu_mesh != NULL) ? gpu_mesh->id : 0);
}

static void gpu_upload_single_uniform(struct Gpu_Program const * gpu_program, struct Gpu_Program_Field_Internal const * field, void const * data) {
	switch (field->base.type) {
		default: logger_to_console("unsupported field type '0x%x'\n", field->base.type); DEBUG_BREAK(); break;

		case DATA_TYPE_UNIT_U:
		case DATA_TYPE_UNIT_S:
		case DATA_TYPE_UNIT_F: {
			GLint * units = alloca(sizeof(GLint) * field->base.array_size);
			uint32_t units_count = 0;

			// @todo: automatically rebind in a circular buffer manner
			struct Ref const * gpu_texture_refs = (struct Ref const *)data;
			for (uint32_t i = 0; i < field->base.array_size; i++) {
				uint32_t unit = graphics_unit_find(gpu_texture_refs[i]);
				if (unit == 0) {
					unit = gpu_unit_init(gpu_texture_refs[i]);
				}
				units[units_count++] = (GLint)unit;
			}
			glProgramUniform1iv(gpu_program->id, field->location, (GLsizei)field->base.array_size, units);
			break;
		}

		case DATA_TYPE_R32_U:    glProgramUniform1uiv(gpu_program->id, field->location, (GLsizei)field->base.array_size, data); break;
		case DATA_TYPE_RG32_U:   glProgramUniform2uiv(gpu_program->id, field->location, (GLsizei)field->base.array_size, data); break;
		case DATA_TYPE_RGB32_U:  glProgramUniform3uiv(gpu_program->id, field->location, (GLsizei)field->base.array_size, data); break;
		case DATA_TYPE_RGBA32_U: glProgramUniform4uiv(gpu_program->id, field->location, (GLsizei)field->base.array_size, data); break;

		case DATA_TYPE_R32_S:    glProgramUniform1iv(gpu_program->id, field->location, (GLsizei)field->base.array_size, data); break;
		case DATA_TYPE_RG32_S:   glProgramUniform2iv(gpu_program->id, field->location, (GLsizei)field->base.array_size, data); break;
		case DATA_TYPE_RGB32_S:  glProgramUniform3iv(gpu_program->id, field->location, (GLsizei)field->base.array_size, data); break;
		case DATA_TYPE_RGBA32_S: glProgramUniform4iv(gpu_program->id, field->location, (GLsizei)field->base.array_size, data); break;

		case DATA_TYPE_R32_F:    glProgramUniform1fv(gpu_program->id, field->location, (GLsizei)field->base.array_size, data); break;
		case DATA_TYPE_RG32_F:   glProgramUniform2fv(gpu_program->id, field->location, (GLsizei)field->base.array_size, data); break;
		case DATA_TYPE_RGB32_F:  glProgramUniform3fv(gpu_program->id, field->location, (GLsizei)field->base.array_size, data); break;
		case DATA_TYPE_RGBA32_F: glProgramUniform4fv(gpu_program->id, field->location, (GLsizei)field->base.array_size, data); break;

		case DATA_TYPE_MAT2: glProgramUniformMatrix2fv(gpu_program->id, field->location, (GLsizei)field->base.array_size, GL_FALSE, data); break;
		case DATA_TYPE_MAT3: glProgramUniformMatrix3fv(gpu_program->id, field->location, (GLsizei)field->base.array_size, GL_FALSE, data); break;
		case DATA_TYPE_MAT4: glProgramUniformMatrix4fv(gpu_program->id, field->location, (GLsizei)field->base.array_size, GL_FALSE, data); break;
	}
}

static void gpu_upload_uniforms(struct Gpu_Program const * gpu_program, struct Gfx_Uniforms const * uniforms, uint32_t offset, uint32_t count) {
	for (uint32_t i = offset, last = offset + count; i < last; i++) {
		struct Gfx_Uniforms_Entry const * entry = array_any_at(&uniforms->headers, i);

		struct Gpu_Program_Field_Internal const * uniform = hash_table_u32_get(&gpu_program->uniforms, entry->id);
		if (uniform == NULL) { continue; }

		if (data_type_get_size(uniform->base.type) * uniform->base.array_size != entry->size) { continue; }

		gpu_upload_single_uniform(gpu_program, uniform, (uint8_t *)uniforms->payload.data + entry->offset);
	}
}

static void gpu_set_blend_mode(enum Blend_Mode mode) {
	if (mode == BLEND_MODE_NONE) {
		glDisable(GL_BLEND);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	}
	else {
		struct Gpu_Blend_Mode const gpu_mode = gpu_blend_mode(mode);
		glEnable(GL_BLEND);
		glColorMask(gpu_mode.mask[0], gpu_mode.mask[1], gpu_mode.mask[2], gpu_mode.mask[3]);
		glBlendEquationSeparate(gpu_mode.color_op, gpu_mode.alpha_op);
		glBlendFuncSeparate(
			gpu_mode.color_src, gpu_mode.color_dst,
			gpu_mode.alpha_src, gpu_mode.alpha_dst
		);
		// glBlendColor(mode.color.x, mode.color.y, mode.color.z, mode.color.w);
	}
}

static void gpu_set_depth_mode(enum Depth_Mode mode) {
	if (mode == DEPTH_MODE_NONE) {
		glDisable(GL_DEPTH_TEST);
	}
	else {
		bool const reversed_z = (gs_graphics_state.clip_space.ncp > gs_graphics_state.clip_space.fcp);
		struct Gpu_Depth_Mode const gpu_mode = gpu_depth_mode(mode, reversed_z);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(gpu_mode.mask);
		glDepthFunc(gpu_mode.op);
	}
}

/*
// @note: lacks precision past [FLOAT_INT_MIN .. FLOAT_INT_MAX]
//        but does it really matter?
inline static void gpu_clear_target_color_map01(GLuint target_id, struct Texture_Parameters parameters, uint32_t drawbuffer, struct vec4 color) {
	switch (data_type_get_element_type(parameters.data_type)) {
		case DATA_TYPE_R8_U: glClearNamedFramebufferuiv(target_id, GL_COLOR, (GLint)drawbuffer, (GLuint[]){
			map01_to_u8(color.x),  map01_to_u8(color.y),  map01_to_u8(color.z),  map01_to_u8(color.w),
		}); break;

		case DATA_TYPE_R16_U: glClearNamedFramebufferuiv(target_id, GL_COLOR, (GLint)drawbuffer, (GLuint[]){
			map01_to_u16(color.x), map01_to_u16(color.y), map01_to_u16(color.z), map01_to_u16(color.w),
		}); break;

		case DATA_TYPE_R32_U: glClearNamedFramebufferuiv(target_id, GL_COLOR, (GLint)drawbuffer, (GLuint[]){
			map01_to_u32(color.x), map01_to_u32(color.y), map01_to_u32(color.z), map01_to_u32(color.w),
		}); break;

		case DATA_TYPE_R8_S: glClearNamedFramebufferiv(target_id, GL_COLOR, (GLint)drawbuffer, (GLint[]){
			map01_to_s8(color.x),  map01_to_s8(color.y),  map01_to_s8(color.z),  map01_to_s8(color.w),
		}); break;

		case DATA_TYPE_R16_S: glClearNamedFramebufferiv(target_id, GL_COLOR, (GLint)drawbuffer, (GLint[]){
			map01_to_s16(color.x), map01_to_s16(color.y), map01_to_s16(color.z), map01_to_s16(color.w),
		}); break;

		case DATA_TYPE_R32_S: glClearNamedFramebufferiv(target_id, GL_COLOR, (GLint)drawbuffer, (GLint[]){
			map01_to_s32(color.x), map01_to_s32(color.y), map01_to_s32(color.z), map01_to_s32(color.w),
		}); break;

		default: {
			glClearNamedFramebufferfv(target_id, GL_COLOR, (GLint)drawbuffer, &color.x);
		} break;
	}
}
*/

/*
static void gpu_clear_target(GLuint target_id, struct Texture_Parameters parameters, uint32_t drawbuffer, void const * color) {
	float const depth   = gs_graphics_state.clip_space[3];
	GLint const stencil = 0;

	switch (parameters.texture_type) {
		case TEXTURE_TYPE_NONE: break;

		// @note: demands manual control over memory layout
		case TEXTURE_TYPE_COLOR: {
			switch (data_type_get_element_type(parameters.data_type)) {
				case DATA_TYPE_R8_U:
				case DATA_TYPE_R16_U:
				case DATA_TYPE_R32_U:
					glClearNamedFramebufferuiv(target_id, GL_COLOR, (GLint)drawbuffer, color);
					break;

				case DATA_TYPE_R8_S:
				case DATA_TYPE_R16_S:
				case DATA_TYPE_R32_S:
					glClearNamedFramebufferiv(target_id, GL_COLOR, (GLint)drawbuffer, color);
					break;

				default: {
					glClearNamedFramebufferfv(target_id, GL_COLOR, (GLint)drawbuffer, color);
				} break;
			}
		} break;

		case TEXTURE_TYPE_DEPTH: {
			glClearNamedFramebufferfv(target_id, GL_DEPTH, 0, &depth);
		} break;

		case TEXTURE_TYPE_STENCIL: {
			glClearNamedFramebufferiv(target_id, GL_STENCIL, 0, &stencil);
		} break;

		case TEXTURE_TYPE_DSTENCIL: {
			glClearNamedFramebufferfi(target_id, GL_DEPTH_STENCIL, 0, depth, stencil);
		} break;
	}
}
*/

/*
// @note: I'm actually quite ok with the classic and less verbose approach
//        but it's fun to sketch out and test ideas: integer formats in this instance

// @idea: each color attachment might have different color;
//        or may be API should prove draw buffer index explicitly

if (ref_equals(gs_graphics_state.active.target, c_ref_empty)) {
	struct Texture_Parameters const params[] = {
		(struct Texture_Parameters){
			.texture_type = TEXTURE_TYPE_COLOR,
			.data_type = DATA_TYPE_RGB8_UNORM,
		},
		(struct Texture_Parameters){ // @todo: sync with the actual backbuffer
			.texture_type = TEXTURE_TYPE_DSTENCIL,
			.data_type = DATA_TYPE_R32_F,
		},
	};

	for (uint32_t i = 0; i < SIZE_OF_ARRAY(params); i++) {
		if (!(command->mask & params[i].texture_type)) { continue; }
		gpu_clear_target(0, params[i], 0, &command->color);
	}
}
else {
	struct Gpu_Target * gpu_target = ref_table_get(&gs_graphics_state.targets, gs_graphics_state.active.target);
	for (uint32_t i = 0; i < gpu_target->textures.count; i++) {
		struct Gpu_Target_Texture const * entry = array_any_at(&gpu_target->textures, i);
		struct Gpu_Texture const * gpu_texture = ref_table_get(&gs_graphics_state.textures, entry->texture);
		if (!(command->mask & gpu_texture->parameters.texture_type)) { continue; }
		gpu_clear_target(gpu_target->id, gpu_texture->parameters, entry->drawbuffer, &command->color);
	}
	for (uint32_t i = 0; i < gpu_target->buffers.count; i++) {
		struct Gpu_Target_Buffer const * entry = array_any_at(&gpu_target->buffers, i);
		if (!(command->mask & entry->parameters.texture_type)) { continue; }
		gpu_clear_target(gpu_target->id, entry->parameters, entry->drawbuffer, &command->color);
	}
}
*/

//
#include "framework/graphics/gpu_command.h"

inline static void gpu_execute_cull(struct GPU_Command_Cull const * command) {
	if (command->mode == CULL_MODE_NONE) {
		glDisable(GL_CULL_FACE);
	}
	else {
		glEnable(GL_CULL_FACE);
		glCullFace(gpu_cull_mode(command->mode));
		glFrontFace(gpu_winding_order(command->order));
	}
}

inline static void gpu_execute_target(struct GPU_Command_Target const * command) {
	gpu_select_target(command->gpu_ref);

	uint32_t viewport_size_x = command->screen_size_x, viewport_size_y = command->screen_size_y;
	if (!ref_equals(command->gpu_ref, c_ref_empty)) {
		gpu_target_get_size(command->gpu_ref, &viewport_size_x, &viewport_size_y);
	}

	glViewport(0, 0, (GLsizei)viewport_size_x, (GLsizei)viewport_size_y);
}

inline static void gpu_execute_clear(struct GPU_Command_Clear const * command) {
	if (command->mask == TEXTURE_TYPE_NONE) { logger_to_console("clear mask is empty"); DEBUG_BREAK(); return; }

	float const depth   = gs_graphics_state.clip_space.fcp;
	GLint const stencil = 0;

	// @todo: ever need variations?
	gpu_set_blend_mode(BLEND_MODE_NONE);
	gpu_set_depth_mode(DEPTH_MODE_OPAQUE);

	GLbitfield clear_bitfield = 0;
	if (command->mask & TEXTURE_TYPE_COLOR)   { clear_bitfield |= GL_COLOR_BUFFER_BIT; }
	if (command->mask & TEXTURE_TYPE_DEPTH)   { clear_bitfield |= GL_DEPTH_BUFFER_BIT; }
	if (command->mask & TEXTURE_TYPE_STENCIL) { clear_bitfield |= GL_STENCIL_BUFFER_BIT; }
	
	glClearColor(command->color.x, command->color.y, command->color.z, command->color.w);
	glClearDepthf(depth);
	glClearStencil(stencil);
	glClear(clear_bitfield);
}

inline static void gpu_execute_material(struct GPU_Command_Material const * command) {
	if (command->material == NULL) { logger_to_console("material is null"); DEBUG_BREAK(); return; }
	if (ref_equals(command->material->gpu_program_ref, c_ref_empty)) { logger_to_console("program is null"); DEBUG_BREAK(); return; }

	struct Gpu_Program const * gpu_program = ref_table_get(&gs_graphics_state.programs, command->material->gpu_program_ref);
	if (gpu_program == NULL) { logger_to_console("program is null"); DEBUG_BREAK(); return; }

	gpu_set_blend_mode(command->material->blend_mode);
	gpu_set_depth_mode(command->material->depth_mode);
	gpu_select_program(command->material->gpu_program_ref);
	gpu_upload_uniforms(gpu_program, &command->material->uniforms, 0, command->material->uniforms.headers.count);
}

inline static void gpu_execute_uniform(struct GPU_Command_Uniform const * command) {
	if (ref_equals(command->gpu_program_ref, c_ref_empty)) {
		FOR_REF_TABLE (&gs_graphics_state.programs, it) {
			gpu_upload_uniforms(it.value, command->override.uniforms, command->override.offset, command->override.count);
		}
	}
	else {
		struct Gpu_Program const * gpu_program = ref_table_get(&gs_graphics_state.programs, command->gpu_program_ref);
		gpu_upload_uniforms(gpu_program, command->override.uniforms, command->override.offset, command->override.count);
	}
}

inline static void gpu_execute_draw(struct GPU_Command_Draw const * command) {
	if (ref_equals(command->gpu_mesh_ref, c_ref_empty)) { logger_to_console("mesh is null"); DEBUG_BREAK(); return; }
	struct Gpu_Mesh const * mesh = ref_table_get(&gs_graphics_state.meshes, command->gpu_mesh_ref);
	if (mesh == NULL) { logger_to_console("mesh is null"); DEBUG_BREAK(); return; }
	if (mesh->elements_index == INDEX_EMPTY) { logger_to_console("mesh has no elements buffer"); DEBUG_BREAK(); return; }

	//
	struct Gpu_Mesh_Buffer const * buffer = array_any_at(&mesh->buffers, mesh->elements_index);
	if (command->length == 0) {
		if (buffer == NULL) { return; }
		if (buffer->count == 0) { return; }
	}

	uint32_t const elements_count = (command->length != 0)
		? command->length
		: buffer->count;

	gpu_select_mesh(command->gpu_mesh_ref);

	if (buffer != NULL) {
		enum Data_Type const elements_type = buffer->parameters.type;
		uint32_t const elements_offset = (command->length != 0)
			? command->offset * data_type_get_size(elements_type)
			: 0;

		glDrawElements(
			GL_TRIANGLES,
			(GLsizei)elements_count,
			gpu_data_type(elements_type),
			(void const *)(size_t)elements_offset
		);
	}
	else {
		logger_to_console("not implemented"); DEBUG_BREAK();
		// @todo: implement
		// uint32_t const elements_offset = (command->length != 0)
		// 	? command->offset * 3
		// 	: 0;
		// 
		// glDrawArrays(
		// 	GL_TRIANGLES,
		// 	(GLint)elements_offset,
		// 	(GLsizei)elements_count
		// );
	}
}

void gpu_execute(uint32_t length, struct GPU_Command const * commands) {
	for (uint32_t i = 0; i < length; i++) {
		struct GPU_Command const * command = commands + i;
		switch (command->type) {
			default:                        logger_to_console("unknown command"); DEBUG_BREAK(); break;
			case GPU_COMMAND_TYPE_CULL:     gpu_execute_cull(&command->as.cull);         break;
			case GPU_COMMAND_TYPE_TARGET:   gpu_execute_target(&command->as.target);     break;
			case GPU_COMMAND_TYPE_CLEAR:    gpu_execute_clear(&command->as.clear);       break;
			case GPU_COMMAND_TYPE_MATERIAL: gpu_execute_material(&command->as.material); break;
			case GPU_COMMAND_TYPE_UNIFORM:  gpu_execute_uniform(&command->as.uniform);   break;
			case GPU_COMMAND_TYPE_DRAW:     gpu_execute_draw(&command->as.draw);         break;
		}
	}
}

//
#include "internal/graphics_to_gpu_library.h"

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
void graphics_to_gpu_library_init(void) {
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
	common_memset(&gs_graphics_state, 0, sizeof(gs_graphics_state));
	gs_graphics_state.extensions = allocate_extensions_string();

	gs_graphics_state.uniforms = strings_init();

	// init gpu objects
	gs_graphics_state.programs = ref_table_init(sizeof(struct Gpu_Program));
	gs_graphics_state.targets  = ref_table_init(sizeof(struct Gpu_Target));
	gs_graphics_state.textures = ref_table_init(sizeof(struct Gpu_Texture));
	gs_graphics_state.meshes   = ref_table_init(sizeof(struct Gpu_Mesh));

	gs_graphics_state.actions = array_any_init(sizeof(struct Graphics_Action));

	//
	GLint max_units;
	GLint max_units_vertex_shader, max_units_fragment_shader, max_units_compute_shader;
	GLint max_texture_size, max_renderbuffer_size;
	GLint max_elements_vertices, max_elements_indices;
	GLint max_uniform_locations;

	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_units);
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS,          &max_units_fragment_shader);
	glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS,   &max_units_vertex_shader);
	glGetIntegerv(GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS,  &max_units_compute_shader);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE,                 &max_texture_size);
	glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE,            &max_renderbuffer_size);
	glGetIntegerv(GL_MAX_ELEMENTS_VERTICES,            &max_elements_vertices);
	glGetIntegerv(GL_MAX_ELEMENTS_INDICES,             &max_elements_indices);
	glGetIntegerv(GL_MAX_UNIFORM_LOCATIONS,            &max_uniform_locations);

	logger_to_console(
		"> OpenGL limits:\n"
		"  units ......... %d\n"
		"  - FS .......... %d\n"
		"  - VS .......... %d\n"
		"  - CS .......... %d\n"
		"  texture size .. %d\n"
		"  target size ... %d\n"
		"  vertices ...... %d\n"
		"  indices ....... %d\n"
		"  uniforms ...... %d\n"
		"\n",
		max_units,
		max_units_fragment_shader,
		max_units_vertex_shader,
		max_units_compute_shader,
		max_texture_size,
		max_renderbuffer_size,
		max_elements_vertices,
		max_elements_indices,
		max_uniform_locations
	);

	gs_graphics_state.limits = (struct Graphics_State_Limits){
		.max_units_vertex_shader   = (uint32_t)max_units_vertex_shader,
		.max_units_fragment_shader = (uint32_t)max_units_fragment_shader,
		.max_units_compute_shader  = (uint32_t)max_units_compute_shader,
		.max_texture_size          = (uint32_t)max_texture_size,
		.max_renderbuffer_size     = (uint32_t)max_renderbuffer_size,
		.max_elements_vertices     = (uint32_t)max_elements_vertices,
		.max_elements_indices      = (uint32_t)max_elements_indices,
		.max_uniform_locations     = (uint32_t)max_uniform_locations,
	};

	gs_graphics_state.units_capacity = (uint32_t)max_units;
	gs_graphics_state.units = MEMORY_ALLOCATE_ARRAY(struct Gpu_Unit, max_units);
	common_memset(gs_graphics_state.units, 0, sizeof(* gs_graphics_state.units) * (size_t)max_units);

	// @note: manage OpenGL's clip space instead of ours
	bool const can_control_clip_space = (gs_ogl_version >= 45) || contains_full_word(gs_graphics_state.extensions, S_("GL_ARB_clip_control"));
	if (can_control_clip_space) { glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE); }

	gs_graphics_state.clip_space = (struct Gpu_Clip_Space){
		.origin = {
			0.0f,
			can_control_clip_space ? 0.0f : 1.0f,
		},
		.ncp = can_control_clip_space ? 1.0f : 0.0f,
		.fcp = can_control_clip_space ? 0.0f : 1.0f,
	};

	// glDepthRangef(gs_graphics_state.clip_space.ncp, gs_graphics_state.clip_space.fcp);
}

void graphics_to_gpu_library_free(void) {
#define GPU_FREE(data, action) do { \
	if (data.buffer.count > 0) { logger_to_console("dangling \"" #data "\": %u\n", data.buffer.count); } \
	FOR_REF_TABLE (&data, it) { action(it.value); } \
	ref_table_free(&data); \
} while (false)\

	GPU_FREE(gs_graphics_state.programs, gpu_program_free_internal);
	GPU_FREE(gs_graphics_state.targets,  gpu_target_free_internal);
	GPU_FREE(gs_graphics_state.textures, gpu_texture_free_internal);
	GPU_FREE(gs_graphics_state.meshes,   gpu_mesh_free_internal);

	//
	strings_free(&gs_graphics_state.uniforms);
	array_any_free(&gs_graphics_state.actions);
	MEMORY_FREE(gs_graphics_state.extensions);
	MEMORY_FREE(gs_graphics_state.units);
	common_memset(&gs_graphics_state, 0, sizeof(gs_graphics_state));

#undef GPU_FREE
}

//

static char * allocate_extensions_string(void) {
	// @note: used as a convenience
	struct Buffer string = buffer_init(NULL);

	GLint extensions_count = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &extensions_count);

	// @note: heuristically reserve the buffer memory
	buffer_resize(&string, (uint32_t)(extensions_count * 26));
	for(GLint i = 0; i < extensions_count; i++) {
		GLubyte const * value = glGetStringi(GL_EXTENSIONS, (GLuint)i);
		buffer_push_many(&string, (uint32_t)strlen((char const *)value), value);
		buffer_push_many(&string, 1, " ");
	}
	buffer_push_many(&string, 1, "\0");

	// @note: memory ownership transfer
	return (char *)string.data;
}

//

static void verify_shader(GLuint id) {
	GLint status;
	glGetShaderiv(id, GL_COMPILE_STATUS, &status);
	if (status) { return; }

	GLint max_length;
	glGetShaderiv(id, GL_INFO_LOG_LENGTH, &max_length);

	if (max_length > 0) {
		// @todo: (?) arena/stack allocator
		GLchar * buffer = alloca(sizeof(GLchar) * (size_t)max_length);
		glGetShaderInfoLog(id, max_length, &max_length, buffer);
		logger_to_console("%s\n", buffer);
	}

	DEBUG_BREAK();
}

static void verify_program(GLuint id) {
	GLint status;
	glGetProgramiv(id, GL_LINK_STATUS, &status);
	if (status) { return; }

	GLint max_length;
	glGetProgramiv(id, GL_INFO_LOG_LENGTH, &max_length);

	if (max_length > 0) {
		// @todo: (?) arena/stack allocator
		GLchar * buffer = alloca(sizeof(GLchar) * (size_t)max_length);
		glGetProgramInfoLog(id, max_length, &max_length, buffer);
		logger_to_console("%s\n", buffer);
	}

	DEBUG_BREAK();
}

//

static char const * opengl_debug_get_severity(GLenum value) {
	switch (value) {
		case GL_DEBUG_SEVERITY_HIGH:         return "[crt]"; // All OpenGL Errors, shader compilation/linking errors, or highly-dangerous undefined behavior
		case GL_DEBUG_SEVERITY_MEDIUM:       return "[err]"; // Major performance warnings, shader compilation/linking warnings, or the use of deprecated functionality
		case GL_DEBUG_SEVERITY_LOW:          return "[wrn]"; // Redundant state change performance warning, or unimportant undefined behavior
		case GL_DEBUG_SEVERITY_NOTIFICATION: return "[trc]"; // Anything that isn't an error or performance issue.
	}
	return "[???]";
}

static char const * opengl_debug_get_source(GLenum value) {
	switch (value) {
		case GL_DEBUG_SOURCE_API:             return "API";
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   return "window system";
		case GL_DEBUG_SOURCE_SHADER_COMPILER: return "shader compiler";
		case GL_DEBUG_SOURCE_THIRD_PARTY:     return "thirdparty";
		case GL_DEBUG_SOURCE_APPLICATION:     return "application";
		case GL_DEBUG_SOURCE_OTHER:           return "other";
	}
	return "unknown";
}

static char const * opengl_debug_get_type(GLenum value) {
	switch (value) {
		case GL_DEBUG_TYPE_ERROR:               return "error";
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "deprecated behavior";
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  return "undefined behavior";
		case GL_DEBUG_TYPE_PORTABILITY:         return "portability";
		case GL_DEBUG_TYPE_PERFORMANCE:         return "performance";
		case GL_DEBUG_TYPE_MARKER:              return "marker";
		case GL_DEBUG_TYPE_PUSH_GROUP:          return "push group";
		case GL_DEBUG_TYPE_POP_GROUP:           return "pop group";
		case GL_DEBUG_TYPE_OTHER:               return "other";
	}
	return "unknown";
}

static void __stdcall opengl_debug_message_callback(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar *message,
	const void *userParam
) {
#define STACKTRACE_OFFSET 4

	(void)userParam;
	logger_to_console(
		"> OpenGL message '0x%x'\n"
		"  severity: %s\n"
		"  source:   %s\n"
		"  type:     %s\n"
		"  message:  %.*s\n"
		"\n",
		id,
		opengl_debug_get_severity(severity),
		opengl_debug_get_source(source),
		opengl_debug_get_type(type),
		length, message
	);
	REPORT_CALLSTACK(STACKTRACE_OFFSET); DEBUG_BREAK();

#undef STACKTRACE_OFFSET
}

/*
MSI Afterburner uses deprecated functions like `glPushAttrib/glPopAttrib`
*/
