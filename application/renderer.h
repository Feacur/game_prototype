#if !defined(APPLICATION_RENDERER)
#define APPLICATION_RENDERER

#include "framework/containers/array.h"
#include "framework/graphics/material.h"

struct Batcher_2D;

extern struct Renderer {
	struct Batcher_2D * batcher_2d;
	struct Gfx_Uniforms uniforms;
	struct Array gpu_commands; // `struct GPU_Command`
} gs_renderer;

//

void renderer_init(void);
void renderer_free(void);

void renderer_start_frame(void);
void renderer_end_frame(void);

#endif
