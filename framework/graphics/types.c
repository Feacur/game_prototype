#include "framework/logger.h"
#include "framework/containers/ref.h"

//
#include "types.h"

enum Data_Type data_type_get_element_type(enum Data_Type value) {
	switch (value) {
		default: break;

		case DATA_TYPE_RG32_U:
		case DATA_TYPE_RGB32_U:
		case DATA_TYPE_RGBA32_U:
			return DATA_TYPE_R32_U;

		case DATA_TYPE_RG32_S:
		case DATA_TYPE_RGB32_S:
		case DATA_TYPE_RGBA32_S:
			return DATA_TYPE_R32_S;

		case DATA_TYPE_RG32_F:
		case DATA_TYPE_RGB32_F:
		case DATA_TYPE_RGBA32_F:
			return DATA_TYPE_R32_F;

		case DATA_TYPE_MAT2:
		case DATA_TYPE_MAT3:
		case DATA_TYPE_MAT4:
			return DATA_TYPE_R32_F;
	}
	return value;
}

uint32_t data_type_get_count(enum Data_Type value) {
	switch (value) {
		case DATA_TYPE_NONE: break;

		case DATA_TYPE_UNIT: return 1;

		case DATA_TYPE_R8_U:  return 1;
		case DATA_TYPE_R8_S:  return 1;

		case DATA_TYPE_R16_U: return 1;
		case DATA_TYPE_R16_S: return 1;
		case DATA_TYPE_R16_F: return 1;

		case DATA_TYPE_R32_U:    return 1;
		case DATA_TYPE_RG32_U:   return 2;
		case DATA_TYPE_RGB32_U:  return 3;
		case DATA_TYPE_RGBA32_U: return 4;

		case DATA_TYPE_R32_S:    return 1;
		case DATA_TYPE_RG32_S:   return 2;
		case DATA_TYPE_RGB32_S:  return 3;
		case DATA_TYPE_RGBA32_S: return 4;

		case DATA_TYPE_R32_F:    return 1;
		case DATA_TYPE_RG32_F:   return 2;
		case DATA_TYPE_RGB32_F:  return 3;
		case DATA_TYPE_RGBA32_F: return 4;

		case DATA_TYPE_MAT2: return 2 * 2;
		case DATA_TYPE_MAT3: return 3 * 3;
		case DATA_TYPE_MAT4: return 4 * 4;

		case DATA_TYPE_R64_U: return 1;
		case DATA_TYPE_R64_S: return 1;
		case DATA_TYPE_R64_F: return 1;
	}
	logger_to_console("unknown data type\n"); DEBUG_BREAK();
	return 0;
}

uint32_t data_type_get_size(enum Data_Type value) {
	switch (value) {
		case DATA_TYPE_NONE: break;

		case DATA_TYPE_UNIT: return sizeof(struct Ref);

		case DATA_TYPE_R8_U:  return sizeof(uint8_t);
		case DATA_TYPE_R8_S:  return sizeof(int8_t);

		case DATA_TYPE_R16_U: return sizeof(uint16_t);
		case DATA_TYPE_R16_S: return sizeof(int16_t);
		case DATA_TYPE_R16_F: return sizeof(float) / 2;

		case DATA_TYPE_R32_U:    return sizeof(uint32_t);
		case DATA_TYPE_RG32_U:   return sizeof(uint32_t) * 2;
		case DATA_TYPE_RGB32_U:  return sizeof(uint32_t) * 3;
		case DATA_TYPE_RGBA32_U: return sizeof(uint32_t) * 4;

		case DATA_TYPE_R32_S:    return sizeof(int32_t);
		case DATA_TYPE_RG32_S:   return sizeof(int32_t) * 2;
		case DATA_TYPE_RGB32_S:  return sizeof(int32_t) * 3;
		case DATA_TYPE_RGBA32_S: return sizeof(int32_t) * 4;

		case DATA_TYPE_R32_F:    return sizeof(float);
		case DATA_TYPE_RG32_F:   return sizeof(float) * 2;
		case DATA_TYPE_RGB32_F:  return sizeof(float) * 3;
		case DATA_TYPE_RGBA32_F: return sizeof(float) * 4;

		case DATA_TYPE_MAT2: return sizeof(float) * 2 * 2;
		case DATA_TYPE_MAT3: return sizeof(float) * 3 * 3;
		case DATA_TYPE_MAT4: return sizeof(float) * 4 * 4;

		case DATA_TYPE_R64_U: return sizeof(uint64_t);
		case DATA_TYPE_R64_S: return sizeof(int64_t);
		case DATA_TYPE_R64_F: return sizeof(double);
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
