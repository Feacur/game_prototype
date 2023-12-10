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

#include "framework/graphics/gfx_material.h"

#include "framework/systems/buffer_system.h"
#include "framework/systems/string_system.h"
#include "framework/systems/action_system.h"
#include "framework/systems/material_system.h"

#include "functions.h"
#include "gpu_types.h"

// @todo: GPU scissor test
// @todo: expose screen buffer settings, as well as OpenGL's
// @idea: use dedicated samplers instead of texture defaults
// @idea: support older OpenGL versions (pre direct state access, which is 4.5)

#define GFX_TRACE(format, ...) formatter_log("[gfx] " format "\n", ## __VA_ARGS__)

struct Graphics_Limits {
	uint32_t units;
	uint32_t units_vs;
	uint32_t units_fs;
	uint32_t units_cs;
	//
	uint32_t uniform_blocks;
	uint32_t uniform_blocks_vs;
	uint32_t uniform_blocks_fs;
	uint32_t uniform_blocks_cs;
	uint32_t uniform_blocks_size;
	uint32_t uniform_blocks_alignment;
	//
	uint32_t storage_blocks;
	uint32_t storage_blocks_vs;
	uint32_t storage_blocks_fs;
	uint32_t storage_blocks_cs;
	uint32_t storage_blocks_size;
	uint32_t storage_blocks_alignment;
	//
	uint32_t texture_size;
	uint32_t target_size;
};

static struct Graphics_State {
	struct Buffer          extensions;
	struct Graphics_Limits limits;

	struct Array     units;    // `struct GPU_Unit`
	struct Sparseset programs; // `struct GPU_Program_Internal`
	struct Sparseset targets;  // `struct GPU_Target_Internal`
	struct Sparseset textures; // `struct GPU_Texture_Internal`
	struct Sparseset buffers;  // `struct GPU_Buffer_Internal`
	struct Sparseset meshes;   // `struct GPU_Mesh_Internal`

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
} gs_graphics_state;

//
#include "framework/graphics/gfx_objects.h"

static uint32_t get_buffer_mode_size(enum Buffer_Mode mode) {
	switch (mode) {
		case BUFFER_MODE_NONE:    return 0;
		case BUFFER_MODE_UNIFORM: return gs_graphics_state.limits.uniform_blocks_size;
		case BUFFER_MODE_STORAGE: return gs_graphics_state.limits.storage_blocks_size;
	}
	return 0;
}

static uint32_t get_buffer_mode_alignment(enum Buffer_Mode mode) {
	switch (mode) {
		case BUFFER_MODE_NONE:    return 0;
		case BUFFER_MODE_UNIFORM: return gs_graphics_state.limits.uniform_blocks_alignment;
		case BUFFER_MODE_STORAGE: return gs_graphics_state.limits.storage_blocks_alignment;
	}
	return 0;
}

// ----- ----- ----- ----- -----
//     GPU program part
// ----- ----- ----- ----- -----

static void verify_shader(GLuint id);
static void verify_program(GLuint id);

static void gpu_program_introspect_uniforms(struct GPU_Program_Internal * gpu_program) {
	enum Param {
		PARAM_NAME_LENGTH,
		PARAM_TYPE,
		PARAM_ARRAY_SIZE,
		PARAM_BLOCK_INDEX,
		PARAM_ARRAY_STRIDE,
		PARAM_LOCATION,
	};
	static GLenum const c_props[] = {
		[PARAM_NAME_LENGTH]      = GL_NAME_LENGTH,
		[PARAM_TYPE]             = GL_TYPE,
		[PARAM_ARRAY_SIZE]       = GL_ARRAY_SIZE,
		[PARAM_BLOCK_INDEX]      = GL_BLOCK_INDEX,
		[PARAM_ARRAY_STRIDE]     = GL_ARRAY_STRIDE,
		[PARAM_LOCATION]         = GL_LOCATION,
	};

	GLint count;
	gl.GetProgramInterfaceiv(gpu_program->id, GL_UNIFORM, GL_ACTIVE_RESOURCES, &count);
	hashmap_ensure(&gpu_program->base.uniforms, (uint32_t)count);
	for (GLint i = 0; i < count; i++) {

		GLint params[SIZE_OF_ARRAY(c_props)];
		gl.GetProgramResourceiv(gpu_program->id, GL_UNIFORM, (GLuint)i, SIZE_OF_ARRAY(c_props), c_props, SIZE_OF_ARRAY(params), NULL, params);

		// @note: skip blocks
		if (params[PARAM_BLOCK_INDEX] >= 0) { continue; }
		if (params[PARAM_LOCATION]     < 0) { continue; }

		GLsizei name_length = params[PARAM_NAME_LENGTH] - 1;
		char * name_data = BUFFER_ALLOCATE_SIZE((size_t)name_length + 1);
		gl.GetProgramResourceName(gpu_program->id, GL_UNIFORM, (GLuint)i, (GLsizei)name_length + 1, NULL, name_data);

		// TRC(
		// 	"array %5d block %5d stride %5d loc %5d -- %.*s"
		// 	, params[PARAM_ARRAY_SIZE]
		// 	, params[PARAM_BLOCK_INDEX]
		// 	, params[PARAM_ARRAY_STRIDE]
		// 	, params[PARAM_LOCATION]
		// 	, name_length, name_data
		// );

		uint32_t const id = string_system_add((struct CString){
			.length = (uint32_t)name_length,
			.data = name_data,
		});
		hashmap_set(&gpu_program->base.uniforms, &id, &(struct GPU_Uniform_Internal){
			.base = {
				.type = translate_program_data_type(params[PARAM_TYPE]),
				.array_size = (uint32_t)params[PARAM_ARRAY_SIZE],
			},
			.index = (GLuint)i,
			.location = params[PARAM_LOCATION],
		});
	}
}

static void gpu_program_introspect_block(struct GPU_Program_Internal * gpu_program, GLenum interface) {
	enum Param {
		PARAM_NAME_LENGTH,
		PARAM_BUFFER_BINDING,
		PARAM_BUFFER_DATA_SIZE,
	};
	static GLenum const c_props[] = {
		[PARAM_NAME_LENGTH]      = GL_NAME_LENGTH,
		[PARAM_BUFFER_BINDING]   = GL_BUFFER_BINDING,
		[PARAM_BUFFER_DATA_SIZE] = GL_BUFFER_DATA_SIZE,
	};

	GLint count;
	gl.GetProgramInterfaceiv(gpu_program->id, interface, GL_ACTIVE_RESOURCES, &count);
	for (GLint i = 0; i < count; i++) {

		GLint params[SIZE_OF_ARRAY(c_props)];
		gl.GetProgramResourceiv(gpu_program->id, interface, (GLuint)i, SIZE_OF_ARRAY(c_props), c_props, SIZE_OF_ARRAY(params), NULL, params);

		GLsizei name_length = params[PARAM_NAME_LENGTH] - 1;
		char * name_data = BUFFER_ALLOCATE_SIZE((size_t)name_length + 1);
		gl.GetProgramResourceName(gpu_program->id, interface, (GLuint)i, (GLsizei)name_length + 1, NULL, name_data);

		TRC(
			"bind %5d size %5d -- %.*s"
			, params[PARAM_BUFFER_BINDING]
			, params[PARAM_BUFFER_DATA_SIZE]
			, name_length, name_data
		);
	}
}

