#include "framework/logger.h"

//
#include "types.h"

GLenum gpu_data_type(enum Data_Type value) {
	switch (value) {
		default: break;

		case DATA_TYPE_UNIT: return GL_SAMPLER_2D;

		case DATA_TYPE_R8_U: return GL_UNSIGNED_BYTE;
		case DATA_TYPE_R8_S: return GL_BYTE;

		case DATA_TYPE_R16_U: return GL_UNSIGNED_SHORT;
		case DATA_TYPE_R16_S: return GL_SHORT;
		case DATA_TYPE_R16_F: return GL_HALF_FLOAT;

		case DATA_TYPE_R32_U:    return GL_UNSIGNED_INT;
		case DATA_TYPE_RG32_U:   return GL_UNSIGNED_INT_VEC2;
		case DATA_TYPE_RGB32_U:  return GL_UNSIGNED_INT_VEC3;
		case DATA_TYPE_RGBA32_U: return GL_UNSIGNED_INT_VEC4;

		case DATA_TYPE_R32_S:    return GL_INT;
		case DATA_TYPE_RG32_S:   return GL_INT_VEC2;
		case DATA_TYPE_RGB32_S:  return GL_INT_VEC3;
		case DATA_TYPE_RGBA32_S: return GL_INT_VEC4;

		case DATA_TYPE_R32_F:    return GL_FLOAT;
		case DATA_TYPE_RG32_F:   return GL_FLOAT_VEC2;
		case DATA_TYPE_RGB32_F:  return GL_FLOAT_VEC3;
		case DATA_TYPE_RGBA32_F: return GL_FLOAT_VEC4;

		case DATA_TYPE_MAT2: return GL_FLOAT_MAT2;
		case DATA_TYPE_MAT3: return GL_FLOAT_MAT3;
		case DATA_TYPE_MAT4: return GL_FLOAT_MAT4;

		// case DATA_TYPE_R64_U: return GL_UNSIGNED_INT64_ARB;
		// case DATA_TYPE_R64_S: return GL_INT64_ARB;
		case DATA_TYPE_R64_F: return GL_DOUBLE;
	}
	logger_to_console("unknown data type\n"); DEBUG_BREAK();
	return GL_NONE;
}

enum Data_Type interpret_gl_type(GLint value) {
	switch (value) {
		case GL_SAMPLER_2D: return DATA_TYPE_UNIT;

		case GL_UNSIGNED_BYTE: return DATA_TYPE_R8_U;
		case GL_BYTE:          return DATA_TYPE_R8_S;

		case GL_UNSIGNED_SHORT: return DATA_TYPE_R16_U;
		case GL_SHORT:          return DATA_TYPE_R16_S;
		case GL_HALF_FLOAT:     return DATA_TYPE_R16_F;

		case GL_UNSIGNED_INT:      return DATA_TYPE_R32_U;
		case GL_UNSIGNED_INT_VEC2: return DATA_TYPE_RG32_U;
		case GL_UNSIGNED_INT_VEC3: return DATA_TYPE_RGB32_U;
		case GL_UNSIGNED_INT_VEC4: return DATA_TYPE_RGBA32_U;

		case GL_INT:      return DATA_TYPE_R32_S;
		case GL_INT_VEC2: return DATA_TYPE_RG32_S;
		case GL_INT_VEC3: return DATA_TYPE_RGB32_S;
		case GL_INT_VEC4: return DATA_TYPE_RGBA32_S;

		case GL_FLOAT:      return DATA_TYPE_R32_F;
		case GL_FLOAT_VEC2: return DATA_TYPE_RG32_F;
		case GL_FLOAT_VEC3: return DATA_TYPE_RGB32_F;
		case GL_FLOAT_VEC4: return DATA_TYPE_RGBA32_F;

		case GL_FLOAT_MAT2: return DATA_TYPE_MAT2;
		case GL_FLOAT_MAT3: return DATA_TYPE_MAT3;
		case GL_FLOAT_MAT4: return DATA_TYPE_MAT4;

		// case GL_UNSIGNED_INT64_ARB: return DATA_TYPE_R64_U;
		// case GL_INT64_ARB:          return DATA_TYPE_R64_S;
		case GL_DOUBLE:             return DATA_TYPE_R64_F;
	}
	logger_to_console("unknown GL type\n"); DEBUG_BREAK();
	return DATA_TYPE_NONE;
}

