#if !defined(FRAMEWORK_GRAPHICS_GPU_COMMAND)
#define FRAMEWORK_GRAPHICS_GPU_COMMAND

// interface from `graphics.c` to anywhere
// - gpu draw call

#include "framework/maths_types.h"

#include "gfx_types.h"

struct Gfx_Uniforms;

enum GPU_Command_Type {
	GPU_COMMAND_TYPE_NONE,
	GPU_COMMAND_TYPE_CULL,
	GPU_COMMAND_TYPE_TARGET,
	GPU_COMMAND_TYPE_CLEAR,
	GPU_COMMAND_TYPE_MATERIAL,
	GPU_COMMAND_TYPE_SHADER,
	GPU_COMMAND_TYPE_UNIFORM,
	GPU_COMMAND_TYPE_DRAW,
};

struct GPU_Command_Cull {
	enum Cull_Mode mode;
	enum Winding_Order order;
};

struct GPU_Command_Target {
	struct uvec2 screen_size;
	struct Handle handle;
};

struct GPU_Command_Clear {
	enum Texture_Type mask;
	struct vec4 color;
};

struct GPU_Command_Material {
	struct Handle handle;
};

struct GPU_Command_Shader {
	struct Handle handle;
	enum Blend_Mode blend_mode;
	enum Depth_Mode depth_mode;
};

struct GPU_Command_Uniform {
	struct Handle program_handle;
	struct Gfx_Uniforms const * uniforms;
	uint32_t offset, count;
};

struct GPU_Command_Draw {
	struct Handle mesh_handle;
	uint32_t offset, length;
	uint32_t instances;
};

struct GPU_Command {
	enum GPU_Command_Type type;
	union {
		struct GPU_Command_Cull     cull;
		struct GPU_Command_Target   target;
		struct GPU_Command_Clear    clear;
		struct GPU_Command_Material material;
		struct GPU_Command_Shader   shader;
		struct GPU_Command_Uniform  uniform;
		struct GPU_Command_Draw     draw;
	} as;
};

void gpu_execute(uint32_t length, struct GPU_Command const * commands);

#endif
