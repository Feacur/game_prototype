#if !defined(GAME_ASSET_MESH)
#define GAME_ASSET_MESH

#include "framework/containers/array_byte.h"

#include "framework/graphics/types.h"

struct Mesh {
	uint32_t capacity, count;
	struct Array_Byte * buffers;
	struct Mesh_Parameters * parameters;
};

void asset_mesh_init(struct Mesh * asset_mesh, char const * path);
void asset_mesh_free(struct Mesh * asset_mesh);

#endif
