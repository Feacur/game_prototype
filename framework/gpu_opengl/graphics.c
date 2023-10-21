#include "framework/memory.h"
#include "framework/formatter.h"
#include "framework/maths.h"

#include "framework/containers/strings.h"
#include "framework/containers/buffer.h"
#include "framework/containers/array.h"
#include "framework/containers/array.h"
#include "framework/containers/hashmap.h"
#include "framework/containers/sparseset.h"

#include "framework/assets/mesh.h"
#include "framework/assets/image.h"

#include "framework/graphics/types.h"
#include "framework/graphics/material.h"

#include "framework/systems/string_system.h"
#include "framework/systems/material_system.h"

#include "functions.h"
#include "types.h"

#include <malloc.h> // alloca

// @todo: GPU scissor test
// @todo: expose screen buffer settings, as well as OpenGL's
// @idea: use dedicated samplers instead of texture defaults
// @idea: support older OpenGL versions (pre direct state access, which is 4.5)

struct Gpu_Uniform_Internal {
	struct Gpu_Uniform base;
	GLint location;
};

struct Gpu_Program {
	GLuint id;
	struct Hashmap uniforms; // uniform string id : `struct Gpu_Uniform_Internal`
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
	struct Handle handle;
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
	struct Array textures; // `struct Gpu_Target_Texture`
	struct Array buffers;  // `struct Gpu_Target_Buffer`
	// @idea: add an optional asset source
};

struct Gpu_Mesh_Buffer {
	GLuint id;
	struct Mesh_Parameters parameters;
	uint32_t capacity, count;
};

struct Gpu_Mesh {
	GLuint id;
	struct Array buffers; // `struct Gpu_Mesh_Buffer`
	uint32_t elements_index;
	// @idea: add an optional asset source
};

struct Gpu_Unit {
	struct Handle texture_handle;
};

struct Graphics_Action {
	struct Handle handle;
	void (* action)(struct Handle handle);
};

static struct Graphics_State {
	char * extensions;

	struct Array actions; // `struct Graphics_Action`

	struct Sparseset programs;
	struct Sparseset targets;
	struct Sparseset textures;
	struct Sparseset meshes;

	struct Graphics_State_Active {
		struct Handle program_handle;
		struct Handle target_handle;
		struct Handle mesh_handle;
	} active;

	uint32_t units_capacity;
	struct Gpu_Unit * units;