GLint gpu_min_filter_mode(enum Filter_Mode mipmap, enum Filter_Mode texture) {
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
	logger_to_console("unknown min filter mode\n"); DEBUG_BREAK();
	return GL_NONE;
}

GLint gpu_mag_filter_mode(enum Filter_Mode value) {
	switch (value) {
		case FILTER_MODE_NONE:  return GL_NEAREST;
		case FILTER_MODE_POINT: return GL_NEAREST;
		case FILTER_MODE_LERP:  return GL_LINEAR;
	}
	logger_to_console("unknown mag filter mode\n"); DEBUG_BREAK();
	return GL_NONE;
}

GLint gpu_wrap_mode(enum Wrap_Mode value) {
	switch (value) {
		case WRAP_MODE_NONE:          return GL_CLAMP_TO_EDGE;
		case WRAP_MODE_EDGE:          return GL_CLAMP_TO_EDGE;
		case WRAP_MODE_REPEAT:        return GL_REPEAT;
		case WRAP_MODE_BORDER:        return GL_CLAMP_TO_BORDER;
		case WRAP_MODE_MIRROR_EDGE:   return GL_MIRROR_CLAMP_TO_EDGE;
		case WRAP_MODE_MIRROR_REPEAT: return GL_MIRRORED_REPEAT;
	}
	logger_to_console("unknown wrap mode\n"); DEBUG_BREAK();
	return GL_NONE;
}

GLenum gpu_sized_internal_format(enum Texture_Type texture_type, enum Data_Type data_type) {
	switch (texture_type) {
		case TEXTURE_TYPE_NONE: break;

		case TEXTURE_TYPE_COLOR: switch (data_type) {
			default: break;

			case DATA_TYPE_R8_U:    return GL_R8;
			case DATA_TYPE_RG8_U:   return GL_RG8;
			case DATA_TYPE_RGB8_U:  return GL_RGB8;
			case DATA_TYPE_RGBA8_U: return GL_RGBA8;

			case DATA_TYPE_R16_U:    return GL_R16;
			case DATA_TYPE_RG16_U:   return GL_RG16;
			case DATA_TYPE_RGB16_U:  return GL_RGB16;
			case DATA_TYPE_RGBA16_U: return GL_RGBA16;

			case DATA_TYPE_R32_F:    return GL_R32F;
			case DATA_TYPE_RG32_F:   return GL_RG32F;
			case DATA_TYPE_RGB32_F:  return GL_RGB32F;
			case DATA_TYPE_RGBA32_F: return GL_RGBA32F;
		} break;

		case TEXTURE_TYPE_DEPTH: switch (data_type) {
			default: break;
			case DATA_TYPE_R16_U: return GL_DEPTH_COMPONENT16;
			case DATA_TYPE_R32_U: return GL_DEPTH_COMPONENT24;
			case DATA_TYPE_R32_F: return GL_DEPTH_COMPONENT32F;
		} break;

		case TEXTURE_TYPE_STENCIL: switch (data_type) {
			default: break;
			case DATA_TYPE_R8_U: return GL_STENCIL_INDEX8;
		} break;

		case TEXTURE_TYPE_DSTENCIL: switch (data_type) {
			default: break;
			case DATA_TYPE_R32_U: return GL_DEPTH24_STENCIL8;
			case DATA_TYPE_R32_F: return GL_DEPTH32F_STENCIL8;
		} break;
	}
	logger_to_console("unknown sized internal format\n"); DEBUG_BREAK();
	return GL_NONE;
}

