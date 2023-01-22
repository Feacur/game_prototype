#include "framework/systems/asset_system.h"
#include "framework/graphics/material.h"
#include "framework/graphics/gpu_misc.h"
#include "framework/maths.h"

#include "application/application.h"
#include "application/asset_types.h"
#include "application/batcher_2d.h"
#include "application/renderer.h"

#include "object_camera.h"
#include "game_state.h"

#include "ui.h"

static struct UI {
	struct Gfx_Material font_material;
	struct Asset_Handle font_asset_handle;
	struct mat4 mat4_ProjectionView;
	struct Transform_Rect transform_rect;
	struct vec2 pivot;
	struct rect rect;
} gs_ui;

void ui_init(void) {
	gs_ui.font_material = gfx_material_init(BLEND_MODE_MIX, DEPTH_MODE_NONE);
	gs_ui.transform_rect = c_transform_rect_default;
}

void ui_free(void) {
	gfx_material_free(&gs_ui.font_material);
	common_memset(&gs_ui, 0, sizeof(gs_ui));
}

void ui_start_frame(void) {
	struct uvec2 const screen_size = application_get_screen_size();
	gs_ui.mat4_ProjectionView = camera_get_projection(
		&(struct Camera_Params){
			.mode = CAMERA_MODE_SCREEN,
			.ncp = 0, .fcp = 1, .ortho = 1,
		},
		screen_size.x, screen_size.y
	);
}

void ui_end_frame(void) {
	batcher_2d_issue_commands(gs_renderer.batcher_2d, &gs_renderer.gpu_commands);
}

void ui_set_transform(struct Transform_Rect transform_rect) {
	struct uvec2 const screen_size = application_get_screen_size();
	transform_rect_get_pivot_and_rect(
		&transform_rect,
		screen_size.x, screen_size.y,
		&gs_ui.pivot, &gs_ui.rect
	);

	struct mat4 matrix = gs_ui.mat4_ProjectionView;
	matrix.w.x += gs_ui.pivot.x;
	matrix.w.y += gs_ui.pivot.y;
	batcher_2d_set_matrix(gs_renderer.batcher_2d, &matrix);
}

void ui_set_shader(struct CString name) {
	struct Asset_Shader const * asset_shader = asset_system_aquire_instance(&gs_game.assets, name);
	struct Handle const gpu_handle = asset_shader ? asset_shader->gpu_handle : (struct Handle){0};
	gfx_material_set_shader(&gs_ui.font_material, gpu_handle);

	//
	struct vec4 const color = {1, 1, 1, 1};
	uint32_t const p_Color = graphics_add_uniform_id(S_("p_Color"));
	gfx_uniforms_set(&gs_ui.font_material.uniforms, p_Color, A_(color));
}

void ui_set_font(struct CString name) {
	gs_ui.font_asset_handle = asset_system_aquire(&gs_game.assets, name);
	struct Asset_Font const * asset_font = asset_system_find_instance(&gs_game.assets, gs_ui.font_asset_handle);
	struct Handle const gpu_handle = asset_font ? asset_font->gpu_handle : (struct Handle){0};

	//
	uint32_t const p_Texture = graphics_add_uniform_id(S_("p_Texture"));
	gfx_uniforms_set(&gs_ui.font_material.uniforms, p_Texture, A_(gpu_handle));
}

void ui_text(struct CString value, struct vec2 alignment, bool wrap, float size) {
	struct Asset_Font const * asset_font = asset_system_find_instance(&gs_game.assets, gs_ui.font_asset_handle);
	if (!asset_font) { return; }

	batcher_2d_set_material(gs_renderer.batcher_2d, &gs_ui.font_material);
	batcher_2d_add_text(
		gs_renderer.batcher_2d,
		gs_ui.rect, alignment, wrap,
		asset_font, value, size
	);
}
