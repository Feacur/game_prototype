#if !defined(GAME_ASSETS_MESH)
#define GAME_ASSETS_MESH

#include "framework/containers/buffer.h"

#include "framework/graphics/types.h"

struct Mesh {
	uint32_t capacity, count;
	struct Buffer * buffers;
	struct Mesh_Parameters * parameters;
};

struct Mesh mesh_init(struct CString path);
void mesh_free(struct Mesh * mesh);

#endif
