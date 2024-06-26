#if defined(GAME_ARCH_SHARED)
	#error "this an executable entry point"
#endif

#include "framework/unicode.h"
#include "framework/json_read.h"
#include "framework/maths.h"
#include "framework/input.h"
#include "framework/formatter.h"

#include "framework/platform/system.h"
#include "framework/platform/file.h"
#include "framework/containers/hashmap.h"

#include "framework/systems/memory.h"
#include "framework/systems/strings.h"
#include "framework/systems/defer.h"
#include "framework/systems/materials.h"
#include "framework/systems/assets.h"

#include "framework/graphics/gfx_material.h"
#include "framework/graphics/gfx_objects.h"
#include "framework/graphics/command.h"
#include "framework/graphics/misc.h"

#include "framework/assets/mesh.h"
#include "framework/assets/image.h"
#include "framework/assets/typeface.h"
#include "framework/assets/json.h"

#include "application/json_load.h"
#include "application/application.h"
#include "application/asset_types.h"
#include "application/app_components.h"
#include "application/batcher_2d.h"
#include "application/renderer.h"

#include "object_camera.h"
#include "object_entity.h"
#include "game_state.h"
#include "proto_components.h"
#include "ui.h"


static struct Main_Settings {
	struct Handle sh_config;
	struct Handle sh_scene;
} gs_main_settings;

static void prototype_tick_cameras(void) {
	struct uvec2 const screen_size = application_get_screen_size();
	FOR_ARRAY(&gs_game.cameras, it) {
		struct Camera * camera = it.value;
		camera->cached_size = screen_size;
		if (!handle_is_null(camera->ah_target)) {
			struct Asset_Target const * asset = system_assets_get(camera->ah_target);
			struct GPU_Target const * target = (asset != NULL) ? gpu_target_get(asset->gh_target) : NULL;
			camera->cached_size = (target != NULL) ? target->size : (struct uvec2){0};
		}
	}
}

static void prototype_tick_entities_rect(void) {
	FOR_ARRAY(&gs_game.entities, it) {
		struct Entity * entity = it.value;

		if (entity->type == ENTITY_TYPE_NONE) { continue; }
		if (entity->type == ENTITY_TYPE_MESH) { continue; }

		struct Camera const * camera = array_at(&gs_game.cameras, entity->camera);
		struct uvec2 const viewport_size = camera->cached_size;

		struct vec2 entity_pivot;
		transform_rect_get_pivot_and_rect(
			&entity->rect, viewport_size,
			&entity_pivot, &entity->cached_rect
		);
		entity->transform.position.x = entity_pivot.x;
		entity->transform.position.y = entity_pivot.y;
	}
}

static void prototype_tick_entities_rotation_mode(void) {
	float const dt = (float)application_get_delta_time();
	FOR_ARRAY(&gs_game.entities, it) {
		struct Entity * entity = it.value;

		switch (entity->rotation_mode) {
			case ENTITY_ROTATION_MODE_NONE: break;

			case ENTITY_ROTATION_MODE_X: {
				entity->transform.rotation = vec4_norm(quat_mul(entity->transform.rotation, quat_radians(
					(struct vec3){1 * dt, 0, 0}
				)));
			} break;

			case ENTITY_ROTATION_MODE_Y: {
				entity->transform.rotation = vec4_norm(quat_mul(entity->transform.rotation, quat_radians(
					(struct vec3){0, 1 * dt, 0}
				)));
			} break;

			case ENTITY_ROTATION_MODE_Z: {
				entity->transform.rotation = vec4_norm(quat_mul(entity->transform.rotation, quat_radians(
					(struct vec3){0, 0, 1 * dt}
				)));
			} break;
		}
	}
}

