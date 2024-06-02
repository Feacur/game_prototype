#include "framework/formatter.h"
#include "framework/maths.h"
#include "framework/parsing.h"

#include "framework/graphics/gfx_objects.h"
#include "framework/graphics/command.h"

#include "application/json_load.h"
#include "application/asset_types.h"
#include "application/app_components.h"
#include "application/batcher_2d.h"


//
#include "renderer.h"

struct Renderer gs_renderer;

void renderer_init(void) {
	gs_renderer = (struct Renderer){
		.batcher_2d = batcher_2d_init(),
		.global = buffer_init(),
		.gh_global = gpu_buffer_init(&(struct Buffer){0}),
		.camera = buffer_init(),
		.gh_camera = gpu_buffer_init(&(struct Buffer){0}),
		.uniforms = gfx_uniforms_init(),
		.gpu_commands = array_init(sizeof(struct GPU_Command)),
	};
}

void renderer_free(void) {
	batcher_2d_free(gs_renderer.batcher_2d);
	buffer_free(&gs_renderer.global);
	gpu_buffer_free(gs_renderer.gh_global);
	buffer_free(&gs_renderer.camera);
	gpu_buffer_free(gs_renderer.gh_camera);
	gfx_uniforms_free(&gs_renderer.uniforms);
	array_free(&gs_renderer.gpu_commands);
	cbuffer_clear(CBM_(gs_renderer));
}

void renderer_start_frame(void) {
	batcher_2d_clear(gs_renderer.batcher_2d);
	buffer_clear(&gs_renderer.global);
	buffer_clear(&gs_renderer.camera);
	gfx_uniforms_clear(&gs_renderer.uniforms);
	array_clear(&gs_renderer.gpu_commands);
}

void renderer_end_frame(void) {
	batcher_2d_bake(gs_renderer.batcher_2d);
	gpu_buffer_update(gs_renderer.gh_global, &gs_renderer.global);
	gpu_buffer_update(gs_renderer.gh_camera, &gs_renderer.camera);
	gpu_execute(gs_renderer.gpu_commands.count, gs_renderer.gpu_commands.data);
}
