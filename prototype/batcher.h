#if !defined(GAME_BATCHER)
#define GAME_BATCHER

#include "framework/graphics/types.h"
#include "framework/vector_types.h"

struct Batch_Mesh_2D;
struct Array_Any;
struct Gfx_Material;
struct Gpu_Mesh;
struct Gpu_Target;
struct Gpu_Texture;
struct Font_Image;

struct Batch {
	uint32_t offset, length;
	struct Gfx_Material * material;
	struct Gpu_Texture * texture;
	struct Blend_Mode blend_mode;
	struct Depth_Mode depth_mode;
	struct mat4 camera, transform;
};

struct Batcher {
	struct Batch pass;
	struct Array_Any * passes;
	struct Batch_Mesh_2D * buffer;
	struct Gpu_Mesh * gpu_mesh;
};

void batcher_init(struct Batcher * batcher);
void batcher_free(struct Batcher * batcher);

void batcher_set_camera(struct Batcher * batcher, struct mat4 camera);
void batcher_set_transform(struct Batcher * batcher, struct mat4 transform);
void batcher_set_blend_mode(struct Batcher * batcher, struct Blend_Mode blend_mode);
void batcher_set_depth_mode(struct Batcher * batcher, struct Depth_Mode depth_mode);
void batcher_set_material(struct Batcher * batcher, struct Gfx_Material * material);
void batcher_set_texture(struct Batcher * batcher, struct Gpu_Texture * texture);

void batcher_add_quad(
	struct Batcher * batcher,
	float const * rect, float const * uv
);

void batcher_add_text(struct Batcher * batcher, struct Font_Image * font, uint64_t length, uint8_t const * data, float x, float y);

void batcher_draw(struct Batcher * batcher, uint32_t size_x, uint32_t size_y, struct Gpu_Target * gpu_target);

#endif
