#if !defined(GAME_BATCH_MESH)
#define GAME_BATCH_MESH

#include "framework/common.h"

struct Batch_Mesh;
struct Asset_Mesh;

struct Batch_Mesh * batch_mesh_init(uint32_t attributes_count, uint32_t const * attributes);
void batch_mesh_free(struct Batch_Mesh * batch_mesh);

void batch_mesh_clear(struct Batch_Mesh * batch_mesh);
struct Asset_Mesh * batch_mesh_get_asset(struct Batch_Mesh * batch_mesh);

void batch_mesh_add(
	struct Batch_Mesh * batch_mesh,
	uint32_t vertices_count, float * vertices,
	uint32_t indices_count, uint32_t * indices
);

void batch_mesh_add_quad(
	struct Batch_Mesh * batch_mesh,
	float * rect, float * uv
);

#endif
