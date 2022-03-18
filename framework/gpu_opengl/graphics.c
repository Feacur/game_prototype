#include "framework/memory.h"
#include "framework/logger.h"

#include "framework/containers/strings.h"
#include "framework/containers/buffer.h"
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
// @idea: condense graphics material's buffers into a single bytes arrays?

#define MAX_UNIFORMS 32
#define MAX_UNITS_PER_MATERIAL 64
#define MAX_TARGET_ATTACHMENTS 4
#define MAX_MESH_BUFFERS 2

struct Gpu_Program {
	GLuint id;
	struct Gpu_Program_Field uniforms[MAX_UNIFORMS];
	GLint uniform_locations[MAX_UNIFORMS];
	uint32_t uniforms_count;
	// @idea: add an optional asset source
};

struct Gpu_Texture {
	GLuint id;
	uint32_t size_x, size_y;
	struct Texture_Parameters parameters;
	struct Texture_Settings settings;
	// @idea: add an optional asset source
};

struct Gpu_Target {
	GLuint id;
	uint32_t size_x, size_y;
	struct Ref texture_refs[MAX_TARGET_ATTACHMENTS];
	uint32_t textures_count;
	GLuint buffers[MAX_TARGET_ATTACHMENTS];
	uint32_t buffers_count;
};

struct Gpu_Mesh {
	GLuint id;
	uint32_t buffers_count;
	GLuint buffer_ids[MAX_MESH_BUFFERS];
	struct Mesh_Parameters parameters[MAX_MESH_BUFFERS];
	uint32_t capacities[MAX_MESH_BUFFERS];
	uint32_t counts[MAX_MESH_BUFFERS];
	uint32_t elements_index;
	// @idea: add an optional asset source
};

struct Gpu_Unit {
	struct Ref gpu_texture_ref;
};

static struct Graphics_State {
	char * extensions;

	struct Strings uniforms;

	struct Ref_Table programs;
	struct Ref_Table targets;
	struct Ref_Table textures;
	struct Ref_Table meshes;

	struct Ref active_program_ref;
	struct Ref active_target_ref;
	struct Ref active_mesh_ref;

	uint32_t units_capacity;
	struct Gpu_Unit * units;

	float clip_space[4]; // origin XY; normalized-space near and far

	uint32_t max_units_vertex_shader;
	uint32_t max_units_fragment_shader;
	uint32_t max_units_compute_shader;
	uint32_t max_texture_size;
	uint32_t max_renderbuffer_size;
	uint32_t max_elements_vertices;
	uint32_t max_elements_indices;
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
				.type = GL_ ## shader_type, \
				.text = S_("#define " #shader_type "\n"), \
			}; \
		} \
	} while (false) \

	// a mandatory version header
	GLchar glsl_version[20];
	GLint glsl_version_length = (GLint)logger_to_buffer(
		glsl_version, "#version %d core\n",
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
	GLchar * uniform_name_buffer = MEMORY_ALLOCATE_ARRAY(&gs_graphics_state, GLchar, uniform_name_buffer_length);

	struct Gpu_Program_Field uniforms[MAX_UNIFORMS];
	GLint uniform_locations[MAX_UNIFORMS];
	for (GLint i = 0; i < uniforms_count; i++) {
		enum Param {
			PARAM_TYPE,
			PARAM_ARRAY_SIZE,
			PARAM_LOCATION,
			// PARAM_NAME_LENGTH,
		};
		GLenum const props[] = {
			[PARAM_TYPE] = GL_TYPE,
			[PARAM_ARRAY_SIZE] = GL_ARRAY_SIZE,
			[PARAM_LOCATION] = GL_LOCATION,
			// [PARAM_NAME_LENGTH] = GL_NAME_LENGTH,
		};
		GLint params[SIZE_OF_ARRAY(props)];
		glGetProgramResourceiv(program_id, GL_UNIFORM, (GLuint)i, SIZE_OF_ARRAY(props), props, SIZE_OF_ARRAY(params), NULL, params);

		GLsizei name_length;
		glGetProgramResourceName(program_id, GL_UNIFORM, (GLuint)i, uniform_name_buffer_length, &name_length, uniform_name_buffer);

		if (params[PARAM_ARRAY_SIZE] > 1) {
			// @todo: improve reflection/introspection/whatever;
			//        simple arrays have names ending with a `[0]`;
			//        more specifically the very first elememnt is tagget such a way
			if (common_memcmp(uniform_name_buffer + name_length - 3, "[0]", 3) != 0) { continue; }
			name_length -= 3;
		}

		uniforms[i] = (struct Gpu_Program_Field){
			.id = strings_add(&gs_graphics_state.uniforms, (struct CString){
				.length = (uint32_t)name_length,
				.data = uniform_name_buffer,
			}),
			.type = interpret_gl_type(params[PARAM_TYPE]),
			.array_size = (uint32_t)params[PARAM_ARRAY_SIZE],
		};
		uniform_locations[i] = params[PARAM_LOCATION];
	}

	MEMORY_FREE(&gs_graphics_state, uniform_name_buffer);

	//
	struct Gpu_Program gpu_program = (struct Gpu_Program){
		.id = program_id,
		.uniforms_count = (uint32_t)uniforms_count,
	};
	common_memcpy(gpu_program.uniforms, uniforms, sizeof(*uniforms) * (size_t)uniforms_count);
	common_memcpy(gpu_program.uniform_locations, uniform_locations, sizeof(*uniform_locations) * (size_t)uniforms_count);

	return ref_table_aquire(&gs_graphics_state.programs, &gpu_program);
	// https://www.khronos.org/opengl/wiki/Program_Introspection

#undef ADD_HEADER
}

static void gpu_program_free_internal(struct Gpu_Program const * gpu_program) {
	glDeleteProgram(gpu_program->id);
}

void gpu_program_free(struct Ref gpu_program_ref) {
	if (gs_graphics_state.active_program_ref.id == gpu_program_ref.id && gs_graphics_state.active_program_ref.gen == gpu_program_ref.gen) {
		gs_graphics_state.active_program_ref = (struct Ref){0};
	}
	struct Gpu_Program const * gpu_program = ref_table_get(&gs_graphics_state.programs, gpu_program_ref);
	if (gpu_program != NULL) {
		gpu_program_free_internal(gpu_program);
		ref_table_discard(&gs_graphics_state.programs, gpu_program_ref);
	}
}

