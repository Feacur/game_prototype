#if !defined(FRAMEWORK_ASSETS_MESH)
#define FRAMEWORK_ASSETS_MESH

#include "framework/containers/array.h"
#include "framework/containers/buffer.h"
#include "framework/graphics/gfx_types.h"

struct Buffer;

struct Mesh_Buffer {
	struct Buffer buffer;
	struct Mesh_Buffer_Parameters parameters;
	struct Mesh_Buffer_Attributes attributes;
	bool is_index;
};

struct Mesh {
	struct Array buffers; // `struct Mesh_Buffer`
};

struct Mesh mesh_init(struct Buffer const * source);
void mesh_free(struct Mesh * mesh);

#endif
