#include "framework/memory.h"
#include "framework/unicode.h"
#include "framework/platform_file.h"
#include "framework/logger.h"

#include "framework/maths.h"
#include "framework/input.h"

#include "framework/graphics/types.h"
#include "framework/graphics/material.h"
#include "framework/graphics/gpu_objects.h"
#include "framework/graphics/gpu_misc.h"
#include "framework/graphics/gpu_pass.h"

#include "application/application.h"

#include "framework/containers/array_byte.h"
#include "framework/containers/ref_table.h"
#include "framework/containers/strings.h"
#include "framework/containers/hash_table_any.h"

#include "framework/assets/mesh.h"
#include "framework/assets/image.h"
#include "framework/assets/font.h"
#include "framework/assets/json.h"

#include "framework/systems/asset_system.h"

#include "application/application.h"

#include "object_camera.h"
#include "object_entity.h"
#include "game_state.h"
#include "components.h"
#include "batcher_2d.h"
#include "asset_types.h"

#include <stdlib.h>

static struct Main_Uniforms {
	uint32_t camera;
	uint32_t color;
	uint32_t texture;
	uint32_t transform;
} uniforms;

static uint8_t const test111[] = "abcdefghigklmnopqrstuvwxyz\n0123456789\nABCDEFGHIGKLMNOPQRSTUVWXYZ";
static uint32_t const test111_length = sizeof(test111) / (sizeof(*test111)) - 1;