void gpu_program_get_uniforms(struct Ref gpu_program_ref, uint32_t * count, struct Gpu_Program_Field const ** values) {
	struct Gpu_Program const * gpu_program = ref_table_get(&gs_graphics_state.programs, gpu_program_ref);
	*count = gpu_program->uniforms_count;
	*values = gpu_program->uniforms;
}

// ----- ----- ----- ----- -----
//     GPU texture part
// ----- ----- ----- ----- -----

static struct Ref gpu_texture_allocate(
	uint32_t size_x, uint32_t size_y,
	struct Texture_Parameters const * parameters,
	struct Texture_Settings const * settings,
	void const * data
) {
	if (size_x > gs_graphics_state.max_texture_size) {
		logger_to_console("requested size is too large\n"); DEBUG_BREAK();
		size_x = gs_graphics_state.max_texture_size;
	}

	if (size_y > gs_graphics_state.max_texture_size) {
		logger_to_console("requested size is too large\n"); DEBUG_BREAK();
		size_y = gs_graphics_state.max_texture_size;
	}

	GLuint texture_id;
	glCreateTextures(GL_TEXTURE_2D, 1, &texture_id);

	// allocate buffer
	GLint const level = 0;
	if (data == NULL && !(parameters->flags & (TEXTURE_FLAG_WRITE | TEXTURE_FLAG_READ | TEXTURE_FLAG_INTERNAL))) {
		logger_to_console("non-internal storage should have initial data\n"); DEBUG_BREAK();
	}
	else if (parameters->flags & TEXTURE_FLAG_MUTABLE) {
		glBindTexture(GL_TEXTURE_2D, texture_id);
		glTexImage2D(
			GL_TEXTURE_2D, level,
			(GLint)gpu_sized_internal_format(parameters->texture_type, parameters->data_type, parameters->channels),
			(GLsizei)size_x, (GLsizei)size_y, 0,
			gpu_pixel_data_format(parameters->texture_type, parameters->channels),
			gpu_pixel_data_type(parameters->texture_type, parameters->data_type),
			data
		);
	}
	else if (size_x > 0 && size_y > 0) {
		GLsizei const levels = 1;
		glTextureStorage2D(
			texture_id, levels,
			gpu_sized_internal_format(parameters->texture_type, parameters->data_type, parameters->channels),
			(GLsizei)size_x, (GLsizei)size_y
		);
		if (data != NULL) {
			glTextureSubImage2D(
				texture_id, level,
				0, 0, (GLsizei)size_x, (GLsizei)size_y,
				gpu_pixel_data_format(parameters->texture_type, parameters->channels),
				gpu_pixel_data_type(parameters->texture_type, parameters->data_type),
				data
			);
		}
	}
	else {
		logger_to_console("immutable storage should have non-zero size\n"); DEBUG_BREAK();
	}

	// chart buffer
	glTextureParameteri(texture_id, GL_TEXTURE_MIN_FILTER, gpu_min_filter_mode(settings->mipmap, settings->minification));
	glTextureParameteri(texture_id, GL_TEXTURE_MAG_FILTER, gpu_mag_filter_mode(settings->magnification));
	glTextureParameteri(texture_id, GL_TEXTURE_WRAP_S, gpu_wrap_mode(settings->wrap_x, settings->mirror_wrap_x));
	glTextureParameteri(texture_id, GL_TEXTURE_WRAP_T, gpu_wrap_mode(settings->wrap_y, settings->mirror_wrap_y));

	switch (parameters->channels) {
		case 1: {
			GLint const swizzle[] = {GL_ONE, GL_ONE, GL_ONE, GL_RED};
			glTextureParameteriv(texture_id, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
			break;
		}
	}

	//
	struct Gpu_Texture gpu_texture = (struct Gpu_Texture){
		.id = texture_id,
		.size_x = size_x,
		.size_y = size_y,
		.parameters = *parameters,
		.settings = *settings,
	};

	return ref_table_aquire(&gs_graphics_state.textures, &gpu_texture);
}

struct Ref gpu_texture_init(struct Image const * asset) {
	struct Ref const gpu_texture_ref = gpu_texture_allocate(
		asset->size_x, asset->size_y, &asset->parameters, &asset->settings, asset->data
	);

	gpu_texture_update(gpu_texture_ref, asset);

	return gpu_texture_ref;
}

static void gpu_texture_free_internal(struct Gpu_Texture const * gpu_texture) {
	glDeleteTextures(1, &gpu_texture->id);
}

void gpu_texture_free(struct Ref gpu_texture_ref) {
	for (uint32_t i = 1; i < gs_graphics_state.units_capacity; i++) {
		struct Gpu_Unit * unit = gs_graphics_state.units + i;
		if (unit->gpu_texture_ref.id == gpu_texture_ref.id && unit->gpu_texture_ref.gen == gpu_texture_ref.gen) {
			unit->gpu_texture_ref = (struct Ref){0};
		}
	}
	struct Gpu_Texture const * gpu_texture = ref_table_get(&gs_graphics_state.textures, gpu_texture_ref);
	if (gpu_texture != NULL) {
		gpu_texture_free_internal(gpu_texture);
		ref_table_discard(&gs_graphics_state.textures, gpu_texture_ref);
	}
}

void gpu_texture_get_size(struct Ref gpu_texture_ref, uint32_t * x, uint32_t * y) {
	struct Gpu_Texture const * gpu_texture = ref_table_get(&gs_graphics_state.textures, gpu_texture_ref);
	*x = gpu_texture->size_x;
	*y = gpu_texture->size_y;
}

void gpu_texture_update(struct Ref gpu_texture_ref, struct Image const * asset) {
	struct Gpu_Texture * gpu_texture = ref_table_get(&gs_graphics_state.textures, gpu_texture_ref);

	// @todo: compare texture and asset parameters?

	if (asset->data == NULL) { return; }
	if (asset->size_x == 0) { return; }
	if (asset->size_y == 0) { return; }

	GLint const level = 0;
	if (gpu_texture->size_x >= asset->size_x && gpu_texture->size_y >= asset->size_y) {
		glTextureSubImage2D(
			gpu_texture->id, level,
			0, 0, (GLsizei)asset->size_x, (GLsizei)asset->size_y,
			gpu_pixel_data_format(asset->parameters.texture_type, asset->parameters.channels),
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
			(GLint)gpu_sized_internal_format(gpu_texture->parameters.texture_type, gpu_texture->parameters.data_type, gpu_texture->parameters.channels),
			(GLsizei)asset->size_x, (GLsizei)asset->size_y, 0,
			gpu_pixel_data_format(asset->parameters.texture_type, asset->parameters.channels),
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
	struct Texture_Parameters const * parameters,
	uint32_t count
) {
	GLuint target_id;
	glCreateFramebuffers(1, &target_id);

	struct Ref texture_refs[MAX_TARGET_ATTACHMENTS];
	uint32_t textures_count = 0;

	GLuint buffers[MAX_TARGET_ATTACHMENTS];
	uint32_t buffers_count = 0;

	// allocate buffers
	for (uint32_t i = 0; i < count; i++) {
		if (parameters[i].flags & TEXTURE_FLAG_READ) {
			texture_refs[textures_count++] = gpu_texture_allocate(size_x, size_y, parameters + i, &(struct Texture_Settings){
				.wrap_x = WRAP_MODE_CLAMP,
				.wrap_y = WRAP_MODE_CLAMP,
			}, NULL);
		}
		else {
			GLuint buffer_id;
			glCreateRenderbuffers(1, &buffer_id);
			glNamedRenderbufferStorage(
				buffer_id,
				gpu_sized_internal_format(parameters[i].texture_type, parameters[i].data_type, parameters[i].channels),
				(GLsizei)size_x, (GLsizei)size_y
			);
			buffers[buffers_count++] = buffer_id;
		}
	}

	// chart buffers
	for (uint32_t i = 0, texture_index = 0, color_index = 0, buffer_index = 0; i < count; i++) {
		if (!(parameters[i].flags & TEXTURE_FLAG_READ)) {
			glNamedFramebufferRenderbuffer(
				target_id,
				gpu_attachment_point(parameters[i].texture_type, color_index),
				GL_RENDERBUFFER,
				buffers[buffer_index++]
			);
		}
		else {
			struct Ref const gpu_texture_ref = texture_refs[texture_index++];
			struct Gpu_Texture const * gpu_texture = ref_table_get(&gs_graphics_state.textures, gpu_texture_ref);

			GLint const level = 0;
			glNamedFramebufferTexture(
				target_id,
				gpu_attachment_point(parameters[i].texture_type, color_index),
				gpu_texture->id,
				level
			);
		}

		if (parameters[i].texture_type == TEXTURE_TYPE_COLOR) { color_index++; }
	}

	//
	struct Gpu_Target gpu_target = (struct Gpu_Target){
		.id = target_id,
		.size_x = size_x,
		.size_y = size_y,
		.textures_count = textures_count,
		.buffers_count = buffers_count,
	};
	common_memcpy(gpu_target.texture_refs, texture_refs, sizeof(*texture_refs) * textures_count);
	common_memcpy(gpu_target.buffers, buffers, sizeof(*buffers) * buffers_count);

	return ref_table_aquire(&gs_graphics_state.targets, &gpu_target);
}

static void gpu_target_free_internal(struct Gpu_Target const * gpu_target) {
	for (uint32_t i = 0; i < gpu_target->textures_count; i++) {
		gpu_texture_free(gpu_target->texture_refs[i]);
	}
	for (uint32_t i = 0; i < gpu_target->buffers_count; i++) {
		glDeleteRenderbuffers(1, gpu_target->buffers + i);
	}
	glDeleteFramebuffers(1, &gpu_target->id);
}

void gpu_target_free(struct Ref gpu_target_ref) {
	if (gs_graphics_state.active_target_ref.id == gpu_target_ref.id && gs_graphics_state.active_target_ref.gen == gpu_target_ref.gen) {
		gs_graphics_state.active_target_ref = (struct Ref){0};
	}
	struct Gpu_Target const * gpu_target = ref_table_get(&gs_graphics_state.targets, gpu_target_ref);
	if (gpu_target != NULL) {
		gpu_target_free_internal(gpu_target);
		ref_table_discard(&gs_graphics_state.targets, gpu_target_ref);
	}
}

void gpu_target_get_size(struct Ref gpu_target_ref, uint32_t * x, uint32_t * y) {
	struct Gpu_Target const * gpu_target = ref_table_get(&gs_graphics_state.targets, gpu_target_ref);
	*x = gpu_target->size_x;
	*y = gpu_target->size_y;
}

struct Ref gpu_target_get_texture_ref(struct Ref gpu_target_ref, enum Texture_Type type, uint32_t index) {
	struct Gpu_Target const * gpu_target = ref_table_get(&gs_graphics_state.targets, gpu_target_ref);
	for (uint32_t i = 0, color_index = 0; i < gpu_target->textures_count; i++) {
		struct Ref const gpu_texture_ref = gpu_target->texture_refs[i];
		struct Gpu_Texture const * gpu_texture = ref_table_get(&gs_graphics_state.textures, gpu_texture_ref);
		if (gpu_texture->parameters.texture_type == type) {
			if (type == TEXTURE_TYPE_COLOR && color_index != index) { color_index++; continue; }
			return gpu_texture_ref;
		}
	}

	logger_to_console("failure: target doesn't have requested texture\n"); DEBUG_BREAK();
	return ref_empty;
}

// ----- ----- ----- ----- -----
//     GPU mesh part
// ----- ----- ----- ----- -----

static struct Ref gpu_mesh_allocate(
	uint32_t buffers_count,
	uint32_t * byte_lengths,
	struct Mesh_Parameters const * parameters_set,
	void ** data
) {
	if (buffers_count > MAX_MESH_BUFFERS) {
		logger_to_console("too many buffers\n"); DEBUG_BREAK();
		buffers_count = MAX_MESH_BUFFERS;
	}

	GLuint mesh_id;
	glCreateVertexArrays(1, &mesh_id);

	GLuint buffer_ids[MAX_MESH_BUFFERS];
	uint32_t capacities[MAX_MESH_BUFFERS];

	// allocate buffers
	for (uint32_t i = 0; i < buffers_count; i++) {
		struct Mesh_Parameters const * parameters = parameters_set + i;

		glCreateBuffers(1, buffer_ids + i);

		if (data[i] == NULL && !(parameters->flags & (MESH_FLAG_WRITE | MESH_FLAG_READ | MESH_FLAG_INTERNAL))) {
			logger_to_console("non-internal storage should have initial data\n"); DEBUG_BREAK();
		}
		else if (parameters->flags & MESH_FLAG_MUTABLE) {
			glNamedBufferData(
				buffer_ids[i],
				(GLsizeiptr)byte_lengths[i], data[i],
				gpu_mesh_usage_pattern(parameters->flags)
			);
		}
		else if (byte_lengths[i] != 0) {
			glNamedBufferStorage(
				buffer_ids[i],
				(GLsizeiptr)byte_lengths[i], data[i],
				gpu_mesh_immutable_flag(parameters->flags)
			);
		}
		else {
			logger_to_console("immutable storage should have non-zero size\n"); DEBUG_BREAK();
		}

		capacities[i] = byte_lengths[i] / data_type_get_size(parameters->type);
	}

	// chart buffers
	uint32_t elements_index = INDEX_EMPTY;
	for (uint32_t i = 0; i < buffers_count; i++) {
		struct Mesh_Parameters const * parameters = parameters_set + i;

		// element buffer
		if (parameters->flags & MESH_FLAG_INDEX) {
			elements_index = i;
			glVertexArrayElementBuffer(mesh_id, buffer_ids[i]);
			continue;
		}

		// vertex buffer
		uint32_t all_attributes_size = 0;
		for (uint32_t atti = 0; atti < parameters->attributes_count; atti++) {
			all_attributes_size += parameters->attributes[atti * 2 + 1] * data_type_get_size(parameters->type);
		}

		GLuint buffer_index = 0;
		GLintptr buffer_start = 0;
		glVertexArrayVertexBuffer(mesh_id, buffer_index, buffer_ids[i], buffer_start, (GLsizei)all_attributes_size);

		uint32_t attribute_offset = 0;
		for (uint32_t atti = 0; atti < parameters->attributes_count; atti++) {
			GLuint location = (GLuint)parameters->attributes[atti * 2];
			uint32_t size = parameters->attributes[atti * 2 + 1];
			glEnableVertexArrayAttrib(mesh_id, location);
			glVertexArrayAttribBinding(mesh_id, location, buffer_index);
			glVertexArrayAttribFormat(
				mesh_id, location,
				(GLint)size, gpu_data_type(parameters->type),
				GL_FALSE, (GLuint)attribute_offset
			);
			attribute_offset += size * data_type_get_size(parameters->type);
		}
	}

	//
	if (elements_index == INDEX_EMPTY) {
		logger_to_console("not element buffer\n"); DEBUG_BREAK();
	}

	struct Gpu_Mesh gpu_mesh = (struct Gpu_Mesh){
		.id = mesh_id,
		.buffers_count = buffers_count,
		.elements_index = elements_index,
	};
	common_memcpy(gpu_mesh.buffer_ids, buffer_ids,     sizeof(*buffer_ids) * buffers_count);
	common_memcpy(gpu_mesh.parameters, parameters_set, sizeof(*parameters_set) * buffers_count);
	common_memcpy(gpu_mesh.capacities, capacities,     sizeof(*capacities) * buffers_count);
	for (uint32_t i = 0; i < buffers_count; i++) {
		gpu_mesh.counts[i] = (data[i] != NULL) ? capacities[i] : 0;
	}

	return ref_table_aquire(&gs_graphics_state.meshes, &gpu_mesh);
}

struct Ref gpu_mesh_init(struct Mesh const * asset) {
	uint32_t byte_lengths[MAX_MESH_BUFFERS];
	void * data[MAX_MESH_BUFFERS];
	for (uint32_t i = 0; i < asset->count; i++) {
		byte_lengths[i] = (uint32_t)asset->buffers[i].count;
		data[i] = asset->buffers[i].data;
	}

	struct Ref const gpu_mesh_ref = gpu_mesh_allocate(
		asset->count, byte_lengths, asset->parameters, data
	);

	gpu_mesh_update(gpu_mesh_ref, asset);
	return gpu_mesh_ref;
}

static void gpu_mesh_free_internal(struct Gpu_Mesh const * gpu_mesh) {
	glDeleteBuffers((GLsizei)gpu_mesh->buffers_count, gpu_mesh->buffer_ids);
	glDeleteVertexArrays(1, &gpu_mesh->id);
}

void gpu_mesh_free(struct Ref gpu_mesh_ref) {
	if (gs_graphics_state.active_mesh_ref.id == gpu_mesh_ref.id && gs_graphics_state.active_mesh_ref.gen == gpu_mesh_ref.gen) {
		gs_graphics_state.active_mesh_ref = (struct Ref){0};
	}
	struct Gpu_Mesh const * gpu_mesh = ref_table_get(&gs_graphics_state.meshes, gpu_mesh_ref);
	if (gpu_mesh != NULL) {
		gpu_mesh_free_internal(gpu_mesh);
		ref_table_discard(&gs_graphics_state.meshes, gpu_mesh_ref);
	}
}

void gpu_mesh_update(struct Ref gpu_mesh_ref, struct Mesh const * asset) {
	struct Gpu_Mesh * gpu_mesh = ref_table_get(&gs_graphics_state.meshes, gpu_mesh_ref);
	for (uint32_t i = 0; i < gpu_mesh->buffers_count; i++) {
		// @todo: compare mesh and asset parameters?
		// struct Mesh_Parameters const * asset_parameters = asset->parameters + i;

		struct Mesh_Parameters const * parameters = gpu_mesh->parameters + i;
		if (!(parameters->flags & MESH_FLAG_WRITE)) { continue; }
		gpu_mesh->counts[i] = 0;

		struct Buffer const * buffer = asset->buffers + i;
		if (buffer->count == 0) { continue; }
		if (buffer->data == NULL) { continue; }

		uint32_t const data_type_size = data_type_get_size(parameters->type);

		gpu_mesh->counts[i] = ((uint32_t)buffer->count) / data_type_size;

		if (gpu_mesh->capacities[i] * data_type_size >= buffer->count) {
			glNamedBufferSubData(
				gpu_mesh->buffer_ids[i], 0,
				(GLsizeiptr)buffer->count,
				buffer->data
			);
		}
		else if (parameters->flags & MESH_FLAG_MUTABLE) {
			// logger_to_console("WARNING! reallocating a buffer\n");
			gpu_mesh->capacities[i] = gpu_mesh->counts[i];
			glNamedBufferData(
				gpu_mesh->buffer_ids[i],
				(GLsizeiptr)buffer->count, buffer->data,
				gpu_mesh_usage_pattern(parameters->flags)
			);
		}
		else {
			logger_to_console("trying to reallocate an immutable buffer"); DEBUG_BREAK();
			// @todo: completely recreate object instead of using a mutable storage?
		}
	}
}

//
#include "framework/graphics/gpu_misc.h"

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
		if (unit->gpu_texture_ref.id != gpu_texture_ref.id) { continue; }
		if (unit->gpu_texture_ref.gen != gpu_texture_ref.gen) { continue; }
		return i;
	}
	return 0;
}

static uint32_t graphics_unit_init(struct Ref gpu_texture_ref) {
	uint32_t unit = graphics_unit_find(gpu_texture_ref);
	if (unit != 0) { return unit; }

	unit = graphics_unit_find((struct Ref){0});
	if (unit == 0) {
		logger_to_console("failure: no spare texture/sampler units\n"); DEBUG_BREAK();
		return unit;
	}

	gs_graphics_state.units[unit] = (struct Gpu_Unit){
		.gpu_texture_ref = gpu_texture_ref,
	};

	struct Gpu_Texture const * gpu_texture = ref_table_get(&gs_graphics_state.textures, gpu_texture_ref);
	glBindTextureUnit((GLuint)unit, gpu_texture->id);

	return unit;
}

// static void graphics_unit_free(struct Gpu_Texture * gpu_texture) {
// 	if (ogl_version > 0) {
// 		uint32_t unit = graphics_unit_find(gpu_texture);
// 		if (unit == 0) { return; }
// 
// 		graphics_state.units[unit] = (struct Gpu_Unit){
// 			.gpu_texture = NULL,
// 		};
// 
// 		glBindTextureUnit((GLuint)unit, 0);
// 	}
// }

static void graphics_select_program(struct Ref gpu_program_ref) {
	if (gs_graphics_state.active_program_ref.id == gpu_program_ref.id && gs_graphics_state.active_program_ref.gen == gpu_program_ref.gen) {
		return;
	}
	gs_graphics_state.active_program_ref = gpu_program_ref;
	struct Gpu_Program const * gpu_program = ref_table_get(&gs_graphics_state.programs, gpu_program_ref);
	glUseProgram((gpu_program != NULL) ? gpu_program->id : 0);
}

static void graphics_select_target(struct Ref gpu_target_ref) {
	if (gs_graphics_state.active_target_ref.id == gpu_target_ref.id && gs_graphics_state.active_target_ref.gen == gpu_target_ref.gen) {
		return;
	}
	gs_graphics_state.active_target_ref = gpu_target_ref;
	struct Gpu_Target const * gpu_target = ref_table_get(&gs_graphics_state.targets, gpu_target_ref);
	glBindFramebuffer(GL_FRAMEBUFFER, (gpu_target != NULL) ? gpu_target->id : 0);
}

static void graphics_select_mesh(struct Ref gpu_mesh_ref) {
	if (gs_graphics_state.active_mesh_ref.id == gpu_mesh_ref.id && gs_graphics_state.active_mesh_ref.gen == gpu_mesh_ref.gen) {
		return;
	}
	gs_graphics_state.active_mesh_ref = gpu_mesh_ref;
	struct Gpu_Mesh const * gpu_mesh = ref_table_get(&gs_graphics_state.meshes, gpu_mesh_ref);
	glBindVertexArray((gpu_mesh != NULL) ? gpu_mesh->id : 0);
}

static void graphics_upload_single_uniform(struct Gpu_Program const * gpu_program, uint32_t uniform_index, void const * data) {
	struct Gpu_Program_Field const * field = gpu_program->uniforms + uniform_index;
	GLint const location = gpu_program->uniform_locations[uniform_index];

	switch (field->type) {
		default: logger_to_console("unsupported field type '0x%x'\n", field->type); DEBUG_BREAK(); break;

		case DATA_TYPE_UNIT: {
			GLint units[MAX_UNITS_PER_MATERIAL];
			uint32_t units_count = 0;

			// @todo: automatically rebind in a circular buffer manner
			struct Ref const * gpu_texture_refs = (struct Ref const *)data;
			for (uint32_t i = 0; i < field->array_size; i++) {
				uint32_t unit = graphics_unit_find(gpu_texture_refs[i]);
				if (unit == 0) {
					unit = graphics_unit_init(gpu_texture_refs[i]);
				}
				units[units_count++] = (GLint)unit;
			}
			glProgramUniform1iv(gpu_program->id, location, (GLsizei)field->array_size, units);
			break;
		}

		case DATA_TYPE_U32:   glProgramUniform1uiv(gpu_program->id, location, (GLsizei)field->array_size, data); break;
		case DATA_TYPE_UVEC2: glProgramUniform2uiv(gpu_program->id, location, (GLsizei)field->array_size, data); break;
		case DATA_TYPE_UVEC3: glProgramUniform3uiv(gpu_program->id, location, (GLsizei)field->array_size, data); break;
		case DATA_TYPE_UVEC4: glProgramUniform4uiv(gpu_program->id, location, (GLsizei)field->array_size, data); break;

		case DATA_TYPE_S32:   glProgramUniform1iv(gpu_program->id, location, (GLsizei)field->array_size, data); break;
		case DATA_TYPE_SVEC2: glProgramUniform2iv(gpu_program->id, location, (GLsizei)field->array_size, data); break;
		case DATA_TYPE_SVEC3: glProgramUniform3iv(gpu_program->id, location, (GLsizei)field->array_size, data); break;
		case DATA_TYPE_SVEC4: glProgramUniform4iv(gpu_program->id, location, (GLsizei)field->array_size, data); break;

		case DATA_TYPE_R32:  glProgramUniform1fv(gpu_program->id, location, (GLsizei)field->array_size, data); break;
		case DATA_TYPE_VEC2: glProgramUniform2fv(gpu_program->id, location, (GLsizei)field->array_size, data); break;
		case DATA_TYPE_VEC3: glProgramUniform3fv(gpu_program->id, location, (GLsizei)field->array_size, data); break;
		case DATA_TYPE_VEC4: glProgramUniform4fv(gpu_program->id, location, (GLsizei)field->array_size, data); break;

		case DATA_TYPE_MAT2: glProgramUniformMatrix2fv(gpu_program->id, location, (GLsizei)field->array_size, GL_FALSE, data); break;
		case DATA_TYPE_MAT3: glProgramUniformMatrix3fv(gpu_program->id, location, (GLsizei)field->array_size, GL_FALSE, data); break;
		case DATA_TYPE_MAT4: glProgramUniformMatrix4fv(gpu_program->id, location, (GLsizei)field->array_size, GL_FALSE, data); break;
	}
}

static void graphics_upload_uniforms(struct Gfx_Material const * material, struct Gfx_Material_Override const * override) {
	struct Gpu_Program const * gpu_program = ref_table_get(&gs_graphics_state.programs, material->gpu_program_ref);
	uint32_t const uniforms_count = gpu_program->uniforms_count;

	uint32_t unit_offset = 0, u32_offset = 0, s32_offset = 0, float_offset = 0;
	for (uint32_t i = 0; i < uniforms_count; i++) {
		struct Gpu_Program_Field const * field = gpu_program->uniforms + i;
		// struct CString const field_name = graphics_get_uniform_value(field->id); (void)field_name;

		void const * data = gfx_material_override_find(override, field->id, data_type_get_size(field->type) * field->array_size);

		enum Data_Type const element_type   = data_type_get_element_type(field->type);
		uint32_t       const elements_count = data_type_get_count(field->type) * field->array_size;
		switch (element_type) {
			default: logger_to_console("unknown element type '0x%x'\n", element_type); DEBUG_BREAK(); break;

			case DATA_TYPE_UNIT: {
				if (data == NULL) { data = array_any_at(&material->textures, unit_offset); }
				unit_offset += elements_count;
				break;
			}

			case DATA_TYPE_U32: {
				if (data == NULL) { data = material->values_u32.data + u32_offset; }
				u32_offset += elements_count;
				break;
			}

			case DATA_TYPE_S32: {
				if (data == NULL) { data = material->values_s32.data + s32_offset; }
				s32_offset += elements_count;
				break;
			}

			case DATA_TYPE_R32: {
				if (data == NULL) { data = material->values_float.data + float_offset; }
				float_offset += elements_count;
				break;
			}
		}

		if (data != NULL) { graphics_upload_single_uniform(gpu_program, i, data); }
	}
}

static bool graphics_should_blend(struct Blend_Func const * func) {
	return (func != NULL)
		&& (func->op != BLEND_OP_NONE)
		&& (
			(func->src != BLEND_FACTOR_ONE) ||
			(func->dst != BLEND_FACTOR_ZERO)
		);
}

static void graphics_set_blend_mode(struct Blend_Mode const * mode) {
	glColorMask(
		(mode->mask & COLOR_CHANNEL_RED),
		(mode->mask & COLOR_CHANNEL_GREEN),
		(mode->mask & COLOR_CHANNEL_BLUE),
		(mode->mask & COLOR_CHANNEL_ALPHA)
	);

	glBlendColor(
		((mode->rgba >> 24) & 0xff) / 255.0f,
		((mode->rgba >> 16) & 0xff) / 255.0f,
		((mode->rgba >> 8) & 0xff) / 255.0f,
		((mode->rgba >> 0) & 0xff) / 255.0f
	);

	//
	bool const would_blend_rgb = graphics_should_blend(&mode->rgb);
	bool const would_blend_a = graphics_should_blend(&mode->a);
	if (!(would_blend_rgb || would_blend_a)) {
		glDisable(GL_BLEND);
	}
	else {
		glEnable(GL_BLEND);

		glBlendEquationSeparate(
			gpu_blend_op(would_blend_rgb ? mode->rgb.op : BLEND_OP_ADD),
			gpu_blend_op(would_blend_a ? mode->a.op : BLEND_OP_ADD)
		);

		glBlendFuncSeparate(
			gpu_blend_factor(would_blend_rgb ? mode->rgb.src : BLEND_FACTOR_ONE),
			gpu_blend_factor(would_blend_rgb ? mode->rgb.dst : BLEND_FACTOR_ZERO),
			gpu_blend_factor(would_blend_a ? mode->a.src : BLEND_FACTOR_ONE),
			gpu_blend_factor(would_blend_a ? mode->a.dst : BLEND_FACTOR_ZERO)
		);
	}
}

static void graphics_set_depth_mode(struct Depth_Mode const * mode) {
	glDepthMask((mode->enabled && mode->mask) ? GL_TRUE : GL_FALSE);
	if (mode->enabled) {
		glEnable(GL_DEPTH_TEST);
	}
	else {
		glDisable(GL_DEPTH_TEST);
	}
}

static void graphics_clear(enum Texture_Type mask, uint32_t rgba) {
	if (mask == TEXTURE_TYPE_NONE) { return; }

	GLbitfield clear_bitfield = 0;
	if (mask & TEXTURE_TYPE_COLOR)   { clear_bitfield |= GL_COLOR_BUFFER_BIT; }
	if (mask & TEXTURE_TYPE_DEPTH)   { clear_bitfield |= GL_DEPTH_BUFFER_BIT; }
	if (mask & TEXTURE_TYPE_STENCIL) { clear_bitfield |= GL_STENCIL_BUFFER_BIT; }

	glClearColor(
		((rgba >> 24) & 0xff) / 255.0f,
		((rgba >> 16) & 0xff) / 255.0f,
		((rgba >>  8) & 0xff) / 255.0f,
		((rgba >>  0) & 0xff) / 255.0f
	);

	glClear(clear_bitfield);
}

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
	graphics_select_target(command->gpu_ref);

	uint32_t viewport_size_x = command->screen_size_x, viewport_size_y = command->screen_size_y;
	if (command->gpu_ref.id != 0 && command->gpu_ref.id != ref_empty.id) {
		gpu_target_get_size(command->gpu_ref, &viewport_size_x, &viewport_size_y);
	}

	glViewport(0, 0, (GLsizei)viewport_size_x, (GLsizei)viewport_size_y);
}