static void gpu_program_introspect_buffer(struct GPU_Program_Internal * gpu_program) {
	enum Param {
		PARAM_NAME_LENGTH,
		PARAM_ARRAY_SIZE,
		PARAM_BLOCK_INDEX,
		PARAM_ARRAY_STRIDE,
		PARAM_TOP_LEVEL_ARRAY_SIZE,
		PARAM_TOP_LEVEL_ARRAY_STRIDE,
	};
	static GLenum const c_props[] = {
		[PARAM_NAME_LENGTH]      = GL_NAME_LENGTH,
		[PARAM_ARRAY_SIZE]       = GL_ARRAY_SIZE,
		[PARAM_BLOCK_INDEX]      = GL_BLOCK_INDEX,
		[PARAM_ARRAY_STRIDE]     = GL_ARRAY_STRIDE,
		[PARAM_TOP_LEVEL_ARRAY_SIZE]         = GL_TOP_LEVEL_ARRAY_SIZE,
		[PARAM_TOP_LEVEL_ARRAY_STRIDE]         = GL_TOP_LEVEL_ARRAY_STRIDE,
	};

	GLint count;
	gl.GetProgramInterfaceiv(gpu_program->id, GL_BUFFER_VARIABLE, GL_ACTIVE_RESOURCES, &count);
	for (GLint i = 0; i < count; i++) {

		GLint params[SIZE_OF_ARRAY(c_props)];
		gl.GetProgramResourceiv(gpu_program->id, GL_BUFFER_VARIABLE, (GLuint)i, SIZE_OF_ARRAY(c_props), c_props, SIZE_OF_ARRAY(params), NULL, params);

		GLsizei name_length = params[PARAM_NAME_LENGTH] - 1;
		char * name_data = BUFFER_ALLOCATE_SIZE((size_t)name_length + 1);
		gl.GetProgramResourceName(gpu_program->id, GL_BUFFER_VARIABLE, (GLuint)i, (GLsizei)name_length + 1, NULL, name_data);

		TRC(
			"array %5d block %5d stride %5d tl array %5d tl stride %5d -- %.*s"
			, params[PARAM_ARRAY_SIZE]
			, params[PARAM_BLOCK_INDEX]
			, params[PARAM_ARRAY_STRIDE]
			, params[PARAM_TOP_LEVEL_ARRAY_SIZE]
			, params[PARAM_TOP_LEVEL_ARRAY_STRIDE]
			, name_length, name_data
		);
	}
}

static void gpu_program_introspect_input(struct GPU_Program_Internal * gpu_program) {
	enum Param {
		PARAM_NAME_LENGTH,
		PARAM_TYPE,
		PARAM_ARRAY_SIZE,
		PARAM_LOCATION,
		PARAM_LOCATION_COMPONENT,
	};
	static GLenum const c_props[] = {
		[PARAM_NAME_LENGTH]        = GL_NAME_LENGTH,
		[PARAM_TYPE]               = GL_TYPE,
		[PARAM_ARRAY_SIZE]         = GL_ARRAY_SIZE,
		[PARAM_LOCATION]           = GL_LOCATION,
		[PARAM_LOCATION_COMPONENT] = GL_LOCATION_COMPONENT,
	};

	GLint count;
	gl.GetProgramInterfaceiv(gpu_program->id, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, &count);
	for (GLint i = 0; i < count; i++) {

		GLint params[SIZE_OF_ARRAY(c_props)];
		gl.GetProgramResourceiv(gpu_program->id, GL_PROGRAM_INPUT, (GLuint)i, SIZE_OF_ARRAY(c_props), c_props, SIZE_OF_ARRAY(params), NULL, params);

		if (params[PARAM_LOCATION] < 0) { continue; }

		GLsizei name_length = params[PARAM_NAME_LENGTH] - 1;
		char * name_data = BUFFER_ALLOCATE_SIZE((size_t)name_length + 1);
		gl.GetProgramResourceName(gpu_program->id, GL_PROGRAM_INPUT, (GLuint)i, (GLsizei)name_length + 1, NULL, name_data);

		TRC(
			"array %5d location %5d component %5d -- %.*s"
			, params[PARAM_ARRAY_SIZE]
			, params[PARAM_LOCATION]
			, params[PARAM_LOCATION_COMPONENT]
			, name_length, name_data
		);
	}
}

static void gpu_program_introspect_output(struct GPU_Program_Internal * gpu_program) {
	enum Param {
		PARAM_NAME_LENGTH,
		PARAM_TYPE,
		PARAM_ARRAY_SIZE,
		PARAM_LOCATION,
		PARAM_LOCATION_INDEX,
		PARAM_LOCATION_COMPONENT,
	};
	static GLenum const c_props[] = {
		[PARAM_NAME_LENGTH]        = GL_NAME_LENGTH,
		[PARAM_TYPE]               = GL_TYPE,
		[PARAM_ARRAY_SIZE]         = GL_ARRAY_SIZE,
		[PARAM_LOCATION]           = GL_LOCATION,
		[PARAM_LOCATION_INDEX]     = GL_LOCATION_INDEX,
		[PARAM_LOCATION_COMPONENT] = GL_LOCATION_COMPONENT,
	};

	GLint count;
	gl.GetProgramInterfaceiv(gpu_program->id, GL_PROGRAM_OUTPUT, GL_ACTIVE_RESOURCES, &count);
	for (GLint i = 0; i < count; i++) {

		GLint params[SIZE_OF_ARRAY(c_props)];
		gl.GetProgramResourceiv(gpu_program->id, GL_PROGRAM_OUTPUT, (GLuint)i, SIZE_OF_ARRAY(c_props), c_props, SIZE_OF_ARRAY(params), NULL, params);

		if (params[PARAM_LOCATION] < 0) { continue; }

		GLsizei name_length = params[PARAM_NAME_LENGTH] - 1;
		char * name_data = BUFFER_ALLOCATE_SIZE((size_t)name_length + 1);
		gl.GetProgramResourceName(gpu_program->id, GL_PROGRAM_OUTPUT, (GLuint)i, (GLsizei)name_length + 1, NULL, name_data);

		TRC(
			"array %5d location %5d index %5d component %5d -- %.*s"
			, params[PARAM_ARRAY_SIZE]
			, params[PARAM_LOCATION]
			, params[PARAM_LOCATION_INDEX]
			, params[PARAM_LOCATION_COMPONENT]
			, name_length, name_data
		);
	}
}

