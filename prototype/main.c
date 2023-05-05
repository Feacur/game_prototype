#include "framework/memory.h"
#include "framework/unicode.h"
#include "framework/json_read.h"
#include "framework/platform_system.h"
#include "framework/platform_file.h"
#include "framework/logger.h"

#include "framework/maths.h"
#include "framework/input.h"

#include "framework/systems/uniform_system.h"
#include "framework/systems/material_system.h"
#include "framework/systems/asset_system.h"

#include "framework/graphics/material.h"
#include "framework/graphics/gpu_objects.h"
#include "framework/graphics/gpu_command.h"

#include "framework/containers/hash_table_any.h"

#include "framework/assets/mesh.h"
#include "framework/assets/image.h"
#include "framework/assets/typeface.h"
#include "framework/assets/json.h"

#include "application/json_load.h"
#include "application/application.h"
#include "application/asset_registry.h"
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
		if (!handle_is_null(camera->gpu_target_handle)) {
			gpu_target_get_size(camera->gpu_target_handle, &camera->cached_size.x, &camera->cached_size.y);
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

		struct Asset_Material const * material = asset_system_take(entity->material_asset_handle);
		struct uvec2 const content_size = entity_get_content_size(entity, material->handle, viewport_size.x, viewport_size.y);
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
	if (gs_main_settings.scene_id == 0) {
		logger_to_console("no scene to initialize with\n");
		return;
	}

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

	if (gs_game.cameras.count == 0) {
		array_any_push_many(&gs_renderer.gpu_commands, 2, (struct GPU_Command[]){
			{
				.type = GPU_COMMAND_TYPE_TARGET,
				.as.target = {
					.screen_size_x = screen_size.x,
					.screen_size_y = screen_size.y,
				},
			},
			{
				.type = GPU_COMMAND_TYPE_CLEAR,
				.as.clear = {
					.mask  = TEXTURE_TYPE_COLOR | TEXTURE_TYPE_DEPTH | TEXTURE_TYPE_STENCIL,
				},
			},
		});
		return;
	}

	uint32_t const gpu_commands_count_estimate = gs_game.cameras.count * 2 + gs_game.entities.count;
	array_any_ensure(&gs_renderer.gpu_commands, gpu_commands_count_estimate);

	struct Handle previous_gpu_target_handle = { // @note: deliberately wrong handle
		.id = INDEX_EMPTY, .gen = INDEX_EMPTY,
	};

	batcher_2d_set_color(gs_renderer.batcher_2d, (struct vec4){1, 1, 1, 1});

	for (uint32_t camera_i = 0; camera_i < gs_game.cameras.count; camera_i++) {
		struct Camera const * camera = array_any_at(&gs_game.cameras, camera_i);
		struct uvec2 const u_ViewportSize = camera->cached_size;

		struct mat4 const u_Projection = camera_get_projection(&camera->params, u_ViewportSize.x, u_ViewportSize.y);
		struct mat4 const u_View = mat4_set_inverse_transformation(camera->transform.position, camera->transform.scale, camera->transform.rotation);
		struct mat4 const u_ProjectionView = mat4_mul_mat(u_Projection, u_View);

		// process camera
		{
			uint32_t const override_offset = gs_renderer.uniforms.headers.count;
			gfx_uniforms_push(&gs_renderer.uniforms, S_("u_ProjectionView"), A_(u_ProjectionView));
			gfx_uniforms_push(&gs_renderer.uniforms, S_("u_Projection"),     A_(u_Projection));
			gfx_uniforms_push(&gs_renderer.uniforms, S_("u_View"),           A_(u_View));
			gfx_uniforms_push(&gs_renderer.uniforms, S_("u_ViewportSize"),   A_(u_ViewportSize));

			array_any_push_many(&gs_renderer.gpu_commands, 1, &(struct GPU_Command){
				.type = GPU_COMMAND_TYPE_UNIFORM,
				.as.uniform = {
					.uniforms = &gs_renderer.uniforms,
					.offset = override_offset,
					.count = (gs_renderer.uniforms.headers.count - override_offset),
				},
			});
		}

		if (!handle_equals(previous_gpu_target_handle, camera->gpu_target_handle)) {
			previous_gpu_target_handle = camera->gpu_target_handle;
			batcher_2d_issue_commands(gs_renderer.batcher_2d, &gs_renderer.gpu_commands);
			array_any_push_many(&gs_renderer.gpu_commands, 1, &(struct GPU_Command){
				.type = GPU_COMMAND_TYPE_TARGET,
				.as.target = {
					.screen_size_x = screen_size.x,
					.screen_size_y = screen_size.y,
					.handle = camera->gpu_target_handle,
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

			struct Asset_Material const * material_asset = asset_system_take(entity->material_asset_handle);
			struct Gfx_Material const * material = material_system_take(material_asset->handle);

			struct mat4 const u_Model = mat4_set_transformation(
				entity->transform.position,
				entity->transform.scale,
				entity->transform.rotation
			);

			switch (entity->type) {
				case ENTITY_TYPE_NONE: break;
				case ENTITY_TYPE_MESH: break;

				case ENTITY_TYPE_QUAD_2D:
				case ENTITY_TYPE_TEXT_2D: {
					struct mat4 const matrix = mat4_mul_mat(u_ProjectionView, u_Model);
					batcher_2d_set_matrix(gs_renderer.batcher_2d, matrix);
					batcher_2d_set_material(gs_renderer.batcher_2d, material_asset->handle);
				} break;
			}

			switch (entity->type) {
				case ENTITY_TYPE_NONE: break;

				case ENTITY_TYPE_MESH: {
					batcher_2d_issue_commands(gs_renderer.batcher_2d, &gs_renderer.gpu_commands);

					struct Entity_Mesh const * mesh = &entity->as.mesh;
					struct Asset_Model const * model = asset_system_take(mesh->asset_handle);

					uint32_t const override_offset = gs_renderer.uniforms.headers.count;
					gfx_uniforms_push(&gs_renderer.uniforms, S_("u_Model"), A_(u_Model));

					array_any_push_many(&gs_renderer.gpu_commands, 3, (struct GPU_Command[]){
						(struct GPU_Command){
							.type = GPU_COMMAND_TYPE_MATERIAL,
							.as.material = {
								.handle = material_asset->handle,
							},
						},
						(struct GPU_Command){
							.type = GPU_COMMAND_TYPE_UNIFORM,
							.as.uniform = {
								.program_handle = material->gpu_program_handle,
								.uniforms = &gs_renderer.uniforms,
								.offset = override_offset,
								.count = (gs_renderer.uniforms.headers.count - override_offset),
							},
						},
						(struct GPU_Command){
							.type = GPU_COMMAND_TYPE_DRAW,
							.as.draw = {
								.mesh_handle = model->gpu_handle,
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
					struct Asset_Bytes const * message = asset_system_take(text->message_asset_handle);
					struct CString const value = {
						.length = message->length,
						.data = (char const *)message->data,
					};
					batcher_2d_add_text(
						gs_renderer.batcher_2d,
						entity->cached_rect, text->alignment, true,
						text->font_asset_handle, value, text->size
					);
				} break;
			}
		}
	}
	batcher_2d_issue_commands(gs_renderer.batcher_2d, &gs_renderer.gpu_commands);
}

static struct CString prototype_get_fps_cstring(void) {
	static char buffer[32];

	double const delta_time = application_get_delta_time();
	uint32_t const fps = (uint32_t)r64_floor(1.0 / delta_time);

	uint32_t const length = logger_to_buffer(sizeof(buffer), buffer, "FPS: %03d (%.5f ms)", fps, delta_time);
	return (struct CString){.length = length, .data = buffer};
}

static void prototype_draw_ui(void) {
	ui_start_frame();

	ui_set_transform((struct Transform_Rect){
		.anchor_min = (struct vec2){1, 1},
		.anchor_max = (struct vec2){1, 1},
		.extents = (struct vec2){200, 100},
		.pivot = (struct vec2){1, 1},
	});
	ui_set_color((struct vec4){0.4f, 0.4f, 0.4f, 1});
	ui_quad((struct rect){.max = {1, 1}});

	ui_set_transform((struct Transform_Rect){.anchor_max = (struct vec2){1, 1}});
	ui_set_color((struct vec4){0.8f, 0.8f, 0.8f, 1});
	ui_text(prototype_get_fps_cstring(), (struct vec2){1, 1}, false, 16);

	ui_end_frame();
}

// ----- ----- ----- ----- -----
//     app callbacks part
// ----- ----- ----- ----- -----

static bool gs_main_exit;

static void app_init(void) {
	material_system_init();
	asset_system_init();
	asset_types_init();
	renderer_init();
	game_init();

	ui_init();
	ui_set_shader(S_("assets/shaders/batcher_2d.glsl"));
	ui_set_image(S_("assets/images/ui.image"));
	ui_set_font(S_("assets/test.font"));

	prototype_init();
}

static void app_free(void) {
	prototype_free();

	ui_free();

	game_free();
	renderer_free();
	asset_types_free();
	asset_system_free();
	material_system_free();
}

static void app_fixed_tick(void) {
}

static void app_frame_tick(void) {
	if (input_key(KC_ALT) && input_key(KC_F4)) {
		gs_main_exit = !input_key(KC_SHIFT);
		application_exit();
	}

	struct uvec2 const screen_size = application_get_screen_size();
	prototype_tick_entities();
	if (screen_size.x > 0 && screen_size.y > 0) {
		renderer_start_frame();
		prototype_draw_objects();
		prototype_draw_ui();
		renderer_end_frame();
	}
}

static void app_platform_quit(void) {
	gs_main_exit = true;
	application_exit();
}

static bool app_window_close(void) {
	gs_main_exit = !input_key(KC_SHIFT);
	return true;
}

// ----- ----- ----- ----- -----
//     main part
// ----- ----- ----- ----- -----

static void main_fill_settings(struct JSON const * json, void * data) {
	if (json->type == JSON_ERROR) { DEBUG_BREAK(); return; }
	if (data != &gs_main_settings) { DEBUG_BREAK(); return; }
	struct Main_Settings * result = data;
	*result = (struct Main_Settings){
		.strings = strings_init(),
	};
	result->config_id = strings_add(&result->strings, json_get_string(json, S_("config")));
	result->scene_id = strings_add(&result->strings, json_get_string(json, S_("scene")));
}

static void main_fill_config(struct JSON const * json, void * data) {
	if (json->type == JSON_ERROR) { DEBUG_BREAK(); return; }
	struct Application_Config * result = data;
	*result = (struct Application_Config){
		.size = {
			.x = (uint32_t)json_get_number(json, S_("size_x")),
			.y = (uint32_t)json_get_number(json, S_("size_y")),
		},
		.resizable = json_get_boolean(json, S_("resizable")),
		.vsync               = (int32_t)json_get_number(json, S_("vsync")),
		.target_refresh_rate = (uint32_t)json_get_number(json, S_("target_refresh_rate")),
		.fixed_refresh_rate  = (uint32_t)json_get_number(json, S_("fixed_refresh_rate")),
		.slow_frames_limit   = (uint32_t)json_get_number(json, S_("slow_frames_limit")),
	};
}

static void main_run_application(void) {
	process_json(S_("assets/main.json"), &gs_main_settings, main_fill_settings);
	if (gs_main_settings.config_id == 0) { goto fail; }

	struct Application_Config config = {0};
	struct CString const config_path = strings_get(&gs_main_settings.strings, gs_main_settings.config_id);
	process_json(config_path, &config, main_fill_config);

	logger_to_console("launched application\n");
	application_run(config, (struct Application_Callbacks){
		.init = app_init,
		.free = app_free,
		.fixed_tick = app_fixed_tick,
		.frame_tick = app_frame_tick,
		.window_callbacks = {
			.close = app_window_close,
		},
	});
	logger_to_console("application has ended\n");

	finalize:
	strings_free(&gs_main_settings.strings);
	common_memset(&gs_main_settings, 0, sizeof(gs_main_settings));
	return;

	// process errors
	fail: logger_to_console("failed to launch application\n");
	platform_system_sleep(1000);
	goto finalize;
}

int main (int argc, char * argv[]) {
	logger_to_console("> main arguments:\n");
	for (int i = 0; i < argc; i++) {
		logger_to_console("  %s\n", argv[i]);
	}

	platform_system_init((struct Platform_Callbacks){
		.quit = app_platform_quit,
	});

	while (!gs_main_exit && !platform_system_is_error()) {
		main_run_application();
	}

	platform_system_free();
	return 0;
}
