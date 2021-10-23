#if !defined(GAME_ASSETS_MESH)
#define GAME_ASSETS_MESH

#include "framework/containers/buffer.h"

#include "framework/graphics/types.h"

struct Mesh {
	uint32_t capacity, count;
	struct Buffer * buffers;
	struct Mesh_Parameters * parameters;
};

void mesh_init(struct Mesh * mesh, struct CString path);
void mesh_free(struct Mesh * mesh);

#endif
