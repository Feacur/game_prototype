#if !defined(GAME_FRAMEWORK_BATCH_MESH)
#define GAME_FRAMEWORK_BATCH_MESH

#include "framework/common.h"

struct Generic_Batch_Mesh;
struct Asset_Mesh;

struct Generic_Batch_Mesh * generic_batch_mesh_init(uint32_t attributes_count, uint32_t const * attributes);
void generic_batch_mesh_free(struct Generic_Batch_Mesh * batch_mesh);

void generic_batch_mesh_clear(struct Generic_Batch_Mesh * batch_mesh);
struct Asset_Mesh * generic_batch_mesh_get_asset(struct Generic_Batch_Mesh * batch_mesh);

void generic_batch_mesh_add_quad_xy(
	struct Generic_Batch_Mesh * batch_mesh,
	float const * rect, float const * uv
);

#endif
