#if !defined(GAME_FRAMEWORK_BATCH_MESH_3D)
#define GAME_FRAMEWORK_BATCH_MESH_3D

#include "framework/common.h"

struct Game_Batch_Mesh_3D;
struct Asset_Mesh;

struct Game_Batch_Mesh_3D * game_batch_mesh_3d_init(void);
void game_batch_mesh_3d_free(struct Game_Batch_Mesh_3D * batch_mesh);

void game_batch_mesh_3d_clear(struct Game_Batch_Mesh_3D * batch_mesh);
struct Asset_Mesh * game_batch_mesh_3d_get_asset(struct Game_Batch_Mesh_3D * batch_mesh);

void game_batch_mesh_3d_add_quad(
	struct Game_Batch_Mesh_3D * batch_mesh,
	float const * rect, float const * uv
);

#endif
