#include "framework/memory.h"
#include "framework/containers/strings.h"
#include "framework/containers/array_byte.h"
#include "framework/assets/asset_mesh.h"
#include "framework/assets/asset_image.h"
#include "framework/graphics_types.h"
#include "framework/material.h"

#include "functions.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GLenum gpu_data_type(enum Data_Type value);
static enum Data_Type interpret_gl_type(GLint value);

static GLint gpu_min_filter_mode(enum Filter_Mode mipmap, enum Filter_Mode texture);
static GLint gpu_mag_filter_mode(enum Filter_Mode value);
static GLint gpu_wrap_mode(enum Wrap_Mode value, bool mirror);

static GLenum gpu_sized_internal_format(enum Texture_Type texture_type, enum Data_Type data_type, uint32_t channels);
static GLenum gpu_pixel_data_format(enum Texture_Type texture_type, uint32_t channels);
static GLenum gpu_pixel_data_type(enum Texture_Type texture_type, enum Data_Type data_type);
static GLenum gpu_attachment_point(enum Texture_Type texture_type, uint32_t index);

static GLenum gpu_mesh_usage_pattern(enum Mesh_Frequency frequency, enum Mesh_Access access);

static GLenum gpu_comparison_op(enum Comparison_Op value);
static GLenum gpu_cull_mode(enum Cull_Mode value);
static GLenum gpu_winding_order(enum Winding_Order value);
static GLenum gpu_stencil_op(enum Stencil_Op value);

static GLenum gpu_blend_op(enum Blend_Op value);
static GLenum gpu_blend_factor(enum Blend_Factor value);

#define MAX_UNIFORMS 32
#define MAX_UNITS_PER_MATERIAL 64

struct Gpu_Program {
	GLuint id;
	struct Gpu_Program_Field uniforms[MAX_UNIFORMS];
	GLint uniform_locations[MAX_UNIFORMS];
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

static struct Graphics_State {
	char * extensions;

	struct Strings * uniforms;

	struct Gpu_Program const * active_program;
	struct Gpu_Mesh const * active_mesh;

