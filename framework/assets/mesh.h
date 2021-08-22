#if !defined(GAME_ASSETS_MESH)
#define GAME_ASSETS_MESH

#include "framework/containers/array_byte.h"

#include "framework/graphics/types.h"

struct Mesh {
	uint32_t capacity, count;
	struct Array_Byte * buffers;
	struct Mesh_Parameters * parameters;
};

void mesh_init(struct Mesh * mesh, char const * path);
void mesh_free(struct Mesh * mesh);

#endif
