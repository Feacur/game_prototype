#include "framework/memory.h"
#include "framework/unicode.h"
#include "framework/platform_file.h"
#include "framework/logger.h"

#include "framework/maths.h"
#include "framework/input.h"

#include "framework/graphics/material.h"
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

#include "object_camera.h"
#include "object_entity.h"
#include "game_state.h"
#include "components.h"
#include "batcher_2d.h"
#include "asset_types.h"

static struct Main_Uniforms {
	uint32_t camera;
	uint32_t color;
	uint32_t texture;
	uint32_t transform;
} uniforms; // @note: global state

static void game_init(void) {
	state_init();

	gpu_execute(1, &(struct GPU_Command){
		.type = GPU_COMMAND_TYPE_CULL,
		.as.cull = {
			.mode = CULL_MODE_BACK,
			.order = WINDING_ORDER_POSITIVE,
		},
	});

	uniforms.color = graphics_add_uniform_id(S_("u_Color"));
	uniforms.texture = graphics_add_uniform_id(S_("u_Texture"));
	uniforms.camera = graphics_add_uniform_id(S_("u_Camera"));
	uniforms.transform = graphics_add_uniform_id(S_("u_Transform"));

	//
	struct Asset_JSON const * json_test = asset_system_find_instance(&state.asset_system, S_("assets/sandbox/test.json"));
	if (json_test != NULL) { state_read_json(&json_test->value); }

	// objects
	{
		struct Asset_Bytes const * text_test = asset_system_find_instance(&state.asset_system, S_("assets/sandbox/test.txt"));
		struct Asset_Font const * asset_font = asset_system_find_instance(&state.asset_system, S_("assets/fonts/OpenSans-Regular.ttf"));

		// > entities
		array_any_push(&state.entities, &(struct Entity){
			.material = 3,
			.camera = 1,
			.transform = transform_3d_default,
			.rect = (struct Transform_Rect){
				.min_relative = (struct vec2){0.5f, 0.25f},
				.max_relative = (struct vec2){0.5f, 0.25f},
				.max_absolute = (struct vec2){250, 150},
				.pivot = (struct vec2){0.5f, 0.5f},
			},
			//
			.type = ENTITY_TYPE_QUAD_2D,
			.as.quad = {
				.texture_uniform = uniforms.texture,
			},
		});

		array_any_push(&state.entities, &(struct Entity){
			.material = 3,
			.camera = 1,
			.transform = transform_3d_default,
			.rect = (struct Transform_Rect){
				.max_relative = (struct vec2){0.5f, 0.25f},
				.pivot = (struct vec2){0.5f, 0.5f},
			},
			//
			.type = ENTITY_TYPE_QUAD_2D,
			.as.quad = {
				.texture_uniform = uniforms.texture,
			},
		});

		array_any_push(&state.entities, &(struct Entity){
			.material = 2,
			.camera = 1,
			.transform = transform_3d_default,
			.rect = (struct Transform_Rect){
				.min_relative = (struct vec2){0.5f, 0.25f},
				.max_relative = (struct vec2){0.5f, 0.25f},
				.max_absolute = (struct vec2){250, 150},
				.pivot = (struct vec2){0.5f, 0.5f},
			},
			//
			.type = ENTITY_TYPE_TEXT_2D,
			.as.text = {
				.length = text_test->length,
				.data = text_test->data,
				.font = asset_font,
			},
		});
	}
}

static void game_free(void) {
	state_free();
}

static void game_fixed_update(uint64_t elapsed, uint64_t per_second) {
	float const delta_time = (float)((double)elapsed / (double)per_second);
	(void)delta_time;
}

