#if !defined(FRAMEWORK_GRAPHICS_TYPES)
#define FRAMEWORK_GRAPHICS_TYPES

#include "framework/maths_types.h"
#include "framework/containers/array.h"
#include "framework/containers/hashmap.h"

enum Data_Type {
	DATA_TYPE_NONE,

	// samplers
	DATA_TYPE_UNIT_U,
	DATA_TYPE_UNIT_S,
	DATA_TYPE_UNIT_F,

	// u8
	DATA_TYPE_R8_U,
	DATA_TYPE_RG8_U,
	DATA_TYPE_RGB8_U,
	DATA_TYPE_RGBA8_U,

	DATA_TYPE_R8_UNORM,
	DATA_TYPE_RG8_UNORM,
	DATA_TYPE_RGB8_UNORM,
	DATA_TYPE_RGBA8_UNORM,

	// s8
	DATA_TYPE_R8_S,
	DATA_TYPE_RG8_S,
	DATA_TYPE_RGB8_S,
	DATA_TYPE_RGBA8_S,

	DATA_TYPE_R8_SNORM,
	DATA_TYPE_RG8_SNORM,
	DATA_TYPE_RGB8_SNORM,
	DATA_TYPE_RGBA8_SNORM,

	// u16
	DATA_TYPE_R16_U,
	DATA_TYPE_RG16_U,
	DATA_TYPE_RGB16_U,
	DATA_TYPE_RGBA16_U,

	DATA_TYPE_R16_UNORM,
	DATA_TYPE_RG16_UNORM,
	DATA_TYPE_RGB16_UNORM,
	DATA_TYPE_RGBA16_UNORM,

	// s16
	DATA_TYPE_R16_S,
	DATA_TYPE_RG16_S,
	DATA_TYPE_RGB16_S,
	DATA_TYPE_RGBA16_S,

	DATA_TYPE_R16_SNORM,
	DATA_TYPE_RG16_SNORM,
	DATA_TYPE_RGB16_SNORM,
	DATA_TYPE_RGBA16_SNORM,

	// u32
	DATA_TYPE_R32_U,
	DATA_TYPE_RG32_U,
	DATA_TYPE_RGB32_U,
	DATA_TYPE_RGBA32_U,

	// s32
	DATA_TYPE_R32_S,
	DATA_TYPE_RG32_S,
	DATA_TYPE_RGB32_S,
	DATA_TYPE_RGBA32_S,

	// floating
	DATA_TYPE_R16_F,
	DATA_TYPE_RG16_F,
	DATA_TYPE_RGB16_F,
	DATA_TYPE_RGBA16_F,

	DATA_TYPE_R32_F,
	DATA_TYPE_RG32_F,
	DATA_TYPE_RGB32_F,
	DATA_TYPE_RGBA32_F,

	DATA_TYPE_R64_F,
	DATA_TYPE_RG64_F,
	DATA_TYPE_RGB64_F,
	DATA_TYPE_RGBA64_F,

	// matrices
	DATA_TYPE_MAT2,
	DATA_TYPE_MAT3,
	DATA_TYPE_MAT4,
};

enum Filter_Mode {
	FILTER_MODE_NONE,
	FILTER_MODE_POINT,
	FILTER_MODE_LERP,
};

enum Wrap_Mode {
	WRAP_MODE_NONE,
	WRAP_MODE_EDGE,
	WRAP_MODE_REPEAT,
	WRAP_MODE_BORDER,
	WRAP_MODE_MIRROR_EDGE,
	WRAP_MODE_MIRROR_REPEAT,
};

enum Texture_Type {
	TEXTURE_TYPE_NONE    = 0,
	TEXTURE_TYPE_COLOR   = (1 << 0),
	TEXTURE_TYPE_DEPTH   = (1 << 1),
	TEXTURE_TYPE_STENCIL = (1 << 2),
	// shorthands
	TEXTURE_TYPE_DSTENCIL = TEXTURE_TYPE_DEPTH | TEXTURE_TYPE_STENCIL,
};

enum Texture_Flag {
	TEXTURE_FLAG_NONE   = 0,
	TEXTURE_FLAG_OPAQUE = (1 << 0),
};

enum Swizzle_Op {
	SWIZZLE_OP_NONE,
	SWIZZLE_OP_0,
	SWIZZLE_OP_1,
	SWIZZLE_OP_R,
	SWIZZLE_OP_G,
	SWIZZLE_OP_B,
	SWIZZLE_OP_A,
};

enum Mesh_Flag {
	MESH_FLAG_NONE  = 0,
	MESH_FLAG_INDEX = (1 << 0),
};

enum Comparison_Op {
	COMPARISON_OP_NONE,
	COMPARISON_OP_FALSE,
	COMPARISON_OP_TRUE,
	COMPARISON_OP_LESS,
	COMPARISON_OP_EQUAL,
	COMPARISON_OP_MORE,
	COMPARISON_OP_NOT_EQUAL,
	COMPARISON_OP_LESS_EQUAL,
	COMPARISON_OP_MORE_EQUAL,
};

enum Cull_Mode {
	CULL_MODE_NONE  = 0,
	CULL_MODE_BACK  = (1 << 0),
	CULL_MODE_FRONT = (1 << 1),
	// shorthands
	CULL_MODE_BOTH = CULL_MODE_BACK | CULL_MODE_FRONT,
};

