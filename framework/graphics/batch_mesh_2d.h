#if !defined(GAME_FRAMEWORK_BATCH_MESH_2D)
#define GAME_FRAMEWORK_BATCH_MESH_2D

#include "framework/common.h"

struct Game_Batch_Mesh_2D;
struct Asset_Mesh;

struct Game_Batch_Mesh_2D * game_batch_mesh_2d_init(void);
void game_batch_mesh_2d_free(struct Game_Batch_Mesh_2D * batch_mesh);

void game_batch_mesh_2d_clear(struct Game_Batch_Mesh_2D * batch_mesh);
struct Asset_Mesh * game_batch_mesh_2d_get_asset(struct Game_Batch_Mesh_2D * batch_mesh);

void game_batch_mesh_2d_add_quad(
	struct Game_Batch_Mesh_2D * batch_mesh,
	float const * rect, float const * uv
);

#endif
