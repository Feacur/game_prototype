#include "framework/memory.h"
#include "framework/unicode.h"
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

#include "application/application.h"
#include "application/asset_types.h"
#include "application/utilities.h"
#include "application/components.h"
#include "application/batcher_2d.h"
#include "application/renderer.h"

#include "object_camera.h"
#include "object_entity.h"
#include "game_state.h"
#include "components.h"

static struct Main_Settings {
	struct Strings strings;
	uint32_t config_id;
	uint32_t scene_id;
} gs_main_settings;

static void prototype_tick_entities_rotation_mode(void) {
	float const delta_time = application_get_delta_time();
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
	struct uvec2 const screen_size = application_get_screen_size();
	for (uint32_t entity_i = 0; entity_i < gs_game.entities.count; entity_i++) {
		struct Entity * entity = array_any_at(&gs_game.entities, entity_i);
		if (entity->type != ENTITY_TYPE_QUAD_2D) { continue; }
		struct Entity_Quad * quad = &entity->as.quad;
		if (quad->mode == ENTITY_QUAD_MODE_NONE) { continue; }

		// @todo: precalculate all cameras?
		struct Camera const * camera = array_any_at(&gs_game.cameras, entity->camera);
		struct uvec2 viewport_size = screen_size;
		if (!ref_equals(camera->gpu_target_ref, c_ref_empty)) {
			gpu_target_get_size(camera->gpu_target_ref, &viewport_size.x, &viewport_size.y);
		}

		//
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

static void prototype_tick_entities_text_2d(void) {
	for (uint32_t entity_i = 0; entity_i < gs_game.entities.count; entity_i++) {
		struct Entity * entity = array_any_at(&gs_game.entities, entity_i);
		if (entity->type != ENTITY_TYPE_TEXT_2D) { continue; }
		struct Entity_Text * text = &entity->as.text;

		struct Asset_Bytes const * text_text = asset_system_find_instance(&gs_game.assets, text->message);
		uint32_t const text_length = text_text->length;
		text->visible_length = (text->visible_length + 1) % text_length;
	}
}

// ----- ----- ----- ----- -----
//     prototype part
// ----- ----- ----- ----- -----

static void prototype_init(void) {
	process_json(strings_get(&gs_main_settings.strings, gs_main_settings.scene_id), &gs_game, game_fill_scene);
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

	prototype_tick_entities_rotation_mode();
	prototype_tick_entities_quad_2d();
	prototype_tick_entities_text_2d();
}

static void prototype_draw_entities(void) {
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

		// prepare camera
		struct uvec2 viewport_size = screen_size;
		if (!ref_equals(camera->gpu_target_ref, c_ref_empty)) {
			gpu_target_get_size(camera->gpu_target_ref, &viewport_size.x, &viewport_size.y);
		}

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
			batcher_2d_issue_commands(gs_renderer.batcher, &gs_renderer.gpu_commands);
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
			batcher_2d_issue_commands(gs_renderer.batcher, &gs_renderer.gpu_commands);
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
			struct Entity * entity = array_any_at(&gs_game.entities, entity_i);
			if (entity->camera != camera_i) { continue; }

			struct Asset_Material const * material = asset_system_find_instance(&gs_game.assets, entity->material);

			struct vec2 entity_rect_min, entity_rect_max, entity_pivot;
			entity_get_rect(
				entity,
				viewport_size.x, viewport_size.y,
				&entity_rect_min, &entity_rect_max, &entity_pivot
			);
			struct mat4 const mat4_Model = mat4_set_transformation(
				(struct vec3){
					.x = entity_pivot.x,
					.y = entity_pivot.y,
					.z = entity->transform.position.z,
				},
				entity->transform.scale,
				entity->transform.rotation
			);

			if (entity_get_is_batched(entity)) {
				batcher_2d_set_matrix(gs_renderer.batcher, (struct mat4[]){
					mat4_mul_mat(mat4_ProjectionView, mat4_Model)
				});
				batcher_2d_set_material(gs_renderer.batcher, &material->value);
			}

			switch (entity->type) {
				case ENTITY_TYPE_NONE: break;

				case ENTITY_TYPE_MESH: {
					batcher_2d_issue_commands(gs_renderer.batcher, &gs_renderer.gpu_commands);

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
					// struct Entity_Quad const * quad = &entity->as.quad;
					batcher_2d_add_quad(
						gs_renderer.batcher,
						entity_rect_min, entity_rect_max, entity_pivot,
						(float[]){0,0,1,1}
					);
				} break;

				case ENTITY_TYPE_TEXT_2D: {
					struct Entity_Text const * text = &entity->as.text;
					struct Asset_Font const * font = asset_system_find_instance(&gs_game.assets, text->font);
					struct Asset_Bytes const * message = asset_system_find_instance(&gs_game.assets, text->message);
					batcher_2d_add_text(
						gs_renderer.batcher,
						entity_rect_min, entity_rect_max, entity_pivot,
						font,
						text->visible_length,
						message->data,
						text->size
					);
				} break;
			}
		}
	}
	batcher_2d_issue_commands(gs_renderer.batcher, &gs_renderer.gpu_commands);
}

static void prototype_draw_ui(void) {
	// struct uvec2 const screen_size = application_get_screen_size();
}

// ----- ----- ----- ----- -----
//     app callbacks part
// ----- ----- ----- ----- -----

static void app_init(void) {
	renderer_init();
	game_init();
	prototype_init();
}

static void app_free(void) {
	prototype_free();
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
		renderer_frame_init();
		prototype_draw_entities();
		prototype_draw_ui();
		renderer_frame_free();
	}
}

// ----- ----- ----- ----- -----
//     main part
// ----- ----- ----- ----- -----

static void main_fill_settings(struct JSON const * json, void * data) {
	struct Main_Settings * result = data;
	*result = (struct Main_Settings){
		.strings = strings_init(),
	};
	result->config_id = strings_add(&result->strings, json_get_string(json, S_("config")));
	result->scene_id = strings_add(&result->strings, json_get_string(json, S_("scene")));
	// @note: `gs_main_settings.strings` will be freed in the `app_free` function
}

static void main_fill_config(struct JSON const * json, void * data) {
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
	logger_to_console("Bonjour!\n\n");

	platform_system_init();

	logger_to_console("> arguments:\n");
	for (int i = 0; i < argc; i++) {
		logger_to_console("  %s\n", argv[i]);
	}
	logger_to_console("\n");

	process_json(S_("assets/main.json"), &gs_main_settings, main_fill_settings);

	struct Application_Config config;
	process_json(strings_get(&gs_main_settings.strings, gs_main_settings.config_id), &config, main_fill_config);

	application_run(config, (struct Application_Callbacks){
		.init = app_init,
		.free = app_free,
		.fixed_tick = app_fixed_tick,
		.frame_tick = app_frame_tick,
	});

	platform_system_free();
	return 0;
}
