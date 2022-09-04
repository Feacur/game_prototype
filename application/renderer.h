#if !defined(APPLICATION_RENDERER)
#define APPLICATION_RENDERER

#include "framework/containers/array_any.h"
#include "framework/graphics/material.h"

struct Batcher_2D;

extern struct Renderer {
	struct Batcher_2D * batcher;
	struct Gfx_Uniforms uniforms;
	struct Array_Any gpu_commands; // `struct GPU_Command`
} gs_renderer;

//

void renderer_init(void);
void renderer_free(void);

void renderer_update(void);

#endif
