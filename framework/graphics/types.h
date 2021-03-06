#if !defined(GAME_GRAPHICS_TYPES)
#define GAME_GRAPHICS_TYPES

#include "framework/common.h"

enum Data_Type {
	DATA_TYPE_NONE,
	DATA_TYPE_UNIT,

	DATA_TYPE_U8,
	DATA_TYPE_U16,
	DATA_TYPE_U32,
	DATA_TYPE_U64,

	DATA_TYPE_S8,
	DATA_TYPE_S16,
	DATA_TYPE_S32,
	DATA_TYPE_S64,

	DATA_TYPE_R32,
	DATA_TYPE_R64,

	DATA_TYPE_UVEC2,
	DATA_TYPE_UVEC3,
	DATA_TYPE_UVEC4,

	DATA_TYPE_SVEC2,
	DATA_TYPE_SVEC3,
	DATA_TYPE_SVEC4,

	DATA_TYPE_VEC2,
	DATA_TYPE_VEC3,
	DATA_TYPE_VEC4,

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
	WRAP_MODE_CLAMP,
	WRAP_MODE_REPEAT,
};

enum Texture_Type {
	TEXTURE_TYPE_NONE     = 0,
	TEXTURE_TYPE_COLOR    = (1 << 0),
	TEXTURE_TYPE_DEPTH    = (1 << 1),
	TEXTURE_TYPE_STENCIL  = (1 << 2),
	// shorthands
	TEXTURE_TYPE_DSTENCIL = TEXTURE_TYPE_DEPTH | TEXTURE_TYPE_STENCIL,
};

enum Texture_Flag {
	TEXTURE_FLAG_NONE     = 0,
	TEXTURE_FLAG_MUTABLE  = (1 << 0),
	TEXTURE_FLAG_WRITE    = (1 << 1),
	TEXTURE_FLAG_READ     = (1 << 2),
	TEXTURE_FLAG_INTERNAL = (1 << 3),
};

enum Mesh_Flag {
	MESH_FLAG_NONE     = 0,
	MESH_FLAG_MUTABLE  = (1 << 0),
	MESH_FLAG_WRITE    = (1 << 1),
	MESH_FLAG_READ     = (1 << 2),
	MESH_FLAG_INTERNAL = (1 << 3),
	MESH_FLAG_INDEX    = (1 << 4),
	MESH_FLAG_FREQUENT = (1 << 5),
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
	CULL_MODE_NONE,
	CULL_MODE_BACK,
	CULL_MODE_FRONT,
	CULL_MODE_BOTH,
};

enum Winding_Order {
	WINDING_ORDER_NEGATIVE,
	WINDING_ORDER_POSITIVE,
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

enum Blend_Op {
	BLEND_OP_NONE,
	BLEND_OP_ADD,
	BLEND_OP_SUB,
	BLEND_OP_MIN,
	BLEND_OP_MAX,
	BLEND_OP_REVERSE_SUB,
};

enum Blend_Factor {
	BLEND_FACTOR_ZERO,
	BLEND_FACTOR_ONE,

	BLEND_FACTOR_SRC_COLOR,
	BLEND_FACTOR_SRC_ALPHA,
	BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
	BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,

	BLEND_FACTOR_DST_COLOR,
	BLEND_FACTOR_DST_ALPHA,
	BLEND_FACTOR_ONE_MINUS_DST_COLOR,
	BLEND_FACTOR_ONE_MINUS_DST_ALPHA,

	BLEND_FACTOR_CONST_COLOR,
	BLEND_FACTOR_CONST_ALPHA,
	BLEND_FACTOR_ONE_MINUS_CONST_COLOR,
	BLEND_FACTOR_ONE_MINUS_CONST_ALPHA,

	BLEND_FACTOR_SRC1_COLOR,
	BLEND_FACTOR_SRC1_ALPHA,
	BLEND_FACTOR_ONE_MINUS_SRC1_COLOR,
	BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA,
};

enum Color_Channel {
	COLOR_CHANNEL_NONE  = 0,
	COLOR_CHANNEL_RED   = (1 << 0),
	COLOR_CHANNEL_GREEN = (1 << 1),
	COLOR_CHANNEL_BLUE  = (1 << 2),
	COLOR_CHANNEL_ALPHA = (1 << 3),
	// shorthands
	COLOR_CHANNEL_RGB = COLOR_CHANNEL_RED | COLOR_CHANNEL_GREEN | COLOR_CHANNEL_BLUE,
	COLOR_CHANNEL_FULL = COLOR_CHANNEL_RED | COLOR_CHANNEL_GREEN | COLOR_CHANNEL_BLUE | COLOR_CHANNEL_ALPHA,
};

struct Gpu_Program_Field {
	uint32_t id;
	enum Data_Type type;
	uint32_t array_size;
};

struct Blend_Func {
	enum Blend_Op op;
	enum Blend_Factor src, dst;
};

struct Blend_Mode {
	struct Blend_Func rgb, a;
	enum Color_Channel mask;
	uint32_t rgba;
};

struct Depth_Mode {
	bool enabled, mask;
};

struct Texture_Parameters {
	enum Texture_Type texture_type;
	enum Data_Type data_type;
	enum Texture_Flag flags;
	uint32_t channels;
};

struct Texture_Settings {
	enum Filter_Mode mipmap, minification, magnification;
	enum Wrap_Mode wrap_x, wrap_y;
	bool mirror_wrap_x, mirror_wrap_y;
};

enum Attribute_Type {
	ATTRIBUTE_TYPE_POSITION,
	ATTRIBUTE_TYPE_TEXCOORD,
	ATTRIBUTE_TYPE_NORMAL,
	ATTRIBUTE_TYPE_COLOR,
	//
	ATTRIBUTE_TYPE_INTERNAL_COUNT,
};

#define MAX_MESH_ATTRIBUTES (2 * ATTRIBUTE_TYPE_INTERNAL_COUNT)

struct Mesh_Parameters {
	enum Data_Type type;
	enum Mesh_Flag flags;
	uint32_t attributes_count;
	uint32_t attributes[MAX_MESH_ATTRIBUTES];
};

enum Data_Type data_type_get_element_type(enum Data_Type value);
uint32_t data_type_get_count(enum Data_Type value);
uint32_t data_type_get_size(enum Data_Type value);

#endif