	struct Gpu_Clip_Space {
		struct vec2 origin;
		float depth_near, depth_far;
		float ndc_near, ndc_far;
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

static void gpu_program_on_aquire(struct Gpu_Program * gpu_program) {
	gpu_program->id = glCreateProgram();
	gpu_program->uniforms = hashmap_init(&hash32, sizeof(uint32_t), sizeof(struct Gpu_Uniform_Internal));
	LOG("[gfx] aquire program %d\n", gpu_program->id);
}

static void gpu_program_on_discard(struct Gpu_Program * gpu_program) {
	LOG("[gfx] discard program %d\n", gpu_program->id);
	hashmap_free(&gpu_program->uniforms);
	glDeleteProgram(gpu_program->id);
}

static void verify_shader(GLuint id);
static void verify_program(GLuint id);
struct Handle gpu_program_init(struct Buffer const * asset) {
#define ADD_SECTION_HEADER(shader_type, version) \
	do { \
		if (common_strstr((char const *)asset->data, #shader_type)) {\
			if (gs_ogl_version < (version)) { LOG("'" #shader_type "' is unavailable\n"); \
				REPORT_CALLSTACK(); DEBUG_BREAK(); break; \
			} \
			sections[sections_count++] = (struct Section_Header){ \
				.type = GL_##shader_type, \
				.text = S_("#define " #shader_type "\n\n"), \
			}; \
		} \
	} while (false) \

	struct Gpu_Program program; gpu_program_on_aquire(&program);

	uint32_t glsl_version = 0;
	switch (gs_ogl_version) {
		case 20: glsl_version = 110; break;
		case 21: glsl_version = 120; break;
		case 30: glsl_version = 130; break;
		case 31: glsl_version = 140; break;
		case 32: glsl_version = 150; break;
		default: glsl_version = gs_ogl_version * 10; break;
	}

	// header
	GLchar header[256];
	uint32_t header_length = formatter_fmt(
		SIZE_OF_ARRAY(header), header, ""
		"#version %u core\n"
		"\n"
		"#define ATTRIBUTE_TYPE_POSITION %u\n"
		"#define ATTRIBUTE_TYPE_TEXCOORD %u\n"
		"#define ATTRIBUTE_TYPE_NORMAL   %u\n"
		"#define ATTRIBUTE_TYPE_COLOR    %u\n"
		"\n"
		"#define DEPTH_NEAR %g\n"
		"#define DEPTH_FAR  %g\n"
		"#define NDC_NEAR %g\n"
		"#define NDC_FAR  %g\n"
		// "\n"
		// "#define R32_POS_INFINITY uintBitsToFloat(0x7f800000)\n"
		"\n",
		glsl_version,
		//
		ATTRIBUTE_TYPE_POSITION - 1,
		ATTRIBUTE_TYPE_TEXCOORD - 1,
		ATTRIBUTE_TYPE_NORMAL - 1,
		ATTRIBUTE_TYPE_COLOR - 1,
		//
		(double)gs_graphics_state.clip_space.depth_near,
		(double)gs_graphics_state.clip_space.depth_far,
		(double)gs_graphics_state.clip_space.ndc_near,
		(double)gs_graphics_state.clip_space.ndc_far
	);

	// section headers, per shader type
	struct Section_Header {
		GLenum type;
		struct CString text;
	};

	uint32_t sections_count = 0;
	struct Section_Header sections[4];
	ADD_SECTION_HEADER(VERTEX_SHADER,   20);
	ADD_SECTION_HEADER(FRAGMENT_SHADER, 20);
	ADD_SECTION_HEADER(GEOMETRY_SHADER, 32);
	ADD_SECTION_HEADER(COMPUTE_SHADER,  43);

	// compile shader objects
	GLuint shader_ids[4];
	for (uint32_t i = 0; i < sections_count; i++) {
		GLchar const * code[]   = {header,               sections[i].text.data,          (GLchar *)asset->data};
		GLint const    length[] = {(GLint)header_length, (GLint)sections[i].text.length, (GLint)asset->size};

		GLuint shader_id = glCreateShader(sections[i].type);
		glShaderSource(shader_id, SIZE_OF_ARRAY(code), code, length);
		glCompileShader(shader_id);
		verify_shader(shader_id);

		shader_ids[i] = shader_id;
	}

	// link shader objects into a program
	for (uint32_t i = 0; i < sections_count; i++) {
		glAttachShader(program.id, shader_ids[i]);
	}

	glLinkProgram(program.id);
	verify_program(program.id);

	// free redundant resources
	for (uint32_t i = 0; i < sections_count; i++) {
		glDetachShader(program.id, shader_ids[i]);
		glDeleteShader(shader_ids[i]);
	}

	// introspect the program
	GLint uniforms_count;
	glGetProgramInterfaceiv(program.id, GL_UNIFORM, GL_ACTIVE_RESOURCES, &uniforms_count);
	
	GLint uniform_name_buffer_length; // includes zero-terminator
	glGetProgramInterfaceiv(program.id, GL_UNIFORM, GL_MAX_NAME_LENGTH, &uniform_name_buffer_length);
	GLchar * uniform_name_buffer = alloca(sizeof(GLchar) * (size_t)uniform_name_buffer_length);

	hashmap_ensure(&program.uniforms, (uint32_t)uniforms_count);

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
		glGetProgramResourceiv(program.id, GL_UNIFORM, (GLuint)i, SIZE_OF_ARRAY(c_props), c_props, SIZE_OF_ARRAY(params), NULL, params);

		GLsizei name_length;
		glGetProgramResourceName(program.id, GL_UNIFORM, (GLuint)i, uniform_name_buffer_length, &name_length, uniform_name_buffer);

		struct CString uniform_name = {
			.data = uniform_name_buffer,
			.length = (uint32_t)name_length,
		};

		if (cstring_contains(uniform_name, S_("[0][0]"))) {
			// @todo: provide a convenient API for nested arrays in GLSL
			LOG("nested arrays are not supported\n");
			REPORT_CALLSTACK(); DEBUG_BREAK(); continue;
		}

		if (cstring_contains(uniform_name, S_("[0]."))) {
			// @todo: provide a convenient API for array of structs in GLSL
			LOG("arrays of structs are not supported\n");
			REPORT_CALLSTACK(); DEBUG_BREAK(); continue;
		}

		if (params[PARAM_ARRAY_SIZE] > 1) {
			uniform_name.length -= 3; // arrays have suffix `[0]`
		}

		uint32_t const id = string_system_add(uniform_name);
		hashmap_set(&program.uniforms, &id, &(struct Gpu_Uniform_Internal){
			.base = {
				.type = translate_program_data_type(params[PARAM_TYPE]),
				.array_size = (uint32_t)params[PARAM_ARRAY_SIZE],
			},
			.location = params[PARAM_LOCATION],
		});
	}

	//
	return sparseset_aquire(&gs_graphics_state.programs, &program);
	// https://www.khronos.org/opengl/wiki/Program_Introspection

#undef ADD_HEADER
}

static void gpu_program_free_immediately(struct Handle handle) {
	if (handle_equals(gs_graphics_state.active.program_handle, handle)) {
		gs_graphics_state.active.program_handle = (struct Handle){0};
	}
	struct Gpu_Program * gpu_program = sparseset_get(&gs_graphics_state.programs, handle);
	if (gpu_program != NULL) {
		gpu_program_on_discard(gpu_program);
		sparseset_discard(&gs_graphics_state.programs, handle);
	}
}

void gpu_program_free(struct Handle handle) {
	array_push_many(&gs_graphics_state.actions, 1, &(struct Graphics_Action){
		.handle = handle,
		.action = gpu_program_free_immediately,
	});
}

struct Hashmap const * gpu_program_get_uniforms(struct Handle handle) {
	struct Gpu_Program const * gpu_program = sparseset_get(&gs_graphics_state.programs, handle);
	return (gpu_program != NULL) ? &gpu_program->uniforms : NULL;
}

// ----- ----- ----- ----- -----
//     GPU texture part
// ----- ----- ----- ----- -----

static void gpu_texture_on_aquire(struct Gpu_Texture * gpu_texture) {
	glCreateTextures(GL_TEXTURE_2D, 1, &gpu_texture->id);
	LOG("[gfx] aquire texture %d\n", gpu_texture->id);
}

static void gpu_texture_on_discard(struct Gpu_Texture * gpu_texture) {
	LOG("[gfx] discard texture %d\n", gpu_texture->id);
	glDeleteTextures(1, &gpu_texture->id);
}

static struct Handle gpu_texture_allocate(
	uint32_t size_x, uint32_t size_y,
	struct Texture_Parameters parameters,
	struct Texture_Settings settings,
	uint32_t max_lod
) {
	if (size_x > gs_graphics_state.limits.max_texture_size) {
		LOG("requested size is too large\n");
		REPORT_CALLSTACK(); DEBUG_BREAK();
		size_x = gs_graphics_state.limits.max_texture_size;
	}

	if (size_y > gs_graphics_state.limits.max_texture_size) {
		LOG("requested size is too large\n");
		REPORT_CALLSTACK(); DEBUG_BREAK();
		size_y = gs_graphics_state.limits.max_texture_size;
	}

	GLenum const internalformat = gpu_sized_internal_format(parameters.texture_type, parameters.data_type);
	GLenum const format = gpu_pixel_data_format(parameters.texture_type, parameters.data_type);
	GLenum const type = gpu_pixel_data_type(parameters.texture_type, parameters.data_type);

	struct Gpu_Texture texture; gpu_texture_on_aquire(&texture);
	texture.size_x     = size_x;
	texture.size_y     = size_y;
	texture.parameters = parameters;
	texture.settings   = settings;

	// allocate buffer
	if (parameters.flags & TEXTURE_FLAG_MUTABLE) {
		uint32_t lod_size_x = size_x;
		uint32_t lod_size_y = size_y;
		glBindTexture(GL_TEXTURE_2D, texture.id);
		for (uint32_t lod = 0; lod <= max_lod; lod++) {
			glTexImage2D(
				GL_TEXTURE_2D, (GLint)lod, (GLint)internalformat,
				(GLsizei)lod_size_x, (GLsizei)lod_size_y, 0,
				format, type, NULL
			);
			lod_size_x = max_u32(1, lod_size_x / 2);
			lod_size_y = max_u32(1, lod_size_y / 2);
		}
	}
	else if (size_x > 0 && size_y > 0) {
		glTextureStorage2D(
			texture.id, (GLsizei)(max_lod + 1), internalformat,
			(GLsizei)size_x, (GLsizei)size_y
		);
	}
	else {
		LOG("immutable storage should have non-zero size\n");
		REPORT_CALLSTACK(); DEBUG_BREAK();
	}

	// chart buffer
	glTextureParameteri(texture.id, GL_TEXTURE_MAX_LEVEL, (GLint)max_lod);
	glTextureParameteri(texture.id, GL_TEXTURE_MIN_FILTER, gpu_min_filter_mode(settings.mipmap, settings.minification));
	glTextureParameteri(texture.id, GL_TEXTURE_MAG_FILTER, gpu_mag_filter_mode(settings.magnification));
	glTextureParameteri(texture.id, GL_TEXTURE_WRAP_S, gpu_wrap_mode(settings.wrap_x));
	glTextureParameteri(texture.id, GL_TEXTURE_WRAP_T, gpu_wrap_mode(settings.wrap_y));
	// glTextureParameterf(texture.id, GL_TEXTURE_LOD_BIAS, 0);

	glTextureParameteriv(texture.id, GL_TEXTURE_SWIZZLE_RGBA, (GLint[]){
		gpu_swizzle_op(settings.swizzle[0], 0),
		gpu_swizzle_op(settings.swizzle[1], 1),
		gpu_swizzle_op(settings.swizzle[2], 2),
		gpu_swizzle_op(settings.swizzle[3], 3),
	});

	glTextureParameterfv(texture.id, GL_TEXTURE_BORDER_COLOR, &settings.border.x);

	//
	return sparseset_aquire(&gs_graphics_state.textures, &texture);
}

static void gpu_texture_upload(struct Handle handle, struct Image const * asset) {
	struct Gpu_Texture * gpu_texture = sparseset_get(&gs_graphics_state.textures, handle);
	glTextureSubImage2D(
		gpu_texture->id, 0,
		0, 0, (GLsizei)asset->size_x, (GLsizei)asset->size_y,
		gpu_pixel_data_format(asset->parameters.texture_type, asset->parameters.data_type),
		gpu_pixel_data_type(asset->parameters.texture_type, asset->parameters.data_type),
		asset->data
	);
	if (gpu_texture->settings.mipmap != FILTER_MODE_NONE) {
		glGenerateTextureMipmap(gpu_texture->id);
	}
}

struct Handle gpu_texture_init(struct Image const * asset) {
	struct Handle const handle = gpu_texture_allocate(
		asset->size_x, asset->size_y, asset->parameters, asset->settings, asset->settings.max_lod
	);

	if (!(asset->parameters.flags & TEXTURE_FLAG_WRITE) && asset->data != NULL) {
		gpu_texture_upload(handle, asset);
	}

	gpu_texture_update(handle, asset);
	return handle;
}

static void gpu_texture_free_immediately(struct Handle handle) {
	for (uint32_t i = 1; i < gs_graphics_state.units_capacity; i++) {
		struct Gpu_Unit * unit = gs_graphics_state.units + i;
		if (handle_equals(unit->texture_handle, handle)) {
			unit->texture_handle = (struct Handle){0};
		}
	}
	struct Gpu_Texture * gpu_texture = sparseset_get(&gs_graphics_state.textures, handle);
	if (gpu_texture != NULL) {
		gpu_texture_on_discard(gpu_texture);
		sparseset_discard(&gs_graphics_state.textures, handle);
	}
}

void gpu_texture_free(struct Handle handle) {
	array_push_many(&gs_graphics_state.actions, 1, &(struct Graphics_Action){
		.handle = handle,
		.action = gpu_texture_free_immediately,
	});
}

void gpu_texture_get_size(struct Handle handle, uint32_t * x, uint32_t * y) {
	struct Gpu_Texture const * gpu_texture = sparseset_get(&gs_graphics_state.textures, handle);
	if (gpu_texture != NULL) {
		*x = gpu_texture->size_x;
		*y = gpu_texture->size_y;
	}
	else {
		*x = 0;
		*y = 0;
	}
}

void gpu_texture_update(struct Handle handle, struct Image const * asset) {
	struct Gpu_Texture * gpu_texture = sparseset_get(&gs_graphics_state.textures, handle);
	if (gpu_texture == NULL) { return; }

	if (!(gpu_texture->parameters.flags & TEXTURE_FLAG_WRITE)) {
		// LOG("trying to write into a read-only buffer\n");
		// REPORT_CALLSTACK(); DEBUG_BREAK();
		return;
	}

	// @todo: compare texture and asset parameters?

	if (asset->data == NULL) { return; }
	if (asset->size_x == 0) { return; }
	if (asset->size_y == 0) { return; }

	GLint const level = 0;
	if (gpu_texture->size_x >= asset->size_x && gpu_texture->size_y >= asset->size_y) {
		gpu_texture_upload(handle, asset);
	}
	else if (gpu_texture->parameters.flags & TEXTURE_FLAG_MUTABLE) {
		// LOG("WARNING! reallocating a buffer\n");
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
		if (gpu_texture->settings.mipmap != FILTER_MODE_NONE) {
			glGenerateTextureMipmap(gpu_texture->id);
		}
	}
	else {
		LOG("trying to reallocate an immutable buffer\n");
		REPORT_CALLSTACK(); DEBUG_BREAK();
		// @todo: completely recreate object instead of using a mutable storage?
	}
}

// ----- ----- ----- ----- -----
//     GPU target part
// ----- ----- ----- ----- -----

static void gpu_target_on_aquire(struct Gpu_Target * gpu_target) {
	glCreateFramebuffers(1, &gpu_target->id);
	gpu_target->textures = array_init(sizeof(struct Gpu_Target_Texture));
	gpu_target->buffers  = array_init(sizeof(struct Gpu_Target_Buffer));
	LOG("[gfx] aquire target %d\n", gpu_target->id);
}

static void gpu_target_on_discard(struct Gpu_Target * gpu_target) {
	LOG("[gfx] discard target %d\n", gpu_target->id);
	for (uint32_t i = 0; i < gpu_target->textures.count; i++) {
		struct Gpu_Target_Texture const * entry = array_at(&gpu_target->textures, i);
		gpu_texture_free(entry->handle);
	}
	for (uint32_t i = 0; i < gpu_target->buffers.count; i++) {
		struct Gpu_Target_Buffer const * entry = array_at(&gpu_target->buffers, i);
		glDeleteRenderbuffers(1, &entry->id);
	}
	array_free(&gpu_target->textures);
	array_free(&gpu_target->buffers);
	glDeleteFramebuffers(1, &gpu_target->id);
}

struct Handle gpu_target_init(
	uint32_t size_x, uint32_t size_y,
	uint32_t parameters_count,
	struct Texture_Parameters const * parameters
) {
	struct Gpu_Target target; gpu_target_on_aquire(&target);
	target.size_x = size_x;
	target.size_y = size_y;

	// allocate exact space for the buffers
	uint32_t textures_count = 0;
	for (uint32_t i = 0; i < parameters_count; i++) {
		if (parameters[i].flags & TEXTURE_FLAG_READ) {
			textures_count++;
		}
	}
	uint32_t buffers_count = parameters_count - textures_count;

	array_resize(&target.textures, textures_count);
	array_resize(&target.buffers,  buffers_count);

	// allocate buffers
	for (uint32_t i = 0, color_index = 0; i < parameters_count; i++) {
		bool const is_color = (parameters[i].texture_type == TEXTURE_TYPE_COLOR);
		if (parameters[i].flags & TEXTURE_FLAG_READ) {
			// @idea: expose settings
			struct Handle const handle = gpu_texture_allocate(size_x, size_y, parameters[i], (struct Texture_Settings){
				.wrap_x = WRAP_MODE_EDGE,
				.wrap_y = WRAP_MODE_EDGE,
			}, 0);
			array_push_many(&target.textures, 1, &(struct Gpu_Target_Texture){
				.handle = handle,
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
			array_push_many(&target.buffers, 1, &(struct Gpu_Target_Buffer){
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
			struct Gpu_Target_Texture const * texture = array_at(&target.textures, texture_index++);
			struct Gpu_Texture const * gpu_texture = sparseset_get(&gs_graphics_state.textures, texture->handle);
			if (gpu_texture != NULL) {
				GLint const level = 0;
				glNamedFramebufferTexture(
					target.id,
					gpu_attachment_point(parameters[i].texture_type, color_index),
					gpu_texture->id,
					level
				);
			}
		}
		else {
			struct Gpu_Target_Buffer const * buffer = array_at(&target.buffers, buffer_index++);
			glNamedFramebufferRenderbuffer(
				target.id,
				gpu_attachment_point(parameters[i].texture_type, color_index),
				GL_RENDERBUFFER,
				buffer->id
			);
		}

		if (is_color) { color_index++; }
	}

	//
	return sparseset_aquire(&gs_graphics_state.targets, &target);
}

static void gpu_target_free_immediately(struct Handle handle) {
	if (handle_equals(gs_graphics_state.active.target_handle, handle)) {
		gs_graphics_state.active.target_handle = (struct Handle){0};
	}
	struct Gpu_Target * gpu_target = sparseset_get(&gs_graphics_state.targets, handle);
	if (gpu_target != NULL) {
		gpu_target_on_discard(gpu_target);
		sparseset_discard(&gs_graphics_state.targets, handle);
	}
}

void gpu_target_free(struct Handle handle) {
	array_push_many(&gs_graphics_state.actions, 1, &(struct Graphics_Action){
		.handle = handle,
		.action = gpu_target_free_immediately,
	});
}

void gpu_target_get_size(struct Handle handle, uint32_t * x, uint32_t * y) {
	struct Gpu_Target const * gpu_target = sparseset_get(&gs_graphics_state.targets, handle);
	if (gpu_target != NULL) {
		*x = gpu_target->size_x;
		*y = gpu_target->size_y;
	}
}

struct Handle gpu_target_get_texture_handle(struct Handle handle, enum Texture_Type type, uint32_t index) {
	struct Gpu_Target const * gpu_target = sparseset_get(&gs_graphics_state.targets, handle);
	if (gpu_target == NULL) { return (struct Handle){0}; }
	for (uint32_t i = 0, color_index = 0; i < gpu_target->textures.count; i++) {
		struct Gpu_Target_Texture const * texture = array_at(&gpu_target->textures, i);
		struct Gpu_Texture const * gpu_texture = sparseset_get(&gs_graphics_state.textures, texture->handle);
		if (gpu_texture->parameters.texture_type == type) {
			if (type == TEXTURE_TYPE_COLOR && color_index != index) { color_index++; continue; }
			return texture->handle;
		}
	}

	LOG("failure: target doesn't have requested texture\n");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return (struct Handle){0};
}

// ----- ----- ----- ----- -----
//     GPU mesh part
// ----- ----- ----- ----- -----

static void gpu_mesh_on_aquire(struct Gpu_Mesh * gpu_mesh) {
	glCreateVertexArrays(1, &gpu_mesh->id);
	gpu_mesh->buffers = array_init(sizeof(struct Gpu_Mesh_Buffer));
	gpu_mesh->elements_index = INDEX_EMPTY;
	LOG("[gfx] aquire mesh %d\n", gpu_mesh->id);
}

static void gpu_mesh_on_discard(struct Gpu_Mesh * gpu_mesh) {
	LOG("[gfx] discard mesh %d\n", gpu_mesh->id);
	for (uint32_t i = 0; i < gpu_mesh->buffers.count; i++) {
		struct Gpu_Mesh_Buffer const * buffer = array_at(&gpu_mesh->buffers, i);
		glDeleteBuffers(1, &buffer->id);
	}
	array_free(&gpu_mesh->buffers);
	glDeleteVertexArrays(1, &gpu_mesh->id);
}

static struct Handle gpu_mesh_allocate(
	uint32_t buffers_count,
	struct Buffer * asset_buffers,
	struct Mesh_Parameters const * parameters_set
) {
	struct Gpu_Mesh mesh; gpu_mesh_on_aquire(&mesh);
	array_resize(&mesh.buffers, buffers_count);

	// allocate buffers
	for (uint32_t i = 0; i < buffers_count; i++) {
		struct Buffer const * asset_buffer = asset_buffers + i;

		struct Mesh_Parameters const parameters = parameters_set[i];
		array_push_many(&mesh.buffers, 1, &(struct Gpu_Mesh_Buffer){
			.parameters = parameters,
		});

		struct Gpu_Mesh_Buffer * buffer = array_at(&mesh.buffers, i);
		glCreateBuffers(1, &buffer->id);

		if (parameters.flags & MESH_FLAG_MUTABLE) {
			glNamedBufferData(
				buffer->id,
				(GLsizeiptr)asset_buffer->size,
				(parameters.flags & MESH_FLAG_WRITE) ? NULL : asset_buffer->data,
				gpu_mesh_usage_pattern(parameters.flags)
			);
		}
		else if (asset_buffer->size != 0) {
			glNamedBufferStorage(
				buffer->id,
				(GLsizeiptr)asset_buffer->size,
				(parameters.flags & MESH_FLAG_WRITE) ? NULL : asset_buffer->data,
				gpu_mesh_immutable_flag(parameters.flags)
			);
		}
		else {
			LOG("immutable storage should have non-zero size\n");
			REPORT_CALLSTACK(); DEBUG_BREAK(); continue;
		}

		// @note: deliberately using count instead of capacity, so as to drop junk data
		buffer->capacity = (uint32_t)asset_buffer->size / data_type_get_size(parameters.type);
		buffer->count = (asset_buffer->data != NULL) ? buffer->capacity : 0;
	}

	// chart buffers
	for (uint32_t i = 0; i < buffers_count; i++) {
		struct Gpu_Mesh_Buffer const * buffer = array_at(&mesh.buffers, i);
		struct Mesh_Parameters const parameters = buffer->parameters;

		// element buffer
		if (parameters.flags & MESH_FLAG_INDEX) {
			glVertexArrayElementBuffer(mesh.id, buffer->id);
			continue;
		}

		// vertex buffer
		uint32_t all_attributes_size = 0;
		for (uint32_t atti = 0; atti < ATTRIBUTE_TYPE_INTERNAL_COUNT; atti++) {
			uint32_t count = parameters.attributes[atti * 2 + 1];
			all_attributes_size += count * data_type_get_size(parameters.type);
		}

		GLuint buffer_index = 0;
		GLintptr buffer_start = 0;
		glVertexArrayVertexBuffer(mesh.id, buffer_index, buffer->id, buffer_start, (GLsizei)all_attributes_size);

		uint32_t attribute_offset = 0;
		for (uint32_t atti = 0; atti < ATTRIBUTE_TYPE_INTERNAL_COUNT; atti++) {
			enum Attribute_Type const type = (enum Attribute_Type)parameters.attributes[atti * 2];
			if (type == ATTRIBUTE_TYPE_NONE) { continue; }

			uint32_t count = parameters.attributes[atti * 2 + 1];
			if (count == 0) { continue; }

			GLuint const location = (GLuint)(type - 1);
			glEnableVertexArrayAttrib(mesh.id, location);
			glVertexArrayAttribBinding(mesh.id, location, buffer_index);
			glVertexArrayAttribFormat(
				mesh.id, location,
				(GLint)count, gpu_vertex_type(parameters.type),
				GL_FALSE, (GLuint)attribute_offset
			);
			attribute_offset += count * data_type_get_size(parameters.type);
		}
	}

	//
	for (uint32_t i = 0; i < buffers_count; i++) {
		struct Gpu_Mesh_Buffer const * buffer = array_at(&mesh.buffers, i);
		struct Mesh_Parameters const parameters = buffer->parameters;
		if (parameters.flags & MESH_FLAG_INDEX) {
			mesh.elements_index = i;
			break;
		}
	}
	if (mesh.elements_index == INDEX_EMPTY) {
		LOG("no element buffer\n");
		REPORT_CALLSTACK(); DEBUG_BREAK();
	}

	return sparseset_aquire(&gs_graphics_state.meshes, &mesh);
}

struct Handle gpu_mesh_init(struct Mesh const * asset) {
	struct Handle const handle = gpu_mesh_allocate(
		asset->count, asset->buffers, asset->parameters
	);

	gpu_mesh_update(handle, asset);
	return handle;
}

static void gpu_mesh_free_immediately(struct Handle handle) {
	if (handle_equals(gs_graphics_state.active.mesh_handle, handle)) {
		gs_graphics_state.active.mesh_handle = (struct Handle){0};
	}
	struct Gpu_Mesh * gpu_mesh = sparseset_get(&gs_graphics_state.meshes, handle);
	if (gpu_mesh != NULL) {
		gpu_mesh_on_discard(gpu_mesh);
		sparseset_discard(&gs_graphics_state.meshes, handle);
	}
}

void gpu_mesh_free(struct Handle handle) {
	array_push_many(&gs_graphics_state.actions, 1, &(struct Graphics_Action){
		.handle = handle,
		.action = gpu_mesh_free_immediately,
	});
}

void gpu_mesh_update(struct Handle handle, struct Mesh const * asset) {
	struct Gpu_Mesh * gpu_mesh = sparseset_get(&gs_graphics_state.meshes, handle);
	if (gpu_mesh == NULL) { return; }
	for (uint32_t i = 0; i < gpu_mesh->buffers.count; i++) {
		struct Gpu_Mesh_Buffer * buffer = array_at(&gpu_mesh->buffers, i);
		struct Mesh_Parameters const parameters = buffer->parameters;
		// @todo: compare mesh and asset parameters?
		// struct Mesh_Parameters const * asset_parameters = asset->parameters + i;

		if (!(parameters.flags & MESH_FLAG_WRITE)) {
			// LOG("trying to write into a read-only buffer\n");
			// REPORT_CALLSTACK(); DEBUG_BREAK();
			continue;
		}

		struct Buffer const * asset_buffer = asset->buffers + i;
		if (asset_buffer->size == 0) { continue; }
		if (asset_buffer->data == NULL) { continue; }

		uint32_t const data_type_size = data_type_get_size(parameters.type);
		uint32_t const elements_count = (uint32_t)asset_buffer->size / data_type_size;

		if (buffer->capacity >= elements_count) {
			glNamedBufferSubData(
				buffer->id, 0,
				(GLsizeiptr)asset_buffer->size,
				asset_buffer->data
			);
		}
		else if (parameters.flags & MESH_FLAG_MUTABLE) {
			// LOG("WARNING! reallocating a buffer\n");
			buffer->capacity = elements_count;
			glNamedBufferData(
				buffer->id,
				(GLsizeiptr)asset_buffer->size, asset_buffer->data,
				gpu_mesh_usage_pattern(parameters.flags)
			);
		}
		else {
			// @todo: completely recreate object instead of using a mutable storage?
			LOG("trying to reallocate an immutable buffer\n");
			REPORT_CALLSTACK(); DEBUG_BREAK(); continue;
		}

		buffer->count = elements_count;
	}
}

//
#include "framework/graphics/gpu_misc.h"

void graphics_update(void) {
	for (uint32_t i = 0; i < gs_graphics_state.actions.count; i++) {
		struct Graphics_Action const * it = array_at(&gs_graphics_state.actions, i);
		it->action(it->handle);
	}
	array_clear(&gs_graphics_state.actions);
}

struct mat4 graphics_set_projection_mat4(
	struct vec2 scale_xy, struct vec2 offset_xy,
	float view_near, float view_far, float ortho
) {
	return mat4_set_projection(
		scale_xy, offset_xy,
		view_near, view_far, ortho,
		gs_graphics_state.clip_space.ndc_near, gs_graphics_state.clip_space.ndc_far
	);
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

static uint32_t graphics_unit_find(struct Handle texture_handle) {
	// @note: consider `id == 0` empty
	for (uint32_t i = 1; i < gs_graphics_state.units_capacity; i++) {
		struct Gpu_Unit const * unit = gs_graphics_state.units + i;
		if (unit->texture_handle.id != texture_handle.id) { continue; }
		if (unit->texture_handle.gen != texture_handle.gen) { continue; }
		return i;
	}
	return 0;
}

static uint32_t gpu_unit_init(struct Handle texture_handle) {
	uint32_t const existing_unit = graphics_unit_find(texture_handle);
	if (existing_unit != 0) { return existing_unit; }

	uint32_t const free_unit = graphics_unit_find((struct Handle){0});
	if (free_unit == 0) {
		LOG("failure: no spare texture/sampler units\n");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return 0;
	}

	struct Gpu_Texture const * gpu_texture = sparseset_get(&gs_graphics_state.textures, texture_handle);
	if (gpu_texture == NULL) {
		LOG("failure: no spare texture/sampler units\n");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return 0;
	}

	gs_graphics_state.units[free_unit] = (struct Gpu_Unit){
		.texture_handle = texture_handle,
	};

	glBindTextureUnit((GLuint)free_unit, gpu_texture->id);
	return free_unit;
}

// static void gpu_unit_free(struct Handle texture_handle) {
// 	if (gs_ogl_version > 0) {
// 		uint32_t unit = graphics_unit_find(texture_handle);
// 		if (unit == 0) { return; }
// 
// 		gs_graphics_state.units[unit] = (struct Gpu_Unit){
// 			.gpu_texture_handle = (struct Handle){0},
// 		};
// 
// 		glBindTextureUnit((GLuint)unit, 0);
// 	}
// }

static void gpu_select_program(struct Handle handle) {
	if (handle_equals(gs_graphics_state.active.program_handle, handle)) { return; }
	gs_graphics_state.active.program_handle = handle;
	struct Gpu_Program const * gpu_program = sparseset_get(&gs_graphics_state.programs, handle);
	glUseProgram((gpu_program != NULL) ? gpu_program->id : 0);
}

static void gpu_select_target(struct Handle handle) {
	if (handle_equals(gs_graphics_state.active.target_handle, handle)) { return; }
	gs_graphics_state.active.target_handle = handle;
	struct Gpu_Target const * gpu_target = sparseset_get(&gs_graphics_state.targets, handle);
	glBindFramebuffer(GL_FRAMEBUFFER, (gpu_target != NULL) ? gpu_target->id : 0);
}

static void gpu_select_mesh(struct Handle handle) {
	if (handle_equals(gs_graphics_state.active.mesh_handle, handle)) { return; }
	gs_graphics_state.active.mesh_handle = handle;
	struct Gpu_Mesh const * gpu_mesh = sparseset_get(&gs_graphics_state.meshes, handle);
	glBindVertexArray((gpu_mesh != NULL) ? gpu_mesh->id : 0);
}

static void gpu_upload_single_uniform(struct Gpu_Program const * gpu_program, struct Gpu_Uniform_Internal const * field, void const * data) {
	switch (field->base.type) {
		default: {
			LOG("unsupported field type '0x%x'\n", field->base.type);
			REPORT_CALLSTACK(); DEBUG_BREAK();
		} break;

		case DATA_TYPE_UNIT_U:
		case DATA_TYPE_UNIT_S:
		case DATA_TYPE_UNIT_F: {
			GLint * units = alloca(sizeof(GLint) * field->base.array_size);
			uint32_t units_count = 0;

			// @todo: automatically rebind in a circular buffer manner
			struct Handle const * texture_handles = (struct Handle const *)data;
			for (uint32_t i = 0; i < field->base.array_size; i++) {
				uint32_t unit = graphics_unit_find(texture_handles[i]);
				if (unit == 0) {
					unit = gpu_unit_init(texture_handles[i]);
				}
				units[units_count++] = (GLint)unit;
			}
			glProgramUniform1iv(gpu_program->id, field->location, (GLsizei)field->base.array_size, units);
		} break;

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
		struct Gfx_Uniforms_Entry const * entry = array_at(&uniforms->headers, i);

		struct Gpu_Uniform_Internal const * uniform = hashmap_get(&gpu_program->uniforms, &entry->id);
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
		bool const reversed_z = (gs_graphics_state.clip_space.depth_near > gs_graphics_state.clip_space.depth_far);
		struct Gpu_Depth_Mode const gpu_mode = gpu_depth_mode(mode, reversed_z);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(gpu_mode.mask);
		glDepthFunc(gpu_mode.op);
	}
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
	gpu_select_target(command->handle);

	uint32_t viewport_size_x = command->screen_size_x, viewport_size_y = command->screen_size_y;
	if (!handle_is_null(command->handle)) {
		gpu_target_get_size(command->handle, &viewport_size_x, &viewport_size_y);
	}

	glViewport(0, 0, (GLsizei)viewport_size_x, (GLsizei)viewport_size_y);
}

inline static void gpu_execute_clear(struct GPU_Command_Clear const * command) {
	if (command->mask == TEXTURE_TYPE_NONE) {
		LOG("clear mask is empty\n");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return;
	}

	float const depth   = gs_graphics_state.clip_space.depth_far;
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
	struct Gfx_Material const * material = material_system_take(command->handle);
	struct Handle const gpu_program_handle = (material != NULL)
		? material->gpu_program_handle
		: (struct Handle){0};

	gpu_select_program(gpu_program_handle);
	gpu_set_blend_mode(material->blend_mode);
	gpu_set_depth_mode(material->depth_mode);

	struct Gpu_Program const * gpu_program = sparseset_get(&gs_graphics_state.programs, gpu_program_handle);
	if (gpu_program != NULL) {
		gpu_upload_uniforms(gpu_program, &material->uniforms, 0, material->uniforms.headers.count);
	}
}

inline static void gpu_execute_shader(struct GPU_Command_Shader const * command) {
	gpu_select_program(command->handle);
	gpu_set_blend_mode(command->blend_mode);
	gpu_set_depth_mode(command->depth_mode);
}

inline static void gpu_execute_uniform(struct GPU_Command_Uniform const * command) {
	if (handle_is_null(command->program_handle)) {
		FOR_SPARSESET (&gs_graphics_state.programs, it) {
			gpu_upload_uniforms(it.value, command->uniforms, command->offset, command->count);
		}
	}
	else {
		struct Gpu_Program const * gpu_program = sparseset_get(&gs_graphics_state.programs, command->program_handle);
		gpu_upload_uniforms(gpu_program, command->uniforms, command->offset, command->count);
	}
}

inline static void gpu_execute_draw(struct GPU_Command_Draw const * command) {
	if (handle_is_null(command->mesh_handle)) { return; }

	struct Gpu_Mesh const * mesh = sparseset_get(&gs_graphics_state.meshes, command->mesh_handle);
	if (mesh == NULL) { return; }

	if (mesh->elements_index == INDEX_EMPTY) {
		LOG("mesh has no elements buffer\n");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return;
	}

	//
	struct Gpu_Mesh_Buffer const * buffer = array_at(&mesh->buffers, mesh->elements_index);
	if (command->length == 0) {
		if (buffer == NULL) { return; }
		if (buffer->count == 0) { return; }
	}

	uint32_t const elements_count = (command->length != 0)
		? command->length
		: buffer->count;

	gpu_select_mesh(command->mesh_handle);

	if (buffer != NULL) {
		enum Data_Type const elements_type = buffer->parameters.type;
		uint32_t const elements_offset = (command->length != 0)
			? command->offset * data_type_get_size(elements_type)
			: 0;

		glDrawElements(
			GL_TRIANGLES,
			(GLsizei)elements_count,
			gpu_vertex_type(elements_type),
			(void const *)(size_t)elements_offset
		);
	}
	else {
		LOG("not implemented\n");
		REPORT_CALLSTACK(); DEBUG_BREAK();
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
			default: {
				LOG("unknown command\n");
				REPORT_CALLSTACK(); DEBUG_BREAK();
			} break;

			case GPU_COMMAND_TYPE_CULL:     gpu_execute_cull(&command->as.cull);         break;
			case GPU_COMMAND_TYPE_TARGET:   gpu_execute_target(&command->as.target);     break;
			case GPU_COMMAND_TYPE_CLEAR:    gpu_execute_clear(&command->as.clear);       break;
			case GPU_COMMAND_TYPE_MATERIAL: gpu_execute_material(&command->as.material); break;
			case GPU_COMMAND_TYPE_SHADER:   gpu_execute_shader(&command->as.shader);     break;
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

	// init gpu objects
	gs_graphics_state.programs = sparseset_init(sizeof(struct Gpu_Program));
	gs_graphics_state.targets  = sparseset_init(sizeof(struct Gpu_Target));
	gs_graphics_state.textures = sparseset_init(sizeof(struct Gpu_Texture));
	gs_graphics_state.meshes   = sparseset_init(sizeof(struct Gpu_Mesh));

	gs_graphics_state.actions = array_init(sizeof(struct Graphics_Action));

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

	LOG(
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
		""
		, max_units
		, max_units_fragment_shader
		, max_units_vertex_shader
		, max_units_compute_shader
		, max_texture_size
		, max_renderbuffer_size
		, max_elements_vertices
		, max_elements_indices
		, max_uniform_locations
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

	bool const reverse_z = true || can_control_clip_space;
	gs_graphics_state.clip_space = (struct Gpu_Clip_Space){
		.origin = {
			0,
			can_control_clip_space ? 0.0f : 1.0f,
		},
		.depth_near = reverse_z ? 1.0f : 0.0f,
		.depth_far  = reverse_z ? 0.0f : 1.0f,
		.ndc_near   = can_control_clip_space ? 0.0f : -1.0f,
		.ndc_far    = can_control_clip_space ? 1.0f :  1.0f,
	};

	// @todo: check if swapping `ndc_near` and `ndc_far` is any better
	glDepthRangef(gs_graphics_state.clip_space.depth_near, gs_graphics_state.clip_space.depth_far);

	// if (reverse_z) {
	// 	float const temp = gs_graphics_state.clip_space.ndc_near;
	// 	gs_graphics_state.clip_space.ndc_near = gs_graphics_state.clip_space.ndc_far;
	// 	gs_graphics_state.clip_space.ndc_far = temp;
	// }
}

void graphics_to_gpu_library_free(void) {
#define GPU_FREE(name, action) do { \
	struct Sparseset * data = &gs_graphics_state.name; \
	inst_count += sparseset_get_count(data); \
	FOR_SPARSESET (data, it) { action(it.value); } \
	sparseset_free(data); \
} while (false) \

	graphics_update();

	uint32_t inst_count = 0;
	GPU_FREE(programs, gpu_program_on_discard);
	GPU_FREE(targets,  gpu_target_on_discard);
	GPU_FREE(textures, gpu_texture_on_discard);
	GPU_FREE(meshes,   gpu_mesh_on_discard);

	//
	array_free(&gs_graphics_state.actions);
	MEMORY_FREE(gs_graphics_state.extensions);
	MEMORY_FREE(gs_graphics_state.units);
	common_memset(&gs_graphics_state, 0, sizeof(gs_graphics_state));

	if (inst_count > 0) { DEBUG_BREAK(); }

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
		size_t const size = strlen((char const *)value);
		buffer_push_many(&string, size, value);
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
	if (max_length <= 0) { return; }

	// @todo: (?) arena/stack allocator
	GLchar * buffer = alloca(sizeof(GLchar) * (size_t)max_length);
	glGetShaderInfoLog(id, max_length, &max_length, buffer);
	LOG("%s\n", buffer);
}

static void verify_program(GLuint id) {
	GLint status;
	glGetProgramiv(id, GL_LINK_STATUS, &status);
	if (status) { return; }

	GLint max_length;
	glGetProgramiv(id, GL_INFO_LOG_LENGTH, &max_length);
	if (max_length <= 0) { return; }

	// @todo: (?) arena/stack allocator
	GLchar * buffer = alloca(sizeof(GLchar) * (size_t)max_length);
	glGetProgramInfoLog(id, max_length, &max_length, buffer);
	LOG("%s\n", buffer);
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
		case GL_DEBUG_SOURCE_THIRD_PARTY:     return "third party";
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
	(void)userParam;
	LOG(
		"> OpenGL message '0x%x'\n"
		"  severity: %s\n"
		"  source:   %s\n"
		"  type:     %s\n"
		"  message:  %.*s\n"
		""
		, id
		, opengl_debug_get_severity(severity)
		, opengl_debug_get_source(source)
		, opengl_debug_get_type(type)
		, length, message
	);
	REPORT_CALLSTACK();
	if (type == GL_DEBUG_TYPE_ERROR) { DEBUG_BREAK(); }
}

/*
MSI Afterburner uses deprecated functions like `glPushAttrib/glPopAttrib`
*/