	uint32_t units_capacity;
	struct Gpu_Unit * units;
} graphics_state;

//
#include "framework/gpu_objects.h"

// -- GPU program part
static void verify_shader(GLuint id, GLenum parameter);
static void verify_program(GLuint id, GLenum parameter);
struct Gpu_Program * gpu_program_init(struct Array_Byte * asset) {
#define ADD_SECTION_HEADER(shader_type, version) \
	do { \
		if (strstr((char const *)asset->data, #shader_type)) {\
			if (ogl_version < (version)) { fprintf(stderr, "'" #shader_type "' is unavailable\n"); DEBUG_BREAK(); break; } \
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

	struct Gpu_Program_Field uniforms[MAX_UNIFORMS];
	GLint uniform_locations[MAX_UNIFORMS];
	for (GLint i = 0; i < uniforms_count; i++) {
		// GL_NAME_LENGTH // includes zero-terminator
		GLenum const props[] = {GL_TYPE, GL_ARRAY_SIZE, GL_LOCATION};
		GLint params[sizeof(props) / sizeof(*props)];
		glGetProgramResourceiv(program_id, GL_UNIFORM, (GLuint)i, sizeof(props) / sizeof(*props), props, sizeof(params) / sizeof(*params), NULL, params);

		enum Data_Type type = interpret_gl_type(params[0]); (void)type;

		GLsizei name_length;
		glGetProgramResourceName(program_id, GL_UNIFORM, (GLuint)i, uniform_name_buffer_length, &name_length, uniform_name_buffer);

		if (params[1] > 1) {
			// simple arrays have names ending with a `[0]`
			if (memcmp(uniform_name_buffer + name_length - 3, "[0]", 3) == 0) {
				name_length -= 3;
			}
		}

		uniforms[i] = (struct Gpu_Program_Field){
			.id = strings_add(graphics_state.uniforms, (uint32_t)name_length, uniform_name_buffer),
			.type = interpret_gl_type(params[0]),
			.array_size = (uint32_t)params[1],
		};
		uniform_locations[i] = params[2];
	}

	MEMORY_FREE(uniform_name_buffer);

	//
	struct Gpu_Program * gpu_program = MEMORY_ALLOCATE(struct Gpu_Program);
	*gpu_program = (struct Gpu_Program){
		.id = program_id,
		.uniforms_count = (uint32_t)uniforms_count,
	};
	memcpy(gpu_program->uniforms, uniforms, sizeof(*uniforms) * (size_t)uniforms_count);
	memcpy(gpu_program->uniform_locations, uniform_locations, sizeof(*uniform_locations) * (size_t)uniforms_count);
	return gpu_program;
	// https://www.khronos.org/opengl/wiki/Program_Introspection

#undef ADD_HEADER
}

void gpu_program_free(struct Gpu_Program * gpu_program) {
	if (ogl_version > 0) {
		if (graphics_state.active_program == gpu_program) { graphics_state.active_program = NULL; }
		glDeleteProgram(gpu_program->id);
	}
	memset(gpu_program, 0, sizeof(*gpu_program));
	MEMORY_FREE(gpu_program);
}

void gpu_program_get_uniforms(struct Gpu_Program * gpu_program, uint32_t * count, struct Gpu_Program_Field const ** values) {
	*count = gpu_program->uniforms_count;
	*values = gpu_program->uniforms;
}

// -- GPU texture part
struct Gpu_Texture * gpu_texture_init(struct Asset_Image * asset) {
	GLuint texture_id;
	glCreateTextures(GL_TEXTURE_2D, 1, &texture_id);

	// allocate buffer
	GLsizei levels = 1;
	glTextureStorage2D(
		texture_id, levels,
		gpu_sized_internal_format(TEXTURE_TYPE_COLOR, DATA_TYPE_U8, asset->channels),
		(GLsizei)asset->size_x, (GLsizei)asset->size_y
	);

	// chart buffer
	glTextureParameteri(texture_id, GL_TEXTURE_MIN_FILTER, gpu_min_filter_mode(FILTER_MODE_NONE, FILTER_MODE_POINT));
	glTextureParameteri(texture_id, GL_TEXTURE_MAG_FILTER, gpu_mag_filter_mode(FILTER_MODE_POINT));
	glTextureParameteri(texture_id, GL_TEXTURE_WRAP_S, gpu_wrap_mode(WRAP_MODE_REPEAT, false));
	glTextureParameteri(texture_id, GL_TEXTURE_WRAP_T, gpu_wrap_mode(WRAP_MODE_REPEAT, false));

	// load buffer
	GLint level = 0;
	glTextureSubImage2D(
		texture_id, level,
		0, 0, (GLsizei)asset->size_x, (GLsizei)asset->size_y,
		gpu_pixel_data_format(TEXTURE_TYPE_COLOR, asset->channels),
		gpu_pixel_data_type(TEXTURE_TYPE_COLOR, DATA_TYPE_U8),
		asset->data
	);

	//
	struct Gpu_Texture * gpu_texture = MEMORY_ALLOCATE(struct Gpu_Texture);
	*gpu_texture = (struct Gpu_Texture){
		.id = texture_id,
	};
	return gpu_texture;
}

void gpu_texture_free(struct Gpu_Texture * gpu_texture) {
	if (ogl_version > 0) {
		for (uint32_t i = 1; i < graphics_state.units_capacity; i++) {
			if (graphics_state.units[i].gpu_texture == gpu_texture) {
				graphics_state.units[i].gpu_texture = NULL;
				// glBindTextureUnit((GLuint)i, 0);
			}
		}
		glDeleteTextures(1, &gpu_texture->id);
	}
	memset(gpu_texture, 0, sizeof(*gpu_texture));
	MEMORY_FREE(gpu_texture);
}

// -- GPU mesh part
struct Gpu_Mesh * gpu_mesh_init(struct Asset_Mesh * asset) {
	GLuint mesh_id;
	glCreateVertexArrays(1, &mesh_id);

	// allocate buffer: vertices
	GLuint vertices_buffer_id;
	glCreateBuffers(1, &vertices_buffer_id);
	// data_type_get_size(DATA_TYPE_R32);
	glNamedBufferData(
		vertices_buffer_id,
		asset->vertices.count * sizeof(*asset->vertices.data),
		NULL, gpu_mesh_usage_pattern(MESH_FREQUENCY_STATIC, MESH_ACCESS_DRAW)
	);

	// allocate buffer: indices
	GLuint indices_buffer_id;
	glCreateBuffers(1, &indices_buffer_id);
	// data_type_get_size(DATA_TYPE_U32);
	glNamedBufferData(
		indices_buffer_id,
		asset->indices.count * sizeof(*asset->indices.data),
		NULL, gpu_mesh_usage_pattern(MESH_FREQUENCY_STATIC, MESH_ACCESS_DRAW)
	);

	// chart buffer: vertices
	GLsizei all_attributes_size = 0;
	for (uint32_t i = 0; i < asset->sizes.count; i++) {
		// data_type_get_size(DATA_TYPE_R32);
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
		// data_type_get_size(DATA_TYPE_R32);
		attribute_offset += size * (GLuint)sizeof(*asset->vertices.data);
	}

	// chart buffer: indices
	glVertexArrayElementBuffer(mesh_id, indices_buffer_id);

	// load buffer: vertices
	// data_type_get_size(DATA_TYPE_R32);
	glNamedBufferSubData(vertices_buffer_id, 0, asset->vertices.count * sizeof(*asset->vertices.data), asset->vertices.data);

	// load buffer: indices
	// data_type_get_size(DATA_TYPE_U32);
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
		if (graphics_state.active_mesh == gpu_mesh) { graphics_state.active_mesh = NULL; }
		glDeleteBuffers(1, &gpu_mesh->vertices_buffer_id);
		glDeleteBuffers(1, &gpu_mesh->indices_buffer_id);
		glDeleteVertexArrays(1, &gpu_mesh->id);
	}
	memset(gpu_mesh, 0, sizeof(*gpu_mesh));
	MEMORY_FREE(gpu_mesh);
}

//
#include "graphics.h"

uint32_t graphics_add_uniform(char const * name) {
	return strings_add(graphics_state.uniforms, (uint32_t)strlen(name), name);
}

uint32_t graphics_find_uniform(char const * name) {
	return strings_find(graphics_state.uniforms, (uint32_t)strlen(name), name);
}

void graphics_clear(void) {
	uint32_t framebuffer_id = 0;
	enum Texture_Type clear_target = TEXTURE_TYPE_COLOR | TEXTURE_TYPE_DEPTH;
	float clear_depth = 1;
	// uint32_t clear_stencil = 0;

	GLbitfield clear_mask = 0;
	if (clear_target & TEXTURE_TYPE_COLOR) { clear_mask |= GL_COLOR_BUFFER_BIT; }
	if (clear_target & TEXTURE_TYPE_DEPTH) { clear_mask |= GL_DEPTH_BUFFER_BIT; }
	if (clear_target & TEXTURE_TYPE_STENCIL) { clear_mask |= GL_STENCIL_BUFFER_BIT; }

	glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)framebuffer_id);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glClearColor(0.2f, 0.2f, 0.2f, 1);
	glDepthMask(GL_TRUE);
	glClearDepthf(clear_depth);
	// glClearStencil((GLint)clear_stencil);
	glClear(clear_mask);
}

void graphics_viewport(uint32_t x, uint32_t y, uint32_t size_x, uint32_t size_y) {
	glViewport((GLsizei)x, (GLsizei)y, (GLsizei)size_x, (GLsizei)size_y);
}

static void graphics_blending(void) {
	enum Color_Channel color_mask = COLOR_CHANNEL_NONE;

	uint32_t blend_rgba = 0;

	enum Blend_Op rgb_op = BLEND_OP_NONE;
	enum Blend_Factor rgb_src = BLEND_FACTOR_ONE;
	enum Blend_Factor rgb_dst = BLEND_FACTOR_ZERO;

	enum Blend_Op alpha_op = BLEND_OP_NONE;
	enum Blend_Factor alpha_src = BLEND_FACTOR_ONE;
	enum Blend_Factor alpha_dst = BLEND_FACTOR_ZERO;

	//
	glColorMask(
		color_mask & COLOR_CHANNEL_RED,
		color_mask & COLOR_CHANNEL_GREEN,
		color_mask & COLOR_CHANNEL_BLUE,
		color_mask & COLOR_CHANNEL_ALPHA
	);

	glBlendColor(
		((blend_rgba >> 24) & 0xff) / 255.0f,
		((blend_rgba >> 16) & 0xff) / 255.0f,
		((blend_rgba >>  8) & 0xff) / 255.0f,
		((blend_rgba >>  0) & 0xff) / 255.0f
	);

	//
	bool const blend_rgb = (rgb_op != BLEND_OP_NONE)
	              && ((rgb_src != BLEND_FACTOR_ONE) || (rgb_dst != BLEND_FACTOR_ZERO))
	              && (color_mask & ~COLOR_CHANNEL_ALPHA);

	bool const blend_alpha = (alpha_op != BLEND_OP_NONE)
	                && ((alpha_src != BLEND_FACTOR_ONE) || (alpha_dst != BLEND_FACTOR_ZERO))
	                && (color_mask & COLOR_CHANNEL_ALPHA);

	if (!(blend_rgb || blend_alpha)) {
		glDisable(GL_BLEND);
	}
	else {
		glEnable(GL_BLEND);

		glBlendEquationSeparate(
			gpu_blend_op(blend_rgb ? rgb_op : BLEND_OP_ADD),
			gpu_blend_op(blend_alpha ? alpha_op : BLEND_OP_ADD)
		);

		glBlendFuncSeparate(
			gpu_blend_factor(blend_rgb ? rgb_src : BLEND_FACTOR_ONE),
			gpu_blend_factor(blend_rgb ? rgb_dst : BLEND_FACTOR_ZERO),
			gpu_blend_factor(blend_alpha ? alpha_src : BLEND_FACTOR_ONE),
			gpu_blend_factor(blend_alpha ? alpha_dst : BLEND_FACTOR_ZERO)
		);
	}
}

static void graphics_depth_test(void) {
	enum Comparison_Op comparison_op = COMPARISON_OP_NONE;
	bool write_mask = true;

	//
	if (comparison_op == COMPARISON_OP_NONE) {
		glDisable(GL_DEPTH_TEST);
	}
	else {
		glEnable(GL_DEPTH_TEST);

		glDepthMask(write_mask ? GL_TRUE : GL_FALSE);

		glDepthFunc(gpu_comparison_op(comparison_op));
	}
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

static void graphics_culling(void) {
	enum Cull_Mode mode = CULL_MODE_NONE;
	enum Winding_Order order = WINDING_ORDER_POSITIVE;

	//
	glFrontFace(gpu_winding_order(order));

	if (mode == CULL_MODE_NONE) {
		glDisable(GL_CULL_FACE);
	}
	else {
		glEnable(GL_CULL_FACE);

		glCullFace(gpu_cull_mode(mode));
	}
}

static uint32_t graphics_unit_find(struct Gpu_Texture * gpu_texture) {
	// @note: 0 value is considered empty
	for (uint32_t i = 1; i < graphics_state.units_capacity; i++) {
		if (graphics_state.units[i].gpu_texture == gpu_texture) { return i; }
	}
	return 0;
}

static uint32_t graphics_unit_init(struct Gpu_Texture * gpu_texture) {
	uint32_t unit = graphics_unit_find(gpu_texture);
	if (unit != 0) { return unit; }

	unit = graphics_unit_find(NULL);
	if (unit == 0) {
		fprintf(stderr, "'graphics_unit_find' failed\n"); DEBUG_BREAK();
		return unit;
	}

	graphics_state.units[unit] = (struct Gpu_Unit){
		.gpu_texture = gpu_texture,
	};

	glBindTextureUnit((GLuint)unit, gpu_texture->id);

	return unit;
}

// static void graphics_unit_free(struct Gpu_Texture * gpu_texture) {
// 	if (ogl_version > 0) {
// 		uint32_t unit = graphics_unit_find(gpu_texture);
// 		if (unit == 0) {
// 			fprintf(stderr, "'graphics_unit_find' failed\n"); DEBUG_BREAK();
// 			return;
// 		}
// 
// 		graphics_state.units[unit] = (struct Gpu_Unit){
// 			.gpu_texture = NULL,
// 		};
// 
// 		glBindTextureUnit((GLuint)unit, 0);
// 	}
// }

static void graphics_select_program(struct Gpu_Program const * gpu_program) {
	if (graphics_state.active_program == gpu_program) { return; }
	graphics_state.active_program = gpu_program;
	glUseProgram(gpu_program->id);
}

static void graphics_select_mesh(struct Gpu_Mesh const * gpu_mesh) {
	if (graphics_state.active_mesh == gpu_mesh) { return; }
	graphics_state.active_mesh = gpu_mesh;
	glBindVertexArray(gpu_mesh->id);
}

static void graphics_set_uniform(struct Gpu_Program * gpu_program, uint32_t uniform_index, void const * data) {
	struct Gpu_Program_Field const * field = gpu_program->uniforms + uniform_index;
	GLint const location = gpu_program->uniform_locations[uniform_index];

	switch (field->type) {
		default: break;

		case DATA_TYPE_UNIT: {
			GLint units[MAX_UNITS_PER_MATERIAL];
			uint32_t units_count = 0;

			struct Gpu_Texture * const * gpu_textures = (struct Gpu_Texture * const *)data;
			for (uint32_t i = 0; i < field->array_size; i++) {
				uint32_t unit = graphics_unit_find(gpu_textures[i]);
				if (unit == 0) {
					unit = graphics_unit_init(gpu_textures[i]);
					if (unit == 0) {
						fprintf(stderr, "failed to find a vacant texture/sampler unit\n"); DEBUG_BREAK();
					}
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

static void graphics_upload_uniforms(struct Material const * material) {
	uint32_t const uniforms_count = material->program->uniforms_count;
	struct Gpu_Program_Field const * uniforms = material->program->uniforms;

	uint32_t unit_offset = 0, u32_offset = 0, s32_offset = 0, float_offset = 0;
	for (uint32_t i = 0; i < uniforms_count; i++) {
		uint32_t const elements_count = data_type_get_count(uniforms[i].type) * uniforms[i].array_size;
		switch (data_type_get_element_type(uniforms[i].type)) {
			default: fprintf(stderr, "unknown data type\n"); DEBUG_BREAK(); break;

			case DATA_TYPE_UNIT: {
				graphics_set_uniform(material->program, i, material->textures.data + unit_offset);
				unit_offset += elements_count;
				break;
			}

			case DATA_TYPE_U32: {
				graphics_set_uniform(material->program, i, material->values_u32.data + u32_offset);
				u32_offset += elements_count;
				break;
			}

			case DATA_TYPE_S32: {
				graphics_set_uniform(material->program, i, material->values_s32.data + s32_offset);
				s32_offset += elements_count;
				break;
			}

			case DATA_TYPE_R32: {
				graphics_set_uniform(material->program, i, material->values_float.data + float_offset);
				float_offset += elements_count;
				break;
			}
		}
	}
}

void graphics_draw(struct Material const * material, struct Gpu_Mesh const * gpu_mesh) {
	graphics_select_program(material->program);
	graphics_upload_uniforms(material);

	graphics_select_mesh(gpu_mesh);
	glDrawElements(
		GL_TRIANGLES,
		gpu_mesh->indices_count,
		gpu_data_type(DATA_TYPE_U32),
		NULL
	);
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
	memset(&graphics_state, 0, sizeof(graphics_state));
	graphics_state.extensions = allocate_extensions_string();

	//
	graphics_state.uniforms = strings_init();
	strings_add(graphics_state.uniforms, 0, "");

	//
	GLint max_units;
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_units);
	graphics_state.units_capacity = (uint32_t)max_units;
	graphics_state.units = MEMORY_ALLOCATE_ARRAY(struct Gpu_Unit, max_units);
	memset(graphics_state.units, 0, sizeof(* graphics_state.units) * (size_t)max_units);
}

void graphics_to_glibrary_free(void) {
	strings_free(graphics_state.uniforms);
	MEMORY_FREE(graphics_state.extensions);
	MEMORY_FREE(graphics_state.units);
	memset(&graphics_state, 0, sizeof(graphics_state));

	(void)gpu_attachment_point;
	(void)gpu_comparison_op;
	(void)gpu_cull_mode;
	(void)gpu_winding_order;
	(void)gpu_stencil_op;
	(void)gpu_blend_op;
	(void)gpu_blend_factor;

	(void)graphics_blending;
	(void)graphics_depth_test;
	// (void)graphics_stencil_test;
	(void)graphics_culling;
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
		array_byte_write_many(&string, (uint32_t)strlen((char const *)value), value);
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

//
static GLenum gpu_data_type(enum Data_Type value) {
	switch (value) {
		default: break;

		case DATA_TYPE_UNIT: return GL_SAMPLER_2D;

		case DATA_TYPE_U8:  return GL_UNSIGNED_BYTE;
		case DATA_TYPE_U16: return GL_UNSIGNED_SHORT;

		case DATA_TYPE_S8:  return GL_BYTE;
		case DATA_TYPE_S16: return GL_SHORT;

		case DATA_TYPE_R64: return GL_DOUBLE;

		case DATA_TYPE_U32:   return GL_UNSIGNED_INT;
		case DATA_TYPE_UVEC2: return GL_UNSIGNED_INT_VEC2;
		case DATA_TYPE_UVEC3: return GL_UNSIGNED_INT_VEC3;
		case DATA_TYPE_UVEC4: return GL_UNSIGNED_INT_VEC4;

		case DATA_TYPE_S32:   return GL_INT;
		case DATA_TYPE_SVEC2: return GL_INT_VEC2;
		case DATA_TYPE_SVEC3: return GL_INT_VEC3;
		case DATA_TYPE_SVEC4: return GL_INT_VEC4;

		case DATA_TYPE_R32:  return GL_FLOAT;
		case DATA_TYPE_VEC2: return GL_FLOAT_VEC2;
		case DATA_TYPE_VEC3: return GL_FLOAT_VEC3;
		case DATA_TYPE_VEC4: return GL_FLOAT_VEC4;
		case DATA_TYPE_MAT2: return GL_FLOAT_MAT2;
		case DATA_TYPE_MAT3: return GL_FLOAT_MAT3;
		case DATA_TYPE_MAT4: return GL_FLOAT_MAT4;
	}
	fprintf(stderr, "unknown data type\n"); DEBUG_BREAK();
	return GL_NONE;
}

static enum Data_Type interpret_gl_type(GLint value) {
	switch (value) {
		case GL_SAMPLER_2D:        return DATA_TYPE_UNIT;

		case GL_UNSIGNED_BYTE:     return DATA_TYPE_U8;
		case GL_UNSIGNED_SHORT:    return DATA_TYPE_U16;

		case GL_BYTE:              return DATA_TYPE_S8;
		case GL_SHORT:             return DATA_TYPE_S16;

		case GL_DOUBLE:            return DATA_TYPE_R64;

		case GL_UNSIGNED_INT:      return DATA_TYPE_U32;
		case GL_UNSIGNED_INT_VEC2: return DATA_TYPE_UVEC2;
		case GL_UNSIGNED_INT_VEC3: return DATA_TYPE_UVEC3;
		case GL_UNSIGNED_INT_VEC4: return DATA_TYPE_UVEC4;

		case GL_INT:               return DATA_TYPE_S32;
		case GL_INT_VEC2:          return DATA_TYPE_SVEC2;
		case GL_INT_VEC3:          return DATA_TYPE_SVEC3;
		case GL_INT_VEC4:          return DATA_TYPE_SVEC4;

		case GL_FLOAT:             return DATA_TYPE_R32;
		case GL_FLOAT_VEC2:        return DATA_TYPE_VEC2;
		case GL_FLOAT_VEC3:        return DATA_TYPE_VEC3;
		case GL_FLOAT_VEC4:        return DATA_TYPE_VEC4;
		case GL_FLOAT_MAT2:        return DATA_TYPE_MAT2;
		case GL_FLOAT_MAT3:        return DATA_TYPE_MAT3;
		case GL_FLOAT_MAT4:        return DATA_TYPE_MAT4;
	}
	fprintf(stderr, "unknown GL type\n"); DEBUG_BREAK();
	return DATA_TYPE_NONE;
}

static GLint gpu_min_filter_mode(enum Filter_Mode mipmap, enum Filter_Mode texture) {
	switch (mipmap) {
		case FILTER_MODE_NONE: switch (texture) {
			case FILTER_MODE_NONE:  return GL_NEAREST;
			case FILTER_MODE_POINT: return GL_NEAREST;
			case FILTER_MODE_LERP:  return GL_LINEAR;
		} break;

		case FILTER_MODE_POINT: switch (texture) {
			case FILTER_MODE_NONE:  return GL_NEAREST_MIPMAP_NEAREST;
			case FILTER_MODE_POINT: return GL_NEAREST_MIPMAP_NEAREST;
			case FILTER_MODE_LERP:  return GL_LINEAR_MIPMAP_NEAREST;
		} break;

		case FILTER_MODE_LERP: switch (texture) {
			case FILTER_MODE_NONE:  return GL_NEAREST_MIPMAP_LINEAR;
			case FILTER_MODE_POINT: return GL_NEAREST_MIPMAP_LINEAR;
			case FILTER_MODE_LERP:  return GL_LINEAR_MIPMAP_LINEAR;
		} break;
	}
	fprintf(stderr, "unknown min filter mode\n"); DEBUG_BREAK();
	return GL_NONE;
}

static GLint gpu_mag_filter_mode(enum Filter_Mode value) {
	switch (value) {
		case FILTER_MODE_NONE:  return GL_NEAREST;
		case FILTER_MODE_POINT: return GL_NEAREST;
		case FILTER_MODE_LERP:  return GL_LINEAR;
	}
	fprintf(stderr, "unknown mag filter mode\n"); DEBUG_BREAK();
	return GL_NONE;
}

static GLint gpu_wrap_mode(enum Wrap_Mode value, bool mirror) {
	switch (value) {
		case WRAP_MODE_REPEAT: return mirror ? GL_MIRRORED_REPEAT : GL_REPEAT;
		case WRAP_MODE_CLAMP:  return mirror ? GL_MIRROR_CLAMP_TO_EDGE : GL_CLAMP_TO_EDGE;
	}
	fprintf(stderr, "unknown wrap mode\n"); DEBUG_BREAK();
	return GL_NONE;
}

static GLenum gpu_sized_internal_format(enum Texture_Type texture_type, enum Data_Type data_type, uint32_t channels) {
	switch (texture_type) {
		case TEXTURE_TYPE_NONE: break;
		case TEXTURE_TYPE_INTERNAL_0: break;
		case TEXTURE_TYPE_INTERNAL_1: break;
		case TEXTURE_TYPE_INTERNAL_2: break;

		case TEXTURE_TYPE_COLOR: switch (data_type) {
			default: break;

			case DATA_TYPE_U8: switch (channels) {
				case 1: return GL_R8;
				case 2: return GL_RG8;
				case 3: return GL_RGB8;
				case 4: return GL_RGBA8;
			} break;

			case DATA_TYPE_U16: switch (channels) {
				case 1: return GL_R16;
				case 2: return GL_RG16;
				case 3: return GL_RGB16;
				case 4: return GL_RGBA16;
			} break;

			case DATA_TYPE_U32: switch (channels) {
				case 1: return GL_R32UI;
				case 2: return GL_RG32UI;
				case 3: return GL_RGB32UI;
				case 4: return GL_RGBA32UI;
			} break;

			case DATA_TYPE_R32: switch (channels) {
				case 1: return GL_R32F;
				case 2: return GL_RG32F;
				case 3: return GL_RGB32F;
				case 4: return GL_RGBA32F;
			} break;
		} break;

		case TEXTURE_TYPE_DEPTH: switch (data_type) {
			default: break;
			case DATA_TYPE_U16: return GL_DEPTH_COMPONENT16;
			case DATA_TYPE_U32: return GL_DEPTH_COMPONENT24;
			case DATA_TYPE_R32: return GL_DEPTH_COMPONENT32F;
		} break;

		case TEXTURE_TYPE_STENCIL: switch (data_type) {
			default: break;
			case DATA_TYPE_U8: return GL_STENCIL_INDEX8;
		} break;

		case TEXTURE_TYPE_DSTENCIL: switch (data_type) {
			default: break;
			case DATA_TYPE_U32: return GL_DEPTH24_STENCIL8;
			case DATA_TYPE_R32: return GL_DEPTH32F_STENCIL8;
		} break;
	}
	fprintf(stderr, "unknown sized internal format\n"); DEBUG_BREAK();
	return GL_NONE;
}

static GLenum gpu_pixel_data_format(enum Texture_Type texture_type, uint32_t channels) {
	switch (texture_type) {
		case TEXTURE_TYPE_NONE: break;
		case TEXTURE_TYPE_INTERNAL_0: break;
		case TEXTURE_TYPE_INTERNAL_1: break;
		case TEXTURE_TYPE_INTERNAL_2: break;

		case TEXTURE_TYPE_COLOR: switch (channels) {
			case 1: return GL_RED;
			case 2: return GL_RG;
			case 3: return GL_RGB;
			case 4: return GL_RGBA;
		} break;

		case TEXTURE_TYPE_DEPTH:    return GL_DEPTH_COMPONENT;
		case TEXTURE_TYPE_STENCIL:  return GL_STENCIL_INDEX;
		case TEXTURE_TYPE_DSTENCIL: return GL_DEPTH_STENCIL;
	}
	fprintf(stderr, "unknown pixel data format\n"); DEBUG_BREAK();
	return GL_NONE;
}

static GLenum gpu_pixel_data_type(enum Texture_Type texture_type, enum Data_Type data_type) {
	switch (texture_type) {
		case TEXTURE_TYPE_NONE: break;
		case TEXTURE_TYPE_INTERNAL_0: break;
		case TEXTURE_TYPE_INTERNAL_1: break;
		case TEXTURE_TYPE_INTERNAL_2: break;

		case TEXTURE_TYPE_COLOR: switch (data_type) {
			default: break;
			case DATA_TYPE_U8:  return GL_UNSIGNED_BYTE;
			case DATA_TYPE_U16: return GL_UNSIGNED_SHORT;
			case DATA_TYPE_U32: return GL_UNSIGNED_INT;
			case DATA_TYPE_R32: return GL_FLOAT;
		} break;

		case TEXTURE_TYPE_DEPTH: switch (data_type) {
			default: break;
			case DATA_TYPE_U16: return GL_UNSIGNED_SHORT;
			case DATA_TYPE_U32: return GL_UNSIGNED_INT;
			case DATA_TYPE_R32: return GL_FLOAT;
		} break;

		case TEXTURE_TYPE_STENCIL: switch (data_type) {
			default: break;
			case DATA_TYPE_U8: return GL_UNSIGNED_BYTE;
		} break;

		case TEXTURE_TYPE_DSTENCIL: switch (data_type) {
			default: break;
			case DATA_TYPE_U32: return GL_UNSIGNED_INT_24_8;
			case DATA_TYPE_R32: return GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
		} break;
	}
	fprintf(stderr, "unknown pixel data type\n"); DEBUG_BREAK();
	return GL_NONE;
}

static GLenum gpu_attachment_point(enum Texture_Type texture_type, uint32_t index) {
	switch (texture_type) {
		case TEXTURE_TYPE_NONE: break;
		case TEXTURE_TYPE_INTERNAL_0: break;
		case TEXTURE_TYPE_INTERNAL_1: break;
		case TEXTURE_TYPE_INTERNAL_2: break;

		case TEXTURE_TYPE_COLOR:    return GL_COLOR_ATTACHMENT0 + index;
		case TEXTURE_TYPE_DEPTH:    return GL_DEPTH_ATTACHMENT;
		case TEXTURE_TYPE_STENCIL:  return GL_STENCIL_ATTACHMENT;
		case TEXTURE_TYPE_DSTENCIL: return GL_DEPTH_STENCIL_ATTACHMENT;
	}
	fprintf(stderr, "unknown attachment point\n"); DEBUG_BREAK();
	return GL_NONE;
}

static GLenum gpu_mesh_usage_pattern(enum Mesh_Frequency frequency, enum Mesh_Access access) {
	switch (frequency) {
		case MESH_FREQUENCY_STATIC: switch (access) {
			case MESH_ACCESS_DRAW: return GL_STATIC_DRAW;
			case MESH_ACCESS_READ: return GL_STATIC_READ;
			case MESH_ACCESS_COPY: return GL_STATIC_COPY;
		} break;

		case MESH_FREQUENCY_DYNAMIC: switch (access) {
			case MESH_ACCESS_DRAW: return GL_DYNAMIC_DRAW;
			case MESH_ACCESS_READ: return GL_DYNAMIC_READ;
			case MESH_ACCESS_COPY: return GL_DYNAMIC_COPY;
		} break;

		case MESH_FREQUENCY_STREAM: switch (access) {
			case MESH_ACCESS_DRAW: return GL_STREAM_DRAW;
			case MESH_ACCESS_READ: return GL_STREAM_READ;
			case MESH_ACCESS_COPY: return GL_STREAM_COPY;
		} break;
	}
	fprintf(stderr, "unknown mesh usage\n"); DEBUG_BREAK();
	return GL_NONE;
}

static GLenum gpu_comparison_op(enum Comparison_Op value) {
	switch (value) {
		case COMPARISON_OP_NONE:       break;
		case COMPARISON_OP_FALSE:      return GL_NEVER;
		case COMPARISON_OP_TRUE:       return GL_ALWAYS;
		case COMPARISON_OP_LESS:       return GL_LESS;
		case COMPARISON_OP_EQUAL:      return GL_EQUAL;
		case COMPARISON_OP_MORE:       return GL_GREATER;
		case COMPARISON_OP_NOT_EQUAL:  return GL_NOTEQUAL;
		case COMPARISON_OP_LESS_EQUAL: return GL_LEQUAL;
		case COMPARISON_OP_MORE_EQUAL: return GL_GEQUAL;
	}
	fprintf(stderr, "unknown comparison operation\n"); DEBUG_BREAK();
	return GL_NONE;
}

static GLenum gpu_cull_mode(enum Cull_Mode value) {
	switch (value) {
		case CULL_MODE_NONE:  break;
		case CULL_MODE_BACK:  return GL_BACK;
		case CULL_MODE_FRONT: return GL_FRONT;
		case CULL_MODE_BOTH:  return GL_FRONT_AND_BACK;
	}
	fprintf(stderr, "unknown cull mode\n"); DEBUG_BREAK();
	return GL_NONE;
}

static GLenum gpu_winding_order(enum Winding_Order value) {
	switch (value) {
		case WINDING_ORDER_NEGATIVE: return GL_CCW;
		case WINDING_ORDER_POSITIVE:  return GL_CW;
	}
	fprintf(stderr, "unknown winding order\n"); DEBUG_BREAK();
	return GL_NONE;
}

static GLenum gpu_stencil_op(enum Stencil_Op value) {
	switch (value) {
		case STENCIL_OP_ZERO:      return GL_ZERO;
		case STENCIL_OP_KEEP:      return GL_KEEP;
		case STENCIL_OP_REPLACE:   return GL_REPLACE;
		case STENCIL_OP_INVERT:    return GL_INVERT;
		case STENCIL_OP_INCR:      return GL_INCR;
		case STENCIL_OP_DECR:      return GL_DECR;
		case STENCIL_OP_INCR_WRAP: return GL_INCR_WRAP;
		case STENCIL_OP_DECR_WRAP: return GL_DECR_WRAP;
	}
	fprintf(stderr, "unknown stencil operation\n"); DEBUG_BREAK();
	return GL_NONE;
}

static GLenum gpu_blend_op(enum Blend_Op value) {
	switch (value) {
		case BLEND_OP_NONE:        break;
		case BLEND_OP_ADD:         return GL_FUNC_ADD;
		case BLEND_OP_SUB:         return GL_FUNC_SUBTRACT;
		case BLEND_OP_MIN:         return GL_MIN;
		case BLEND_OP_MAX:         return GL_MAX;
		case BLEND_OP_REVERSE_SUB: return GL_FUNC_REVERSE_SUBTRACT;
	}
	fprintf(stderr, "unknown blend operation\n"); DEBUG_BREAK();
	return GL_NONE;
}

static GLenum gpu_blend_factor(enum Blend_Factor value) {
	switch (value) {
		case BLEND_FACTOR_ZERO:                  return GL_ZERO;
		case BLEND_FACTOR_ONE:                   return GL_ONE;

		case BLEND_FACTOR_SRC_COLOR:             return GL_SRC_COLOR;
		case BLEND_FACTOR_SRC_ALPHA:             return GL_SRC_ALPHA;
		case BLEND_FACTOR_ONE_MINUS_SRC_COLOR:   return GL_ONE_MINUS_SRC_COLOR;
		case BLEND_FACTOR_ONE_MINUS_SRC_ALPHA:   return GL_ONE_MINUS_SRC_ALPHA;

		case BLEND_FACTOR_DST_COLOR:             return GL_DST_COLOR;
		case BLEND_FACTOR_DST_ALPHA:             return GL_DST_ALPHA;
		case BLEND_FACTOR_ONE_MINUS_DST_COLOR:   return GL_ONE_MINUS_DST_COLOR;
		case BLEND_FACTOR_ONE_MINUS_DST_ALPHA:   return GL_ONE_MINUS_DST_ALPHA;

		case BLEND_FACTOR_CONST_COLOR:           return GL_CONSTANT_COLOR;
		case BLEND_FACTOR_CONST_ALPHA:           return GL_CONSTANT_ALPHA;
		case BLEND_FACTOR_ONE_MINUS_CONST_COLOR: return GL_ONE_MINUS_CONSTANT_COLOR;
		case BLEND_FACTOR_ONE_MINUS_CONST_ALPHA: return GL_ONE_MINUS_CONSTANT_ALPHA;

		case BLEND_FACTOR_SRC1_COLOR:            return GL_SRC1_COLOR;
		case BLEND_FACTOR_SRC1_ALPHA:            return GL_SRC1_ALPHA;
		case BLEND_FACTOR_ONE_MINUS_SRC1_COLOR:  return GL_ONE_MINUS_SRC1_COLOR;
		case BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA:  return GL_ONE_MINUS_SRC1_ALPHA;
	}
	fprintf(stderr, "unknown blend factor\n"); DEBUG_BREAK();
	return GL_NONE;
}