GLenum gpu_pixel_data_format(enum Texture_Type texture_type, enum Data_Type data_type) {
	switch (texture_type) {
		case TEXTURE_TYPE_NONE: break;

		case TEXTURE_TYPE_COLOR: switch (data_type) {
			default: break;

			case DATA_TYPE_R8_U:    return GL_RED;
			case DATA_TYPE_RG8_U:   return GL_RG;
			case DATA_TYPE_RGB8_U:  return GL_RGB;
			case DATA_TYPE_RGBA8_U: return GL_RGBA;

			case DATA_TYPE_R16_U:    return GL_RED;
			case DATA_TYPE_RG16_U:   return GL_RG;
			case DATA_TYPE_RGB16_U:  return GL_RGB;
			case DATA_TYPE_RGBA16_U: return GL_RGBA;

			case DATA_TYPE_R32_F:    return GL_RED;
			case DATA_TYPE_RG32_F:   return GL_RG;
			case DATA_TYPE_RGB32_F:  return GL_RGB;
			case DATA_TYPE_RGBA32_F: return GL_RGBA;
		} break;

		case TEXTURE_TYPE_DEPTH:    return GL_DEPTH_COMPONENT;
		case TEXTURE_TYPE_STENCIL:  return GL_STENCIL_INDEX;
		case TEXTURE_TYPE_DSTENCIL: return GL_DEPTH_STENCIL;
	}
	logger_to_console("unknown pixel data format\n"); DEBUG_BREAK();
	return GL_NONE;
}

GLenum gpu_pixel_data_type(enum Texture_Type texture_type, enum Data_Type data_type) {
	enum Data_Type const element_type = data_type_get_element_type(data_type);
	switch (texture_type) {
		case TEXTURE_TYPE_NONE: break;

		case TEXTURE_TYPE_COLOR: switch (element_type) {
			default: break;
			case DATA_TYPE_R8_U:  return GL_UNSIGNED_BYTE;
			case DATA_TYPE_R16_U: return GL_UNSIGNED_SHORT;
			case DATA_TYPE_R32_F: return GL_FLOAT;
		} break;

		case TEXTURE_TYPE_DEPTH: switch (element_type) {
			default: break;
			case DATA_TYPE_R16_U: return GL_UNSIGNED_SHORT;
			case DATA_TYPE_R32_U: return GL_UNSIGNED_INT;
			case DATA_TYPE_R32_F: return GL_FLOAT;
		} break;

		case TEXTURE_TYPE_STENCIL: switch (element_type) {
			default: break;
			case DATA_TYPE_R8_U: return GL_UNSIGNED_BYTE;
		} break;

		case TEXTURE_TYPE_DSTENCIL: switch (element_type) {
			default: break;
			case DATA_TYPE_R32_U: return GL_UNSIGNED_INT_24_8;
			case DATA_TYPE_R32_F: return GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
		} break;
	}
	logger_to_console("unknown pixel data type\n"); DEBUG_BREAK();
	return GL_NONE;
}

GLenum gpu_attachment_point(enum Texture_Type texture_type, uint32_t index) {
	switch (texture_type) {
		case TEXTURE_TYPE_NONE: break;

		case TEXTURE_TYPE_COLOR:    return GL_COLOR_ATTACHMENT0 + index;
		case TEXTURE_TYPE_DEPTH:    return GL_DEPTH_ATTACHMENT;
		case TEXTURE_TYPE_STENCIL:  return GL_STENCIL_ATTACHMENT;
		case TEXTURE_TYPE_DSTENCIL: return GL_DEPTH_STENCIL_ATTACHMENT;
	}
	logger_to_console("unknown attachment point\n"); DEBUG_BREAK();
	return GL_NONE;
}

GLenum gpu_mesh_usage_pattern(enum Mesh_Flag flags) {
#if defined(__clang__)
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wcomma"
#elif defined(_MSC_VER)
	#pragma warning(push, 0)
#endif

	// @note: those are hints, not directives
	if (flags & MESH_FLAG_MUTABLE) {
		if (flags & MESH_FLAG_FREQUENT) {
			return (flags & MESH_FLAG_READ)     ? GL_STREAM_READ
			     : (flags & MESH_FLAG_WRITE)    ? GL_STREAM_DRAW
			     : (flags & MESH_FLAG_INTERNAL) ? GL_STREAM_COPY
			                                    : (DEBUG_BREAK(), GL_NONE); // catch first
		}
		return (flags & MESH_FLAG_READ)     ? GL_DYNAMIC_READ
		     : (flags & MESH_FLAG_WRITE)    ? GL_DYNAMIC_DRAW
		     : (flags & MESH_FLAG_INTERNAL) ? GL_DYNAMIC_COPY
		                                    : (DEBUG_BREAK(), GL_NONE); // catch first
	}
	return (flags & MESH_FLAG_READ)     ? GL_STATIC_READ
	     : (flags & MESH_FLAG_WRITE)    ? GL_STATIC_DRAW
	     : (flags & MESH_FLAG_INTERNAL) ? GL_STATIC_COPY
	                                    : (DEBUG_BREAK(), GL_NONE); // catch first

#if defined(__clang__)
	#pragma clang diagnostic pop
#elif defined(_MSC_VER)
	#pragma warning(pop)
#endif
}

