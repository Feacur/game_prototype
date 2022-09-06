#include "framework/systems/asset_system.h"
#include "framework/graphics/material.h"
#include "framework/graphics/gpu_misc.h"

#include "application/application.h"
#include "application/asset_types.h"
#include "application/batcher_2d.h"
#include "application/renderer.h"

#include "object_camera.h"
#include "game_state.h"

#include "ui.h"

static struct UI {
	struct Asset_Ref shader_asset_ref;
	struct Gfx_Material font_material;
	//
	struct Asset_Ref font_asset_ref;
} gs_ui;

void ui_init(struct CString shader_name) {
	gs_ui.shader_asset_ref = asset_system_aquire(&gs_game.assets, shader_name);
	struct Asset_Shader const * asset_shader = asset_system_find_instance(&gs_game.assets, gs_ui.shader_asset_ref);

	//
	gs_ui.font_material = gfx_material_init(asset_shader->gpu_ref, BLEND_MODE_MIX, DEPTH_MODE_NONE);

	//
	struct vec4 const p_Color_value = {1, 1, 1, 1};
	uint32_t const p_Color = graphics_add_uniform_id(S_("p_Color"));
	gfx_uniforms_set(&gs_ui.font_material.uniforms, p_Color, (struct Gfx_Uniform_In){
		.size = sizeof(p_Color_value),
		.data = &p_Color_value.x,
	});
}

void ui_free(void) {
	gfx_material_free(&gs_ui.font_material);
	common_memset(&gs_ui, 0, sizeof(gs_ui));
}

void ui_set_font(struct CString name) {
	gs_ui.font_asset_ref = asset_system_aquire(&gs_game.assets, name);
	struct Asset_Font const * asset_font = asset_system_find_instance(&gs_game.assets, gs_ui.font_asset_ref);

	//
	uint32_t const p_Texture = graphics_add_uniform_id(S_("p_Texture"));
	gfx_uniforms_set(&gs_ui.font_material.uniforms, p_Texture, (struct Gfx_Uniform_In){
		.size = sizeof(asset_font->gpu_ref),
		.data = &asset_font->gpu_ref,
	});
}

void ui_start_frame(void) {
	struct uvec2 const screen_size = application_get_screen_size();
	struct mat4 mat4_Projection = camera_get_projection(
		&(struct Camera_Params){
			.mode = CAMERA_MODE_SCREEN,
			.ncp = 0, .fcp = 1, .ortho = 1,
		},
		screen_size.x, screen_size.y
	);
	batcher_2d_set_matrix(gs_renderer.batcher, &mat4_Projection);
}

void ui_end_frame(void) {
	batcher_2d_issue_commands(gs_renderer.batcher, &gs_renderer.gpu_commands);
}

void ui_text(struct rect rect, struct CString value, struct vec2 alignment, bool wrap, float size) {
	struct Asset_Font const * asset_font = asset_system_find_instance(&gs_game.assets, gs_ui.font_asset_ref);
	batcher_2d_set_material(gs_renderer.batcher, &gs_ui.font_material);
	batcher_2d_add_text(
		gs_renderer.batcher,
		rect, alignment, wrap,
		asset_font, value, size
	);
}
