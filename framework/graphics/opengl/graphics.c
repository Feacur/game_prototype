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

#include "framework/graphics/gfx_types.h"
#include "framework/graphics/gfx_material.h"

#include "framework/systems/string_system.h"
#include "framework/systems/material_system.h"

#include "functions.h"
#include "gpu_types.h"

#include <malloc.h> // alloca

// @todo: GPU scissor test
// @todo: expose screen buffer settings, as well as OpenGL's
// @idea: use dedicated samplers instead of texture defaults
// @idea: support older OpenGL versions (pre direct state access, which is 4.5)

#define GFX_TRACE(format, ...) formatter_log("[gfx] " format "\n", ## __VA_ARGS__)

struct GPU_Uniform_Internal {
	struct GPU_Uniform base;
	GLint location;
};

struct GPU_Program {
	GLuint id;
	struct Hashmap uniforms; // uniform string id : `struct GPU_Uniform_Internal`
	// @idea: add an optional asset source
};

struct GPU_Texture {
	GLuint id;
	struct uvec2 size;
	struct Texture_Parameters parameters;
	struct Texture_Settings settings;
	struct Sampler_Settings sampler;
	// @idea: add an optional asset source
};

struct GPU_Target_Buffer {
	GLuint id;
	struct Texture_Parameters parameters;
};

struct GPU_Target {
	GLuint id;
	struct uvec2 size;
	struct Array textures; // `struct Handle`
	struct Array buffers;  // `struct GPU_Target_Buffer`
	// @idea: add an optional asset source
};

struct GPU_Buffer {
	GLuint id;
	size_t capacity, size;
};

struct GPU_Mesh {
	GLuint id;
	struct Array buffers;    // `struct Handle`
	struct Array parameters; // `struct Mesh_Parameters`
	// @idea: add an optional asset source
};

struct GPU_Unit {
	struct Handle gh_texture;
};

struct Graphics_Action {
	struct Handle handle;
	void (* action)(struct Handle handle);
};

static struct Graphics_State {
	char * extensions;

	struct Array actions; // `struct Graphics_Action`
	struct Array units;   // `struct GPU_Unit`

	struct Sparseset programs; // `struct GPU_Program`
	struct Sparseset targets;  // `struct GPU_Target`
	struct Sparseset textures; // `struct GPU_Texture`
	struct Sparseset buffers;  // `struct GPU_Buffer`
	struct Sparseset meshes;   // `struct GPU_Mesh`

	struct Graphics_State_Active {
		struct Handle gh_program;
		struct Handle gh_target;
		struct Handle gh_mesh;
	} active;

	struct GPU_Clip_Space {
		struct vec2 origin;
		float depth_near, depth_far;
		float ndc_near, ndc_far;
	} clip_space;

	struct Graphics_State_Limits {
		uint32_t units_vs;
		uint32_t units_fs;
		uint32_t units_cs;
		//
		uint32_t uniform_blocks_vs;
		uint32_t uniform_blocks_fs;
		uint32_t uniform_blocks_cs;
		//
		uint32_t storage_blocks_vs;
		uint32_t storage_blocks_fs;
		uint32_t storage_blocks_cs;
		//
		uint32_t texture_size;
		uint32_t renderbuffer_size;
		uint32_t elements_vertices;
		uint32_t elements_indices;
		uint32_t uniform_locations;
	} limits;
} gs_graphics_state;

//
#include "framework/graphics/objects.h"

// ----- ----- ----- ----- -----
//     GPU program part
// ----- ----- ----- ----- -----

static void verify_shader(GLuint id);
static void verify_program(GLuint id);