static void game_init(void) {
	state_init();

	uniforms.color = graphics_add_uniform_id("u_Color");
	uniforms.texture = graphics_add_uniform_id("u_Texture");
	uniforms.camera = graphics_add_uniform_id("u_Camera");
	uniforms.transform = graphics_add_uniform_id("u_Transform");

	//
	struct Asset_JSON const * json_test = asset_system_find_instance(&state.asset_system, "assets/sandbox/test.json");
	state_read_json(&json_test->value, asset_json_strings);

	// objects
	{
		struct Asset_Model const * mesh_cube = asset_system_find_instance(&state.asset_system, "assets/sandbox/cube.obj");
		struct Asset_Bytes const * text_test = asset_system_find_instance(&state.asset_system, "assets/sandbox/test.txt");
		struct Asset_Font const * asset_font = asset_system_find_instance(&state.asset_system, "assets/fonts/OpenSans-Regular.ttf");

		// > cameras
		array_any_push(&state.cameras, &(struct Camera){
			.transform = {
				.scale = (struct vec3){1, 1, 1},
				.rotation = quat_set_radians((struct vec3){MATHS_TAU / 16, 0, 0}),
				.position = (struct vec3){0, 3, -5},
			},
			//
			.mode = CAMERA_MODE_ASPECT_X,
			.ncp = 0.1f, .fcp = 10, .ortho = 0,
			//
			.gpu_target_ref = *(struct Ref *)array_any_at(&state.targets, 0),
			.clear_mask = TEXTURE_TYPE_COLOR | TEXTURE_TYPE_DEPTH,
			.clear_rgba = 0x303030ff,
		});

		array_any_push(&state.cameras, &(struct Camera){
			.transform = transform_3d_default,
			//
			.mode = CAMERA_MODE_SCREEN,
			.ncp = 0, .fcp = 1, .ortho = 1,
			//
			.clear_mask = TEXTURE_TYPE_COLOR | TEXTURE_TYPE_DEPTH,
			.clear_rgba = 0x000000ff,
		});

		// > entities
		array_any_push(&state.entities, &(struct Entity){
			.material = 0,
			.camera = 0,
			.transform = transform_3d_default,
			.rect = transform_rect_default,
			//
			.type = ENTITY_TYPE_MESH,
			.as.mesh = {
				.gpu_mesh_ref = mesh_cube->gpu_ref,
			},
		});

		array_any_push(&state.entities, &(struct Entity){
			.material = 1,
			.camera = 1,
			.transform = transform_3d_default,
			.rect = transform_rect_default,
			//
			.rect_mode = ENTITY_RECT_MODE_FIT,
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
				.min_relative = (struct vec2){0.0f, 0.25f},
				.min_absolute = (struct vec2){ 50,  50},
				.max_relative = (struct vec2){0.0f, 0.25f},
				.max_absolute = (struct vec2){250, 150},
				.pivot = (struct vec2){0.5f, 0.5f},
			},
			//
			.type = ENTITY_TYPE_TEXT_2D,
			.as.text = {
				.length = test111_length,
				.data = test111,
				.font = asset_font,
			},
		});

		array_any_push(&state.entities, &(struct Entity){
			.material = 2,
			.camera = 1,
			.transform = transform_3d_default,
			.rect = (struct Transform_Rect){
				.min_relative = (struct vec2){0.5f, 0.25f},
				.min_absolute = (struct vec2){ 50,  50},
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

		array_any_push(&state.entities, &(struct Entity){
			.material = 2,
			.camera = 1,
			.transform = transform_3d_default,
			.rect = (struct Transform_Rect){
				.min_relative = (struct vec2){0.0f, 0.25f},
				.min_absolute = (struct vec2){ 50, 150},
				.max_relative = (struct vec2){0.0f, 0.25f},
				.max_absolute = (struct vec2){250, 350},
				.pivot = (struct vec2){0.5f, 0.5f},
			},
			//
			.rect_mode = ENTITY_RECT_MODE_CONTENT,
			.type = ENTITY_TYPE_QUAD_2D,
			.as.quad = {
				.texture_uniform = uniforms.texture,
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
		if (camera->gpu_target_ref.id != 0) {
			gpu_target_get_size(camera->gpu_target_ref, &viewport_size_x, &viewport_size_y);
		}

		// rect behaviour
		// struct uvec2 entity_content_size = {1, 1};
		struct uvec2 const entity_content_size = entity_get_content_size(entity, viewport_size_x, viewport_size_y);
		if (entity_content_size.x == 0 || entity_content_size.y == 0) { continue; }

		switch (entity->rect_mode) {
			case ENTITY_RECT_MODE_NONE: break;

			case ENTITY_RECT_MODE_FIT: {
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
			} break;

			case ENTITY_RECT_MODE_CONTENT: {
				// @note: lags one frame
				entity->rect.max_relative = entity->rect.min_relative;
				entity->rect.max_absolute = (struct vec2){
					.x = entity->rect.min_absolute.x + (float)entity_content_size.x,
					.y = entity->rect.min_absolute.y + (float)entity_content_size.y,
				};
			} break;
		}

		// entity behaviour
		switch (entity->type) {
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

	// @todo: iterate though cameras
	//        > sub-iterate through relevant entities (masks, layers?)
	//        render stuff to buffers or screen (camera settings)

	// @todo: how to batch such entities? an explicit mark?
	//        text is alway a block; it's ok to reuse batcher
	//        UI should be batched or split in chunks and batched
	//        Unity does that through `Canvas` components, which basically
	//        denotes a batcher

	for (uint32_t camera_i = 0; camera_i < state.cameras.count; camera_i++) {
		struct Camera const * camera = array_any_at(&state.cameras, camera_i);

		// prepare camera
		uint32_t viewport_size_x = screen_size_x, viewport_size_y = screen_size_y;
		if (camera->gpu_target_ref.id != 0) {
			gpu_target_get_size(camera->gpu_target_ref, &viewport_size_x, &viewport_size_y);
		}

		struct mat4 const mat4_camera = mat4_mul_mat(
			camera_get_projection(camera, viewport_size_x, viewport_size_y),
			mat4_set_inverse_transformation(camera->transform.position, camera->transform.scale, camera->transform.rotation)
		);

		// process camera
		graphics_process(&(struct Render_Pass){
			.type = RENDER_PASS_TYPE_TARGET,
			.as.target = {
				.screen_size_x = screen_size_x, .screen_size_y = screen_size_y,
				.gpu_ref = camera->gpu_target_ref,
			},
		});

		if (camera->clear_mask != TEXTURE_TYPE_NONE) {
			graphics_process(&(struct Render_Pass){
				.type = RENDER_PASS_TYPE_CLEAR,
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
				(struct vec3){ // @note: `entity_pivot` includes `entity->transform.position`
					.x = entity_pivot.x,
					.y = entity_pivot.y,
					.z = entity->transform.position.z,
				},
				entity->transform.scale,
				entity->transform.rotation
			);

			bool const entity_is_batched = entity_get_is_batched(entity);
			if (entity_is_batched) {
				batcher_2d_push_matrix(state.batcher, mat4_mul_mat(mat4_camera, mat4_entity));
				batcher_2d_set_material(state.batcher, material);
			}

			switch (entity->type) {
				case ENTITY_TYPE_MESH: {
					// @todo: make a draw commands buffer?
					// @note: flush the batcher before drawing a mesh
					batcher_2d_draw(state.batcher);

					//
					struct Entity_Mesh const * mesh = &entity->as.mesh;

					gfx_material_set_float(material, uniforms.camera, 4*4, &mat4_camera.x.x);
					gfx_material_set_float(material, uniforms.transform, 4*4, &mat4_entity.x.x);
					graphics_process(&(struct Render_Pass){
						.type = RENDER_PASS_TYPE_DRAW,
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
						text->font,
						text->visible_length,
						text->data,
						entity_rect_min, entity_rect_max, entity_pivot
					);
				} break;
			}

			if (entity_is_batched) {
				batcher_2d_pop_matrix(state.batcher);
			}
		}

		batcher_2d_draw(state.batcher);
	}
}

//

static void main_get_config(struct Application_Config * config) {
#define JSON_GET(key) json_get(&settings, &strings, key)
#define JSON_GET_NUMBER(key, type, default_value) (type)json_as_number(JSON_GET(key), default_value)

	struct Array_Byte buffer;
	bool const read_success = platform_file_read_entire("assets/sandbox/application.json", &buffer);
	if (!read_success || buffer.count == 0) { DEBUG_BREAK(); }

	struct Strings strings;
	strings_init(&strings);

	struct JSON settings;
	json_init(&settings, &strings, (char const *)buffer.data);
	array_byte_free(&buffer);

	*config = (struct Application_Config){
		.size_x = JSON_GET_NUMBER("size_x", uint32_t, 960),
		.size_y = JSON_GET_NUMBER("size_y", uint32_t, 540),
		.vsync = JSON_GET_NUMBER("vsync", int32_t, 0),
		.target_refresh_rate = JSON_GET_NUMBER("target_refresh_rate", uint32_t, 60),
		.fixed_refresh_rate = JSON_GET_NUMBER("fixed_refresh_rate", uint32_t, 30),
		.slow_frames_limit = JSON_GET_NUMBER("slow_frames_limit", uint32_t, 2),
	};

	json_free(&settings);
	strings_free(&strings);

#undef JSON_GET
#undef JSON_GET_NUMBER
}

#include <stdlib.h>

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

	return EXIT_SUCCESS;
}
