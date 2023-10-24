#include "framework/formatter.h"
#include "framework/maths.h"
#include "framework/parsing.h"

#include "framework/containers/strings.h"

#include "framework/graphics/material.h"
#include "framework/graphics/objects.h"
#include "framework/graphics/command.h"

#include "application/json_load.h"
#include "application/asset_types.h"
#include "application/asset_registry.h"
#include "application/components.h"
#include "application/batcher_2d.h"

//
#include "renderer.h"

struct Renderer gs_renderer;

void renderer_init(void) {
	gs_renderer = (struct Renderer){
		.batcher_2d = batcher_2d_init(),
		.uniforms = gfx_uniforms_init(),
		.gpu_commands = array_init(sizeof(struct GPU_Command)),
	};
}

void renderer_free(void) {
	batcher_2d_free(gs_renderer.batcher_2d);
	gfx_uniforms_free(&gs_renderer.uniforms);
	array_free(&gs_renderer.gpu_commands);
	common_memset(&gs_renderer, 0, sizeof(gs_renderer));
}

void renderer_start_frame(void) {
	batcher_2d_clear(gs_renderer.batcher_2d);
	gfx_uniforms_clear(&gs_renderer.uniforms);
	array_clear(&gs_renderer.gpu_commands);
}

void renderer_end_frame(void) {
	batcher_2d_bake(gs_renderer.batcher_2d);
	gpu_execute(gs_renderer.gpu_commands.count, gs_renderer.gpu_commands.data);
}