enum Winding_Order {
	WINDING_ORDER_POSITIVE,
	WINDING_ORDER_NEGATIVE,
};

enum Stencil_Op {
	STENCIL_OP_ZERO,
	STENCIL_OP_KEEP,
	STENCIL_OP_REPLACE,
	STENCIL_OP_INVERT,
	STENCIL_OP_INCR,
	STENCIL_OP_DECR,
	STENCIL_OP_INCR_WRAP,
	STENCIL_OP_DECR_WRAP,
};

enum Color_Channel {
	COLOR_CHANNEL_NONE  = 0,
	COLOR_CHANNEL_RED   = (1 << 0),
	COLOR_CHANNEL_GREEN = (1 << 1),
	COLOR_CHANNEL_BLUE  = (1 << 2),
	COLOR_CHANNEL_ALPHA = (1 << 3),
	// shorthands
	COLOR_CHANNEL_RGB  = COLOR_CHANNEL_RED | COLOR_CHANNEL_GREEN | COLOR_CHANNEL_BLUE,
	COLOR_CHANNEL_FULL = COLOR_CHANNEL_RED | COLOR_CHANNEL_GREEN | COLOR_CHANNEL_BLUE | COLOR_CHANNEL_ALPHA,
};

enum Blend_Mode {
	BLEND_MODE_NONE,
	BLEND_MODE_MIX, // lerp(Drgb, Srgb, Sa), max(Da, Sa)
	BLEND_MODE_PMA, // Drgb * (1 - Sa) + Srgb, max(Da, Sa)
	BLEND_MODE_ADD, // D + S
	BLEND_MODE_SUB, // D - S
	BLEND_MODE_MUL, // D * S
	BLEND_MODE_SCR, // lerp(D, 1, S)
};

enum Depth_Mode {
	DEPTH_MODE_NONE,
	DEPTH_MODE_TRANSPARENT, // test, skip writing
	DEPTH_MODE_OPAQUE,      // test and write
};

struct Texture_Parameters {
	enum Texture_Type texture_type;
	enum Data_Type data_type;
	enum Texture_Flag flags;
};

struct Texture_Settings {
	uint32_t max_lod;
	enum Swizzle_Op swizzle[4];
};

struct Sampler_Settings {
	enum Filter_Mode mipmap, minification, magnification;
	enum Wrap_Mode wrap_x, wrap_y;
	struct vec4 border;
};

enum Mesh_Mode {
	MESH_MODE_NONE,
	MESH_MODE_POINTS,
	MESH_MODE_LINES,
	MESH_MODE_LINE_STRIP,
	MESH_MODE_LINE_LOOP,
	MESH_MODE_TRIANGLES,
	MESH_MODE_TRIANGLE_STRIP,
	MESH_MODE_TRIANGLE_FAN,
};

enum Attribute_Type {
	ATTRIBUTE_TYPE_NONE,
	ATTRIBUTE_TYPE_POSITION,
	ATTRIBUTE_TYPE_TEXCOORD,
	ATTRIBUTE_TYPE_NORMAL,
	ATTRIBUTE_TYPE_COLOR,
	//
	ATTRIBUTE_TYPE_INTERNAL_COUNT,
};

#define MESH_ATTRIBUTES_CAPACITY (2 * ATTRIBUTE_TYPE_INTERNAL_COUNT)

struct Mesh_Parameters {
	enum Mesh_Mode mode;
	enum Data_Type type;
	enum Mesh_Flag flags;
	uint32_t attributes[MESH_ATTRIBUTES_CAPACITY];
};

enum Buffer_Mode {
	BUFFER_MODE_NONE,
	BUFFER_MODE_UNIFORM,
	BUFFER_MODE_STORAGE,
};

enum Block_Type {
	BLOCK_TYPE_NONE,
	BLOCK_TYPE_GLOBAL,
	BLOCK_TYPE_CAMERA,
	BLOCK_TYPE_MODEL,
	BLOCK_TYPE_DYNAMIC,
};

//

struct GPU_Unit {
	struct Handle gh_texture;
};

struct GPU_Uniform {
	enum Data_Type type;
	uint32_t array_size;
};

struct GPU_Program {
	struct Hashmap uniforms; // uniform string id : `struct GPU_Uniform` (at least)
	// @idea: add an optional asset source
};

struct GPU_Texture {
	struct uvec2 size;
	struct Texture_Parameters parameters;
	struct Texture_Settings settings;
	struct Sampler_Settings sampler;
	// @idea: add an optional asset source
};

struct GPU_Target_Buffer {
	struct Texture_Parameters parameters;
};

struct GPU_Target {
	struct uvec2 size;
	struct Array textures; // `struct Handle`
	struct Array buffers;  // `struct GPU_Target_Buffer` (at least)
	// @idea: add an optional asset source
};

struct GPU_Buffer {
	size_t capacity, size;
};

struct GPU_Mesh {
	struct Array buffers;    // `struct Handle`
	struct Array parameters; // `struct Mesh_Parameters`
	// @idea: add an optional asset source
};

//

enum Data_Type data_type_get_element_type(enum Data_Type value);
enum Data_Type data_type_get_vector_type(enum Data_Type value, uint32_t channels);
uint32_t data_type_get_count(enum Data_Type value);
uint32_t data_type_get_size(enum Data_Type value);
bool data_type_is_integer(enum Data_Type value);

#endif