inline static void gpu_execute_clear(struct GPU_Command_Clear const * command) {
	if (command->mask == TEXTURE_TYPE_NONE) { logger_to_console("clear mask is empty"); DEBUG_BREAK(); return; }

	// @todo: ever need variations?
	graphics_set_blend_mode((struct Blend_Mode[]){c_blend_mode_opaque});
	graphics_set_depth_mode(&(struct Depth_Mode){.enabled = true, .mask = true});

	graphics_clear(command->mask, command->rgba);
}

inline static void gpu_execute_draw(struct GPU_Command_Draw const * command) {
	if (command->material == NULL) { logger_to_console("material is null"); DEBUG_BREAK(); return; }
	if (command->material->gpu_program_ref.id == 0) { logger_to_console("program is null"); DEBUG_BREAK(); return; }

	if (command->gpu_mesh_ref.id == 0) { logger_to_console("mesh is null"); DEBUG_BREAK(); return; }
	struct Gpu_Mesh const * mesh = ref_table_get(&gs_graphics_state.meshes, command->gpu_mesh_ref);
	if (mesh->elements_index == INDEX_EMPTY) { logger_to_console("mesh has no elements buffer"); DEBUG_BREAK(); return; }

	uint32_t const elements_count = (command->length != 0)
		? command->length
		: mesh->counts[mesh->elements_index];
	if (elements_count == 0) { return; }

	if (command->material->blend_mode.mask == COLOR_CHANNEL_NONE) { return; }
	graphics_set_blend_mode(&command->material->blend_mode);
	graphics_set_depth_mode(&command->material->depth_mode);

	graphics_select_program(command->material->gpu_program_ref);
	graphics_upload_uniforms(command->material, &command->override);

	graphics_select_mesh(command->gpu_mesh_ref);

	enum Data_Type const elements_type = mesh->parameters[mesh->elements_index].type;
	size_t const elements_offset = command->offset * data_type_get_size(elements_type);

	glDrawElements(
		GL_TRIANGLES,
		(GLsizei)elements_count,
		gpu_data_type(elements_type),
		(void const *)(size_t)elements_offset
	);
}

