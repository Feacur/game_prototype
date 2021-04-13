#if !defined(GAME_FRAMEWORK_BATCH_MESH_3D)
#define GAME_FRAMEWORK_BATCH_MESH_3D

#include "framework/common.h"

struct Batch_Mesh_3D;
struct Asset_Mesh;

struct Batch_Mesh_3D * batch_mesh_3d_init(void);
void batch_mesh_3d_free(struct Batch_Mesh_3D * batch_mesh);

void batch_mesh_3d_clear(struct Batch_Mesh_3D * batch_mesh);
struct Asset_Mesh * batch_mesh_3d_get_asset(struct Batch_Mesh_3D * batch_mesh);

uint32_t batch_mesh_3d_get_offset(struct Batch_Mesh_3D * batch_mesh);

void batch_mesh_3d_add_quad(
	struct Batch_Mesh_3D * batch_mesh,
	float const * rect, float const * uv
	// float const * mixer
);

#endif
