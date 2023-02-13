#if !defined(FRAMEWORK_GRAPHICS_GPU_COMMAND)
#define FRAMEWORK_GRAPHICS_GPU_COMMAND

// interface from `gpu_*/graphics.c` to anywhere
// - gpu draw call

#include "framework/vector_types.h"
#include "framework/containers/handle.h"

#include "types.h"
#include "material_override.h"

enum GPU_Command_Type {
	GPU_COMMAND_TYPE_NONE,
	GPU_COMMAND_TYPE_CULL,
	GPU_COMMAND_TYPE_TARGET,
	GPU_COMMAND_TYPE_CLEAR,
	GPU_COMMAND_TYPE_MATERIAL,
	GPU_COMMAND_TYPE_UNIFORM,
	GPU_COMMAND_TYPE_DRAW,
};

struct GPU_Command_Cull {
	enum Cull_Mode mode;
	enum Winding_Order order;
};

struct GPU_Command_Target {
	uint32_t screen_size_x, screen_size_y;
	struct Handle handle;
};

struct GPU_Command_Clear {
	enum Texture_Type mask;
	struct vec4 color;
};

struct GPU_Command_Material {
	struct Handle handle;
};

struct GPU_Command_Uniform {
	struct Handle program_handle;
	struct Gfx_Material_Override override;
};

struct GPU_Command_Draw {
	struct Handle mesh_handle;
	uint32_t offset, length;
};

struct GPU_Command {
	enum GPU_Command_Type type;
	union {
		struct GPU_Command_Cull     cull;
		struct GPU_Command_Target   target;
		struct GPU_Command_Clear    clear;
		struct GPU_Command_Material material;
		struct GPU_Command_Uniform  uniform;
		struct GPU_Command_Draw     draw;
	} as;
};

void gpu_execute(uint32_t length, struct GPU_Command const * commands);

#endif
