#if !defined(FRAMEWORK_ASSETS_MESH)
#define FRAMEWORK_ASSETS_MESH

#include "framework/common.h"

struct Buffer;
struct Mesh_Parameters;

struct Mesh {
	uint32_t capacity, count;
	struct Buffer * buffers;
	struct Mesh_Parameters * parameters;
};

struct Mesh mesh_init(struct Buffer const * buffer);
void mesh_free(struct Mesh * mesh);

#endif
