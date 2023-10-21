#include "framework/formatter.h"

//
#include "types.h"

enum Data_Type data_type_get_element_type(enum Data_Type value) {
	switch (value) {
		default: break;

		// u8
		case DATA_TYPE_RG8_U:
		case DATA_TYPE_RGB8_U:
		case DATA_TYPE_RGBA8_U:
			return DATA_TYPE_R8_U;

		case DATA_TYPE_RG8_UNORM:
		case DATA_TYPE_RGB8_UNORM:
		case DATA_TYPE_RGBA8_UNORM:
			return DATA_TYPE_R8_UNORM;

		// s8
		case DATA_TYPE_RG8_S:
		case DATA_TYPE_RGB8_S:
		case DATA_TYPE_RGBA8_S:
			return DATA_TYPE_R8_S;

		case DATA_TYPE_RG8_SNORM:
		case DATA_TYPE_RGB8_SNORM:
		case DATA_TYPE_RGBA8_SNORM:
			return DATA_TYPE_R8_SNORM;

		// u16
		case DATA_TYPE_RG16_U:
		case DATA_TYPE_RGB16_U:
		case DATA_TYPE_RGBA16_U:
			return DATA_TYPE_R16_U;

		case DATA_TYPE_RG16_UNORM:
		case DATA_TYPE_RGB16_UNORM:
		case DATA_TYPE_RGBA16_UNORM:
			return DATA_TYPE_R16_UNORM;

		// s16
		case DATA_TYPE_RG16_S:
		case DATA_TYPE_RGB16_S:
		case DATA_TYPE_RGBA16_S:
			return DATA_TYPE_R16_S;

		case DATA_TYPE_RG16_SNORM:
		case DATA_TYPE_RGB16_SNORM:
		case DATA_TYPE_RGBA16_SNORM:
			return DATA_TYPE_R16_SNORM;

		// u32
		case DATA_TYPE_RG32_U:
		case DATA_TYPE_RGB32_U:
		case DATA_TYPE_RGBA32_U:
			return DATA_TYPE_R32_U;

		// s32
		case DATA_TYPE_RG32_S:
		case DATA_TYPE_RGB32_S:
		case DATA_TYPE_RGBA32_S:
			return DATA_TYPE_R32_S;

		// floats
		case DATA_TYPE_RG32_F:
		case DATA_TYPE_RGB32_F:
		case DATA_TYPE_RGBA32_F:
			return DATA_TYPE_R32_F;

		case DATA_TYPE_RG64_F:
		case DATA_TYPE_RGB64_F:
		case DATA_TYPE_RGBA64_F:
			return DATA_TYPE_R64_F;

		// matrices
		case DATA_TYPE_MAT2:
		case DATA_TYPE_MAT3:
		case DATA_TYPE_MAT4:
			return DATA_TYPE_R32_F;
	}
	return value;
}

