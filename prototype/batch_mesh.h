#if !defined(GAME_BATCH_RENDERER)
#define GAME_BATCH_RENDERER

struct Batch_Mesh;
struct Asset_Mesh;

struct Batch_Mesh * batch_mesh_init(uint32_t attributes_count, uint32_t const * attributes);
void batch_mesh_free(struct Batch_Mesh * batch);

struct Asset_Mesh * batch_mesh_get_mesh(struct Batch_Mesh * batch);

#endif
