#if !defined(GAME_FRAMEWORK_BATCH_MESH)
#define GAME_FRAMEWORK_BATCH_MESH

#include "framework/common.h"

struct Batch_Mesh_Generic;
struct Asset_Mesh;

struct Batch_Mesh_Generic * batch_mesh_generic_init(uint32_t attributes_count, uint32_t const * attributes);
void batch_mesh_generic_free(struct Batch_Mesh_Generic * batch_mesh);

void batch_mesh_generic_clear(struct Batch_Mesh_Generic * batch_mesh);
struct Asset_Mesh * batch_mesh_generic_get_asset(struct Batch_Mesh_Generic * batch_mesh);

uint32_t batch_mesh_generic_get_offset(struct Batch_Mesh_Generic * batch_mesh);

void batch_mesh_generic_add_quad_xy(
	struct Batch_Mesh_Generic * batch_mesh,
	float const * rect, float const * uv
);

#endif