enum Data_Type data_type_get_vector_type(enum Data_Type value, uint32_t channels) {
	switch (value) {
		default: break;

		// u8
		case DATA_TYPE_R8_U: switch(channels) {
			case 1: return DATA_TYPE_R8_U;
			case 2: return DATA_TYPE_RG8_U;
			case 3: return DATA_TYPE_RGB8_U;
			case 4: return DATA_TYPE_RGBA8_U;
		} break;

		case DATA_TYPE_R8_UNORM: switch(channels) {
			case 1: return DATA_TYPE_R8_UNORM;
			case 2: return DATA_TYPE_RG8_UNORM;
			case 3: return DATA_TYPE_RGB8_UNORM;
			case 4: return DATA_TYPE_RGBA8_UNORM;
		} break;

		// s8
		case DATA_TYPE_R8_S: switch(channels) {
			case 1: return DATA_TYPE_R8_S;
			case 2: return DATA_TYPE_RG8_S;
			case 3: return DATA_TYPE_RGB8_S;
			case 4: return DATA_TYPE_RGBA8_S;
		} break;

		case DATA_TYPE_R8_SNORM: switch(channels) {
			case 1: return DATA_TYPE_R8_SNORM;
			case 2: return DATA_TYPE_RG8_SNORM;
			case 3: return DATA_TYPE_RGB8_SNORM;
			case 4: return DATA_TYPE_RGBA8_SNORM;
		} break;

		// u16
		case DATA_TYPE_R16_U: switch(channels) {
			case 1: return DATA_TYPE_R16_U;
			case 2: return DATA_TYPE_RG16_U;
			case 3: return DATA_TYPE_RGB16_U;
			case 4: return DATA_TYPE_RGBA16_U;
		} break;

		case DATA_TYPE_R16_UNORM: switch(channels) {
			case 1: return DATA_TYPE_R16_UNORM;
			case 2: return DATA_TYPE_RG16_UNORM;
			case 3: return DATA_TYPE_RGB16_UNORM;
			case 4: return DATA_TYPE_RGBA16_UNORM;
		} break;

		// s16
		case DATA_TYPE_R16_S: switch(channels) {
			case 1: return DATA_TYPE_R16_S;
			case 2: return DATA_TYPE_RG16_S;
			case 3: return DATA_TYPE_RGB16_S;
			case 4: return DATA_TYPE_RGBA16_S;
		} break;

		case DATA_TYPE_R16_SNORM: switch(channels) {
			case 1: return DATA_TYPE_R16_SNORM;
			case 2: return DATA_TYPE_RG16_SNORM;
			case 3: return DATA_TYPE_RGB16_SNORM;
			case 4: return DATA_TYPE_RGBA16_SNORM;
		} break;

		// u32
		case DATA_TYPE_R32_U: switch(channels) {
			case 1: return DATA_TYPE_R32_U;
			case 2: return DATA_TYPE_RG32_U;
			case 3: return DATA_TYPE_RGB32_U;
			case 4: return DATA_TYPE_RGBA32_U;
		} break;

		// s32
		case DATA_TYPE_R32_S: switch(channels) {
			case 1: return DATA_TYPE_R32_S;
			case 2: return DATA_TYPE_RG32_S;
			case 3: return DATA_TYPE_RGB32_S;
			case 4: return DATA_TYPE_RGBA32_S;
		} break;

		// floats
		case DATA_TYPE_R32_F: switch(channels) {
			case 1: return DATA_TYPE_R32_F;
			case 2: return DATA_TYPE_RG32_F;
			case 3: return DATA_TYPE_RGB32_F;
			case 4: return DATA_TYPE_RGBA32_F;
		} break;

		case DATA_TYPE_R64_F: switch(channels) {
			case 1: return DATA_TYPE_R64_F;
			case 2: return DATA_TYPE_RG64_F;
			case 3: return DATA_TYPE_RGB64_F;
			case 4: return DATA_TYPE_RGBA64_F;
		} break;
	}
	ERR("unknown vector type\n");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return DATA_TYPE_NONE;
}

