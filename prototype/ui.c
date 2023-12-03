#include "framework/systems/asset_system.h"
#include "framework/systems/material_system.h"

#include "framework/graphics/material.h"
#include "framework/graphics/command.h"

#include "framework/maths.h"

#include "application/application.h"
#include "application/asset_types.h"
#include "application/batcher_2d.h"
#include "application/renderer.h"

#include "object_camera.h"
#include "game_state.h"

#include "ui.h"

static struct UI {
	struct Handle ah_shader;
	struct Handle ah_image;
	struct Handle ah_font;
	//
	struct mat4 camera;
	struct vec2 pivot;
	struct rect rect;
} gs_ui;

void ui_init(void) {
	gs_ui = (struct UI){0};
}

void ui_free(void) {
	asset_system_drop(gs_ui.ah_shader);
	asset_system_drop(gs_ui.ah_image);
	asset_system_drop(gs_ui.ah_font);
	common_memset(&gs_ui, 0, sizeof(gs_ui));
}

static void ui_internal_push_shader(void) {
	struct Asset_Shader const * shader = asset_system_get(gs_ui.ah_shader);
	batcher_2d_set_shader(
		gs_renderer.batcher_2d,
		shader->gpu_handle,
		BLEND_MODE_MIX, DEPTH_MODE_NONE
	);
}

static void ui_internal_push_image(void) {
	struct Asset_Image const * asset = asset_system_get(gs_ui.ah_image);
	struct Handle const gpu_handle = asset ? asset->gpu_handle : (struct Handle){0};
	batcher_2d_uniforms_push(gs_renderer.batcher_2d, S_("p_Texture"), A_(gpu_handle));
}

static void ui_internal_push_font(void) {
	struct Asset_Font const * asset = asset_system_get(gs_ui.ah_font);
	struct Handle const gpu_handle = asset ? asset->gpu_handle : (struct Handle){0};
	batcher_2d_uniforms_push(gs_renderer.batcher_2d, S_("p_Texture"), A_(gpu_handle));
}

static void ui_internal_push_tint(void) {
	struct vec4 const vec4_Tint = {1, 1, 1, 1};
	batcher_2d_uniforms_push(gs_renderer.batcher_2d, S_("p_Tint"), A_(vec4_Tint));
}

void ui_start_frame(void) {
	gs_ui.camera = camera_get_projection(
		&(struct Camera_Params){
			.mode = CAMERA_MODE_SCREEN,
			.ncp = 0, .fcp = 1, .ortho = 1,
		},
		application_get_screen_size()
	);
}

void ui_end_frame(void) {
	batcher_2d_issue_commands(gs_renderer.batcher_2d, &gs_renderer.gpu_commands);
}

void ui_set_transform(struct Transform_Rect transform_rect) {
	struct uvec2 const screen_size = application_get_screen_size();
	transform_rect_get_pivot_and_rect(
		&transform_rect, screen_size,
		&gs_ui.pivot, &gs_ui.rect
	);

	struct mat4 matrix = gs_ui.camera;
	matrix.w.x += matrix.x.x * gs_ui.pivot.x + matrix.y.x * gs_ui.pivot.y;
	matrix.w.y += matrix.x.y * gs_ui.pivot.x + matrix.y.y * gs_ui.pivot.y;
	matrix.w.z += matrix.x.z * gs_ui.pivot.x + matrix.y.z * gs_ui.pivot.y;
	matrix.w.w += matrix.x.w * gs_ui.pivot.x + matrix.y.w * gs_ui.pivot.y;
	batcher_2d_set_matrix(gs_renderer.batcher_2d, matrix);

/*
	matrix = mat4_mul_mat(gs_ui.camera, translation matrix by gs_ui.pivot);
	// or
	matrix.w = mat4_mul_vec(gs_ui.camera, gs_ui.pivot as vec4);
*/
}

void ui_set_color(struct vec4 color) {
	batcher_2d_set_color(gs_renderer.batcher_2d, color);
}

void ui_set_shader(struct CString name) {
	struct Handle const prev = gs_ui.ah_shader;
	gs_ui.ah_shader = asset_system_load(name);
	asset_system_drop(prev);
}

void ui_set_image(struct CString name) {
	struct Handle const prev = gs_ui.ah_image;
	gs_ui.ah_image = asset_system_load(name);
	asset_system_drop(prev);
}

void ui_set_font(struct CString name) {
	struct Handle const prev = gs_ui.ah_font;
	gs_ui.ah_font = asset_system_load(name);
	asset_system_drop(prev);
}

void ui_quad(struct rect uv) {
	ui_internal_push_shader();
	ui_internal_push_image();
	ui_internal_push_tint();
	batcher_2d_add_quad(gs_renderer.batcher_2d, gs_ui.rect, uv);
}

void ui_text(struct CString value, struct vec2 alignment, bool wrap, float size) {
	ui_internal_push_shader();
	ui_internal_push_font();
	ui_internal_push_tint();
	batcher_2d_add_text(
		gs_renderer.batcher_2d,
		gs_ui.rect, alignment, wrap,
		gs_ui.ah_font, value, size
	);
}