static struct GPU_Program_Internal gpu_program_on_aquire(struct Buffer const * asset) {
#define ADD_SECTION_HEADER(shader_type, target) \
	do { \
		if (common_strstr((char const *)asset->data, #shader_type)) {\
			if (gl.version < (target)) { \
				ERR("'" #shader_type "' is unavailable"); \
				DEBUG_BREAK(); \
			} \
			sections[sections_count++] = (struct Section_Header){ \
				.type = GL_##shader_type, \
				.text = S_("#define " #shader_type "\n\n"), \
			}; \
		} \
	} while (false) \

	struct GPU_Program_Internal gpu_program = {.base = {
		.uniforms = hashmap_init(&hash32, sizeof(uint32_t), sizeof(struct GPU_Uniform_Internal)),
	}};

	// common header
	GLchar header[1024];
	uint32_t header_length = formatter_fmt(
		SIZE_OF_ARRAY(header), header, ""
		"#version %u core\n"
		"\n"
		"#define ATTRIBUTE_POSITION layout(location = %u) in\n"
		"#define ATTRIBUTE_TEXCOORD layout(location = %u) in\n"
		"#define ATTRIBUTE_NORMAL   layout(location = %u) in\n"
		"#define ATTRIBUTE_COLOR    layout(location = %u) in\n"
		"\n"
		"#define BLOCK_GLOBAL  layout(std140, binding = %u) uniform\n"
		"#define BLOCK_CAMERA  layout(std140, binding = %u) uniform\n"
		"#define BLOCK_MODEL   layout(std140, binding = %u) uniform\n"
		"#define BLOCK_DYNAMIC layout(std430, binding = %u) readonly buffer\n"
		"\n"
		"#define DEPTH_NEAR %g\n"
		"#define DEPTH_FAR  %g\n"
		"#define NDC_NEAR %g\n"
		"#define NDC_FAR  %g\n"
		"\n",
		gl.glsl,
		//
		ATTRIBUTE_TYPE_POSITION - 1,
		ATTRIBUTE_TYPE_TEXCOORD - 1,
		ATTRIBUTE_TYPE_NORMAL - 1,
		ATTRIBUTE_TYPE_COLOR - 1,
		//
		BLOCK_TYPE_GLOBAL - 1,
		BLOCK_TYPE_CAMERA - 1,
		BLOCK_TYPE_MODEL - 1,
		BLOCK_TYPE_DYNAMIC - 1,
		//
		(double)gs_graphics_state.clip_space.depth_near,
		(double)gs_graphics_state.clip_space.depth_far,
		(double)gs_graphics_state.clip_space.ndc_near,
		(double)gs_graphics_state.clip_space.ndc_far
	);

	// section headers
	struct Section_Header {
		GLenum type;
		struct CString text;
	};

	uint32_t sections_count = 0;
	struct Section_Header sections[3];
	ADD_SECTION_HEADER(VERTEX_SHADER,   20);
	ADD_SECTION_HEADER(FRAGMENT_SHADER, 20);
	ADD_SECTION_HEADER(COMPUTE_SHADER,  43);

	// compile shader objects
	GLuint shader_ids[ARRAYSIZE(sections)];
	for (uint32_t i = 0; i < sections_count; i++) {
		GLchar const * code[]   = {header,               sections[i].text.data,          (GLchar *)asset->data};
		GLint const    length[] = {(GLint)header_length, (GLint)sections[i].text.length, (GLint)asset->size};

		GLuint shader_id = gl.CreateShader(sections[i].type);
		gl.ShaderSource(shader_id, SIZE_OF_ARRAY(code), code, length);
		gl.CompileShader(shader_id);
		verify_shader(shader_id);

		shader_ids[i] = shader_id;
	}

	// link shader objects into a program
	gpu_program.id = gl.CreateProgram();
	for (uint32_t i = 0; i < sections_count; i++) {
		gl.AttachShader(gpu_program.id, shader_ids[i]);
	}

	gl.LinkProgram(gpu_program.id);
	verify_program(gpu_program.id);

	// free redundant resources
	for (uint32_t i = 0; i < sections_count; i++) {
		gl.DetachShader(gpu_program.id, shader_ids[i]);
		gl.DeleteShader(shader_ids[i]);
	}

	// introspect the program
	gpu_program_introspect_uniforms(&gpu_program);
	(void)gpu_program_introspect_block; // (&gpu_program, GL_UNIFORM_BLOCK);
	(void)gpu_program_introspect_block; // (&gpu_program, GL_SHADER_STORAGE_BLOCK);
	(void)gpu_program_introspect_buffer; // (&gpu_program);
	(void)gpu_program_introspect_input; // (&gpu_program);
	(void)gpu_program_introspect_output; // (&gpu_program);

	GFX_TRACE("aquire program %d", gpu_program.id);
	return gpu_program;
	// https://www.khronos.org/opengl/wiki/Program_Introspection

#undef ADD_HEADER
}

static void gpu_program_on_discard(struct GPU_Program_Internal * gpu_program) {
	if (gpu_program->id == 0) { return; }
	GFX_TRACE("discard program %d", gpu_program->id);
	hashmap_free(&gpu_program->base.uniforms);
	gl.DeleteProgram(gpu_program->id);
}

struct Handle gpu_program_init(struct Buffer const * asset) {
	struct GPU_Program_Internal gpu_program = gpu_program_on_aquire(asset);
	return sparseset_aquire(&gs_graphics_state.programs, &gpu_program);
}

void gpu_program_free(struct Handle handle) {
	if (handle_equals(gs_graphics_state.active.gh_program, handle)) {
		gs_graphics_state.active.gh_program = (struct Handle){0};
	}
	struct GPU_Program_Internal * gpu_program = sparseset_get(&gs_graphics_state.programs, handle);
	if (gpu_program != NULL) {
		gpu_program_on_discard(gpu_program);
		sparseset_discard(&gs_graphics_state.programs, handle);
	}
}

void gpu_program_update(struct Handle handle, struct Buffer const * asset) {
	struct GPU_Program_Internal * gpu_program = sparseset_get(&gs_graphics_state.programs, handle);
	if (gpu_program == NULL) { return; }

	gpu_program_on_discard(gpu_program);
	*gpu_program = gpu_program_on_aquire(asset);
}

struct GPU_Program const * gpu_program_get(struct Handle handle) {
	return sparseset_get(&gs_graphics_state.programs, handle);
}

// ----- ----- ----- ----- -----
//     GPU texture part
// ----- ----- ----- ----- -----

static bool gpu_texture_upload(struct GPU_Texture_Internal * gpu_texture, struct Image const * asset) {
	if (gpu_texture->base.size.x != asset->size.x) { return false; }
	if (gpu_texture->base.size.y != asset->size.y) { return false; }

	if (!equals(
		&gpu_texture->base.parameters, &asset->parameters
		, sizeof(struct Texture_Settings)
	)) { return false; }

	if (!equals(
		&gpu_texture->base.settings, &asset->settings
		, sizeof(struct Texture_Settings)
	)) { return false; }

	if (!equals(
		&gpu_texture->base.sampler, &asset->sampler
		, sizeof(struct Texture_Settings)
	)) { return false; }

	if (asset->data == NULL) { return true; }
	if (asset->size.x == 0)  { return true; }
	if (asset->size.y == 0)  { return true; }

	gl.TextureSubImage2D(
		gpu_texture->id, 0,
		0, 0, (GLsizei)asset->size.x, (GLsizei)asset->size.y,
		gpu_pixel_data_format(asset->parameters.texture_type, asset->parameters.data_type),
		gpu_pixel_data_type(asset->parameters.texture_type, asset->parameters.data_type),
		asset->data
	);
	if (gpu_texture->base.settings.max_lod != 0) {
		gl.GenerateTextureMipmap(gpu_texture->id);
	}

	return true;
}

static struct GPU_Texture_Internal gpu_texture_on_aquire(struct Image const * asset) {
	struct GPU_Texture_Internal gpu_texture = {.base = {
		.size = {
			min_u32(asset->size.x, gs_graphics_state.limits.texture_size),
			min_u32(asset->size.y, gs_graphics_state.limits.texture_size),
		},
		.parameters = asset->parameters,
		.settings   = asset->settings,
		.sampler    = asset->sampler,
	}};
	if (gpu_texture.base.size.x < asset->size.x) { WRN("texture x exceeds limits"); DEBUG_BREAK(); }
	if (gpu_texture.base.size.y < asset->size.y) { WRN("texture y exceeds limits"); DEBUG_BREAK(); }

	if (gpu_texture.base.size.x == 0)  { return gpu_texture; }
	if (gpu_texture.base.size.y == 0)  { return gpu_texture; }

	// allocate and upload
	gl.CreateTextures(GL_TEXTURE_2D, 1, &gpu_texture.id);
	gl.TextureStorage2D(
		gpu_texture.id, (GLsizei)(gpu_texture.base.settings.max_lod + 1)
		, gpu_sized_internal_format(gpu_texture.base.parameters.texture_type, gpu_texture.base.parameters.data_type)
		, (GLsizei)gpu_texture.base.size.x, (GLsizei)gpu_texture.base.size.y
	);
	gpu_texture_upload(&gpu_texture, asset);

	// chart
	gl.TextureParameteri(gpu_texture.id, GL_TEXTURE_MAX_LEVEL, (GLint)gpu_texture.base.settings.max_lod);
	gl.TextureParameteriv(gpu_texture.id, GL_TEXTURE_SWIZZLE_RGBA, (GLint[]){
		gpu_swizzle_op(gpu_texture.base.settings.swizzle[0], 0),
		gpu_swizzle_op(gpu_texture.base.settings.swizzle[1], 1),
		gpu_swizzle_op(gpu_texture.base.settings.swizzle[2], 2),
		gpu_swizzle_op(gpu_texture.base.settings.swizzle[3], 3),
	});

	// @todo: separate sampler objects
	gl.TextureParameterfv(gpu_texture.id, GL_TEXTURE_BORDER_COLOR, &gpu_texture.base.sampler.border.x);
	gl.TextureParameteri(gpu_texture.id, GL_TEXTURE_MIN_FILTER, gpu_min_filter_mode(gpu_texture.base.sampler.mipmap, gpu_texture.base.sampler.minification));
	gl.TextureParameteri(gpu_texture.id, GL_TEXTURE_MAG_FILTER, gpu_mag_filter_mode(gpu_texture.base.sampler.magnification));
	gl.TextureParameteri(gpu_texture.id, GL_TEXTURE_WRAP_S, gpu_wrap_mode(gpu_texture.base.sampler.wrap_x));
	gl.TextureParameteri(gpu_texture.id, GL_TEXTURE_WRAP_T, gpu_wrap_mode(gpu_texture.base.sampler.wrap_y));

	GFX_TRACE("aquire texture %d", gpu_texture.id);
	return gpu_texture;
}

static void gpu_texture_on_discard(struct GPU_Texture_Internal * gpu_texture) {
	if (gpu_texture->id == 0) { return; }
	GFX_TRACE("discard texture %d", gpu_texture->id);
	gl.DeleteTextures(1, &gpu_texture->id);
}

struct Handle gpu_texture_init(struct Image const * asset) {
	struct GPU_Texture_Internal gpu_texture = gpu_texture_on_aquire(asset);
	return sparseset_aquire(&gs_graphics_state.textures, &gpu_texture);
}

void gpu_texture_free(struct Handle handle) {
	FOR_ARRAY(&gs_graphics_state.units, it) {
		struct GPU_Unit * unit = it.value;
		if (handle_equals(unit->gh_texture, handle)) {
			unit->gh_texture = (struct Handle){0};
		}
	}
	struct GPU_Texture_Internal * gpu_texture = sparseset_get(&gs_graphics_state.textures, handle);
	if (gpu_texture != NULL) {
		gpu_texture_on_discard(gpu_texture);
		sparseset_discard(&gs_graphics_state.textures, handle);
	}
}

void gpu_texture_update(struct Handle handle, struct Image const * asset) {
	struct GPU_Texture_Internal * gpu_texture = sparseset_get(&gs_graphics_state.textures, handle);
	if (gpu_texture == NULL) { return; }

	if (gpu_texture_upload(gpu_texture, asset)) { return; }

	gpu_texture_on_discard(gpu_texture);
	*gpu_texture = gpu_texture_on_aquire(asset);
}

struct GPU_Texture const * gpu_texture_get(struct Handle handle) {
	return sparseset_get(&gs_graphics_state.textures, handle);
}

// ----- ----- ----- ----- -----
//     GPU target part
// ----- ----- ----- ----- -----

static struct GPU_Target_Internal gpu_target_on_aquire(struct GPU_Target_Asset asset) {
	struct GPU_Target_Internal gpu_target = {.base = {
		.textures = array_init(sizeof(struct Handle)),
		.buffers  = array_init(sizeof(struct GPU_Target_Buffer_Internal)),
		.size = {
			min_u32(asset.size.x, gs_graphics_state.limits.target_size),
			min_u32(asset.size.y, gs_graphics_state.limits.target_size),
		},
	}};
	if (gpu_target.base.size.x < asset.size.x) { WRN("target x exceeds limits"); DEBUG_BREAK(); }
	if (gpu_target.base.size.y < asset.size.y) { WRN("target y exceeds limits"); DEBUG_BREAK(); }

	{ // prepare arrays
		uint32_t textures_count = 0;
		for (uint32_t i = 0; i < asset.count; i++) {
			if (!(asset.parameters[i].flags & TEXTURE_FLAG_OPAQUE)) {
				textures_count++;
			}
		}
		uint32_t buffers_count = asset.count - textures_count;

		array_resize(&gpu_target.base.textures, textures_count);
		array_resize(&gpu_target.base.buffers,  buffers_count);
	}

	// allocate
	gl.CreateFramebuffers(1, &gpu_target.id);
	for (uint32_t i = 0; i < asset.count; i++) {
		if (!(asset.parameters[i].flags & TEXTURE_FLAG_OPAQUE)) {
			// @idea: expose settings
			struct Handle const gh_texture = gpu_texture_init(&(struct Image){
				.size = gpu_target.base.size,
				.parameters = asset.parameters[i],
				.sampler = (struct Sampler_Settings){
					.wrap_x = WRAP_MODE_EDGE,
					.wrap_y = WRAP_MODE_EDGE,
				},
			});
			array_push_many(&gpu_target.base.textures, 1, &gh_texture);
		}
		else {
			struct GPU_Target_Buffer_Internal buffer = {.base = {
				.parameters = asset.parameters[i],
			}};
			gl.CreateRenderbuffers(1, &buffer.id);
			gl.NamedRenderbufferStorage(
				buffer.id,
				gpu_sized_internal_format(asset.parameters[i].texture_type, asset.parameters[i].data_type),
				(GLsizei)gpu_target.base.size.x, (GLsizei)gpu_target.base.size.y
			);
			array_push_many(&gpu_target.base.buffers, 1, &buffer);
		}
	}

	// chart
	uint32_t attachments_count = 0;
	FOR_ARRAY(&gpu_target.base.textures, it) {
		struct Handle const * gh_texture = it.value;
		struct GPU_Texture_Internal const * gpu_texture = sparseset_get(&gs_graphics_state.textures, *gh_texture);
		if (gpu_texture == NULL) { continue; }

		uint32_t const attachment = attachments_count;
		if (gpu_texture->base.parameters.texture_type == TEXTURE_TYPE_COLOR) { attachments_count++; }

		GLint const level = 0;
		gl.NamedFramebufferTexture(
			gpu_target.id,
			gpu_attachment_point(gpu_texture->base.parameters.texture_type, attachment),
			gpu_texture->id,
			level
		);
	}
	FOR_ARRAY(&gpu_target.base.buffers, it) {
		struct GPU_Target_Buffer_Internal const * buffer = it.value;

		uint32_t const attachment = attachments_count;
		if (buffer->base.parameters.texture_type == TEXTURE_TYPE_COLOR) { attachments_count++; }

		gl.NamedFramebufferRenderbuffer(
			gpu_target.id,
			gpu_attachment_point(buffer->base.parameters.texture_type, attachment),
			GL_RENDERBUFFER,
			buffer->id
		);
	}

	GFX_TRACE("aquire target %d", gpu_target.id);
	return gpu_target;
}

static void gpu_target_on_discard(struct GPU_Target_Internal * gpu_target) {
	if (gpu_target->id == 0) { return; }
	GFX_TRACE("discard target %d", gpu_target->id);
	FOR_ARRAY(&gpu_target->base.textures, it) {
		struct Handle const * gh_texture = it.value;
		gpu_texture_free(*gh_texture);
	}
	FOR_ARRAY(&gpu_target->base.buffers, it) {
		struct GPU_Target_Buffer_Internal const * entry = it.value;
		gl.DeleteRenderbuffers(1, &entry->id);
	}
	array_free(&gpu_target->base.textures);
	array_free(&gpu_target->base.buffers);
	gl.DeleteFramebuffers(1, &gpu_target->id);
}

struct Handle gpu_target_init(struct GPU_Target_Asset asset) {
	struct GPU_Target_Internal gpu_target = gpu_target_on_aquire(asset);
	return sparseset_aquire(&gs_graphics_state.targets, &gpu_target);
}

void gpu_target_free(struct Handle handle) {
	if (handle_equals(gs_graphics_state.active.gh_target, handle)) {
		gs_graphics_state.active.gh_target = (struct Handle){0};
	}
	struct GPU_Target_Internal * gpu_target = sparseset_get(&gs_graphics_state.targets, handle);
	if (gpu_target != NULL) {
		gpu_target_on_discard(gpu_target);
		sparseset_discard(&gs_graphics_state.targets, handle);
	}
}

void gpu_target_update(struct Handle handle, struct GPU_Target_Asset asset) {
	struct GPU_Target_Internal * gpu_target = sparseset_get(&gs_graphics_state.targets, handle);
	if (gpu_target == NULL) { return; }

	gpu_target_on_discard(gpu_target);
	*gpu_target = gpu_target_on_aquire(asset);
}

struct GPU_Target const * gpu_target_get(struct Handle handle) {
	return sparseset_get(&gs_graphics_state.targets, handle);
}

// ----- ----- ----- ----- -----
//     GPU buffer part
// ----- ----- ----- ----- -----

static bool gpu_buffer_upload(struct GPU_Buffer_Internal * gpu_buffer, struct Buffer const * asset) {
	if (gpu_buffer->base.capacity < asset->size) { return false; }

	gpu_buffer->base.size = asset->size;
	if (asset->data == NULL) { return true; }
	if (asset->size == 0)    { return true; }

	gl.NamedBufferSubData(
		gpu_buffer->id, 0,
		(GLsizeiptr)asset->size,
		asset->data
	);
	return true;
}

static struct GPU_Buffer_Internal gpu_buffer_on_aquire(struct Buffer const * asset) {
	struct GPU_Buffer_Internal gpu_buffer = {.base = {
		.capacity = asset->size,
		.size = (asset->data != NULL)
			? asset->size
			: 0,
	}};

	if (gpu_buffer.base.capacity == 0) { return gpu_buffer; }

	// allocate and upload
	gl.CreateBuffers(1, &gpu_buffer.id);
	gl.NamedBufferStorage(
		gpu_buffer.id,
		(GLsizeiptr)gpu_buffer.base.capacity,
		asset->data,
		GL_DYNAMIC_STORAGE_BIT
	);

	GFX_TRACE("aquire buffer %d", gpu_buffer.id);
	return gpu_buffer;
}

static void gpu_buffer_on_discard(struct GPU_Buffer_Internal * gpu_buffer) {
	if (gpu_buffer->id == 0) { return; }
	GFX_TRACE("discard buffer %d", gpu_buffer->id);
	gl.DeleteBuffers(1, &gpu_buffer->id);
}

struct Handle gpu_buffer_init(struct Buffer const * asset) {
	struct GPU_Buffer_Internal gpu_buffer = gpu_buffer_on_aquire(asset);
	return sparseset_aquire(&gs_graphics_state.buffers, &gpu_buffer);
}

void gpu_buffer_free(struct Handle handle) {
	struct GPU_Buffer_Internal * gpu_buffer = sparseset_get(&gs_graphics_state.buffers, handle);
	if (gpu_buffer != NULL) {
		gpu_buffer_on_discard(gpu_buffer);
		sparseset_discard(&gs_graphics_state.buffers, handle);
	}
}

void gpu_buffer_update(struct Handle handle, struct Buffer const * asset) {
	struct GPU_Buffer_Internal * gpu_buffer = sparseset_get(&gs_graphics_state.buffers, handle);
	if (gpu_buffer == NULL) { return; }

	if (gpu_buffer_upload(gpu_buffer, asset)) { return; }

	gpu_buffer_on_discard(gpu_buffer);
	*gpu_buffer = gpu_buffer_on_aquire(asset);
}

struct GPU_Buffer const * gpu_buffer_get(struct Handle handle) {
	return sparseset_get(&gs_graphics_state.buffers, handle);
}

// ----- ----- ----- ----- -----
//     GPU mesh part
// ----- ----- ----- ----- -----

static bool gpu_mesh_upload(struct GPU_Mesh_Internal * gpu_mesh, struct Mesh const * asset) {
	if (gpu_mesh->base.buffers.count != asset->count) { return false; }

	FOR_ARRAY(&gpu_mesh->base.parameters, it) {
		if (!equals(
			it.value, asset->parameters + it.curr
			, sizeof(struct Mesh_Parameters)
		)) { return false; }
	}

	FOR_ARRAY(&gpu_mesh->base.buffers, it) {
		struct Buffer const * asset_buffer = asset->buffers + it.curr;

		struct Handle const * gh_buffer = it.value;
		struct GPU_Buffer_Internal * gpu_buffer = sparseset_get(&gs_graphics_state.buffers, *gh_buffer);

		if (!gpu_buffer_upload(gpu_buffer, asset_buffer)) {
			return false;
		}
	}

	return true;
}

static struct GPU_Mesh_Internal gpu_mesh_on_aquire(struct Mesh const * asset) {
	struct GPU_Mesh_Internal gpu_mesh = {.base = {
		.buffers = array_init(sizeof(struct Handle)),
		.parameters = array_init(sizeof(struct Mesh_Parameters)),
	}};

	{ // prepare arrays
		array_resize(&gpu_mesh.base.buffers, asset->count);
		array_resize(&gpu_mesh.base.parameters, asset->count);
	}

	// allocate and upload
	gl.CreateVertexArrays(1, &gpu_mesh.id);
	for (uint32_t i = 0; i < asset->count; i++) {
		struct Handle gh_buffer = gpu_buffer_init(asset->buffers + i);
		array_push_many(&gpu_mesh.base.buffers, 1, &gh_buffer);
		array_push_many(&gpu_mesh.base.parameters, 1, asset->parameters + i);
	}

	// chart
	uint32_t binding = 0;
	FOR_ARRAY(&gpu_mesh.base.buffers, it) {
		struct Handle const * gh_buffer = it.value;
		struct Mesh_Parameters const * parameters = array_at(&gpu_mesh.base.parameters, it.curr);

		struct GPU_Buffer_Internal const * gpu_buffer = sparseset_get(&gs_graphics_state.buffers, *gh_buffer);

		// element buffer
		if (parameters->flags & MESH_FLAG_INDEX) {
			gl.VertexArrayElementBuffer(gpu_mesh.id, gpu_buffer->id);
			continue;
		}

		// vertex buffer
		uint32_t vertex_size = 0;
		for (uint32_t atti = 0; atti < ATTRIBUTE_TYPE_INTERNAL_COUNT; atti++) {
			uint32_t const count = parameters->attributes[atti * 2 + 1];
			vertex_size += count * data_type_get_size(parameters->type);
		}

		GLintptr const offset = 0;
		gl.VertexArrayVertexBuffer(gpu_mesh.id, binding, gpu_buffer->id, offset, (GLsizei)vertex_size);

		uint32_t attribute_offset = 0;
		for (uint32_t atti = 0; atti < ATTRIBUTE_TYPE_INTERNAL_COUNT; atti++) {
			enum Attribute_Type const type = (enum Attribute_Type)parameters->attributes[atti * 2];
			if (type == ATTRIBUTE_TYPE_NONE) { continue; }

			uint32_t const count = parameters->attributes[atti * 2 + 1];
			if (count == 0) { continue; }

			GLuint const attribute = (GLuint)(type - 1);
			gl.EnableVertexArrayAttrib(gpu_mesh.id, attribute);
			gl.VertexArrayAttribBinding(gpu_mesh.id, attribute, binding);

			gl.VertexArrayAttribFormat(
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

static void gpu_mesh_on_discard(struct GPU_Mesh_Internal * gpu_mesh) {
	if (gpu_mesh->id == 0) { return; }
	GFX_TRACE("discard mesh %d", gpu_mesh->id);
	FOR_ARRAY(&gpu_mesh->base.buffers, it) {
		struct Handle const * gh_buffer = it.value;
		gpu_buffer_free(*gh_buffer);
	}
	array_free(&gpu_mesh->base.buffers);
	array_free(&gpu_mesh->base.parameters);
	gl.DeleteVertexArrays(1, &gpu_mesh->id);
}

struct Handle gpu_mesh_init(struct Mesh const * asset) {
	struct GPU_Mesh_Internal gpu_mesh = gpu_mesh_on_aquire(asset);
	return sparseset_aquire(&gs_graphics_state.meshes, &gpu_mesh);
}

void gpu_mesh_free(struct Handle handle) {
	if (handle_equals(gs_graphics_state.active.gh_mesh, handle)) {
		gs_graphics_state.active.gh_mesh = (struct Handle){0};
	}
	struct GPU_Mesh_Internal * gpu_mesh = sparseset_get(&gs_graphics_state.meshes, handle);
	if (gpu_mesh != NULL) {
		gpu_mesh_on_discard(gpu_mesh);
		sparseset_discard(&gs_graphics_state.meshes, handle);
	}
}

void gpu_mesh_update(struct Handle handle, struct Mesh const * asset) {
	struct GPU_Mesh_Internal * gpu_mesh = sparseset_get(&gs_graphics_state.meshes, handle);
	if (gpu_mesh == NULL) { return; }

	if (gpu_mesh_upload(gpu_mesh, asset)) { return; }

	gpu_mesh_on_discard(gpu_mesh);
	*gpu_mesh = gpu_mesh_on_aquire(asset);
}

struct GPU_Mesh const * gpu_mesh_get(struct Handle handle) {
	return sparseset_get(&gs_graphics_state.meshes, handle);
}

//
#include "framework/graphics/misc.h"

void graphics_update(void) { }

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

void graphics_buffer_align(struct Buffer * buffer, enum Buffer_Mode mode) {
	uint32_t const alignment = get_buffer_mode_alignment(mode);
	size_t const align = alignment - (buffer->size % alignment);
	buffer_push_many(buffer, align, NULL);
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
// 		gl.Disable(GL_STENCIL_TEST);
// 	}
// 	else {
// 		gl.Enable(GL_STENCIL_TEST);
// 
// 		gl.StencilMask((GLuint)write_mask);
// 
// 		gl.StencilFunc(gpu_comparison_op(comparison_op), (GLint)comparison_ref, (GLuint)comparison_mask);
// 
// 		gl.StencilOp(
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
	struct GPU_Texture_Internal const * gpu_texture = sparseset_get(&gs_graphics_state.textures, gh_texture);
	if (gpu_texture == NULL) { return 0; }

	uint32_t const id = graphics_unit_find(gh_texture);
	if (id != 0) { return id; }

	uint32_t const id_new = graphics_unit_find((struct Handle){0});
	if (id_new == 0) {
		WRN("failure: no spare texture/sampler units");
		DEBUG_BREAK(); return 0;
	}

	struct GPU_Unit * unit = array_at(&gs_graphics_state.units, id_new - 1);
	unit->gh_texture = gh_texture;

	gl.BindTextureUnit((GLuint)id_new, gpu_texture->id);
	return id_new;
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
// 		gl.BindTextureUnit((GLuint)unit, 0);
// 	}
// }

static void gpu_select_program(struct Handle handle) {
	if (handle_equals(gs_graphics_state.active.gh_program, handle)) { return; }
	gs_graphics_state.active.gh_program = handle;
	struct GPU_Program_Internal const * gpu_program = sparseset_get(&gs_graphics_state.programs, handle);
	gl.UseProgram((gpu_program != NULL) ? gpu_program->id : 0);
}

static void gpu_select_target(struct Handle handle) {
	if (handle_equals(gs_graphics_state.active.gh_target, handle)) { return; }
	gs_graphics_state.active.gh_target = handle;
	struct GPU_Target_Internal const * gpu_target = sparseset_get(&gs_graphics_state.targets, handle);
	gl.BindFramebuffer(GL_FRAMEBUFFER, (gpu_target != NULL) ? gpu_target->id : 0);
}

static void gpu_select_mesh(struct Handle handle) {
	if (handle_equals(gs_graphics_state.active.gh_mesh, handle)) { return; }
	gs_graphics_state.active.gh_mesh = handle;
	struct GPU_Mesh_Internal const * gpu_mesh = sparseset_get(&gs_graphics_state.meshes, handle);
	gl.BindVertexArray((gpu_mesh != NULL) ? gpu_mesh->id : 0);
}

static void gpu_upload_single_uniform(struct GPU_Program_Internal const * gpu_program, struct GPU_Uniform_Internal const * field, void const * data) {
	switch (field->base.type) {
		default: {
			WRN("unsupported field type '0x%x'", field->base.type);
			DEBUG_BREAK();
		} break;

		case DATA_TYPE_UNIT_U:
		case DATA_TYPE_UNIT_S:
		case DATA_TYPE_UNIT_F: {
			GLint * units = BUFFER_ALLOCATE_SIZE(sizeof(GLint) * field->base.array_size);
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
			gl.ProgramUniform1iv(gpu_program->id, field->location, (GLsizei)field->base.array_size, units);
		} break;

		case DATA_TYPE_R32_U:    gl.ProgramUniform1uiv(gpu_program->id, field->location, (GLsizei)field->base.array_size, data); break;
		case DATA_TYPE_RG32_U:   gl.ProgramUniform2uiv(gpu_program->id, field->location, (GLsizei)field->base.array_size, data); break;
		case DATA_TYPE_RGB32_U:  gl.ProgramUniform3uiv(gpu_program->id, field->location, (GLsizei)field->base.array_size, data); break;
		case DATA_TYPE_RGBA32_U: gl.ProgramUniform4uiv(gpu_program->id, field->location, (GLsizei)field->base.array_size, data); break;

		case DATA_TYPE_R32_S:    gl.ProgramUniform1iv(gpu_program->id, field->location, (GLsizei)field->base.array_size, data); break;
		case DATA_TYPE_RG32_S:   gl.ProgramUniform2iv(gpu_program->id, field->location, (GLsizei)field->base.array_size, data); break;
		case DATA_TYPE_RGB32_S:  gl.ProgramUniform3iv(gpu_program->id, field->location, (GLsizei)field->base.array_size, data); break;
		case DATA_TYPE_RGBA32_S: gl.ProgramUniform4iv(gpu_program->id, field->location, (GLsizei)field->base.array_size, data); break;

		case DATA_TYPE_R32_F:    gl.ProgramUniform1fv(gpu_program->id, field->location, (GLsizei)field->base.array_size, data); break;
		case DATA_TYPE_RG32_F:   gl.ProgramUniform2fv(gpu_program->id, field->location, (GLsizei)field->base.array_size, data); break;
		case DATA_TYPE_RGB32_F:  gl.ProgramUniform3fv(gpu_program->id, field->location, (GLsizei)field->base.array_size, data); break;
		case DATA_TYPE_RGBA32_F: gl.ProgramUniform4fv(gpu_program->id, field->location, (GLsizei)field->base.array_size, data); break;

		case DATA_TYPE_MAT2: gl.ProgramUniformMatrix2fv(gpu_program->id, field->location, (GLsizei)field->base.array_size, GL_FALSE, data); break;
		case DATA_TYPE_MAT3: gl.ProgramUniformMatrix3fv(gpu_program->id, field->location, (GLsizei)field->base.array_size, GL_FALSE, data); break;
		case DATA_TYPE_MAT4: gl.ProgramUniformMatrix4fv(gpu_program->id, field->location, (GLsizei)field->base.array_size, GL_FALSE, data); break;
	}
}

static void gpu_upload_uniforms(struct GPU_Program_Internal const * gpu_program, struct Gfx_Uniforms const * uniforms, uint32_t offset, uint32_t count) {
	for (uint32_t i = offset, last = offset + count; i < last; i++) {
		struct Gfx_Uniforms_Entry const * entry = array_at(&uniforms->headers, i);

		struct GPU_Uniform_Internal const * uniform = hashmap_get(&gpu_program->base.uniforms, &entry->id);
		if (uniform == NULL) { continue; }

		if (data_type_get_size(uniform->base.type) * uniform->base.array_size != entry->size) { continue; }

		gpu_upload_single_uniform(gpu_program, uniform, (uint8_t *)uniforms->payload.data + entry->offset);
	}
}

static void gpu_set_blend_mode(enum Blend_Mode mode) {
	if (mode == BLEND_MODE_NONE) {
		gl.Disable(GL_BLEND);
		gl.ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	}
	else {
		struct GPU_Blend_Mode const gpu_mode = gpu_blend_mode(mode);
		gl.Enable(GL_BLEND);
		gl.ColorMask(gpu_mode.mask[0], gpu_mode.mask[1], gpu_mode.mask[2], gpu_mode.mask[3]);
		gl.BlendEquationSeparate(gpu_mode.color_op, gpu_mode.alpha_op);
		gl.BlendFuncSeparate(
			gpu_mode.color_src, gpu_mode.color_dst,
			gpu_mode.alpha_src, gpu_mode.alpha_dst
		);
		// glBlendColor(mode.color.x, mode.color.y, mode.color.z, mode.color.w);
	}
}

static void gpu_set_depth_mode(enum Depth_Mode mode) {
	if (mode == DEPTH_MODE_NONE) {
		gl.Disable(GL_DEPTH_TEST);
	}
	else {
		bool const reversed_z = (gs_graphics_state.clip_space.depth_near > gs_graphics_state.clip_space.depth_far);
		struct GPU_Depth_Mode const gpu_mode = gpu_depth_mode(mode, reversed_z);
		gl.Enable(GL_DEPTH_TEST);
		gl.DepthMask(gpu_mode.mask);
		gl.DepthFunc(gpu_mode.op);
	}
}

//
#include "framework/graphics/command.h"

inline static void gpu_execute_cull(struct GPU_Command_Cull const * command) {
	if (command->mode == CULL_MODE_NONE) {
		gl.Disable(GL_CULL_FACE);
	}
	else {
		gl.Enable(GL_CULL_FACE);
		gl.CullFace(gpu_cull_mode(command->mode));
		gl.FrontFace(gpu_winding_order(command->order));
	}
}

inline static void gpu_execute_target(struct GPU_Command_Target const * command) {
	gpu_select_target(command->gh_target);

	struct uvec2 viewport_size = command->screen_size;
	if (!handle_is_null(command->gh_target)) {
		struct GPU_Target const * target = gpu_target_get(command->gh_target);
		viewport_size = (target != NULL) ? target->size : (struct uvec2){0};
	}

	gl.Viewport(0, 0, (GLsizei)viewport_size.x, (GLsizei)viewport_size.y);
}

inline static void gpu_execute_clear(struct GPU_Command_Clear const * command) {
	if (command->mask == TEXTURE_TYPE_NONE) {
		WRN("clear mask is empty");
		DEBUG_BREAK(); return;
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
	
	gl.ClearColor(command->color.x, command->color.y, command->color.z, command->color.w);
	gl.ClearDepthf(depth);
	gl.ClearStencil(stencil);
	gl.Clear(clear_bitfield);
}

inline static void gpu_execute_material(struct GPU_Command_Material const * command) {
	struct Gfx_Material const * material = material_system_take(command->mh_mat);
	struct Handle const gh_program = (material != NULL)
		? material->gh_program
		: (struct Handle){0};

	gpu_select_program(gh_program);
	gpu_set_blend_mode(material->blend_mode);
	gpu_set_depth_mode(material->depth_mode);

	struct GPU_Program_Internal const * gpu_program = sparseset_get(&gs_graphics_state.programs, gh_program);
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
		struct GPU_Program_Internal const * gpu_program = sparseset_get(&gs_graphics_state.programs, command->gh_program);
		gpu_upload_uniforms(gpu_program, command->uniforms, command->offset, command->count);
	}
}

inline static void gpu_execute_buffer(struct GPU_Command_Buffer const * command) {
	struct GPU_Buffer_Internal const * gpu_buffer = sparseset_get(&gs_graphics_state.buffers, command->gh_buffer);
	if (gpu_buffer == NULL) { return; }

	if (command->offset == 0 && command->length == 0) {
		gl.BindBufferBase(
			gpu_buffer_mode(command->mode)
			, command->index, gpu_buffer->id
		);
		return;
	}

	uint32_t const length = min_u32(
		(uint32_t)command->length,
		get_buffer_mode_size(command->mode)
	);
	if (length < command->length) { WRN("buffer exceeds limits"); DEBUG_BREAK(); }

	gl.BindBufferRange(
		gpu_buffer_mode(command->mode)
		, command->index, gpu_buffer->id
		, (GLsizeiptr)command->offset, (GLsizeiptr)length
	);
}

inline static void gpu_execute_draw(struct GPU_Command_Draw const * command) {
	gpu_select_mesh(command->gh_mesh);

	struct GPU_Mesh_Internal const * mesh = sparseset_get(&gs_graphics_state.meshes, command->gh_mesh);
	if (mesh == NULL) { return; }

	if (mesh->base.buffers.count == 0 && command->mode != MESH_MODE_NONE) {
		gl.DrawArraysInstanced(
			gpu_mesh_mode(command->mode),
			(GLint)command->offset,
			(GLsizei)command->count,
			(GLsizei)max_u32(command->instances, 1)
		);
	}

	FOR_ARRAY(&mesh->base.buffers, it) {
		struct Handle const * gh_buffer = it.value;
		struct GPU_Buffer_Internal const * gpu_buffer = sparseset_get(&gs_graphics_state.buffers, *gh_buffer);
		struct Mesh_Parameters const * parameters = array_at(&mesh->base.parameters, it.curr);
		if (parameters->mode == MESH_MODE_NONE) { continue; }
		if (gpu_buffer->base.size == 0) { continue; }

		uint32_t const offset = command->offset;
		uint32_t const count = (command->count != 0)
			? command->count
			: (uint32_t)(gpu_buffer->base.size / data_type_get_size(parameters->type));

		if (parameters->flags & MESH_FLAG_INDEX) {
			enum Data_Type const elements_type = parameters->type;
			size_t const bytes_offset = command->offset * data_type_get_size(elements_type);

			gl.DrawElementsInstanced(
				gpu_mesh_mode(parameters->mode),
				(GLsizei)count,
				gpu_index_value_type(elements_type),
				(void const *)bytes_offset,
				(GLsizei)max_u32(command->instances, 1)
			);
		}
		else {
			gl.DrawArraysInstanced(
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
				DEBUG_BREAK();
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

static struct Buffer get_extensions(void);
static struct Graphics_Limits get_limits(void);

void graphics_to_gpu_library_init(void) {
	// setup debug
	if (gl.DebugMessageCallback != NULL) {
		gl.Enable(GL_DEBUG_OUTPUT);
		gl.Enable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		gl.DebugMessageCallback(&opengl_debug_message_callback, NULL);
		if (gl.DebugMessageControl != NULL) {
			gl.DebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
		}
	}

	//
	gs_graphics_state = (struct Graphics_State){
		.extensions = get_extensions(),
		.limits     = get_limits(),
		//
		.units    = array_init(sizeof(struct GPU_Unit)),
		.programs = sparseset_init(sizeof(struct GPU_Program_Internal)),
		.targets  = sparseset_init(sizeof(struct GPU_Target_Internal)),
		.textures = sparseset_init(sizeof(struct GPU_Texture_Internal)),
		.buffers  = sparseset_init(sizeof(struct GPU_Buffer_Internal)),
		.meshes   = sparseset_init(sizeof(struct GPU_Mesh_Internal)),
	};

	//
	array_resize(&gs_graphics_state.units, gs_graphics_state.limits.units);
	for (uint32_t i = 0; i < gs_graphics_state.units.capacity; i++) {
		array_push_many(&gs_graphics_state.units, 1, &(struct GPU_Unit){0});
	}

	// @note: manage OpenGL's clip space instead of ours
	bool const can_control_clip_space = (gl.version >= 45) || contains_full_word(gs_graphics_state.extensions.data, S_("GL_ARB_clip_control"));
	if (can_control_clip_space) { gl.ClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE); }

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
	gl.DepthRangef(gs_graphics_state.clip_space.depth_near, gs_graphics_state.clip_space.depth_far);

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
	buffer_free(&gs_graphics_state.extensions);
	common_memset(&gs_graphics_state, 0, sizeof(gs_graphics_state));

	if (inst_count > 0) { DEBUG_BREAK(); }

#undef GPU_FREE
}

//

static struct Buffer get_extensions(void) {
	struct Buffer buffer = buffer_init(NULL);

	GLint count = 0;
	gl.GetIntegerv(GL_NUM_EXTENSIONS, &count);

	buffer_resize(&buffer, (uint32_t)(count * 26));
	for (GLint i = 0; i < count; i++) {
		GLubyte const * value = gl.GetStringi(GL_EXTENSIONS, (GLuint)i);
		size_t const size = strlen((char const *)value);
		buffer_push_many(&buffer, size, value);
		buffer_push_many(&buffer, 1, " ");
	}
	buffer_push_many(&buffer, 1, "\0");

	LOG(
		"> OpenGL extensions: %.*s\n"
		""
		, (uint32_t)buffer.size - 1
		, (char *)buffer.data
	);

	return buffer;
}

static struct Graphics_Limits get_limits(void) {
	GLint units
	    , units_vs
	    , units_fs
	    , units_cs;

	GLint uniform_blocks
	    , uniform_blocks_vs
	    , uniform_blocks_fs
	    , uniform_blocks_cs
	    , uniform_blocks_size
	    , uniform_blocks_alignment;

	GLint storage_blocks
	    , storage_blocks_vs
	    , storage_blocks_fs
	    , storage_blocks_cs
	    , storage_blocks_size
	    , storage_blocks_alignment;

	GLint texture_size, target_size;

	gl.GetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,       &units);
	gl.GetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS,         &units_vs);
	gl.GetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS,                &units_fs);
	gl.GetIntegerv(GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS,        &units_cs);

	gl.GetIntegerv(GL_MAX_COMBINED_UNIFORM_BLOCKS,            &uniform_blocks);
	gl.GetIntegerv(GL_MAX_VERTEX_UNIFORM_BLOCKS,              &uniform_blocks_vs);
	gl.GetIntegerv(GL_MAX_FRAGMENT_UNIFORM_BLOCKS,            &uniform_blocks_fs);
	gl.GetIntegerv(GL_MAX_COMPUTE_UNIFORM_BLOCKS,             &uniform_blocks_cs);
	gl.GetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE,                 &uniform_blocks_size);
	gl.GetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT,        &uniform_blocks_alignment);

	gl.GetIntegerv(GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS,     &storage_blocks);
	gl.GetIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS,       &storage_blocks_vs);
	gl.GetIntegerv(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS,     &storage_blocks_fs);
	gl.GetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS,      &storage_blocks_cs);
	gl.GetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE,          &storage_blocks_size);
	gl.GetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &storage_blocks_alignment);

	gl.GetIntegerv(GL_MAX_TEXTURE_SIZE,                       &texture_size);
	gl.GetIntegerv(GL_MAX_RENDERBUFFER_SIZE,                  &target_size);

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
		"  - size .......... %d\n"
		"  - alignment ..... %d\n"
		//
		"  storage blocks .. %d\n"
		"  - VS ............ %d\n"
		"  - FS ............ %d\n"
		"  - CS ............ %d\n"
		"  - size .......... %d\n"
		"  - alignment ..... %d\n"
		//
		"  texture size .... %d\n"
		"  target size ..... %d\n"
		//
		""
		, units
		, units_vs
		, units_fs
		, units_cs
		//
		, uniform_blocks
		, uniform_blocks_vs
		, uniform_blocks_fs
		, uniform_blocks_cs
		, uniform_blocks_size
		, uniform_blocks_alignment
		//
		, storage_blocks
		, storage_blocks_vs
		, storage_blocks_fs
		, storage_blocks_cs
		, storage_blocks_size
		, storage_blocks_alignment
		//
		, texture_size
		, target_size
	);

	return (struct Graphics_Limits){
		.units                    = (uint32_t)units,
		.units_vs                 = (uint32_t)units_vs,
		.units_fs                 = (uint32_t)units_fs,
		.units_cs                 = (uint32_t)units_cs,
		//
		.uniform_blocks           = (uint32_t)uniform_blocks,
		.uniform_blocks_vs        = (uint32_t)uniform_blocks_vs,
		.uniform_blocks_fs        = (uint32_t)uniform_blocks_fs,
		.uniform_blocks_cs        = (uint32_t)uniform_blocks_cs,
		.uniform_blocks_size      = (uint32_t)uniform_blocks_size,
		.uniform_blocks_alignment = (uint32_t)uniform_blocks_alignment,
		//
		.storage_blocks           = (uint32_t)storage_blocks,
		.storage_blocks_vs        = (uint32_t)storage_blocks_vs,
		.storage_blocks_fs        = (uint32_t)storage_blocks_fs,
		.storage_blocks_cs        = (uint32_t)storage_blocks_cs,
		.storage_blocks_size      = (uint32_t)storage_blocks_size,
		.storage_blocks_alignment = (uint32_t)storage_blocks_alignment,
		//
		.texture_size             = (uint32_t)texture_size,
		.target_size              = (uint32_t)target_size,
	};
}

//

static void verify_shader(GLuint id) {
	GLint status;
	gl.GetShaderiv(id, GL_COMPILE_STATUS, &status);
	if (status) { return; }

	GLint max_length;
	gl.GetShaderiv(id, GL_INFO_LOG_LENGTH, &max_length);
	if (max_length <= 0) { return; }

	GLchar * buffer = BUFFER_ALLOCATE_SIZE(sizeof(GLchar) * (size_t)max_length);
	gl.GetShaderInfoLog(id, max_length, &max_length, buffer);
	ERR("%s", buffer);
}

static void verify_program(GLuint id) {
	GLint status;
	gl.GetProgramiv(id, GL_LINK_STATUS, &status);
	if (status) { return; }

	GLint max_length;
	gl.GetProgramiv(id, GL_INFO_LOG_LENGTH, &max_length);
	if (max_length <= 0) { return; }

	GLchar * buffer = BUFFER_ALLOCATE_SIZE(sizeof(GLchar) * (size_t)max_length);
	gl.GetProgramInfoLog(id, max_length, &max_length, buffer);
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