static void game_update(uint64_t elapsed, uint64_t per_second) {
	float const delta_time = (float)((double)elapsed / (double)per_second);

	uint32_t screen_size_x, screen_size_y;
	application_get_screen_size(&screen_size_x, &screen_size_y);

	// if (input_mouse(MC_LEFT)) {
	// 	int32_t x, y;
	// 	input_mouse_delta(&x, &y);
	// 	logger_to_console("delta: %d %d\n", x, y);
	// }

	for (uint32_t entity_i = 0; entity_i < state.entities.count; entity_i++) {
		struct Entity * entity = array_any_at(&state.entities, entity_i);
		struct Camera const * camera = array_any_at(&state.cameras, entity->camera);

		// @todo: precalculate all cameras?
		uint32_t viewport_size_x = screen_size_x, viewport_size_y = screen_size_y;
		if (camera->gpu_target_ref.id != 0 && camera->gpu_target_ref.id != ref_empty.id) {
			gpu_target_get_size(camera->gpu_target_ref, &viewport_size_x, &viewport_size_y);
		}

		// rect behaviour
		switch (entity->rect_behaviour) {
			case ENTITY_RECT_BEHAVIOUR_NONE: break;

			case ENTITY_RECT_BEHAVIOUR_FIT: {
				struct uvec2 const entity_content_size = entity_get_content_size(entity, viewport_size_x, viewport_size_y);
				if (entity_content_size.x != 0 && entity_content_size.y != 0)  {
					// @note: `(fit_size_N <= viewport_size_N) == true`
					//        `(fit_offset_N >= 0) == true`
					//        alternatively `fit_axis_is_x` can be calculated as:
					//        `((float)texture_size_x / (float)viewport_size_x > (float)texture_size_y / (float)viewport_size_y)`
					bool const fit_axis_is_x = (entity_content_size.x * viewport_size_y > entity_content_size.y * viewport_size_x);
					uint32_t const fit_size_x = fit_axis_is_x ? viewport_size_x : mul_div_u32(viewport_size_y, entity_content_size.x, entity_content_size.y);
					uint32_t const fit_size_y = fit_axis_is_x ? mul_div_u32(viewport_size_x, entity_content_size.y, entity_content_size.x) : viewport_size_y;
					uint32_t const fit_offset_x = (viewport_size_x - fit_size_x) / 2;
					uint32_t const fit_offset_y = (viewport_size_y - fit_size_y) / 2;

					entity->rect = (struct Transform_Rect){
						.min_absolute = {(float)fit_offset_x, (float)fit_offset_y},
						.max_absolute = {(float)(fit_offset_x + fit_size_x), (float)(fit_offset_y + fit_size_y)},
						.pivot = {0.5f, 0.5f},
					};
				}
				else { entity->rect = transform_rect_default; }
			} break;
		}

		// entity behaviour
		switch (entity->type) {
			case ENTITY_TYPE_NONE: break;

			case ENTITY_TYPE_QUAD_2D: break;

			case ENTITY_TYPE_MESH: {
				// struct Entity_Mesh * mesh = &entity->as.mesh;

				entity->transform.rotation = vec4_norm(quat_mul(entity->transform.rotation, quat_set_radians(
					(struct vec3){0, 1 * delta_time, 0}
				)));
			} break;

			case ENTITY_TYPE_TEXT_2D: {
				struct Entity_Text * text = &entity->as.text;

				text->visible_length = (text->visible_length + 1) % text->length;
			} break;
		}
	}
}

