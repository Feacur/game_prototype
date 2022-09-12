#include "framework/memory.h"
#include "framework/unicode.h"
#include "framework/json_read.h"
#include "framework/platform_system.h"
#include "framework/platform_file.h"
#include "framework/logger.h"

#include "framework/maths.h"
#include "framework/input.h"

#include "framework/graphics/material.h"
#include "framework/graphics/material_override.h"
#include "framework/graphics/gpu_objects.h"
#include "framework/graphics/gpu_misc.h"
#include "framework/graphics/gpu_command.h"

#include "framework/containers/hash_table_any.h"

#include "framework/assets/mesh.h"
#include "framework/assets/image.h"
#include "framework/assets/font.h"
#include "framework/assets/json.h"

#include "application/json_load.h"
#include "application/application.h"
#include "application/asset_types.h"
#include "application/components.h"
#include "application/batcher_2d.h"
#include "application/renderer.h"

#include "object_camera.h"
#include "object_entity.h"
#include "game_state.h"
#include "components.h"
#include "ui.h"

static struct Main_Settings {
	struct Strings strings;
	uint32_t config_id;
	uint32_t scene_id;
} gs_main_settings;

static void prototype_tick_cameras(void) {
	struct uvec2 const screen_size = application_get_screen_size();
	for (uint32_t camera_i = 0; camera_i < gs_game.cameras.count; camera_i++) {
		struct Camera * camera = array_any_at(&gs_game.cameras, camera_i);
		camera->cached_size = screen_size;
		if (!ref_equals(camera->gpu_target_ref, (struct Ref){0})) {
			gpu_target_get_size(camera->gpu_target_ref, &camera->cached_size.x, &camera->cached_size.y);
		}
	}
}

static void prototype_tick_entities_rect(void) {
	for (uint32_t entity_i = 0; entity_i < gs_game.entities.count; entity_i++) {
		struct Entity * entity = array_any_at(&gs_game.entities, entity_i);

		if (entity->type == ENTITY_TYPE_NONE) { continue; }
		if (entity->type == ENTITY_TYPE_MESH) { continue; }

		struct Camera const * camera = array_any_at(&gs_game.cameras, entity->camera);
		struct uvec2 const viewport_size = camera->cached_size;

		struct vec2 entity_pivot;
		transform_rect_get_pivot_and_rect(
			&entity->rect,
			viewport_size.x, viewport_size.y,
			&entity_pivot, &entity->cached_rect
		);
		entity->transform.position.x = entity_pivot.x;
		entity->transform.position.y = entity_pivot.y;
	}
}

static void prototype_tick_entities_rotation_mode(void) {
	float const delta_time = (float)application_get_delta_time();
	for (uint32_t entity_i = 0; entity_i < gs_game.entities.count; entity_i++) {
		struct Entity * entity = array_any_at(&gs_game.entities, entity_i);

		switch (entity->rotation_mode) {
			case ENTITY_ROTATION_MODE_NONE: break;

			case ENTITY_ROTATION_MODE_X: {
				entity->transform.rotation = vec4_norm(quat_mul(entity->transform.rotation, quat_set_radians(
					(struct vec3){1 * delta_time, 0, 0}
				)));
			} break;

			case ENTITY_ROTATION_MODE_Y: {
				entity->transform.rotation = vec4_norm(quat_mul(entity->transform.rotation, quat_set_radians(
					(struct vec3){0, 1 * delta_time, 0}
				)));
			} break;

			case ENTITY_ROTATION_MODE_Z: {
				entity->transform.rotation = vec4_norm(quat_mul(entity->transform.rotation, quat_set_radians(
					(struct vec3){0, 0, 1 * delta_time}
				)));
			} break;
		}
	}
}

