#include "framework/systems/asset_system.h"
#include "framework/graphics/material.h"
#include "framework/graphics/gpu_misc.h"
#include "framework/graphics/gpu_command.h"
#include "framework/systems/material_system.h"
#include "framework/maths.h"

#include "application/application.h"
#include "application/asset_types.h"
#include "application/batcher_2d.h"
#include "application/renderer.h"

#include "object_camera.h"
#include "game_state.h"

#include "ui.h"

static struct UI {
	struct Handle material;
	struct Asset_Handle font_asset_handle;
	struct Asset_Handle image_asset_handle;
	//
	struct mat4 camera;
	struct vec2 pivot;
	struct rect rect;
} gs_ui;

void ui_init(void) {
	gs_ui = (struct UI){
		.material = material_system_aquire(),
	};

	struct Gfx_Material * material = material_system_take(gs_ui.material);
	material->blend_mode = BLEND_MODE_MIX;
	material->depth_mode = DEPTH_MODE_NONE;
}

void ui_free(void) {
	material_system_discard(gs_ui.material);
	common_memset(&gs_ui, 0, sizeof(gs_ui));
}

static void ui_internal_push_image(void) {
	struct Asset_Image const * asset = asset_system_take(gs_ui.image_asset_handle);
	struct Handle const gpu_handle = asset ? asset->gpu_handle : (struct Handle){0};
	uint32_t const p_Texture = graphics_add_uniform_id(S_("p_Texture"));
	batcher_2d_uniforms_push(gs_renderer.batcher_2d, p_Texture, A_(gpu_handle));
}

static void ui_internal_push_font(void) {
	struct Asset_Font const * asset = asset_system_take(gs_ui.font_asset_handle);
	struct Handle const gpu_handle = asset ? asset->gpu_handle : (struct Handle){0};
	uint32_t const p_Texture = graphics_add_uniform_id(S_("p_Texture"));
	batcher_2d_uniforms_push(gs_renderer.batcher_2d, p_Texture, A_(gpu_handle));
}

void ui_start_frame(void) {
	batcher_2d_set_material(gs_renderer.batcher_2d, gs_ui.material);

	struct uvec2 const screen_size = application_get_screen_size();
	gs_ui.camera = camera_get_projection(
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
	struct Asset_Shader const * asset_shader = asset_system_aquire_instance(name);
	struct Handle const gpu_handle = asset_shader ? asset_shader->gpu_handle : (struct Handle){0};

	struct Gfx_Material * material = material_system_take(gs_ui.material);
	gfx_material_set_shader(material, gpu_handle);

	{
		struct vec4 const vec4_Tint = {1, 1, 1, 1};
		uint32_t const p_Tint = graphics_add_uniform_id(S_("p_Tint"));
		gfx_uniforms_set(&material->uniforms, p_Tint, A_(vec4_Tint));
	}

	{
		struct Handle const h_Texture = {0};
		uint32_t const p_Texture = graphics_add_uniform_id(S_("p_Texture"));
		gfx_uniforms_set(&material->uniforms, p_Texture, A_(h_Texture));
	}
}

void ui_set_image(struct CString name) {
	gs_ui.image_asset_handle = asset_system_aquire(name);
}

void ui_set_font(struct CString name) {
	gs_ui.font_asset_handle = asset_system_aquire(name);
}

void ui_quad(struct rect uv) {
	ui_internal_push_image();
	batcher_2d_add_quad(gs_renderer.batcher_2d, gs_ui.rect, uv);
}

void ui_text(struct CString value, struct vec2 alignment, bool wrap, float size) {
	ui_internal_push_font();
	batcher_2d_add_text(
		gs_renderer.batcher_2d,
		gs_ui.rect, alignment, wrap,
		gs_ui.font_asset_handle, value, size
	);
}