uint32_t data_type_get_count(enum Data_Type value) {
	switch (value) {
		case DATA_TYPE_NONE: break;

		// samplers
		case DATA_TYPE_UNIT_U: return 1;
		case DATA_TYPE_UNIT_S: return 1;
		case DATA_TYPE_UNIT_F: return 1;

		// u8
		case DATA_TYPE_R8_U:    return 1;
		case DATA_TYPE_RG8_U:   return 2;
		case DATA_TYPE_RGB8_U:  return 3;
		case DATA_TYPE_RGBA8_U: return 4;

		case DATA_TYPE_R8_UNORM:    return 1;
		case DATA_TYPE_RG8_UNORM:   return 2;
		case DATA_TYPE_RGB8_UNORM:  return 3;
		case DATA_TYPE_RGBA8_UNORM: return 4;

		// s8
		case DATA_TYPE_R8_S:    return 1;
		case DATA_TYPE_RG8_S:   return 2;
		case DATA_TYPE_RGB8_S:  return 3;
		case DATA_TYPE_RGBA8_S: return 4;

		case DATA_TYPE_R8_SNORM:    return 1;
		case DATA_TYPE_RG8_SNORM:   return 2;
		case DATA_TYPE_RGB8_SNORM:  return 3;
		case DATA_TYPE_RGBA8_SNORM: return 4;

		// u16
		case DATA_TYPE_R16_U:    return 1;
		case DATA_TYPE_RG16_U:   return 2;
		case DATA_TYPE_RGB16_U:  return 3;
		case DATA_TYPE_RGBA16_U: return 4;

		case DATA_TYPE_R16_UNORM:    return 1;
		case DATA_TYPE_RG16_UNORM:   return 2;
		case DATA_TYPE_RGB16_UNORM:  return 3;
		case DATA_TYPE_RGBA16_UNORM: return 4;

		// s16
		case DATA_TYPE_R16_S:    return 1;
		case DATA_TYPE_RG16_S:   return 2;
		case DATA_TYPE_RGB16_S:  return 3;
		case DATA_TYPE_RGBA16_S: return 4;

		case DATA_TYPE_R16_SNORM:    return 1;
		case DATA_TYPE_RG16_SNORM:   return 2;
		case DATA_TYPE_RGB16_SNORM:  return 3;
		case DATA_TYPE_RGBA16_SNORM: return 4;

		// u32
		case DATA_TYPE_R32_U:    return 1;
		case DATA_TYPE_RG32_U:   return 2;
		case DATA_TYPE_RGB32_U:  return 3;
		case DATA_TYPE_RGBA32_U: return 4;

		// s32
		case DATA_TYPE_R32_S:    return 1;
		case DATA_TYPE_RG32_S:   return 2;
		case DATA_TYPE_RGB32_S:  return 3;
		case DATA_TYPE_RGBA32_S: return 4;

		// floats
		case DATA_TYPE_R16_F:    return 1;
		case DATA_TYPE_RG16_F:   return 2;
		case DATA_TYPE_RGB16_F:  return 3;
		case DATA_TYPE_RGBA16_F: return 4;

		case DATA_TYPE_R32_F:    return 1;
		case DATA_TYPE_RG32_F:   return 2;
		case DATA_TYPE_RGB32_F:  return 3;
		case DATA_TYPE_RGBA32_F: return 4;

		case DATA_TYPE_R64_F:    return 1;
		case DATA_TYPE_RG64_F:   return 2;
		case DATA_TYPE_RGB64_F:  return 3;
		case DATA_TYPE_RGBA64_F: return 4;

		// matrices
		case DATA_TYPE_MAT2: return 2 * 2;
		case DATA_TYPE_MAT3: return 3 * 3;
		case DATA_TYPE_MAT4: return 4 * 4;
	}
	ERR("unknown data type\n");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return 0;
}

