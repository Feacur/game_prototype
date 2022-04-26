#if !defined(GAME_GPU_TYPES)
#define GAME_GPU_TYPES

#include "framework/graphics/types.h"

#include <KHR/khrplatform.h>
#include <GL/glcorearb.h>

GLenum gpu_data_type(enum Data_Type value);
enum Data_Type interpret_gl_type(GLint value);

GLint gpu_min_filter_mode(enum Filter_Mode mipmap, enum Filter_Mode texture);
GLint gpu_mag_filter_mode(enum Filter_Mode value);
GLint gpu_wrap_mode(enum Wrap_Mode value);

GLenum gpu_sized_internal_format(enum Texture_Type texture_type, enum Data_Type data_type);
GLenum gpu_pixel_data_format(enum Texture_Type texture_type, enum Data_Type data_type);
GLenum gpu_pixel_data_type(enum Texture_Type texture_type, enum Data_Type data_type);
GLenum gpu_attachment_point(enum Texture_Type texture_type, uint32_t index);

GLenum gpu_mesh_usage_pattern(enum Mesh_Flag flags);
GLbitfield gpu_mesh_immutable_flag(enum Mesh_Flag flags);

GLenum gpu_comparison_op(enum Comparison_Op value);
GLenum gpu_cull_mode(enum Cull_Mode value);
GLenum gpu_winding_order(enum Winding_Order value);
GLenum gpu_stencil_op(enum Stencil_Op value);

/*
GLenum gpu_blend_op(enum Blend_Op value);
GLenum gpu_blend_factor(enum Blend_Factor value);
*/
struct Gpu_Blend_Mode {
	GLboolean mask[4];
	GLenum color_src, color_op, color_dst;
	GLenum alpha_src, alpha_op, alpha_dst;
} gpu_blend_mode(enum Blend_Mode value);

struct Gpu_Depth_Mode {
	GLboolean mask; GLenum op;
} gpu_depth_mode(enum Depth_Mode value, bool reversed_z);

GLint gpu_swizzle_op(enum Swizzle_Op value, uint32_t index);

#endif