GLbitfield gpu_mesh_immutable_flag(enum Mesh_Flag flags) {
	GLbitfield bitfield = 0;
	if (flags & MESH_FLAG_WRITE) {
		bitfield |= GL_DYNAMIC_STORAGE_BIT; // for `glNamedBufferSubData`
	}

/*
	if (flags & MESH_FLAG_WRITE) {
		bitfield |= GL_MAP_WRITE_BIT;
	}
	if (flags & MESH_FLAG_READ) {
		bitfield |= GL_MAP_READ_BIT;
	}
	if (bitfield != 0) {
		bitfield |= GL_MAP_PERSISTENT_BIT;
		bitfield |= GL_MAP_COHERENT_BIT;
	}
*/

	return bitfield;
}

GLenum gpu_comparison_op(enum Comparison_Op value) {
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
	logger_to_console("unknown comparison operation\n"); DEBUG_BREAK();
	return GL_NONE;
}

GLenum gpu_cull_mode(enum Cull_Mode value) {
	switch (value) {
		case CULL_MODE_NONE:  break;
		case CULL_MODE_BACK:  return GL_BACK;
		case CULL_MODE_FRONT: return GL_FRONT;
		case CULL_MODE_BOTH:  return GL_FRONT_AND_BACK;
	}
	logger_to_console("unknown cull mode\n"); DEBUG_BREAK();
	return GL_NONE;
}

GLenum gpu_winding_order(enum Winding_Order value) {
	switch (value) {
		case WINDING_ORDER_POSITIVE: return GL_CCW;
		case WINDING_ORDER_NEGATIVE: return GL_CW;
	}
	logger_to_console("unknown winding order\n"); DEBUG_BREAK();
	return GL_NONE;
}

GLenum gpu_stencil_op(enum Stencil_Op value) {
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
	logger_to_console("unknown stencil operation\n"); DEBUG_BREAK();
	return GL_NONE;
}

GLenum gpu_blend_op(enum Blend_Op value) {
	switch (value) {
		case BLEND_OP_NONE:        break;
		case BLEND_OP_ADD:         return GL_FUNC_ADD;
		case BLEND_OP_SUB:         return GL_FUNC_SUBTRACT;
		case BLEND_OP_MIN:         return GL_MIN;
		case BLEND_OP_MAX:         return GL_MAX;
		case BLEND_OP_REVERSE_SUB: return GL_FUNC_REVERSE_SUBTRACT;
	}
	logger_to_console("unknown blend operation\n"); DEBUG_BREAK();
	return GL_NONE;
}

GLenum gpu_blend_factor(enum Blend_Factor value) {
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
	logger_to_console("unknown blend factor\n"); DEBUG_BREAK();
	return GL_NONE;
}

GLint gpu_swizzle_op(enum Swizzle_Op value, uint32_t index) {
	switch (value) {
		case SWIZZLE_OP_NONE: switch (index) {
			case 0: return GL_RED;
			case 1: return GL_GREEN;
			case 2: return GL_BLUE;
			case 3: return GL_ALPHA;
		} break;

		case SWIZZLE_OP_0: return GL_ZERO;
		case SWIZZLE_OP_1: return GL_ONE;
		case SWIZZLE_OP_R: return GL_RED;
		case SWIZZLE_OP_G: return GL_GREEN;
		case SWIZZLE_OP_B: return GL_BLUE;
		case SWIZZLE_OP_A: return GL_ALPHA;
	}
	logger_to_console("unknown swizzle operation\n"); DEBUG_BREAK();
	return GL_NONE;
}
