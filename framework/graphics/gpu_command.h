#if !defined(GAME_GRAPHICS_GPU_COMMAND)
#define GAME_GRAPHICS_GPU_COMMAND

// interface from `gpu_*/graphics.c` to anywhere
// - gpu draw call

#include "framework/vector_types.h"
#include "framework/containers/ref.h"

#include "types.h"
#include "material_override.h"

struct Gfx_Material;

enum GPU_Command_Type {
	GPU_COMMAND_TYPE_NONE,
	GPU_COMMAND_TYPE_CULL,
	GPU_COMMAND_TYPE_TARGET,
	GPU_COMMAND_TYPE_CLEAR,
	GPU_COMMAND_TYPE_DRAW,
};

struct GPU_Command_Cull {
	enum Cull_Mode mode;
	enum Winding_Order order;
};

struct GPU_Command_Target {
	uint32_t screen_size_x, screen_size_y;
	struct Ref gpu_ref;
};

struct GPU_Command_Clear {
	enum Texture_Type mask;
	uint32_t rgba;
};

struct GPU_Command_Draw {
	struct Gfx_Material const * material;
	struct Ref gpu_mesh_ref;
	uint32_t offset, length;
	struct Gfx_Material_Override override;
};

struct GPU_Command {
	enum GPU_Command_Type type;
	union {
		struct GPU_Command_Cull   cull;
		struct GPU_Command_Target target;
		struct GPU_Command_Clear  clear;
		struct GPU_Command_Draw   draw;
	} as;
};

void gpu_execute(uint32_t length, struct GPU_Command const * commands);

#endif