void gpu_execute(uint32_t length, struct GPU_Command const * commands) {
	for (uint32_t i = 0; i < length; i++) {
		struct GPU_Command const * command = commands + i;
		switch (command->type) {
			default:                      logger_to_console("unknown command"); DEBUG_BREAK(); break;
			case GPU_COMMAND_TYPE_CULL:   gpu_execute_cull(&command->as.cull);     break;
			case GPU_COMMAND_TYPE_TARGET: gpu_execute_target(&command->as.target); break;
			case GPU_COMMAND_TYPE_CLEAR:  gpu_execute_clear(&command->as.clear);   break;
			case GPU_COMMAND_TYPE_DRAW:   gpu_execute_draw(&command->as.draw);     break;
		}
	}
}

//
#include "graphics_to_gpu_library.h"

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

	// init uniforms strings, consider 0 id empty
	strings_init(&gs_graphics_state.uniforms);
	strings_add(&gs_graphics_state.uniforms, S_EMPTY);

	// init gpu objects, consider 0 ref.id empty
	ref_table_init(&gs_graphics_state.programs, sizeof(struct Gpu_Program));
	ref_table_init(&gs_graphics_state.targets, sizeof(struct Gpu_Target));
	ref_table_init(&gs_graphics_state.textures, sizeof(struct Gpu_Texture));
	ref_table_init(&gs_graphics_state.meshes, sizeof(struct Gpu_Mesh));

	ref_table_aquire(&gs_graphics_state.programs, &(struct Gpu_Program){0});
	ref_table_aquire(&gs_graphics_state.targets, &(struct Gpu_Target){0});
	ref_table_aquire(&gs_graphics_state.textures, &(struct Gpu_Texture){0});
	ref_table_aquire(&gs_graphics_state.meshes, &(struct Gpu_Mesh){0});

	//
	GLint max_units;
	GLint max_units_vertex_shader, max_units_fragment_shader, max_units_compute_shader;
	GLint max_texture_size, max_renderbuffer_size;
	GLint max_elements_vertices, max_elements_indices;

	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_units);
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS,          &max_units_fragment_shader);
	glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS,   &max_units_vertex_shader);
	glGetIntegerv(GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS,  &max_units_compute_shader);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE,                 &max_texture_size);
	glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE,            &max_renderbuffer_size);
	glGetIntegerv(GL_MAX_ELEMENTS_VERTICES,            &max_elements_vertices);
	glGetIntegerv(GL_MAX_ELEMENTS_INDICES,             &max_elements_indices);

	logger_to_console(
		"\n"
		"> OpenGL limits:\n"
		"  units ......... %d\n"
		"  - FS .......... %d\n"
		"  - VS .......... %d\n"
		"  - CS .......... %d\n"
		"  texture size .. %d\n"
		"  target size ... %d\n"
		"  vertices ...... %d\n"
		"  indices ....... %d\n"
		"",
		max_units,
		max_units_fragment_shader,
		max_units_vertex_shader,
		max_units_compute_shader,
		max_texture_size,
		max_renderbuffer_size,
		max_elements_vertices,
		max_elements_indices
	);

	gs_graphics_state.max_units_vertex_shader   = (uint32_t)max_units_vertex_shader;
	gs_graphics_state.max_units_fragment_shader = (uint32_t)max_units_fragment_shader;
	gs_graphics_state.max_units_compute_shader  = (uint32_t)max_units_compute_shader;
	gs_graphics_state.max_texture_size          = (uint32_t)max_texture_size;
	gs_graphics_state.max_renderbuffer_size     = (uint32_t)max_renderbuffer_size;
	gs_graphics_state.max_elements_vertices     = (uint32_t)max_elements_vertices;
	gs_graphics_state.max_elements_indices      = (uint32_t)max_elements_indices;

	gs_graphics_state.units_capacity = (uint32_t)max_units;
	gs_graphics_state.units = MEMORY_ALLOCATE_ARRAY(&gs_graphics_state, struct Gpu_Unit, max_units);
	common_memset(gs_graphics_state.units, 0, sizeof(* gs_graphics_state.units) * (size_t)max_units);

	// @note: manage OpenGL's clip space instead of ours
	bool const supports_reverse_z = (gs_ogl_version >= 45) || contains_full_word(gs_graphics_state.extensions, S_("GL_ARB_clip_control"));
	if (supports_reverse_z) { glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE); }

	gs_graphics_state.clip_space[0] =                             0.0f; // origin X
	gs_graphics_state.clip_space[1] = supports_reverse_z ? 0.0f : 1.0f; // origin Y
	gs_graphics_state.clip_space[2] = supports_reverse_z ? 1.0f : 0.0f; // normalized-space near
	gs_graphics_state.clip_space[3] = supports_reverse_z ? 0.0f : 1.0f; // normalized-space far

	glDepthRangef(gs_graphics_state.clip_space[2], gs_graphics_state.clip_space[3]);
	glClearDepthf(gs_graphics_state.clip_space[3]);
	glDepthFunc(gpu_comparison_op((gs_graphics_state.clip_space[2] > gs_graphics_state.clip_space[3]) ? COMPARISON_OP_MORE : COMPARISON_OP_LESS));
}

