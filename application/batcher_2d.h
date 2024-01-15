#if !defined(APPLICATION_BATCHER_2D)
#define APPLICATION_BATCHER_2D

#include "framework/maths_types.h"
#include "framework/graphics/gfx_types.h"

struct Batcher_2D;
struct Array;

struct Batcher_2D * batcher_2d_init(void);
void batcher_2d_free(struct Batcher_2D * batcher);

void batcher_2d_set_color(struct Batcher_2D * batcher, struct vec4 const value);
void batcher_2d_set_matrix(struct Batcher_2D * batcher, struct mat4 const value);
void batcher_2d_set_material(struct Batcher_2D * batcher, struct Handle handle);
void batcher_2d_set_shader(
	struct Batcher_2D * batcher,
	struct Handle gh_program,
	enum Blend_Mode blend_mode, enum Depth_Mode depth_mode
);

void batcher_2d_uniforms_push(struct Batcher_2D * batcher, struct CString name, struct CBuffer value);

void batcher_2d_add_quad(
	struct Batcher_2D * batcher,
	struct rect rect,
	struct rect uv,
	uint32_t flags
);

void batcher_2d_add_text(
	struct Batcher_2D * batcher,
	struct rect rect, struct vec2 alignment, bool wrap,
	struct Handle gh_font, struct CString value, float size
);

void batcher_2d_clear(struct Batcher_2D * batcher);
void batcher_2d_issue_commands(struct Batcher_2D * batcher, struct Array * commands);
void batcher_2d_bake(struct Batcher_2D * batcher);

#endif
