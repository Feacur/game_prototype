#if !defined(APPLICATION_BATCHER_2D)
#define APPLICATION_BATCHER_2D

#include "framework/vector_types.h"
#include "framework/graphics/types.h"

// @todo: rename to `Draw_Commands` or something, as current name is quite misleading;
//        probably that's caused by multiple points of responsibility:
//        - handling a mesh
//        - storing, optionally, atomic commands (text only, currently)
struct Batcher_2D;

struct Gfx_Material;
struct Asset_Font;
struct Array_Any;

struct Batcher_2D * batcher_2d_init(void);
void batcher_2d_free(struct Batcher_2D * batcher);

void batcher_2d_set_color(struct Batcher_2D * batcher, struct vec4 const value);
void batcher_2d_set_matrix(struct Batcher_2D * batcher, struct mat4 const value);
void batcher_2d_set_material(struct Batcher_2D * batcher, struct Gfx_Material const * material);

void batcher_2d_uniforms_push(struct Batcher_2D * batcher, uint32_t uniform_id, struct CArray value);

void batcher_2d_add_quad(
	struct Batcher_2D * batcher,
	struct rect rect,
	struct rect uv
);

void batcher_2d_add_text(
	struct Batcher_2D * batcher,
	struct rect rect, struct vec2 alignment, bool wrap,
	struct Asset_Font const * font, struct CString value, float size
);

void batcher_2d_clear(struct Batcher_2D * batcher);
void batcher_2d_issue_commands(struct Batcher_2D * batcher, struct Array_Any * commands);
void batcher_2d_bake(struct Batcher_2D * batcher);

#endif