void graphics_to_gpu_library_free(void) {
	// @note: consider `ref.id == 0` empty
	if (gs_graphics_state.programs.count > 1) { logger_to_console("dangling programs: %u\n", gs_graphics_state.programs.count - 1); }
	if (gs_graphics_state.targets.count  > 1) { logger_to_console("dangling targets:  %u\n", gs_graphics_state.targets.count - 1); }
	if (gs_graphics_state.textures.count > 1) { logger_to_console("dangling textures: %u\n", gs_graphics_state.textures.count - 1); }
	if (gs_graphics_state.meshes.count   > 1) { logger_to_console("dangling meshes:   %u\n", gs_graphics_state.meshes.count - 1); }

	for (struct Ref_Table_Iterator it = {0}; ref_table_iterate(&gs_graphics_state.programs, &it); /*empty*/) {
		gpu_program_free_internal(it.value);
	}
	for (struct Ref_Table_Iterator it = {0}; ref_table_iterate(&gs_graphics_state.targets, &it); /*empty*/) {
		gpu_target_free_internal(it.value);
	}
	for (struct Ref_Table_Iterator it = {0}; ref_table_iterate(&gs_graphics_state.textures, &it); /*empty*/) {
		gpu_texture_free_internal(it.value);
	}
	for (struct Ref_Table_Iterator it = {0}; ref_table_iterate(&gs_graphics_state.meshes, &it); /*empty*/) {
		gpu_mesh_free_internal(it.value);
	}

	//
	ref_table_free(&gs_graphics_state.programs);
	ref_table_free(&gs_graphics_state.textures);
	ref_table_free(&gs_graphics_state.targets);
	ref_table_free(&gs_graphics_state.meshes);

	//
	strings_free(&gs_graphics_state.uniforms);
	MEMORY_FREE(&gs_graphics_state, gs_graphics_state.extensions);
	MEMORY_FREE(&gs_graphics_state, gs_graphics_state.units);
	common_memset(&gs_graphics_state, 0, sizeof(gs_graphics_state));

	(void)gpu_stencil_op;
}