static void prototype_tick_entities_quad_2d(void) {
	for (uint32_t entity_i = 0; entity_i < gs_game.entities.count; entity_i++) {
		struct Entity * entity = array_any_at(&gs_game.entities, entity_i);

		if (entity->type != ENTITY_TYPE_QUAD_2D) { continue; }
		struct Entity_Quad * quad = &entity->as.quad;
		if (quad->mode == ENTITY_QUAD_MODE_NONE) { continue; }

		struct Camera const * camera = array_any_at(&gs_game.cameras, entity->camera);
		struct uvec2 const viewport_size = camera->cached_size;

		struct Asset_Material const * material = asset_system_find_instance(&gs_game.assets, entity->material);
		struct uvec2 const content_size = entity_get_content_size(entity, &material->value, viewport_size.x, viewport_size.y);
		if (content_size.x == 0 || content_size.y == 0) { break; }

		switch (quad->mode) {
			case ENTITY_QUAD_MODE_NONE: break;

			case ENTITY_QUAD_MODE_FIT: {
				uint32_t const factor_x = content_size.x * viewport_size.y;
				uint32_t const factor_y = content_size.y * viewport_size.x;
				entity->rect = (struct Transform_Rect){
					.anchor_min = {0.5f, 0.5f},
					.anchor_max = {0.5f, 0.5f},
					.extents = {
						(float)((factor_x > factor_y) ? viewport_size.x : (factor_x / content_size.y)),
						(float)((factor_x > factor_y) ? (factor_y / content_size.x) : viewport_size.y),
					},
					.pivot = {0.5f, 0.5f},
				};
			} break;

			case ENTITY_QUAD_MODE_SIZE: {
				entity->rect.extents = (struct vec2){
					.x = (float)content_size.x,
					.y = (float)content_size.y,
				};
			} break;
		}
	}
}

// ----- ----- ----- ----- -----
//     prototype part
// ----- ----- ----- ----- -----

static void prototype_init(void) {
	struct CString const scene_path = strings_get(&gs_main_settings.strings, gs_main_settings.scene_id);
	process_json(scene_path, &gs_game, game_fill_scene);
	gpu_execute(1, &(struct GPU_Command){
		.type = GPU_COMMAND_TYPE_CULL,
		.as.cull = {
			.mode = CULL_MODE_BACK,
			.order = WINDING_ORDER_POSITIVE,
		},
	});
}

static void prototype_free(void) {
}

static void prototype_tick_entities(void) {
	// if (input_mouse(MC_LEFT)) {
	// 	int32_t x, y;
	// 	input_mouse_delta(&x, &y);
	// 	logger_to_console("delta: %d %d\n", x, y);
	// }

	prototype_tick_cameras();
	prototype_tick_entities_rect();
	prototype_tick_entities_rotation_mode();
	prototype_tick_entities_quad_2d();
}