static void prototype_tick_entities_quad_2d(void) {
	FOR_ARRAY(&gs_game.entities, it) {
		struct Entity * entity = it.value;

		if (entity->type != ENTITY_TYPE_QUAD_2D) { continue; }
		struct Entity_Quad * e_quad = &entity->as.quad;
		if (e_quad->mode == ENTITY_QUAD_MODE_NONE) { continue; }

		struct Camera const * camera = array_at(&gs_game.cameras, entity->camera);
		struct uvec2 const viewport_size = camera->cached_size;

		struct Asset_Material const * material = system_assets_get(entity->ah_material);
		struct uvec2 const content_size = entity_get_content_size(entity, material->mh_mat, viewport_size);
		if (content_size.x == 0 || content_size.y == 0) { break; }

		switch (e_quad->mode) {
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
	if (handle_is_null(gs_main_settings.sh_scene)) {
		WRN("no scene to initialize with");
		return;
	}

	struct CString const scene_path = system_strings_get(gs_main_settings.sh_scene);
	process_json(scene_path, &gs_game, game_fill_scene);
	gpu_execute(1, &(struct GPU_Command){
		.type = GPU_COMMAND_TYPE_CULL,
		.as.cull = {
			.flags = CULL_FLAG_BACK,
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
	// 	TRC("delta: %d %d", x, y);
	// }

	prototype_tick_cameras();
	prototype_tick_entities_rotation_mode();
	prototype_tick_entities_quad_2d();
	prototype_tick_entities_rect();
}

static void prototype_draw_objects(void) {
	struct uvec2 const screen_size = application_get_screen_size();

	if (gs_game.cameras.count == 0) {
		array_push_many(&gs_renderer.gpu_commands, 2, (struct GPU_Command[]){
			{
				.type = GPU_COMMAND_TYPE_TARGET,
				.as.target = {
					.screen_size = screen_size,
				},
			},
			{
				.type = GPU_COMMAND_TYPE_CLEAR,
				.as.clear = {
					.flags  = TEXTURE_FLAG_COLOR | TEXTURE_FLAG_DEPTH | TEXTURE_FLAG_STENCIL,
				},
			},
		});
		return;
	}

	uint32_t const gpu_commands_count_estimate = gs_game.cameras.count * 2 + gs_game.entities.count;
	array_ensure(&gs_renderer.gpu_commands, gpu_commands_count_estimate);

	struct Handle gh_program_prev = {.gen = 1};

	batcher_2d_set_color(gs_renderer.batcher_2d, (struct vec4){1, 1, 1, 1});

	// process global
	{
		// @note: consider global block unique and extendable
		// graphics_buffer_align(&gs_renderer.global, BUFFER_TARGET_UNIFORM);

		struct Shader_Global {
			float dummy;
		} sg;

		// size_t const offset = gs_renderer.global.size;
		buffer_push_many(&gs_renderer.global, sizeof(sg), &sg);
		array_push_many(&gs_renderer.gpu_commands, 1, &(struct GPU_Command){
			.type = GPU_COMMAND_TYPE_BUFFER,
			.as.buffer = {
				.gh_buffer = gs_renderer.gh_camera,
				// .offset = offset,
				// .length = gs_renderer.global.size - offset,
				.target = BUFFER_TARGET_UNIFORM,
				.index = SHADER_BLOCK_GLOBAL - 1,
			},
		});
	}

	FOR_ARRAY(&gs_game.cameras, it_camera) {
		struct Camera const * camera = it_camera.value;

		struct Shader_Camera {
			struct uvec2 viewport_size;
			uint8_t      _padding0[sizeof(float) * 2]; // to align `.view.x` at 4 floats
			struct mat4  view;
			struct mat4  projection;
			struct mat4  projection_view;
		} sc = {
			.viewport_size = camera->cached_size,
			.view = mat4_inverse_transformation(
				camera->transform.position,
				camera->transform.scale,
				camera->transform.rotation
			),
			.projection = camera_get_projection(
				&camera->params, camera->cached_size
			),
		};
		sc.projection_view = mat4_mul_mat(sc.projection, sc.view);

		// process camera
		{
			graphics_buffer_align(&gs_renderer.camera, BUFFER_TARGET_UNIFORM);

			size_t const offset = gs_renderer.camera.size;
			buffer_push_many(&gs_renderer.camera, sizeof(sc), &sc);
			array_push_many(&gs_renderer.gpu_commands, 1, &(struct GPU_Command){
				.type = GPU_COMMAND_TYPE_BUFFER,
				.as.buffer = {
					.gh_buffer = gs_renderer.gh_camera,
					.offset = offset,
					.length = gs_renderer.camera.size - offset,
					.target = BUFFER_TARGET_UNIFORM,
					.index = SHADER_BLOCK_CAMERA - 1,
				},
			});
		}

		struct Asset_Target const * asset = system_assets_get(camera->ah_target);
		struct Handle const gh_target = (asset != NULL) ? asset->gh_target : (struct Handle){0};
		if (!handle_equals(gh_program_prev, gh_target)) {
			gh_program_prev = gh_target;
			batcher_2d_issue_commands(gs_renderer.batcher_2d, &gs_renderer.gpu_commands);
			array_push_many(&gs_renderer.gpu_commands, 1, &(struct GPU_Command){
				.type = GPU_COMMAND_TYPE_TARGET,
				.as.target = {
					.gh_target   = gh_target,
					.screen_size = screen_size,
				},
			});
		}

		if (camera->clear.flags != TEXTURE_FLAG_NONE) {
			batcher_2d_issue_commands(gs_renderer.batcher_2d, &gs_renderer.gpu_commands);
			array_push_many(&gs_renderer.gpu_commands, 1, &(struct GPU_Command){
				.type = GPU_COMMAND_TYPE_CLEAR,
				.as.clear = {
					.flags = camera->clear.flags,
					.color = camera->clear.color,
				},
			});
		}

		// draw entities
		FOR_ARRAY(&gs_game.entities, it_entity) {
			struct Entity const * entity = it_entity.value;
			if (entity->camera != it_camera.curr) { continue; }

			struct Asset_Material const * material_asset = system_assets_get(entity->ah_material);
			struct Gfx_Material const * material = system_materials_get(material_asset->mh_mat);

			struct mat4 const u_Model = mat4_transformation(
				entity->transform.position,
				entity->transform.scale,
				entity->transform.rotation
			);

			switch (entity->type) {
				case ENTITY_TYPE_NONE: break;
				case ENTITY_TYPE_MESH: break;

				case ENTITY_TYPE_QUAD_2D:
				case ENTITY_TYPE_TEXT_2D: {
					struct mat4 const matrix = mat4_mul_mat(sc.projection_view, u_Model);
					batcher_2d_set_matrix(gs_renderer.batcher_2d, matrix);
					batcher_2d_set_material(gs_renderer.batcher_2d, material_asset->mh_mat);
				} break;
			}

			switch (entity->type) {
				case ENTITY_TYPE_NONE: break;

				case ENTITY_TYPE_MESH: {
					batcher_2d_issue_commands(gs_renderer.batcher_2d, &gs_renderer.gpu_commands);

					struct Entity_Mesh const * e_mesh = &entity->as.mesh;
					struct Asset_Model const * model = system_assets_get(e_mesh->ah_mesh);

					uint32_t const override_offset = gs_renderer.uniforms.headers.count;
					gfx_uniforms_push(&gs_renderer.uniforms, S_("u_Model"), CB_(u_Model));

					array_push_many(&gs_renderer.gpu_commands, 3, (struct GPU_Command[]){
						(struct GPU_Command){
							.type = GPU_COMMAND_TYPE_MATERIAL,
							.as.material = {
								.mh_mat = material_asset->mh_mat,
							},
						},
						(struct GPU_Command){
							.type = GPU_COMMAND_TYPE_UNIFORM,
							.as.uniform = {
								.gh_program = material->gh_program,
								.uniforms = &gs_renderer.uniforms,
								.offset = override_offset,
								.count = (gs_renderer.uniforms.headers.count - override_offset),
							},
						},
						(struct GPU_Command){
							.type = GPU_COMMAND_TYPE_DRAW,
							.as.draw = {
								.gh_mesh = model->gh_mesh,
							},
						},
					});
				} break;

				case ENTITY_TYPE_QUAD_2D: {
					struct Entity_Quad const * e_quad = &entity->as.quad;
					enum Batcher_Flag flag = BATCHER_FLAG_NONE;
					if (handle_equals(e_quad->sh_uniform, system_strings_find(S_("p_Font")))) {
						flag |= BATCHER_FLAG_FONT;
					}
					batcher_2d_add_quad(
						gs_renderer.batcher_2d,
						entity->cached_rect,
						e_quad->view,
						(uint32_t)flag
					);
				} break;

				case ENTITY_TYPE_TEXT_2D: {
					struct Entity_Text const * e_text = &entity->as.text;
					struct Asset_Bytes const * text_bytes = system_assets_get(e_text->ah_text);
					struct CString const value = {
						.length = text_bytes->length,
						.data = (char const *)text_bytes->data,
					};
					batcher_2d_add_text(
						gs_renderer.batcher_2d,
						entity->cached_rect, e_text->alignment, true,
						e_text->ah_font, value, e_text->size
					);
				} break;
			}
		}
	}
	batcher_2d_issue_commands(gs_renderer.batcher_2d, &gs_renderer.gpu_commands);
}

static struct CString prototype_get_fps_cstring(void) {
	static char buffer[32];

	double const dt = application_get_delta_time();
	uint32_t const fps = (uint32_t)r64_floor(1.0 / dt);

	uint32_t const length = formatter_fmt(sizeof(buffer), buffer, "FPS: %5d (%.5f ms)", fps, dt);
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

static void app_init(void) {
	renderer_init();
	game_init();
	ui_init();
	prototype_init();

	// @note: essentially the same as `assets/materials/batcher_2d.material`
	ui_set_shader(S_("assets/shaders/batcher_2d.glsl"));
	ui_set_image(S_("assets/images/ui.png"));
	ui_set_font(S_("assets/test.font"));
}

static void app_free(void) {
	prototype_free();
	ui_free();
	game_free();
	renderer_free();
}

static void app_fixed_tick(void) {
}

static void app_frame_tick(void) {
	if (input_scan(SC_LALT, IT_DOWN) && input_scan(SC_F4, IT_DOWN)) {
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
	application_exit();
}

static bool app_window_close(void) {
	return true;
}

// ----- ----- ----- ----- -----
//     main part
// ----- ----- ----- ----- -----

static JSON_PROCESSOR(main_fill_settings) {
	struct Main_Settings * result = data;
	*result = (struct Main_Settings){0};

	if (json->type == JSON_ERROR)  { REPORT_CALLSTACK(); DEBUG_BREAK(); return; }
	if (data != &gs_main_settings) { REPORT_CALLSTACK(); DEBUG_BREAK(); return; }

	result->sh_config = json_get(json, S_("config"))->as.sh_string;
	result->sh_scene = json_get(json, S_("scene"))->as.sh_string;
}

static JSON_PROCESSOR(main_fill_config) {
	struct Application_Config * result = data;
	*result = (struct Application_Config){0};

	if (json->type == JSON_ERROR) { REPORT_CALLSTACK(); DEBUG_BREAK(); return; }

	json_read_many_u32(json_get(json, S_("size")), 2, &result->size.x);
	result->resizable = json_get_boolean(json, S_("resizable"));
	result->vsync               = (int32_t)json_get_number(json, S_("vsync"));
	result->target_refresh_rate = (uint32_t)json_get_number(json, S_("target_refresh_rate"));
	result->fixed_refresh_rate  = (uint32_t)json_get_number(json, S_("fixed_refresh_rate"));
}

static void main_system_init(void) {
	system_assets_init();
	system_materials_init();
	system_defer_init();
	system_strings_init();
	system_memory_arena_init();
	system_memory_debug_init();

	system_memory_arena_ensure((64 + 32) * (1 << 10));
	platform_system_init((struct Platform_Callbacks){
		.quit = app_platform_quit,
	});
}

static void main_system_free(void) {
	system_assets_free();
	system_materials_free();
	system_defer_free();
	system_strings_free();
	system_memory_arena_free();
	system_memory_debug_free();

	platform_system_free();
}

static void main_run_application(void) {
	asset_types_map();
	asset_types_set();

	process_json(S_("assets/main.json"), &gs_main_settings, main_fill_settings);
	if (handle_is_null(gs_main_settings.sh_config)) { return; }

	struct Application_Config config;
	struct CString const config_path = system_strings_get(gs_main_settings.sh_config);
	process_json(config_path, &config, main_fill_config);

	TRC("launched application");
	application_run(config, (struct Application_Callbacks){
		.init = app_init,
		.free = app_free,
		.fixed_tick = app_fixed_tick,
		.frame_tick = app_frame_tick,
		.window_callbacks = {
			.close = app_window_close,
		},
	});
	TRC("application has ended");
}

int main (int argc, char * argv[]) {
	LOG("> main argumentss:\n");
	for (int i = 0; i < argc; i++) {
		LOG("  %s\n", argv[i]);
	}

	main_system_init();
	main_run_application();
	main_system_free();

	return 0;
}
