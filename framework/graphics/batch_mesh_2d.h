#if !defined(GAME_FRAMEWORK_BATCH_MESH_2D)
#define GAME_FRAMEWORK_BATCH_MESH_2D

#include "framework/common.h"

struct Batch_Mesh_2D;
struct Asset_Mesh;

struct Batch_Mesh_2D * batch_mesh_2d_init(void);
void batch_mesh_2d_free(struct Batch_Mesh_2D * batch_mesh);

void batch_mesh_2d_clear(struct Batch_Mesh_2D * batch_mesh);
struct Asset_Mesh * batch_mesh_2d_get_asset(struct Batch_Mesh_2D * batch_mesh);

uint32_t batch_mesh_2d_get_offset(struct Batch_Mesh_2D * batch_mesh);

void batch_mesh_2d_add_quad(
	struct Batch_Mesh_2D * batch_mesh,
	float const * rect, float const * uv
	// float const * mixer
);

#endif
