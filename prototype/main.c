#include "framework/memory.h"
#include "framework/unicode.h"
#include "framework/platform_file.h"
#include "framework/logger.h"

#include "framework/maths.h"
#include "framework/input.h"

#include "framework/graphics/material.h"
#include "framework/graphics/material_override.h"
#include "framework/graphics/gpu_objects.h"
#include "framework/graphics/gpu_misc.h"
#include "framework/graphics/gpu_command.h"

#include "application/application.h"

#include "framework/containers/hash_table_any.h"

#include "framework/assets/mesh.h"
#include "framework/assets/image.h"
#include "framework/assets/font.h"
#include "framework/assets/json.h"

#include "application/application.h"
#include "application/asset_types.h"
#include "application/utilities.h"

#include "object_camera.h"
#include "object_entity.h"
#include "game_state.h"
#include "components.h"
#include "batcher_2d.h"

static struct Main_Settings {
	struct Strings strings;
	uint32_t config_id;
	uint32_t scene_id;
} gs_main_settings;

static struct Main_Uniforms {
	uint32_t projection;
	uint32_t camera;
	uint32_t transform;
} gs_main_uniforms;

static void app_init(void) {
	gs_main_uniforms = (struct Main_Uniforms){
		.projection = graphics_add_uniform_id(S_("u_Projection")),
		.camera = graphics_add_uniform_id(S_("u_Camera")),
		.transform = graphics_add_uniform_id(S_("u_Transform")),
	};

	gpu_execute(1, &(struct GPU_Command){
		.type = GPU_COMMAND_TYPE_CULL,
		.as.cull = {
			.mode = CULL_MODE_BACK,
			.order = WINDING_ORDER_POSITIVE,
		},
	});

	game_init();

	process_json(game_fill_scene, &gs_game, strings_get(&gs_main_settings.strings, gs_main_settings.scene_id));
}

static void app_free(void) {
	strings_free(&gs_main_settings.strings);
	game_free();
}

static void app_fixed_update(uint64_t elapsed, uint64_t per_second) {
	float const delta_time = (float)((double)elapsed / (double)per_second);
	(void)delta_time;
}

static void app_frame_update(uint64_t elapsed, uint64_t per_second) {
	float const delta_time = (float)((double)elapsed / (double)per_second);

	uint32_t screen_size_x, screen_size_y;
	application_get_screen_size(&screen_size_x, &screen_size_y);

	// if (input_mouse(MC_LEFT)) {
	// 	int32_t x, y;
	// 	input_mouse_delta(&x, &y);
	// 	logger_to_console("delta: %d %d\n", x, y);
	// }

	for (uint32_t entity_i = 0; entity_i < gs_game.entities.count; entity_i++) {
		struct Entity * entity = array_any_at(&gs_game.entities, entity_i);
		struct Camera const * camera = array_any_at(&gs_game.cameras, entity->camera);

		// @todo: precalculate all cameras?
		uint32_t viewport_size_x = screen_size_x, viewport_size_y = screen_size_y;
		if (camera->gpu_target_ref.id != 0 && camera->gpu_target_ref.id != ref_empty.id) {
			gpu_target_get_size(camera->gpu_target_ref, &viewport_size_x, &viewport_size_y);
		}

		// entity behaviour
		switch (entity->type) {
			case ENTITY_TYPE_NONE: break;

			case ENTITY_TYPE_QUAD_2D: {
				struct Entity_Quad * quad = &entity->as.quad;
				switch (quad->mode) {
					case ENTITY_QUAD_MODE_NONE: break;

					case ENTITY_QUAD_MODE_FIT: {
						struct Asset_Material const * material = asset_system_find_instance(&gs_game.assets, entity->material);
						struct uvec2 const entity_content_size = entity_get_content_size(entity, &material->value, viewport_size_x, viewport_size_y);
						if (entity_content_size.x == 0 || entity_content_size.y == 0) { break; }

						// @note: `(fit_size_N <= viewport_size_N) == true`
						//        `(fit_offset_N >= 0) == true`
						//        alternatively `fit_axis_is_x` can be calculated as:
						//        `((float)texture_size_x / (float)viewport_size_x > (float)texture_size_y / (float)viewport_size_y)`
						bool const fit_axis_is_x = (entity_content_size.x * viewport_size_y > entity_content_size.y * viewport_size_x);
						uint32_t const fit_size_x = fit_axis_is_x ? viewport_size_x : mul_div_u32(viewport_size_y, entity_content_size.x, entity_content_size.y);
						uint32_t const fit_size_y = fit_axis_is_x ? mul_div_u32(viewport_size_x, entity_content_size.y, entity_content_size.x) : viewport_size_y;

						entity->rect = (struct Transform_Rect){
							.anchor_min = {0.5f, 0.5f},
							.anchor_max = {0.5f, 0.5f},
							.extents = {(float)fit_size_x, (float)fit_size_y},
							.pivot = {0.5f, 0.5f},
						};
					} break;

					case ENTITY_QUAD_MODE_SIZE: {
						struct Asset_Material const * material = asset_system_find_instance(&gs_game.assets, entity->material);
						struct uvec2 const entity_content_size = entity_get_content_size(entity, &material->value, viewport_size_x, viewport_size_y);
						if (entity_content_size.x == 0 || entity_content_size.y == 0) { break; }

						entity->rect.extents = (struct vec2){
							.x = (float)entity_content_size.x,
							.y = (float)entity_content_size.y,
						};
					} break;
				}
			} break;

			case ENTITY_TYPE_MESH: {
				// struct Entity_Mesh * mesh = &entity->as.mesh;

				entity->transform.rotation = vec4_norm(quat_mul(entity->transform.rotation, quat_set_radians(
					(struct vec3){0, 1 * delta_time, 0}
				)));
			} break;

			case ENTITY_TYPE_TEXT_2D: {
				struct Entity_Text * text = &entity->as.text;
				struct Asset_Bytes const * text_text = asset_system_find_instance(&gs_game.assets, text->message);
				uint32_t const text_length = text_text->length;
				text->visible_length = (text->visible_length + 1) % text_length;
			} break;
		}
	}
}