static struct GPU_Program gpu_program_on_aquire(struct Buffer const * asset) {
#define ADD_SECTION_HEADER(shader_type, version) \
	do { \
		if (common_strstr((char const *)asset->data, #shader_type)) {\
			if (gs_ogl_version < (version)) { ERR("'" #shader_type "' is unavailable"); \
				REPORT_CALLSTACK(); DEBUG_BREAK(); break; \
			} \
			sections[sections_count++] = (struct Section_Header){ \
				.type = GL_##shader_type, \
				.text = S_("#define " #shader_type "\n\n"), \
			}; \
		} \
	} while (false) \

	struct GPU_Program gpu_program = {
		.id = glCreateProgram(),
		.uniforms = hashmap_init(&hash32, sizeof(uint32_t), sizeof(struct GPU_Uniform_Internal)),
	};

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
		glAttachShader(gpu_program.id, shader_ids[i]);
	}

	glLinkProgram(gpu_program.id);
	verify_program(gpu_program.id);

	// free redundant resources
	for (uint32_t i = 0; i < sections_count; i++) {
		glDetachShader(gpu_program.id, shader_ids[i]);
		glDeleteShader(shader_ids[i]);
	}

	// introspect the program
	GLint uniforms_count;
	glGetProgramInterfaceiv(gpu_program.id, GL_UNIFORM, GL_ACTIVE_RESOURCES, &uniforms_count);
	
	GLint uniform_name_buffer_length; // includes zero-terminator
	glGetProgramInterfaceiv(gpu_program.id, GL_UNIFORM, GL_MAX_NAME_LENGTH, &uniform_name_buffer_length);
	GLchar * uniform_name_buffer = alloca(sizeof(GLchar) * (size_t)uniform_name_buffer_length);

	hashmap_ensure(&gpu_program.uniforms, (uint32_t)uniforms_count);

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
		glGetProgramResourceiv(gpu_program.id, GL_UNIFORM, (GLuint)i, SIZE_OF_ARRAY(c_props), c_props, SIZE_OF_ARRAY(params), NULL, params);

		GLsizei name_length;
		glGetProgramResourceName(gpu_program.id, GL_UNIFORM, (GLuint)i, uniform_name_buffer_length, &name_length, uniform_name_buffer);

		struct CString uniform_name = {
			.data = uniform_name_buffer,
			.length = (uint32_t)name_length,
		};

		if (cstring_contains(uniform_name, S_("[0][0]"))) {
			// @todo: provide a convenient API for nested arrays in GLSL
			WRN("nested arrays are not supported");
			REPORT_CALLSTACK(); DEBUG_BREAK(); continue;
		}

		if (cstring_contains(uniform_name, S_("[0]."))) {
			// @todo: provide a convenient API for array of structs in GLSL
			WRN("arrays of structs are not supported");
			REPORT_CALLSTACK(); DEBUG_BREAK(); continue;
		}

		if (params[PARAM_ARRAY_SIZE] > 1) {
			uniform_name.length -= 3; // arrays have suffix `[0]`
		}

		uint32_t const id = string_system_add(uniform_name);
		hashmap_set(&gpu_program.uniforms, &id, &(struct GPU_Uniform_Internal){
			.base = {
				.type = translate_program_data_type(params[PARAM_TYPE]),
				.array_size = (uint32_t)params[PARAM_ARRAY_SIZE],
			},
			.location = params[PARAM_LOCATION],
		});
	}

	GFX_TRACE("aquire program %d", gpu_program.id);
	return gpu_program;
	// https://www.khronos.org/opengl/wiki/Program_Introspection

#undef ADD_HEADER
}

static void gpu_program_on_discard(struct GPU_Program * gpu_program) {
	if (gpu_program->id == 0) { return; }
	GFX_TRACE("discard program %d", gpu_program->id);
	hashmap_free(&gpu_program->uniforms);
	glDeleteProgram(gpu_program->id);
}

struct Handle gpu_program_init(struct Buffer const * asset) {
	struct GPU_Program gpu_program = gpu_program_on_aquire(asset);
	return sparseset_aquire(&gs_graphics_state.programs, &gpu_program);
}

static void gpu_program_free_immediately(struct Handle handle) {
	if (handle_equals(gs_graphics_state.active.gh_program, handle)) {
		gs_graphics_state.active.gh_program = (struct Handle){0};
	}
	struct GPU_Program * gpu_program = sparseset_get(&gs_graphics_state.programs, handle);
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

void gpu_program_update(struct Handle handle, struct Buffer const * asset) {
	struct GPU_Program * gpu_program = sparseset_get(&gs_graphics_state.programs, handle);
	if (gpu_program == NULL) { return; }

	gpu_program_on_discard(gpu_program);
	*gpu_program = gpu_program_on_aquire(asset);
}

struct Hashmap const * gpu_program_get_uniforms(struct Handle handle) {
	struct GPU_Program const * gpu_program = sparseset_get(&gs_graphics_state.programs, handle);
	return (gpu_program != NULL) ? &gpu_program->uniforms : NULL;
}

// ----- ----- ----- ----- -----
//     GPU texture part
// ----- ----- ----- ----- -----

static bool gpu_texture_upload(struct GPU_Texture * gpu_texture, struct Image const * asset) {
	if (gpu_texture->size.x != asset->size.x) { return false; }
	if (gpu_texture->size.y != asset->size.y) { return false; }

	if (!equals(
		&gpu_texture->parameters, &asset->parameters
		, sizeof(struct Texture_Settings)
	)) { return false; }

	if (!equals(
		&gpu_texture->settings, &asset->settings
		, sizeof(struct Texture_Settings)
	)) { return false; }

	if (!equals(
		&gpu_texture->sampler, &asset->sampler
		, sizeof(struct Texture_Settings)
	)) { return false; }

	if (asset->data == NULL) { return true; }
	if (asset->size.x == 0)  { return true; }
	if (asset->size.y == 0)  { return true; }

	glTextureSubImage2D(
		gpu_texture->id, 0,
		0, 0, (GLsizei)asset->size.x, (GLsizei)asset->size.y,
		gpu_pixel_data_format(asset->parameters.texture_type, asset->parameters.data_type),
		gpu_pixel_data_type(asset->parameters.texture_type, asset->parameters.data_type),
		asset->data
	);
	if (gpu_texture->settings.max_lod != 0) {
		glGenerateTextureMipmap(gpu_texture->id);
	}

	return true;
}

static struct GPU_Texture gpu_texture_on_aquire(struct Image const * asset) {
	struct GPU_Texture gpu_texture = {
		.size = {
			min_u32(asset->size.x, gs_graphics_state.limits.texture_size),
			min_u32(asset->size.y, gs_graphics_state.limits.texture_size),
		},
		.parameters = asset->parameters,
		.settings   = asset->settings,
		.sampler    = asset->sampler,
	};

	if (gpu_texture.size.x == 0)  { return gpu_texture; }
	if (gpu_texture.size.y == 0)  { return gpu_texture; }

	// allocate and upload
	glCreateTextures(GL_TEXTURE_2D, 1, &gpu_texture.id);
	glTextureStorage2D(
		gpu_texture.id, (GLsizei)(gpu_texture.settings.max_lod + 1)
		, gpu_sized_internal_format(gpu_texture.parameters.texture_type, gpu_texture.parameters.data_type)
		, (GLsizei)gpu_texture.size.x, (GLsizei)gpu_texture.size.y
	);
	gpu_texture_upload(&gpu_texture, asset);

	// chart
	glTextureParameteri(gpu_texture.id, GL_TEXTURE_MAX_LEVEL, (GLint)gpu_texture.settings.max_lod);
	glTextureParameteriv(gpu_texture.id, GL_TEXTURE_SWIZZLE_RGBA, (GLint[]){
		gpu_swizzle_op(gpu_texture.settings.swizzle[0], 0),
		gpu_swizzle_op(gpu_texture.settings.swizzle[1], 1),
		gpu_swizzle_op(gpu_texture.settings.swizzle[2], 2),
		gpu_swizzle_op(gpu_texture.settings.swizzle[3], 3),
	});

	// @todo: separate sampler objects
	glTextureParameterfv(gpu_texture.id, GL_TEXTURE_BORDER_COLOR, &gpu_texture.sampler.border.x);
	glTextureParameteri(gpu_texture.id, GL_TEXTURE_MIN_FILTER, gpu_min_filter_mode(gpu_texture.sampler.mipmap, gpu_texture.sampler.minification));
	glTextureParameteri(gpu_texture.id, GL_TEXTURE_MAG_FILTER, gpu_mag_filter_mode(gpu_texture.sampler.magnification));
	glTextureParameteri(gpu_texture.id, GL_TEXTURE_WRAP_S, gpu_wrap_mode(gpu_texture.sampler.wrap_x));
	glTextureParameteri(gpu_texture.id, GL_TEXTURE_WRAP_T, gpu_wrap_mode(gpu_texture.sampler.wrap_y));

	GFX_TRACE("aquire texture %d", gpu_texture.id);
	return gpu_texture;
}

static void gpu_texture_on_discard(struct GPU_Texture * gpu_texture) {
	if (gpu_texture->id == 0) { return; }
	GFX_TRACE("discard texture %d", gpu_texture->id);
	glDeleteTextures(1, &gpu_texture->id);
}

struct Handle gpu_texture_init(struct Image const * asset) {
	struct GPU_Texture gpu_texture = gpu_texture_on_aquire(asset);
	return sparseset_aquire(&gs_graphics_state.textures, &gpu_texture);
}

static void gpu_texture_free_immediately(struct Handle handle) {
	FOR_ARRAY(&gs_graphics_state.units, it) {
		struct GPU_Unit * unit = it.value;
		if (handle_equals(unit->gh_texture, handle)) {
			unit->gh_texture = (struct Handle){0};
		}
	}
	struct GPU_Texture * gpu_texture = sparseset_get(&gs_graphics_state.textures, handle);
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

void gpu_texture_update(struct Handle handle, struct Image const * asset) {
	struct GPU_Texture * gpu_texture = sparseset_get(&gs_graphics_state.textures, handle);
	if (gpu_texture == NULL) { return; }

	if (gpu_texture_upload(gpu_texture, asset)) { return; }

	gpu_texture_on_discard(gpu_texture);
	*gpu_texture = gpu_texture_on_aquire(asset);
}

struct uvec2 gpu_texture_get_size(struct Handle handle) {
	struct GPU_Texture const * gpu_texture = sparseset_get(&gs_graphics_state.textures, handle);
	return (gpu_texture != NULL)
		? gpu_texture->size
		: (struct uvec2){0};
}

// ----- ----- ----- ----- -----
//     GPU target part
// ----- ----- ----- ----- -----

static struct GPU_Target gpu_target_on_aquire(struct GPU_Target_Asset asset) {
	struct GPU_Target gpu_target = {
		.textures = array_init(sizeof(struct Handle)),
		.buffers  = array_init(sizeof(struct GPU_Target_Buffer)),
		.size = {
			min_u32(asset.size.x, gs_graphics_state.limits.renderbuffer_size),
			min_u32(asset.size.y, gs_graphics_state.limits.renderbuffer_size),
		},
	};

	{ // prepare arrays
		uint32_t textures_count = 0;
		for (uint32_t i = 0; i < asset.count; i++) {
			if (!(asset.parameters[i].flags & TEXTURE_FLAG_OPAQUE)) {
				textures_count++;
			}
		}
		uint32_t buffers_count = asset.count - textures_count;

		array_resize(&gpu_target.textures, textures_count);
		array_resize(&gpu_target.buffers,  buffers_count);
	}

	// allocate
	glCreateFramebuffers(1, &gpu_target.id);
	for (uint32_t i = 0; i < asset.count; i++) {
		if (!(asset.parameters[i].flags & TEXTURE_FLAG_OPAQUE)) {
			// @idea: expose settings
			struct Handle const gh_texture = gpu_texture_init(&(struct Image){
				.size = gpu_target.size,
				.parameters = asset.parameters[i],
				.sampler = (struct Sampler_Settings){
					.wrap_x = WRAP_MODE_EDGE,
					.wrap_y = WRAP_MODE_EDGE,
				},
			});
			array_push_many(&gpu_target.textures, 1, &gh_texture);
		}
		else {
			struct GPU_Target_Buffer buffer = {
				.parameters = asset.parameters[i],
			};
			glCreateRenderbuffers(1, &buffer.id);
			glNamedRenderbufferStorage(
				buffer.id,
				gpu_sized_internal_format(asset.parameters[i].texture_type, asset.parameters[i].data_type),
				(GLsizei)gpu_target.size.x, (GLsizei)gpu_target.size.y
			);
			array_push_many(&gpu_target.buffers, 1, &buffer);
		}
	}

	// chart
	uint32_t attachments_count = 0;
	FOR_ARRAY(&gpu_target.textures, it) {
		struct Handle const * gh_texture = it.value;
		struct GPU_Texture const * gpu_texture = sparseset_get(&gs_graphics_state.textures, *gh_texture);
		if (gpu_texture == NULL) { continue; }

		uint32_t const attachment = attachments_count;
		if (gpu_texture->parameters.texture_type == TEXTURE_TYPE_COLOR) { attachments_count++; }

		GLint const level = 0;
		glNamedFramebufferTexture(
			gpu_target.id,
			gpu_attachment_point(gpu_texture->parameters.texture_type, attachment),
			gpu_texture->id,
			level
		);
	}
	FOR_ARRAY(&gpu_target.buffers, it) {
		struct GPU_Target_Buffer const * buffer = it.value;

		uint32_t const attachment = attachments_count;
		if (buffer->parameters.texture_type == TEXTURE_TYPE_COLOR) { attachments_count++; }

		glNamedFramebufferRenderbuffer(
			gpu_target.id,
			gpu_attachment_point(buffer->parameters.texture_type, attachment),
			GL_RENDERBUFFER,
			buffer->id
		);
	}

	GFX_TRACE("aquire target %d", gpu_target.id);
	return gpu_target;
}

static void gpu_target_on_discard(struct GPU_Target * gpu_target) {
	if (gpu_target->id == 0) { return; }
	GFX_TRACE("discard target %d", gpu_target->id);
	FOR_ARRAY(&gpu_target->textures, it) {
		struct Handle const * gh_texture = it.value;
		gpu_texture_free(*gh_texture);
	}
	FOR_ARRAY(&gpu_target->buffers, it) {
		struct GPU_Target_Buffer const * entry = it.value;
		glDeleteRenderbuffers(1, &entry->id);
	}
	array_free(&gpu_target->textures);
	array_free(&gpu_target->buffers);
	glDeleteFramebuffers(1, &gpu_target->id);
}

struct Handle gpu_target_init(struct GPU_Target_Asset asset) {
	struct GPU_Target gpu_target = gpu_target_on_aquire(asset);
	return sparseset_aquire(&gs_graphics_state.targets, &gpu_target);
}

static void gpu_target_free_immediately(struct Handle handle) {
	if (handle_equals(gs_graphics_state.active.gh_target, handle)) {
		gs_graphics_state.active.gh_target = (struct Handle){0};
	}
	struct GPU_Target * gpu_target = sparseset_get(&gs_graphics_state.targets, handle);
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

void gpu_target_update(struct Handle handle, struct GPU_Target_Asset asset) {
	struct GPU_Target * gpu_target = sparseset_get(&gs_graphics_state.targets, handle);
	if (gpu_target == NULL) { return; }

	gpu_target_on_discard(gpu_target);
	*gpu_target = gpu_target_on_aquire(asset);
}

struct uvec2 gpu_target_get_size(struct Handle handle) {
	struct GPU_Target const * gpu_target = sparseset_get(&gs_graphics_state.targets, handle);
	return (gpu_target != NULL)
		? gpu_target->size
		: (struct uvec2){0};
}

struct Handle gpu_target_get_texture_handle(struct Handle handle, enum Texture_Type type, uint32_t index) {
	struct GPU_Target const * gpu_target = sparseset_get(&gs_graphics_state.targets, handle);
	if (gpu_target == NULL) { return (struct Handle){0}; }

	uint32_t attachments_count = 0;
	FOR_ARRAY(&gpu_target->textures, it) {
		struct Handle const * gh_texture = it.value;
		struct GPU_Texture const * gpu_texture = sparseset_get(&gs_graphics_state.textures, *gh_texture);
		if (gpu_texture == NULL) { continue; }

		uint32_t const attachment = attachments_count;
		if (gpu_texture->parameters.texture_type == TEXTURE_TYPE_COLOR) { attachments_count++; }

		if (gpu_texture->parameters.texture_type == type && attachment == index) {
			return *gh_texture;
		}
	}

	WRN("failure: target doesn't have requested texture");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return (struct Handle){0};
}

// ----- ----- ----- ----- -----
//     GPU buffer part
// ----- ----- ----- ----- -----

static bool gpu_buffer_upload(struct GPU_Buffer * gpu_buffer, struct Buffer const * asset) {
	if (gpu_buffer->capacity < asset->size) { return false; }

	gpu_buffer->size = asset->size;
	if (asset->data == NULL) { return true; }
	if (asset->size == 0)    { return true; }

	glNamedBufferSubData(
		gpu_buffer->id, 0,
		(GLsizeiptr)asset->size,
		asset->data
	);
	return true;
}

static struct GPU_Buffer gpu_buffer_on_aquire(struct Buffer const * asset) {
	struct GPU_Buffer gpu_buffer = {
		.capacity = asset->size,
		.size = (asset->data != NULL)
			? gpu_buffer.capacity
			: 0,
	};

	if (asset->size == 0) { return gpu_buffer; }

	// allocate and upload
	glCreateBuffers(1, &gpu_buffer.id);
	glNamedBufferStorage(
		gpu_buffer.id,
		(GLsizeiptr)gpu_buffer.capacity,
		asset->data,
		GL_DYNAMIC_STORAGE_BIT
	);

	GFX_TRACE("aquire buffer %d", gpu_buffer.id);
	return gpu_buffer;
}

static void gpu_buffer_on_discard(struct GPU_Buffer * gpu_buffer) {
	if (gpu_buffer->id == 0) { return; }
	GFX_TRACE("discard buffer %d", gpu_buffer->id);
	glDeleteBuffers(1, &gpu_buffer->id);
}

struct Handle gpu_buffer_init(struct Buffer const * asset) {
	struct GPU_Buffer gpu_buffer = gpu_buffer_on_aquire(asset);
	return sparseset_aquire(&gs_graphics_state.buffers, &gpu_buffer);
}

static void gpu_buffer_free_immediately(struct Handle handle) {
	struct GPU_Buffer * gpu_buffer = sparseset_get(&gs_graphics_state.buffers, handle);
	if (gpu_buffer != NULL) {
		gpu_buffer_on_discard(gpu_buffer);
		sparseset_discard(&gs_graphics_state.buffers, handle);
	}
}

void gpu_buffer_free(struct Handle handle) {
	array_push_many(&gs_graphics_state.actions, 1, &(struct Graphics_Action){
		.handle = handle,
		.action = gpu_buffer_free_immediately,
	});
}

void gpu_buffer_update(struct Handle handle, struct Buffer const * asset) {
	struct GPU_Buffer * gpu_buffer = sparseset_get(&gs_graphics_state.buffers, handle);
	if (gpu_buffer == NULL) { return; }

	if (gpu_buffer_upload(gpu_buffer, asset)) { return; }

	gpu_buffer_on_discard(gpu_buffer);
	*gpu_buffer = gpu_buffer_on_aquire(asset);
}

// ----- ----- ----- ----- -----
//     GPU mesh part
// ----- ----- ----- ----- -----

static bool gpu_mesh_upload(struct GPU_Mesh * gpu_mesh, struct Mesh const * asset) {
	if (gpu_mesh->buffers.count != asset->count) { return false; }

	FOR_ARRAY(&gpu_mesh->parameters, it) {
		if (!equals(
			it.value, asset->parameters + it.curr
			, sizeof(struct Mesh_Parameters)
		)) { return false; }
	}

	FOR_ARRAY(&gpu_mesh->buffers, it) {
		struct Buffer const * asset_buffer = asset->buffers + it.curr;

		struct Handle const * gh_buffer = it.value;
		struct GPU_Buffer * gpu_buffer = sparseset_get(&gs_graphics_state.buffers, *gh_buffer);

		if (!gpu_buffer_upload(gpu_buffer, asset_buffer)) {
			return false;
		}
	}

	return true;
}

static struct GPU_Mesh gpu_mesh_on_aquire(struct Mesh const * asset) {
	struct GPU_Mesh gpu_mesh = {
		.buffers = array_init(sizeof(struct Handle)),
		.parameters = array_init(sizeof(struct Mesh_Parameters)),
	};

	{ // prepare arrays
		array_resize(&gpu_mesh.buffers, asset->count);
		array_resize(&gpu_mesh.parameters, asset->count);
	}

	// allocate and upload
	glCreateVertexArrays(1, &gpu_mesh.id);
	for (uint32_t i = 0; i < asset->count; i++) {
		struct Handle gh_buffer = gpu_buffer_init(asset->buffers + i);
		array_push_many(&gpu_mesh.buffers, 1, &gh_buffer);
		array_push_many(&gpu_mesh.parameters, 1, asset->parameters + i);
	}

	// chart
	uint32_t binding = 0;
	FOR_ARRAY(&gpu_mesh.buffers, it) {
		struct Handle const * gh_buffer = it.value;
		struct Mesh_Parameters const * parameters = array_at(&gpu_mesh.parameters, it.curr);

		struct GPU_Buffer const * gpu_buffer = sparseset_get(&gs_graphics_state.buffers, *gh_buffer);

		// element buffer
		if (parameters->flags & MESH_FLAG_INDEX) {
			glVertexArrayElementBuffer(gpu_mesh.id, gpu_buffer->id);
			continue;
		}

		// vertex buffer
		uint32_t vertex_size = 0;
		for (uint32_t atti = 0; atti < ATTRIBUTE_TYPE_INTERNAL_COUNT; atti++) {
			uint32_t const count = parameters->attributes[atti * 2 + 1];
			vertex_size += count * data_type_get_size(parameters->type);
		}

		GLintptr const offset = 0;
		glVertexArrayVertexBuffer(gpu_mesh.id, binding, gpu_buffer->id, offset, (GLsizei)vertex_size);

		uint32_t attribute_offset = 0;
		for (uint32_t atti = 0; atti < ATTRIBUTE_TYPE_INTERNAL_COUNT; atti++) {
			enum Attribute_Type const type = (enum Attribute_Type)parameters->attributes[atti * 2];
			if (type == ATTRIBUTE_TYPE_NONE) { continue; }

			uint32_t const count = parameters->attributes[atti * 2 + 1];
			if (count == 0) { continue; }

			GLuint const attribute = (GLuint)(type - 1);
			glEnableVertexArrayAttrib(gpu_mesh.id, attribute);
			glVertexArrayAttribBinding(gpu_mesh.id, attribute, binding);

			glVertexArrayAttribFormat(
				gpu_mesh.id, attribute,
				(GLint)count, gpu_vertex_value_type(parameters->type),
				GL_FALSE, attribute_offset
			);

			attribute_offset += count * data_type_get_size(parameters->type);
		}

		binding++;
	}

	GFX_TRACE("aquire mesh %d", gpu_mesh.id);
	return gpu_mesh;
}

static void gpu_mesh_on_discard(struct GPU_Mesh * gpu_mesh) {
	if (gpu_mesh->id == 0) { return; }
	GFX_TRACE("discard mesh %d", gpu_mesh->id);
	FOR_ARRAY(&gpu_mesh->buffers, it) {
		struct Handle const * gh_buffer = it.value;
		gpu_buffer_free(*gh_buffer);
	}
	array_free(&gpu_mesh->buffers);
	array_free(&gpu_mesh->parameters);
	glDeleteVertexArrays(1, &gpu_mesh->id);
}

struct Handle gpu_mesh_init(struct Mesh const * asset) {
	struct GPU_Mesh gpu_mesh = gpu_mesh_on_aquire(asset);
	return sparseset_aquire(&gs_graphics_state.meshes, &gpu_mesh);
}

static void gpu_mesh_free_immediately(struct Handle handle) {
	if (handle_equals(gs_graphics_state.active.gh_mesh, handle)) {
		gs_graphics_state.active.gh_mesh = (struct Handle){0};
	}
	struct GPU_Mesh * gpu_mesh = sparseset_get(&gs_graphics_state.meshes, handle);
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
	struct GPU_Mesh * gpu_mesh = sparseset_get(&gs_graphics_state.meshes, handle);
	if (gpu_mesh == NULL) { return; }

	if (gpu_mesh_upload(gpu_mesh, asset)) { return; }

	gpu_mesh_on_discard(gpu_mesh);
	*gpu_mesh = gpu_mesh_on_aquire(asset);
}

//
#include "framework/graphics/misc.h"

void graphics_update(void) {
	FOR_ARRAY(&gs_graphics_state.actions, it) {
		struct Graphics_Action const * entry = it.value;
		entry->action(entry->handle);
	}
	array_clear(&gs_graphics_state.actions);
}

struct mat4 graphics_projection_mat4(
	struct vec2 scale_xy, struct vec2 offset_xy,
	float view_near, float view_far, float ortho
) {
	return mat4_projection(
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

static uint32_t graphics_unit_find(struct Handle gh_texture) {
	FOR_ARRAY(&gs_graphics_state.units, it) {
		struct GPU_Unit const * unit = it.value;
		if (unit->gh_texture.id != gh_texture.id) { continue; }
		if (unit->gh_texture.gen != gh_texture.gen) { continue; }
		return it.curr + 1;
	}
	return 0;
}

static uint32_t gpu_unit_init(struct Handle gh_texture) {
	uint32_t const existing_unit = graphics_unit_find(gh_texture);
	if (existing_unit != 0) { return existing_unit; }

	uint32_t const free_id = graphics_unit_find((struct Handle){0});
	if (free_id == 0) {
		WRN("failure: no spare texture/sampler units");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return 0;
	}

	struct GPU_Texture const * gpu_texture = sparseset_get(&gs_graphics_state.textures, gh_texture);
	if (gpu_texture == NULL) {
		WRN("failure: no spare texture/sampler units");
		REPORT_CALLSTACK(); DEBUG_BREAK(); return 0;
	}

	struct GPU_Unit * unit = array_at(&gs_graphics_state.units, free_id - 1);
	unit->gh_texture = gh_texture;

	glBindTextureUnit((GLuint)free_id, gpu_texture->id);
	return free_id;
}

// static void gpu_unit_free(struct Handle gh_texture) {
// 	if (gs_ogl_version > 0) {
// 		uint32_t unit = graphics_unit_find(gh_texture);
// 		if (unit == 0) { return; }
// 
// 		gs_graphics_state.units[unit] = (struct GPU_Unit){
// 			.gpu_texture_handle = (struct Handle){0},
// 		};
// 
// 		glBindTextureUnit((GLuint)unit, 0);
// 	}
// }

static void gpu_select_program(struct Handle handle) {
	if (handle_equals(gs_graphics_state.active.gh_program, handle)) { return; }
	gs_graphics_state.active.gh_program = handle;
	struct GPU_Program const * gpu_program = sparseset_get(&gs_graphics_state.programs, handle);
	glUseProgram((gpu_program != NULL) ? gpu_program->id : 0);
}

static void gpu_select_target(struct Handle handle) {
	if (handle_equals(gs_graphics_state.active.gh_target, handle)) { return; }
	gs_graphics_state.active.gh_target = handle;
	struct GPU_Target const * gpu_target = sparseset_get(&gs_graphics_state.targets, handle);
	glBindFramebuffer(GL_FRAMEBUFFER, (gpu_target != NULL) ? gpu_target->id : 0);
}

static void gpu_select_mesh(struct Handle handle) {
	if (handle_equals(gs_graphics_state.active.gh_mesh, handle)) { return; }
	gs_graphics_state.active.gh_mesh = handle;
	struct GPU_Mesh const * gpu_mesh = sparseset_get(&gs_graphics_state.meshes, handle);
	glBindVertexArray((gpu_mesh != NULL) ? gpu_mesh->id : 0);
}

static void gpu_upload_single_uniform(struct GPU_Program const * gpu_program, struct GPU_Uniform_Internal const * field, void const * data) {
	switch (field->base.type) {
		default: {
			WRN("unsupported field type '0x%x'", field->base.type);
			REPORT_CALLSTACK(); DEBUG_BREAK();
		} break;

		case DATA_TYPE_UNIT_U:
		case DATA_TYPE_UNIT_S:
		case DATA_TYPE_UNIT_F: {
			GLint * units = alloca(sizeof(GLint) * field->base.array_size);
			uint32_t units_count = 0;

			// @todo: automatically rebind in a circular buffer manner
			struct Handle const * gh_textures = (struct Handle const *)data;
			for (uint32_t i = 0; i < field->base.array_size; i++) {
				uint32_t unit = graphics_unit_find(gh_textures[i]);
				if (unit == 0) {
					unit = gpu_unit_init(gh_textures[i]);
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

static void gpu_upload_uniforms(struct GPU_Program const * gpu_program, struct Gfx_Uniforms const * uniforms, uint32_t offset, uint32_t count) {
	for (uint32_t i = offset, last = offset + count; i < last; i++) {
		struct Gfx_Uniforms_Entry const * entry = array_at(&uniforms->headers, i);

		struct GPU_Uniform_Internal const * uniform = hashmap_get(&gpu_program->uniforms, &entry->id);
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
		struct GPU_Blend_Mode const gpu_mode = gpu_blend_mode(mode);
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
		struct GPU_Depth_Mode const gpu_mode = gpu_depth_mode(mode, reversed_z);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(gpu_mode.mask);
		glDepthFunc(gpu_mode.op);
	}
}

//
#include "framework/graphics/command.h"

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
	gpu_select_target(command->gh_target);

	struct uvec2 viewport_size = command->screen_size;
	if (!handle_is_null(command->gh_target)) {
		viewport_size = gpu_target_get_size(command->gh_target);
	}

	glViewport(0, 0, (GLsizei)viewport_size.x, (GLsizei)viewport_size.y);
}

inline static void gpu_execute_clear(struct GPU_Command_Clear const * command) {
	if (command->mask == TEXTURE_TYPE_NONE) {
		WRN("clear mask is empty");
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
	struct Gfx_Material const * material = material_system_take(command->mh_mat);
	struct Handle const gh_program = (material != NULL)
		? material->gh_program
		: (struct Handle){0};

	gpu_select_program(gh_program);
	gpu_set_blend_mode(material->blend_mode);
	gpu_set_depth_mode(material->depth_mode);

	struct GPU_Program const * gpu_program = sparseset_get(&gs_graphics_state.programs, gh_program);
	if (gpu_program != NULL) {
		gpu_upload_uniforms(gpu_program, &material->uniforms, 0, material->uniforms.headers.count);
	}
}

inline static void gpu_execute_shader(struct GPU_Command_Shader const * command) {
	gpu_select_program(command->gh_program);
	gpu_set_blend_mode(command->blend_mode);
	gpu_set_depth_mode(command->depth_mode);
}

inline static void gpu_execute_uniform(struct GPU_Command_Uniform const * command) {
	if (handle_is_null(command->gh_program)) {
		FOR_SPARSESET (&gs_graphics_state.programs, it) {
			gpu_upload_uniforms(it.value, command->uniforms, command->offset, command->count);
		}
	}
	else {
		struct GPU_Program const * gpu_program = sparseset_get(&gs_graphics_state.programs, command->gh_program);
		gpu_upload_uniforms(gpu_program, command->uniforms, command->offset, command->count);
	}
}

inline static void gpu_execute_buffer(struct GPU_Command_Buffer const * command) {
	struct GPU_Buffer const * gpu_buffer = sparseset_get(&gs_graphics_state.buffers, command->gh_buffer);
	if (gpu_buffer == NULL) { return; }

	glBindBufferRange(GL_SHADER_STORAGE_BUFFER, command->index, gpu_buffer->id, (GLsizeiptr)command->offset, (GLsizeiptr)command->length);
}

inline static void gpu_execute_draw(struct GPU_Command_Draw const * command) {
	gpu_select_mesh(command->gh_mesh);

	struct GPU_Mesh const * mesh = sparseset_get(&gs_graphics_state.meshes, command->gh_mesh);
	if (mesh == NULL) { return; }

	if (mesh->buffers.count == 0 && command->mode != MESH_MODE_NONE) {
		glDrawArraysInstanced(
			gpu_mesh_mode(command->mode),
			(GLint)command->offset,
			(GLsizei)command->count,
			(GLsizei)max_u32(command->instances, 1)
		);
	}

	FOR_ARRAY(&mesh->buffers, it) {
		struct Handle const * gh_buffer = it.value;
		struct GPU_Buffer const * gpu_buffer = sparseset_get(&gs_graphics_state.buffers, *gh_buffer);
		struct Mesh_Parameters const * parameters = array_at(&mesh->parameters, it.curr);
		if (parameters->mode == MESH_MODE_NONE) { continue; }
		if (gpu_buffer->size == 0) { continue; }

		uint32_t const offset = command->offset;
		uint32_t const count = (command->count != 0)
			? command->count
			: (uint32_t)(gpu_buffer->size / data_type_get_size(parameters->type));

		if (parameters->flags & MESH_FLAG_INDEX) {
			enum Data_Type const elements_type = parameters->type;
			size_t const bytes_offset = command->offset * data_type_get_size(elements_type);

			glDrawElementsInstanced(
				gpu_mesh_mode(parameters->mode),
				(GLsizei)count,
				gpu_index_value_type(elements_type),
				(void const *)bytes_offset,
				(GLsizei)max_u32(command->instances, 1)
			);
		}
		else {
			glDrawArraysInstanced(
				gpu_mesh_mode(parameters->mode),
				(GLint)offset,
				(GLsizei)count,
				(GLsizei)max_u32(command->instances, 1)
			);
		}
	}
}

void gpu_execute(uint32_t length, struct GPU_Command const * commands) {
	for (uint32_t i = 0; i < length; i++) {
		struct GPU_Command const * command = commands + i;
		switch (command->type) {
			default: {
				ERR("unknown command");
				REPORT_CALLSTACK(); DEBUG_BREAK();
			} break;

			case GPU_COMMAND_TYPE_CULL:     gpu_execute_cull(&command->as.cull);         break;
			case GPU_COMMAND_TYPE_TARGET:   gpu_execute_target(&command->as.target);     break;
			case GPU_COMMAND_TYPE_CLEAR:    gpu_execute_clear(&command->as.clear);       break;
			case GPU_COMMAND_TYPE_MATERIAL: gpu_execute_material(&command->as.material); break;
			case GPU_COMMAND_TYPE_SHADER:   gpu_execute_shader(&command->as.shader);     break;
			case GPU_COMMAND_TYPE_UNIFORM:  gpu_execute_uniform(&command->as.uniform);   break;
			case GPU_COMMAND_TYPE_BUFFER:   gpu_execute_buffer(&command->as.buffer);     break;
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

	gs_graphics_state.actions = array_init(sizeof(struct Graphics_Action));
	gs_graphics_state.units   = array_init(sizeof(struct GPU_Unit));

	// init gpu objects
	gs_graphics_state.programs = sparseset_init(sizeof(struct GPU_Program));
	gs_graphics_state.targets  = sparseset_init(sizeof(struct GPU_Target));
	gs_graphics_state.textures = sparseset_init(sizeof(struct GPU_Texture));
	gs_graphics_state.buffers  = sparseset_init(sizeof(struct GPU_Buffer));
	gs_graphics_state.meshes   = sparseset_init(sizeof(struct GPU_Mesh));

	//
	GLint max_units
	    , max_units_vertex_shader
	    , max_units_fragment_shader
	    , max_units_compute_shader;

	GLint max_uniform_blocks
	    , max_vertex_uniform_blocks
	    , max_fragment_uniform_blocks
	    , max_compute_uniform_blocks;

	GLint max_shader_storage_blocks
	    , max_vertex_shader_storage_blocks
	    , max_fragment_shader_storage_blocks
	    , max_compute_shader_storage_blocks;

	GLint max_texture_size, max_renderbuffer_size;
	GLint max_elements_vertices, max_elements_indices;
	GLint max_uniform_locations;

	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,   &max_units);
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS,            &max_units_fragment_shader);
	glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS,     &max_units_vertex_shader);
	glGetIntegerv(GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS,    &max_units_compute_shader);

	glGetIntegerv(GL_MAX_COMBINED_UNIFORM_BLOCKS,        &max_uniform_blocks);
	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_BLOCKS,          &max_vertex_uniform_blocks);
	glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_BLOCKS,        &max_fragment_uniform_blocks);
	glGetIntegerv(GL_MAX_COMPUTE_UNIFORM_BLOCKS,         &max_compute_uniform_blocks);

	glGetIntegerv(GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS, &max_shader_storage_blocks);
	glGetIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS,   &max_vertex_shader_storage_blocks);
	glGetIntegerv(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS, &max_fragment_shader_storage_blocks);
	glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS,  &max_compute_shader_storage_blocks);

	glGetIntegerv(GL_MAX_TEXTURE_SIZE,                   &max_texture_size);
	glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE,              &max_renderbuffer_size);
	glGetIntegerv(GL_MAX_ELEMENTS_VERTICES,              &max_elements_vertices);
	glGetIntegerv(GL_MAX_ELEMENTS_INDICES,               &max_elements_indices);
	glGetIntegerv(GL_MAX_UNIFORM_LOCATIONS,              &max_uniform_locations);

	LOG(
		"> OpenGL limits:\n"
		"  units ........... %d\n"
		"  - VS ............ %d\n"
		"  - FS ............ %d\n"
		"  - CS ............ %d\n"
		//
		"  uniform blocks .. %d\n"
		"  - VS ............ %d\n"
		"  - FS ............ %d\n"
		"  - CS ............ %d\n"
		//
		"  storage blocks .. %d\n"
		"  - VS ............ %d\n"
		"  - FS ............ %d\n"
		"  - CS ............ %d\n"
		//
		"  texture size .... %d\n"
		"  target size ..... %d\n"
		"  vertices ........ %d\n"
		"  indices ......... %d\n"
		"  uniforms ........ %d\n"
		""
		, max_units
		, max_units_vertex_shader
		, max_units_fragment_shader
		, max_units_compute_shader
		//
		, max_uniform_blocks
		, max_vertex_uniform_blocks
		, max_fragment_uniform_blocks
		, max_compute_uniform_blocks
		//
		, max_shader_storage_blocks
		, max_vertex_shader_storage_blocks
		, max_fragment_shader_storage_blocks
		, max_compute_shader_storage_blocks
		//
		, max_texture_size
		, max_renderbuffer_size
		, max_elements_vertices
		, max_elements_indices
		, max_uniform_locations
	);

	gs_graphics_state.limits = (struct Graphics_State_Limits){
		.units_vs          = (uint32_t)max_units_vertex_shader,
		.units_fs          = (uint32_t)max_units_fragment_shader,
		.units_cs          = (uint32_t)max_units_compute_shader,
		//
		.uniform_blocks_vs = (uint32_t)max_vertex_uniform_blocks,
		.uniform_blocks_fs = (uint32_t)max_fragment_uniform_blocks,
		.uniform_blocks_cs = (uint32_t)max_compute_uniform_blocks,
		//
		.storage_blocks_vs = (uint32_t)max_vertex_shader_storage_blocks,
		.storage_blocks_fs = (uint32_t)max_fragment_shader_storage_blocks,
		.storage_blocks_cs = (uint32_t)max_compute_shader_storage_blocks,
		//
		.texture_size      = (uint32_t)max_texture_size,
		.renderbuffer_size = (uint32_t)max_renderbuffer_size,
		.elements_vertices = (uint32_t)max_elements_vertices,
		.elements_indices  = (uint32_t)max_elements_indices,
		.uniform_locations = (uint32_t)max_uniform_locations,
	};

	array_resize(&gs_graphics_state.units, (uint32_t)max_units);
	for (uint32_t i = 0; i < gs_graphics_state.units.capacity; i++) {
		array_push_many(&gs_graphics_state.units, 1, &(struct GPU_Unit){0});
	}

	// @note: manage OpenGL's clip space instead of ours
	bool const can_control_clip_space = (gs_ogl_version >= 45) || contains_full_word(gs_graphics_state.extensions, S_("GL_ARB_clip_control"));
	if (can_control_clip_space) { glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE); }

	bool const reverse_z = true || can_control_clip_space;
	gs_graphics_state.clip_space = (struct GPU_Clip_Space){
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
	GPU_FREE(buffers,  gpu_buffer_on_discard);
	GPU_FREE(meshes,   gpu_mesh_on_discard);

	//
	array_free(&gs_graphics_state.units);
	array_free(&gs_graphics_state.actions);
	MEMORY_FREE(gs_graphics_state.extensions);
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
	for (GLint i = 0; i < extensions_count; i++) {
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
	ERR("%s", buffer);
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
	ERR("%s", buffer);
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
