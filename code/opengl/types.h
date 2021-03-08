#if !defined(GAME_GRAPHICS_TYPES)
#define GAME_GRAPHICS_TYPES

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
	WRAP_MODE_REPEAT,
	WRAP_MODE_CLAMP,
};

enum Texture_Type {
	TEXTURE_TYPE_NONE     = 0,
	TEXTURE_TYPE_COLOR    = (1 << 0),
	TEXTURE_TYPE_DEPTH    = (1 << 1),
	TEXTURE_TYPE_STENCIL  = (1 << 2),
	TEXTURE_TYPE_DSTENCIL = TEXTURE_TYPE_DEPTH | TEXTURE_TYPE_STENCIL,
};

enum Mesh_Frequency {
	MESH_FREQUENCY_STATIC,
	MESH_FREQUENCY_DYNAMIC,
	MESH_FREQUENCY_STREAM,
};

enum Mesh_Access {
	MESH_ACCESS_DRAW,
	MESH_ACCESS_READ,
	MESH_ACCESS_COPY,
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
};

#endif
