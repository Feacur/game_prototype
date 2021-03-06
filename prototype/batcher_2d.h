#if !defined(GAME_PROTOTYPE_BATCHER)
#define GAME_PROTOTYPE_BATCHER

#include "framework/vector_types.h"
#include "framework/containers/ref.h"
#include "framework/graphics/types.h"

struct Batcher_2D;

struct Gfx_Material;
struct Font_Image;

struct Batcher_2D * batcher_2d_init(void);
void batcher_2d_free(struct Batcher_2D * batcher);

void batcher_2d_push_matrix(struct Batcher_2D * batcher, struct mat4 matrix);
void batcher_2d_pop_matrix(struct Batcher_2D * batcher);

void batcher_2d_set_blend_mode(struct Batcher_2D * batcher, struct Blend_Mode blend_mode);
void batcher_2d_set_depth_mode(struct Batcher_2D * batcher, struct Depth_Mode depth_mode);
void batcher_2d_set_material(struct Batcher_2D * batcher, struct Gfx_Material * material);
void batcher_2d_set_texture(struct Batcher_2D * batcher, struct Ref gpu_texture_ref);

void batcher_2d_add_quad(
	struct Batcher_2D * batcher,
	float const * rect, float const * uv
);

void batcher_2d_add_text(struct Batcher_2D * batcher, struct Font_Image * font, uint32_t length, uint8_t const * data, float x, float y);

void batcher_2d_draw(struct Batcher_2D * batcher, uint32_t size_x, uint32_t size_y, struct Ref gpu_target_ref);

#endif