//

static char * allocate_extensions_string(void) {
	struct Buffer string;
	buffer_init(&string);

	GLint extensions_count = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &extensions_count);

	buffer_resize(&string, (uint32_t)(extensions_count * 26));
	for(GLint i = 0; i < extensions_count; i++) {
		GLubyte const * value = glGetStringi(GL_EXTENSIONS, (GLuint)i);
		buffer_push_many(&string, (uint32_t)strlen((char const *)value), value);
		buffer_push(&string, ' ');
	}
	buffer_push(&string, '\0');

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

static char const * debug_get_source(GLenum value) {
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

static char const * debug_get_type(GLenum value) {
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

static char const * debug_get_severity(GLenum value) {
	switch (value) {
		case GL_DEBUG_SEVERITY_HIGH:         return "[crt]"; // All OpenGL Errors, shader compilation/linking errors, or highly-dangerous undefined behavior
		case GL_DEBUG_SEVERITY_MEDIUM:       return "[err]"; // Major performance warnings, shader compilation/linking warnings, or the use of deprecated functionality
		case GL_DEBUG_SEVERITY_LOW:          return "[wrn]"; // Redundant state change performance warning, or unimportant undefined behavior
		case GL_DEBUG_SEVERITY_NOTIFICATION: return "[trc]"; // Anything that isn't an error or performance issue.
	}
	return "[???]";
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
	(void)userParam;
	logger_to_console(
		"\n"
		"> OpenGL message '0x%x'\n"
		"  message:  %.*s\n"
		"  severity: %s\n"
		"  type:     %s\n"
		"  source:   %s\n"
		"",
		id,
		length, message,
		debug_get_severity(severity),
		debug_get_type(type),
		debug_get_source(source)
	);

	DEBUG_BREAK();
}

/*
MSI Afterburner uses deprecated functions like `glPushAttrib/glPopAttrib`
*/