static void prototype_draw_objects(void) {
	struct uvec2 const screen_size = application_get_screen_size();

	uint32_t const u_ProjectionView = graphics_add_uniform_id(S_("u_ProjectionView"));
	uint32_t const u_Projection     = graphics_add_uniform_id(S_("u_Projection"));
	uint32_t const u_View           = graphics_add_uniform_id(S_("u_View"));
	uint32_t const u_ViewportSize   = graphics_add_uniform_id(S_("u_ViewportSize"));
	uint32_t const u_Model          = graphics_add_uniform_id(S_("u_Model"));

	uint32_t const gpu_commands_count_estimate = gs_game.cameras.count * 2 + gs_game.entities.count;
	array_any_ensure(&gs_renderer.gpu_commands, gpu_commands_count_estimate);

	struct Ref previous_gpu_target_ref = { // @note: deliberately wrong handle
		.id = INDEX_EMPTY, .gen = INDEX_EMPTY,
	};

	for (uint32_t camera_i = 0; camera_i < gs_game.cameras.count; camera_i++) {
		struct Camera const * camera = array_any_at(&gs_game.cameras, camera_i);
		struct uvec2 const viewport_size = camera->cached_size;

		struct mat4 const mat4_Projection = camera_get_projection(&camera->params, viewport_size.x, viewport_size.y);
		struct mat4 const mat4_View = mat4_set_inverse_transformation(camera->transform.position, camera->transform.scale, camera->transform.rotation);
		struct mat4 const mat4_ProjectionView = mat4_mul_mat(mat4_Projection, mat4_View);

		// process camera
		{
			uint32_t const override_offset = gs_renderer.uniforms.headers.count;
			gfx_uniforms_push(&gs_renderer.uniforms, u_ProjectionView, (struct Gfx_Uniform_In){
				.size = sizeof(mat4_ProjectionView),
				.data = &mat4_ProjectionView,
			});
			gfx_uniforms_push(&gs_renderer.uniforms, u_Projection, (struct Gfx_Uniform_In){
				.size = sizeof(mat4_Projection),
				.data = &mat4_Projection,
			});
			gfx_uniforms_push(&gs_renderer.uniforms, u_View, (struct Gfx_Uniform_In){
				.size = sizeof(mat4_View),
				.data = &mat4_View,
			});
			gfx_uniforms_push(&gs_renderer.uniforms, u_ViewportSize, (struct Gfx_Uniform_In){
				.size = sizeof(viewport_size),
				.data = &viewport_size,
			});

			array_any_push_many(&gs_renderer.gpu_commands, 1, &(struct GPU_Command){
				.type = GPU_COMMAND_TYPE_UNIFORM,
				.as.uniform = {
					.override = {
						.uniforms = &gs_renderer.uniforms,
						.offset = override_offset,
						.count = (gs_renderer.uniforms.headers.count - override_offset),
					},
				},
			});
		}

		if (!ref_equals(previous_gpu_target_ref, camera->gpu_target_ref)) {
			previous_gpu_target_ref = camera->gpu_target_ref;
			batcher_2d_issue_commands(gs_renderer.batcher_2d, &gs_renderer.gpu_commands);
			array_any_push_many(&gs_renderer.gpu_commands, 1, &(struct GPU_Command){
				.type = GPU_COMMAND_TYPE_TARGET,
				.as.target = {
					.screen_size_x = screen_size.x,
					.screen_size_y = screen_size.y,
					.gpu_ref = camera->gpu_target_ref,
				},
			});
		}

		if (camera->clear.mask != TEXTURE_TYPE_NONE) {
			batcher_2d_issue_commands(gs_renderer.batcher_2d, &gs_renderer.gpu_commands);
			array_any_push_many(&gs_renderer.gpu_commands, 1, &(struct GPU_Command){
				.type = GPU_COMMAND_TYPE_CLEAR,
				.as.clear = {
					.mask  = camera->clear.mask,
					.color = camera->clear.color,
				},
			});
		}

		// draw entities
		for (uint32_t entity_i = 0; entity_i < gs_game.entities.count; entity_i++) {
			struct Entity const * entity = array_any_at(&gs_game.entities, entity_i);
			if (entity->camera != camera_i) { continue; }

			struct Asset_Material const * material = asset_system_find_instance(&gs_game.assets, entity->material);

			struct mat4 const mat4_Model = mat4_set_transformation(
				entity->transform.position,
				entity->transform.scale,
				entity->transform.rotation
			);

			switch (entity->type) {
				case ENTITY_TYPE_NONE: break;
				case ENTITY_TYPE_MESH: break;

				case ENTITY_TYPE_QUAD_2D:
				case ENTITY_TYPE_TEXT_2D: {
					struct mat4 const matrix = mat4_mul_mat(mat4_ProjectionView, mat4_Model);
					batcher_2d_set_matrix(gs_renderer.batcher_2d, &matrix);
					batcher_2d_set_material(gs_renderer.batcher_2d, &material->value);
				} break;
			}

			switch (entity->type) {
				case ENTITY_TYPE_NONE: break;

				case ENTITY_TYPE_MESH: {
					batcher_2d_issue_commands(gs_renderer.batcher_2d, &gs_renderer.gpu_commands);

					struct Entity_Mesh const * mesh = &entity->as.mesh;
					struct Asset_Model const * model = asset_system_find_instance(&gs_game.assets, mesh->mesh);

					uint32_t const override_offset = gs_renderer.uniforms.headers.count;
					gfx_uniforms_push(&gs_renderer.uniforms, u_Model, (struct Gfx_Uniform_In){
						.size = sizeof(mat4_Model),
						.data = &mat4_Model,
					});

					array_any_push_many(&gs_renderer.gpu_commands, 3, (struct GPU_Command[]){
						(struct GPU_Command){
							.type = GPU_COMMAND_TYPE_MATERIAL,
							.as.material = {
								.material = &material->value,
							},
						},
						(struct GPU_Command){
							.type = GPU_COMMAND_TYPE_UNIFORM,
							.as.uniform = {
								.gpu_program_ref = material->value.gpu_program_ref,
								.override = {
									.uniforms = &gs_renderer.uniforms,
									.offset = override_offset,
									.count = (gs_renderer.uniforms.headers.count - override_offset),
								},
							},
						},
						(struct GPU_Command){
							.type = GPU_COMMAND_TYPE_DRAW,
							.as.draw = {
								.gpu_mesh_ref = model->gpu_ref,
							},
						},
					});
				} break;

				case ENTITY_TYPE_QUAD_2D: {
					struct Entity_Quad const * quad = &entity->as.quad;
					batcher_2d_add_quad(
						gs_renderer.batcher_2d,
						entity->cached_rect,
						quad->view
					);
				} break;

				case ENTITY_TYPE_TEXT_2D: {
					struct Entity_Text const * text = &entity->as.text;
					struct Asset_Font const * font = asset_system_find_instance(&gs_game.assets, text->font);
					struct Asset_Bytes const * message = asset_system_find_instance(&gs_game.assets, text->message);
					struct CString const value = {
						.length = message->length,
						.data = (char const *)message->data,
					};
					batcher_2d_add_text(
						gs_renderer.batcher_2d,
						entity->cached_rect, text->alignment, true,
						font, value, text->size
					);
				} break;
			}
		}
	}
	batcher_2d_issue_commands(gs_renderer.batcher_2d, &gs_renderer.gpu_commands);
}