static void app_draw_update(uint64_t elapsed, uint64_t per_second) {
	// float const delta_time = (float)((double)elapsed / (double)per_second);
	(void)elapsed; (void)per_second;

	uint32_t screen_size_x, screen_size_y;
	application_get_screen_size(&screen_size_x, &screen_size_y);

	if (screen_size_x == 0) { return; }
	if (screen_size_y == 0) { return; }

	batcher_2d_clear(gs_game.batcher);
	buffer_clear(&gs_game.buffer);
	array_any_clear(&gs_game.gpu_commands);

	// @todo: override material params per shader or material where possible

	uint32_t const gpu_commands_count_estimate = gs_game.cameras.count * 2 + gs_game.entities.count;
	if (gs_game.gpu_commands.capacity < gpu_commands_count_estimate) {
		array_any_resize(&gs_game.gpu_commands, gpu_commands_count_estimate);
	}

	for (uint32_t camera_i = 0; camera_i < gs_game.cameras.count; camera_i++) {
		struct Camera const * camera = array_any_at(&gs_game.cameras, camera_i);

		// prepare camera
		uint32_t viewport_size_x = screen_size_x, viewport_size_y = screen_size_y;
		if (camera->gpu_target_ref.id != 0 && camera->gpu_target_ref.id != ref_empty.id) {
			gpu_target_get_size(camera->gpu_target_ref, &viewport_size_x, &viewport_size_y);
		}

		struct mat4 const mat4_projection = camera_get_projection(&camera->params, viewport_size_x, viewport_size_y);
		struct mat4 const mat4_inverse_camera = mat4_set_inverse_transformation(camera->transform.position, camera->transform.scale, camera->transform.rotation);
		struct mat4 const mat4_camera = mat4_mul_mat(mat4_projection, mat4_inverse_camera);

		// process camera
		array_any_push(&gs_game.gpu_commands, &(struct GPU_Command){
			.type = GPU_COMMAND_TYPE_TARGET,
			.as.target = {
				.screen_size_x = screen_size_x, .screen_size_y = screen_size_y,
				.gpu_ref = camera->gpu_target_ref,
			},
		});

		if (camera->clear.mask != TEXTURE_TYPE_NONE) {
			array_any_push(&gs_game.gpu_commands, &(struct GPU_Command){
				.type = GPU_COMMAND_TYPE_CLEAR,
				.as.clear = {
					.mask = camera->clear.mask,
					.rgba = camera->clear.rgba,
				}
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
				viewport_size_x, viewport_size_y,
				&entity_rect_min, &entity_rect_max, &entity_pivot
			);
			struct mat4 const mat4_entity = mat4_set_transformation(
				(struct vec3){
					.x = entity_pivot.x,
					.y = entity_pivot.y,
					.z = entity->transform.position.z,
				},
				entity->transform.scale,
				entity->transform.rotation
			);

			if (entity_get_is_batched(entity)) {
				batcher_2d_set_matrix(gs_game.batcher, (struct mat4[]){
					mat4_mul_mat(mat4_camera, mat4_entity)
				});
				batcher_2d_set_material(gs_game.batcher, &material->value);
			}

			switch (entity->type) {
				case ENTITY_TYPE_NONE: break;

				case ENTITY_TYPE_MESH: {
					struct Entity_Mesh const * mesh = &entity->as.mesh;
					struct Asset_Model const * model = asset_system_find_instance(&gs_game.assets, mesh->mesh);

					uint32_t const override_offset = (uint32_t)gs_game.buffer.count;
					uint32_t override_count = 0;

					//
					override_count++;
					buffer_push_many(&gs_game.buffer, SIZE_OF_MEMBER(struct Gfx_Material_Override_Entry, header), (void *)&(struct Gfx_Material_Override_Entry){
						.header.id = gs_main_uniforms.projection,
						.header.size = sizeof(mat4_projection),
					});
					buffer_push_many(&gs_game.buffer, sizeof(mat4_projection), (void const *)&mat4_projection);
					buffer_align(&gs_game.buffer);

					//
					override_count++;
					buffer_push_many(&gs_game.buffer, SIZE_OF_MEMBER(struct Gfx_Material_Override_Entry, header), (void *)&(struct Gfx_Material_Override_Entry){
						.header.id = gs_main_uniforms.camera,
						.header.size = sizeof(mat4_inverse_camera),
					});
					buffer_push_many(&gs_game.buffer, sizeof(mat4_inverse_camera), (void const *)&mat4_inverse_camera);
					buffer_align(&gs_game.buffer);

					//
					override_count++;
					buffer_push_many(&gs_game.buffer, SIZE_OF_MEMBER(struct Gfx_Material_Override_Entry, header), (void *)&(struct Gfx_Material_Override_Entry){
						.header.id = gs_main_uniforms.transform,
						.header.size = sizeof(mat4_entity),
					});
					buffer_push_many(&gs_game.buffer, sizeof(mat4_entity), (void const *)&mat4_entity);
					buffer_align(&gs_game.buffer);

					//
					array_any_push(&gs_game.gpu_commands, &(struct GPU_Command){
						.type = GPU_COMMAND_TYPE_DRAW,
						.as.draw = {
							.material = &material->value,
							.gpu_mesh_ref = model->gpu_ref,
							.override = {
								.buffer = &gs_game.buffer,
								.offset = override_offset, .count = override_count,
							}
						},
					});
				} break;

				case ENTITY_TYPE_QUAD_2D: {
					// struct Entity_Quad const * quad = &entity->as.quad;
					batcher_2d_add_quad(
						gs_game.batcher,
						entity_rect_min, entity_rect_max, entity_pivot,
						(float[]){0,0,1,1}
					);
				} break;

				case ENTITY_TYPE_TEXT_2D: {
					struct Entity_Text const * text = &entity->as.text;
					struct Asset_Font const * font = asset_system_find_instance(&gs_game.assets, text->font);
					struct Asset_Bytes const * message = asset_system_find_instance(&gs_game.assets, text->message);
					batcher_2d_add_text(
						gs_game.batcher,
						entity_rect_min, entity_rect_max, entity_pivot,
						font,
						text->visible_length,
						message->data
					);
				} break;
			}
		}
	}

	batcher_2d_bake(gs_game.batcher, &gs_game.gpu_commands);
	gpu_execute(gs_game.gpu_commands.count, gs_game.gpu_commands.data);
}

//

static void main_fill_settings(struct JSON const * json, void * data) {
	struct Main_Settings * result = data;
	strings_init(&result->strings);
	result->config_id = strings_add(&result->strings, json_get_string(json, S_("config"), S_NULL));
	result->scene_id = strings_add(&result->strings, json_get_string(json, S_("scene"), S_NULL));
}

static void main_fill_config(struct JSON const * json, void * data) {
	struct Application_Config * result = data;
	*result = (struct Application_Config){
		.size_x = (uint32_t)json_get_number(json, S_("size_x"), 960),
		.size_y = (uint32_t)json_get_number(json, S_("size_y"), 540),
		.flexible = json_get_boolean(json, S_("flexible"), false),
		.vsync = (int32_t)json_get_number(json, S_("vsync"), 0),
		.target_refresh_rate = (uint32_t)json_get_number(json, S_("target_refresh_rate"), 60),
		.fixed_refresh_rate = (uint32_t)json_get_number(json, S_("fixed_refresh_rate"), 30),
		.slow_frames_limit = (uint32_t)json_get_number(json, S_("slow_frames_limit"), 2),
	};
}

int main (int argc, char * argv[]) {
	logger_to_console("> arguments:\n");
	for (int i = 0; i < argc; i++) {
		logger_to_console("  %s\n", argv[i]);
	}

	struct CString const path = S_("assets/main.json");
	process_json(main_fill_settings, &gs_main_settings, path);

	struct Application_Config config;
	process_json(main_fill_config, &config, strings_get(&gs_main_settings.strings, gs_main_settings.config_id));

	application_run(config, (struct Application_Callbacks){
		.init = app_init,
		.free = app_free,
		.fixed_update = app_fixed_update,
		.frame_update = app_frame_update,
		.draw_update  = app_draw_update,
	});

	return 0;
}