static void game_render(uint64_t elapsed, uint64_t per_second) {
	// float const delta_time = (float)((double)elapsed / (double)per_second);
	(void)elapsed; (void)per_second;

	uint32_t screen_size_x, screen_size_y;
	application_get_screen_size(&screen_size_x, &screen_size_y);

	if (screen_size_x == 0) { return; }
	if (screen_size_y == 0) { return; }

	batcher_2d_clear(state.batcher);
	array_any_clear(&state.gpu_commands);

	// @todo: iterate though cameras
	//        > sub-iterate through relevant entities (masks, layers?)
	//        render stuff to buffers or screen (camera settings)

	// @todo: how to batch such entities? an explicit mark?
	//        text is alway a block; it's ok to reuse batcher
	//        UI should be batched or split in chunks and batched
	//        Unity does that through `Canvas` components, which basically
	//        denotes a batcher

	uint32_t const gpu_commands_count_estimate = state.cameras.count * 2 + state.entities.count;
	if (state.gpu_commands.capacity < gpu_commands_count_estimate) {
		array_any_resize(&state.gpu_commands, gpu_commands_count_estimate);
	}

	for (uint32_t camera_i = 0; camera_i < state.cameras.count; camera_i++) {
		struct Camera const * camera = array_any_at(&state.cameras, camera_i);

		// prepare camera
		uint32_t viewport_size_x = screen_size_x, viewport_size_y = screen_size_y;
		if (camera->gpu_target_ref.id != 0 && camera->gpu_target_ref.id != ref_empty.id) {
			gpu_target_get_size(camera->gpu_target_ref, &viewport_size_x, &viewport_size_y);
		}

		struct mat4 const mat4_camera = mat4_mul_mat(
			camera_get_projection(camera, viewport_size_x, viewport_size_y),
			mat4_set_inverse_transformation(camera->transform.position, camera->transform.scale, camera->transform.rotation)
		);

		// process camera
		array_any_push(&state.gpu_commands, &(struct GPU_Command){
			.type = GPU_COMMAND_TYPE_TARGET,
			.as.target = {
				.screen_size_x = screen_size_x, .screen_size_y = screen_size_y,
				.gpu_ref = camera->gpu_target_ref,
			},
		});

		if (camera->clear_mask != TEXTURE_TYPE_NONE) {
			array_any_push(&state.gpu_commands, &(struct GPU_Command){
				.type = GPU_COMMAND_TYPE_CLEAR,
				.as.clear = {
					.mask = camera->clear_mask,
					.rgba = camera->clear_rgba,
				}
			});
		}

		// draw entities
		for (uint32_t entity_i = 0; entity_i < state.entities.count; entity_i++) {
			struct Entity * entity = array_any_at(&state.entities, entity_i);
			if (entity->camera != camera_i) { continue; }

			struct Gfx_Material * material = array_any_at(&state.materials, entity->material);

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
				batcher_2d_set_matrix(state.batcher, (struct mat4[]){
					mat4_mul_mat(mat4_camera, mat4_entity)
				});
				batcher_2d_set_material(state.batcher, material);
			}

			switch (entity->type) {
				case ENTITY_TYPE_NONE: break;

				case ENTITY_TYPE_MESH: {
					struct Entity_Mesh const * mesh = &entity->as.mesh;
					gfx_material_set_float(material, uniforms.camera, 4*4, &mat4_camera.x.x);
					gfx_material_set_float(material, uniforms.transform, 4*4, &mat4_entity.x.x);
					array_any_push(&state.gpu_commands, &(struct GPU_Command){
						.type = GPU_COMMAND_TYPE_DRAW,
						.as.draw = {
							.material = material,
							.gpu_mesh_ref = mesh->gpu_mesh_ref,
						},
					});
				} break;

				case ENTITY_TYPE_QUAD_2D: {
					// struct Entity_Quad const * quad = &entity->as.quad;
					batcher_2d_add_quad(
						state.batcher,
						entity_rect_min, entity_rect_max, entity_pivot,
						(float[]){0,0,1,1}
					);
				} break;

				case ENTITY_TYPE_TEXT_2D: {
					struct Entity_Text const * text = &entity->as.text;
					batcher_2d_add_text(
						state.batcher,
						entity_rect_min, entity_rect_max, entity_pivot,
						text->font,
						text->visible_length,
						text->data
					);
				} break;
			}
		}
	}

	batcher_2d_bake(state.batcher, &state.gpu_commands);
	gpu_execute(state.gpu_commands.count, state.gpu_commands.data);
}

//

static void main_get_config(struct Application_Config * config) {
	struct Array_Byte buffer;
	bool const read_success = platform_file_read_entire(S_("assets/sandbox/application.json"), &buffer);
	if (!read_success || buffer.count == 0) { DEBUG_BREAK(); }

	struct Strings strings;
	strings_init(&strings);

	struct JSON settings;
	json_init(&settings, &strings, (char const *)buffer.data);
	array_byte_free(&buffer);

	*config = (struct Application_Config){
		.size_x = (uint32_t)json_get_number(&settings, S_("size_x"), 960),
		.size_y = (uint32_t)json_get_number(&settings, S_("size_y"), 540),
		.vsync = (int32_t)json_get_number(&settings, S_("vsync"), 0),
		.target_refresh_rate = (uint32_t)json_get_number(&settings, S_("target_refresh_rate"), 60),
		.fixed_refresh_rate = (uint32_t)json_get_number(&settings, S_("fixed_refresh_rate"), 30),
		.slow_frames_limit = (uint32_t)json_get_number(&settings, S_("slow_frames_limit"), 2),
	};

	json_free(&settings);
	strings_free(&strings);
}

int main (int argc, char * argv[]) {
	(void)argc; (void)argv;

	struct Application_Config config;
	main_get_config(&config);

	application_run(config, (struct Application_Callbacks){
		.init = game_init,
		.free = game_free,
		.fixed_update = game_fixed_update,
		.update = game_update,
		.render = game_render,
	});

	return 0;
}
