#if !defined(FRAMEWORK_GRAPHICS_TYPES)
#define FRAMEWORK_GRAPHICS_TYPES

#include "framework/maths_types.h"

// ----- ----- ----- ----- -----
//     Basic
// ----- ----- ----- ----- -----

enum Gfx_Type {
	GFX_TYPE_NONE,

	// samplers
	GFX_TYPE_UNIT_U,
	GFX_TYPE_UNIT_S,
	GFX_TYPE_UNIT_F,

	// u8
	GFX_TYPE_R8_U,
	GFX_TYPE_RG8_U,
	GFX_TYPE_RGB8_U,
	GFX_TYPE_RGBA8_U,

	GFX_TYPE_R8_UNORM,
	GFX_TYPE_RG8_UNORM,
	GFX_TYPE_RGB8_UNORM,
	GFX_TYPE_RGBA8_UNORM,

	// s8
	GFX_TYPE_R8_S,
	GFX_TYPE_RG8_S,
	GFX_TYPE_RGB8_S,
	GFX_TYPE_RGBA8_S,

	GFX_TYPE_R8_SNORM,
	GFX_TYPE_RG8_SNORM,
	GFX_TYPE_RGB8_SNORM,
	GFX_TYPE_RGBA8_SNORM,

	// u16
	GFX_TYPE_R16_U,
	GFX_TYPE_RG16_U,
	GFX_TYPE_RGB16_U,
	GFX_TYPE_RGBA16_U,

	GFX_TYPE_R16_UNORM,
	GFX_TYPE_RG16_UNORM,
	GFX_TYPE_RGB16_UNORM,
	GFX_TYPE_RGBA16_UNORM,

	// s16
	GFX_TYPE_R16_S,
	GFX_TYPE_RG16_S,
	GFX_TYPE_RGB16_S,
	GFX_TYPE_RGBA16_S,

	GFX_TYPE_R16_SNORM,
	GFX_TYPE_RG16_SNORM,
	GFX_TYPE_RGB16_SNORM,
	GFX_TYPE_RGBA16_SNORM,

	// u32
	GFX_TYPE_R32_U,
	GFX_TYPE_RG32_U,
	GFX_TYPE_RGB32_U,
	GFX_TYPE_RGBA32_U,

	// s32
	GFX_TYPE_R32_S,
	GFX_TYPE_RG32_S,
	GFX_TYPE_RGB32_S,
	GFX_TYPE_RGBA32_S,

	// floating
	GFX_TYPE_R16_F,
	GFX_TYPE_RG16_F,
	GFX_TYPE_RGB16_F,
	GFX_TYPE_RGBA16_F,

	GFX_TYPE_R32_F,
	GFX_TYPE_RG32_F,
	GFX_TYPE_RGB32_F,
	GFX_TYPE_RGBA32_F,

	GFX_TYPE_R64_F,
	GFX_TYPE_RG64_F,
	GFX_TYPE_RGB64_F,
	GFX_TYPE_RGBA64_F,

	// matrices
	GFX_TYPE_MAT2,
	GFX_TYPE_MAT3,
	GFX_TYPE_MAT4,
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
	BLEND_MODE_MIX, // rgb: lerp(D, S, Sa)      a: Da
	BLEND_MODE_PMA, // rgb: D * (1 - Sa) + S    a: Da
	BLEND_MODE_ADD, // rgb: D + S               a: Da
	BLEND_MODE_SUB, // rgb: D - S               a: Da
	BLEND_MODE_MUL, // rgb: D * S               a: Da
	BLEND_MODE_SCR, // rgb: lerp(D, 1, S)       a: Da
};

enum Depth_Mode {
	DEPTH_MODE_NONE,
	DEPTH_MODE_TRANSPARENT, // test, skip writing
	DEPTH_MODE_OPAQUE,      // test and write
};

// ----- ----- ----- ----- -----
//     GPU program part
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
//     GPU sampler part
// ----- ----- ----- ----- -----

enum Lookup_Mode {
	LOOKUP_MODE_NONE,
	LOOKUP_MODE_NEAR,
	LOOKUP_MODE_LERP,
};

enum Addr_Flag {
	ADDR_FLAG_NONE   = 0,
	ADDR_FLAG_MIRROR = (1 << 0),
	ADDR_FLAG_EDGE   = (1 << 1),
	ADDR_FLAG_REPEAT = (1 << 2),
	// shorthands
	ADDR_FLAG_MIRROR_EDGE   = ADDR_FLAG_MIRROR | ADDR_FLAG_EDGE,
	ADDR_FLAG_MIRROR_REPEAT = ADDR_FLAG_MIRROR | ADDR_FLAG_REPEAT,
};

struct Gfx_Sampler {
	enum Lookup_Mode mipmap;
	enum Lookup_Mode filter_min, filter_mag;
	enum Addr_Flag addr_x, addr_y, addr_z;
	struct vec4 border;
};

// ----- ----- ----- ----- -----
//     GPU texture part
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

struct Texture_Format {
	enum Texture_Flag flags;
	enum Gfx_Type     type;
};

struct Texture_Settings {
	enum Swizzle_Op swizzle[4];
	uint32_t levels;
};

// ----- ----- ----- ----- -----
//     GPU unit part
// ----- ----- ----- ----- -----

struct Gfx_Unit {
	struct Handle gh_texture;
	struct Handle gh_sampler;
};

// ----- ----- ----- ----- -----
//     GPU target part
// ----- ----- ----- ----- -----

struct Target_Format {
	struct Texture_Format base;
	bool read;
};

// ----- ----- ----- ----- -----
//     GPU buffer part
// ----- ----- ----- ----- -----

enum Buffer_Target {
	BUFFER_TARGET_NONE,
	BUFFER_TARGET_UNIFORM,
	BUFFER_TARGET_STORAGE,
};

// ----- ----- ----- ----- -----
//     GPU mesh part
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

struct Mesh_Format {
	enum Mesh_Mode mode;
	enum Gfx_Type type;
};

struct Mesh_Attributes {
	uint32_t data[2 * SHADER_ATTRIBUTE_INTERNAL_COUNT]; // type, count
};

// ----- ----- ----- ----- -----
//     Conversion
// ----- ----- ----- ----- -----

enum Gfx_Type gfx_type_get_element_type(enum Gfx_Type value);
enum Gfx_Type gfx_type_get_vector_type(enum Gfx_Type value, uint32_t channels);
uint32_t gfx_type_get_count(enum Gfx_Type value);
uint32_t gfx_type_get_size(enum Gfx_Type value);
bool gfx_type_is_integer(enum Gfx_Type value);

#endif
