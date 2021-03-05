#if !defined(GAME_ASSET_MESH)
#define GAME_ASSET_MESH

#include "array_float.h"
#include "array_u32.h"

struct Asset_Mesh {
	struct Array_Float vertices;
	struct Array_U32 sizes, locations;
	struct Array_U32 indices;
};

void asset_mesh_init(struct Asset_Mesh * asset_mesh, char const * path);
void asset_mesh_free(struct Asset_Mesh * asset_mesh);

#endif