static void prototype_draw_ui(void) {
	ui_start_frame();
	{
		double const delta_time = application_get_delta_time();
		uint32_t const fps = (uint32_t)r64_floor(1.0 / delta_time);

		char buffer[32];
		uint32_t const length = logger_to_buffer(sizeof(buffer), buffer, "FPS: %03d (%.5f ms)", fps, delta_time);
		struct CString const text = (struct CString){.length = length, .data = buffer};

		//
		struct uvec2 const screen_size = application_get_screen_size();
		struct rect const rect = {
			.max = {(float)screen_size.x, (float)screen_size.y}
		};
		ui_text(rect, text, (struct vec2){1, 1}, false, 16);
	}
	ui_end_frame();
}

// ----- ----- ----- ----- -----
//     app callbacks part
// ----- ----- ----- ----- -----

static void app_init(void) {
	renderer_init();
	game_init();

	ui_init(S_("assets/shaders/batcher_2d.glsl"));
	ui_set_font(S_("assets/fonts/Ubuntu-Regular.ttf"));

	prototype_init();
}

static void app_free(void) {
	prototype_free();

	ui_free();

	game_free();
	renderer_free();

	// @note: free strings here, because application checks for memory leaks right after this routine
	//        an alternative solution would be to split `application_run` into stages
	strings_free(&gs_main_settings.strings);
}

static void app_fixed_tick(void) {
}

static void app_frame_tick(void) {
	struct uvec2 const screen_size = application_get_screen_size();

	prototype_tick_entities();

	if (screen_size.x > 0 && screen_size.y > 0) {
		renderer_start_frame();
		prototype_draw_objects();
		prototype_draw_ui();
		renderer_end_frame();
	}
}

// ----- ----- ----- ----- -----
//     main part
// ----- ----- ----- ----- -----

static void main_fill_settings(struct JSON const * json, void * data) {
	if (json->type == JSON_ERROR) { DEBUG_BREAK(); return; }
	struct Main_Settings * result = data;
	*result = (struct Main_Settings){
		.strings = strings_init(),
	};
	result->config_id = strings_add(&result->strings, json_get_string(json, S_("config")));
	result->scene_id = strings_add(&result->strings, json_get_string(json, S_("scene")));
	// @note: `gs_main_settings.strings` will be freed in the `app_free` function
}

static void main_fill_config(struct JSON const * json, void * data) {
	if (json->type == JSON_ERROR) { DEBUG_BREAK(); return; }
	struct Application_Config * result = data;
	*result = (struct Application_Config){
		.size = {
			.x = (uint32_t)json_get_number(json, S_("size_x")),
			.y = (uint32_t)json_get_number(json, S_("size_y")),
		},
		.flexible = json_get_boolean(json, S_("flexible")),
		.vsync              = (int32_t)json_get_number(json, S_("vsync")),
		.frame_refresh_rate = (uint32_t)json_get_number(json, S_("frame_refresh_rate")),
		.fixed_refresh_rate = (uint32_t)json_get_number(json, S_("fixed_refresh_rate")),
		.slow_frames_limit  = (uint32_t)json_get_number(json, S_("slow_frames_limit")),
	};
}

int main (int argc, char * argv[]) {
	bool success = false;

	logger_to_console("Bonjour!\n\n");

	platform_system_init();

	logger_to_console("> Arguments:\n");
	for (int i = 0; i < argc; i++) {
		logger_to_console("  %s\n", argv[i]);
	}
	logger_to_console("\n");

	process_json(S_("assets/main.json"), &gs_main_settings, main_fill_settings);
	if (gs_main_settings.config_id == 0) { goto finalize; }

	struct Application_Config config = {0};
	struct CString const config_path = strings_get(&gs_main_settings.strings, gs_main_settings.config_id);
	process_json(config_path, &config, main_fill_config);
	if (config.fixed_refresh_rate == 0) { goto finalize; }

	success = true;
	application_run(config, (struct Application_Callbacks){
		.init = app_init,
		.free = app_free,
		.fixed_tick = app_fixed_tick,
		.frame_tick = app_frame_tick,
	});

	finalize: if (!success) { DEBUG_BREAK(); }
	platform_system_free();
	return 0;
}
