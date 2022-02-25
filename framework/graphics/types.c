#include "framework/logger.h"

//
#include "types.h"

enum Data_Type data_type_get_element_type(enum Data_Type value) {
	switch (value) {
		default: break;

		case DATA_TYPE_UVEC2:
		case DATA_TYPE_UVEC3:
		case DATA_TYPE_UVEC4:
			return DATA_TYPE_U32;

		case DATA_TYPE_SVEC2:
		case DATA_TYPE_SVEC3:
		case DATA_TYPE_SVEC4:
			return DATA_TYPE_S32;

		case DATA_TYPE_VEC2:
		case DATA_TYPE_VEC3:
		case DATA_TYPE_VEC4:
		case DATA_TYPE_MAT2:
		case DATA_TYPE_MAT3:
		case DATA_TYPE_MAT4:
			return DATA_TYPE_R32;
	}
	return value;
}

uint32_t data_type_get_count(enum Data_Type value) {
	switch (value) {
		case DATA_TYPE_NONE: break;

		case DATA_TYPE_UNIT: return 1;

		case DATA_TYPE_U8:  return 1;
		case DATA_TYPE_U16: return 1;
		case DATA_TYPE_U64: return 1;

		case DATA_TYPE_S8:  return 1;
		case DATA_TYPE_S16: return 1;
		case DATA_TYPE_S64: return 1;

		case DATA_TYPE_R64: return 1;

		case DATA_TYPE_U32:   return 1;
		case DATA_TYPE_UVEC2: return 2;
		case DATA_TYPE_UVEC3: return 3;
		case DATA_TYPE_UVEC4: return 4;

		case DATA_TYPE_S32:   return 1;
		case DATA_TYPE_SVEC2: return 2;
		case DATA_TYPE_SVEC3: return 3;
		case DATA_TYPE_SVEC4: return 4;

		case DATA_TYPE_R32:  return 1;
		case DATA_TYPE_VEC2: return 2;
		case DATA_TYPE_VEC3: return 3;
		case DATA_TYPE_VEC4: return 4;
		case DATA_TYPE_MAT2: return 2 * 2;
		case DATA_TYPE_MAT3: return 3 * 3;
		case DATA_TYPE_MAT4: return 4 * 4;
	}
	logger_to_console("unknown data type\n"); DEBUG_BREAK();
	return 0;
}

uint32_t data_type_get_size(enum Data_Type value) {
	switch (value) {
		case DATA_TYPE_NONE: break;

		case DATA_TYPE_UNIT: return sizeof(uint32_t);

		case DATA_TYPE_U8:  return sizeof(uint8_t);
		case DATA_TYPE_U16: return sizeof(uint16_t);
		case DATA_TYPE_U64: return sizeof(uint64_t);

		case DATA_TYPE_S8:  return sizeof(int8_t);
		case DATA_TYPE_S16: return sizeof(int16_t);
		case DATA_TYPE_S64: return sizeof(int64_t);

		case DATA_TYPE_R64: return sizeof(double);

		case DATA_TYPE_U32:   return sizeof(uint32_t);
		case DATA_TYPE_UVEC2: return sizeof(uint32_t) * 2;
		case DATA_TYPE_UVEC3: return sizeof(uint32_t) * 3;
		case DATA_TYPE_UVEC4: return sizeof(uint32_t) * 4;

		case DATA_TYPE_S32:   return sizeof(int32_t);
		case DATA_TYPE_SVEC2: return sizeof(int32_t) * 2;
		case DATA_TYPE_SVEC3: return sizeof(int32_t) * 3;
		case DATA_TYPE_SVEC4: return sizeof(int32_t) * 4;

		case DATA_TYPE_R32:  return sizeof(float);
		case DATA_TYPE_VEC2: return sizeof(float) * 2;
		case DATA_TYPE_VEC3: return sizeof(float) * 3;
		case DATA_TYPE_VEC4: return sizeof(float) * 4;
		case DATA_TYPE_MAT2: return sizeof(float) * 2 * 2;
		case DATA_TYPE_MAT3: return sizeof(float) * 3 * 3;
		case DATA_TYPE_MAT4: return sizeof(float) * 4 * 4;
	}
	logger_to_console("unknown data type\n"); DEBUG_BREAK();
	return 0;
}

//

// @note: MSVC issues `C2099: initializer is not a constant`
//        when a designated initializer type is specified

struct Blend_Mode const c_blend_mode_opaque = {
	.mask = COLOR_CHANNEL_FULL,
};

struct Blend_Mode const c_blend_mode_transparent = {
	.rgb = {
		.op = BLEND_OP_ADD,
		.src = BLEND_FACTOR_SRC_ALPHA,
		.dst = BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
	},
	.mask = COLOR_CHANNEL_FULL,
};
