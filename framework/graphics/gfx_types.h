#if !defined(FRAMEWORK_GRAPHICS_TYPES)
#define FRAMEWORK_GRAPHICS_TYPES

#include "framework/maths_types.h"

// ----- ----- ----- ----- -----
//     Basic
// ----- ----- ----- ----- -----

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

enum Cull_Flag {
	CULL_FLAG_NONE  = 0,
	CULL_FLAG_BACK  = (1 << 0),
	CULL_FLAG_FRONT = (1 << 1),
	// shorthands
	CULL_FLAG_BOTH = CULL_FLAG_BACK | CULL_FLAG_FRONT,
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

// ----- ----- ----- ----- -----
//     Program part
// ----- ----- ----- ----- -----

enum Shader_Attribute {
	SHADER_ATTRIBUTE_NONE,
	SHADER_ATTRIBUTE_POSITION,
	SHADER_ATTRIBUTE_TEXCOORD,
	SHADER_ATTRIBUTE_NORMAL,
	SHADER_ATTRIBUTE_COLOR,
	//
	SHADER_ATTRIBUTE_INTERNAL_COUNT,
};

enum Shader_Block {
	SHADER_BLOCK_NONE,
	SHADER_BLOCK_GLOBAL,
	SHADER_BLOCK_CAMERA,
	SHADER_BLOCK_MODEL,
	SHADER_BLOCK_DYNAMIC,
};

enum Batcher_Flag {
	BATCHER_FLAG_NONE = 0,
	BATCHER_FLAG_FONT = (1 << 0),
};

// ----- ----- ----- ----- -----
//     Sampler part
// ----- ----- ----- ----- -----

enum Filter_Mode {
	FILTER_MODE_NONE,
	FILTER_MODE_POINT,
	FILTER_MODE_LERP,
};

enum Wrap_Flag {
	WRAP_FLAG_NONE   = 0,
	WRAP_FLAG_MIRROR = (1 << 0),
	WRAP_FLAG_EDGE   = (1 << 1),
	WRAP_FLAG_REPEAT = (1 << 2),
	// shorthands
	WRAP_FLAG_MIRROR_EDGE   = WRAP_FLAG_MIRROR | WRAP_FLAG_EDGE,
	WRAP_FLAG_MIRROR_REPEAT = WRAP_FLAG_MIRROR | WRAP_FLAG_REPEAT,
};

struct Sampler_Settings {
	enum Filter_Mode mipmap, minification, magnification;
	enum Wrap_Flag wrap_x, wrap_y;
	struct vec4 border;
};

// ----- ----- ----- ----- -----
//     Texture part
// ----- ----- ----- ----- -----

enum Texture_Flag {
	TEXTURE_FLAG_NONE    = 0,
	TEXTURE_FLAG_COLOR   = (1 << 0),
	TEXTURE_FLAG_DEPTH   = (1 << 1),
	TEXTURE_FLAG_STENCIL = (1 << 2),
	// shorthands
	TEXTURE_FLAG_DSTENCIL = TEXTURE_FLAG_DEPTH | TEXTURE_FLAG_STENCIL,
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

struct Texture_Parameters {
	enum Texture_Flag flags;
	enum Data_Type    type;
};

struct Texture_Settings {
	uint32_t max_lod;
	enum Swizzle_Op swizzle[4];
};

// ----- ----- ----- ----- -----
//     Target part
// ----- ----- ----- ----- -----

struct Target_Parameters {
	struct Texture_Parameters image;
	bool read;
};

// ----- ----- ----- ----- -----
//     Buffer part
// ----- ----- ----- ----- -----

enum Buffer_Target {
	BUFFER_TARGET_NONE,
	BUFFER_TARGET_UNIFORM,
	BUFFER_TARGET_STORAGE,
};

// ----- ----- ----- ----- -----
//     Mesh part
// ----- ----- ----- ----- -----

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

struct Mesh_Buffer_Parameters {
	enum Mesh_Mode mode;
	enum Data_Type type;
};

struct Mesh_Buffer_Attributes {
	uint32_t data[2 * SHADER_ATTRIBUTE_INTERNAL_COUNT]; // type, count
};

// ----- ----- ----- ----- -----
//     Conversion
// ----- ----- ----- ----- -----

enum Data_Type data_type_get_element_type(enum Data_Type value);
enum Data_Type data_type_get_vector_type(enum Data_Type value, uint32_t channels);
uint32_t data_type_get_count(enum Data_Type value);
uint32_t data_type_get_size(enum Data_Type value);
bool data_type_is_integer(enum Data_Type value);

#endif