uint32_t data_type_get_size(enum Data_Type value) {
	switch (value) {
		case DATA_TYPE_NONE: break;

		// samplers
		case DATA_TYPE_UNIT_U: return sizeof(struct Handle);
		case DATA_TYPE_UNIT_S: return sizeof(struct Handle);
		case DATA_TYPE_UNIT_F: return sizeof(struct Handle);

		// u8
		case DATA_TYPE_R8_U:    return sizeof(uint8_t);
		case DATA_TYPE_RG8_U:   return sizeof(uint8_t) * 2;
		case DATA_TYPE_RGB8_U:  return sizeof(uint8_t) * 3;
		case DATA_TYPE_RGBA8_U: return sizeof(uint8_t) * 4;

		case DATA_TYPE_R8_UNORM:    return sizeof(uint8_t);
		case DATA_TYPE_RG8_UNORM:   return sizeof(uint8_t) * 2;
		case DATA_TYPE_RGB8_UNORM:  return sizeof(uint8_t) * 3;
		case DATA_TYPE_RGBA8_UNORM: return sizeof(uint8_t) * 4;

		// s8
		case DATA_TYPE_R8_S:    return sizeof(int8_t);
		case DATA_TYPE_RG8_S:   return sizeof(int8_t) * 2;
		case DATA_TYPE_RGB8_S:  return sizeof(int8_t) * 3;
		case DATA_TYPE_RGBA8_S: return sizeof(int8_t) * 4;

		case DATA_TYPE_R8_SNORM:    return sizeof(int8_t);
		case DATA_TYPE_RG8_SNORM:   return sizeof(int8_t) * 2;
		case DATA_TYPE_RGB8_SNORM:  return sizeof(int8_t) * 3;
		case DATA_TYPE_RGBA8_SNORM: return sizeof(int8_t) * 4;

		// u16
		case DATA_TYPE_R16_U:    return sizeof(uint16_t);
		case DATA_TYPE_RG16_U:   return sizeof(uint16_t) * 2;
		case DATA_TYPE_RGB16_U:  return sizeof(uint16_t) * 3;
		case DATA_TYPE_RGBA16_U: return sizeof(uint16_t) * 4;

		case DATA_TYPE_R16_UNORM:    return sizeof(uint16_t);
		case DATA_TYPE_RG16_UNORM:   return sizeof(uint16_t) * 2;
		case DATA_TYPE_RGB16_UNORM:  return sizeof(uint16_t) * 3;
		case DATA_TYPE_RGBA16_UNORM: return sizeof(uint16_t) * 4;

		// s16
		case DATA_TYPE_R16_S:    return sizeof(int16_t);
		case DATA_TYPE_RG16_S:   return sizeof(int16_t) * 2;
		case DATA_TYPE_RGB16_S:  return sizeof(int16_t) * 3;
		case DATA_TYPE_RGBA16_S: return sizeof(int16_t) * 4;

		case DATA_TYPE_R16_SNORM:    return sizeof(int16_t);
		case DATA_TYPE_RG16_SNORM:   return sizeof(int16_t) * 2;
		case DATA_TYPE_RGB16_SNORM:  return sizeof(int16_t) * 3;
		case DATA_TYPE_RGBA16_SNORM: return sizeof(int16_t) * 4;

		// u32
		case DATA_TYPE_R32_U:    return sizeof(uint32_t);
		case DATA_TYPE_RG32_U:   return sizeof(uint32_t) * 2;
		case DATA_TYPE_RGB32_U:  return sizeof(uint32_t) * 3;
		case DATA_TYPE_RGBA32_U: return sizeof(uint32_t) * 4;

		// s32
		case DATA_TYPE_R32_S:    return sizeof(int32_t);
		case DATA_TYPE_RG32_S:   return sizeof(int32_t) * 2;
		case DATA_TYPE_RGB32_S:  return sizeof(int32_t) * 3;
		case DATA_TYPE_RGBA32_S: return sizeof(int32_t) * 4;

		// floats
		case DATA_TYPE_R16_F:    return sizeof(float)     / 2;
		case DATA_TYPE_RG16_F:   return sizeof(float) * 2 / 2;
		case DATA_TYPE_RGB16_F:  return sizeof(float) * 3 / 2;
		case DATA_TYPE_RGBA16_F: return sizeof(float) * 4 / 2;

		case DATA_TYPE_R32_F:    return sizeof(float);
		case DATA_TYPE_RG32_F:   return sizeof(float) * 2;
		case DATA_TYPE_RGB32_F:  return sizeof(float) * 3;
		case DATA_TYPE_RGBA32_F: return sizeof(float) * 4;

		case DATA_TYPE_R64_F:    return sizeof(double);
		case DATA_TYPE_RG64_F:   return sizeof(double) * 2;
		case DATA_TYPE_RGB64_F:  return sizeof(double) * 3;
		case DATA_TYPE_RGBA64_F: return sizeof(double) * 4;

		// matrices
		case DATA_TYPE_MAT2: return sizeof(float) * 2 * 2;
		case DATA_TYPE_MAT3: return sizeof(float) * 3 * 3;
		case DATA_TYPE_MAT4: return sizeof(float) * 4 * 4;
	}
	ERR("unknown data type\n");
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return 0;
}

bool data_type_is_integer(enum Data_Type value) {
	switch (data_type_get_element_type(value)) {
		default: break;

		case DATA_TYPE_R8_U:
		case DATA_TYPE_R8_S:
		case DATA_TYPE_R16_U:
		case DATA_TYPE_R16_S:
		case DATA_TYPE_R32_U:
		case DATA_TYPE_R32_S:
			return true;
	}
	return false;
}
