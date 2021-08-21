#include "framework/memory.h"
#include "framework/unicode.h"
#include "framework/platform_file.h"
#include "framework/logger.h"

#include "framework/maths.h"
#include "framework/input.h"

#include "framework/graphics/types.h"
#include "framework/graphics/gpu_objects.h"
#include "framework/graphics/material.h"
#include "framework/graphics/pass.h"
#include "framework/graphics/graphics.h"

#include "application/application.h"

#include "framework/containers/array_byte.h"
#include "framework/containers/ref_table.h"
#include "framework/containers/strings.h"
#include "framework/containers/hash_table_any.h"

#include "framework/assets/mesh.h"
#include "framework/assets/image.h"
#include "framework/assets/font.h"

#include "framework/systems/asset_system.h"

#include "application/application.h"
#include "components.h"
#include "batcher_2d.h"
#include "asset_types.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>

enum Camera_Mode {
	CAMERA_MODE_NONE,
	CAMERA_MODE_SCREEN,
	CAMERA_MODE_ASPECT_X,
	CAMERA_MODE_ASPECT_Y,
};

struct Camera {
	struct Transform_3D transform;
	//
	enum Camera_Mode mode;
	float ncp, fcp, ortho;
	//
	struct Ref gpu_target_ref;
	enum Texture_Type clear_mask;
	uint32_t clear_rgba;
};

enum Entity_Rect_Mode {
	ENTITY_RECT_MODE_NONE,
	ENTITY_RECT_MODE_FIT,
	ENTITY_RECT_MODE_CONTENT,
};

enum Entity_Type {
	ENTITY_TYPE_MESH,
	ENTITY_TYPE_QUAD_2D,
	ENTITY_TYPE_TEXT_2D,
};

struct Entity_Mesh {
	struct Ref gpu_mesh_ref;
};

struct Entity_Quad {
	uint8_t dummy;
};

struct Entity_Text {
	// @todo: a separate type for Asset_Bytes?
	uint32_t visible_length;
	uint32_t length;
	uint8_t const * data;
	// @todo: an asset ref
	struct Asset_Font const * font;
};

struct Entity_Font {
	// @todo: an asset ref
	struct Asset_Font const * font;
};

struct Entity {
	uint32_t material;
	uint32_t camera;
	struct Transform_3D transform;
	struct Transform_Rect rect;
	//
	struct Blend_Mode blend_mode;
	struct Depth_Mode depth_mode;
	//
	enum Entity_Rect_Mode rect_mode;
	enum Entity_Type type;
	union {
		struct Entity_Mesh mesh;
		struct Entity_Quad quad;
		struct Entity_Text text;
	} as;
};

static struct Game_State {
	struct Asset_System asset_system;

	struct Batcher_2D * batcher;

	struct {
		uint32_t camera;
		uint32_t color;
		uint32_t texture;
		uint32_t transform;
	} uniforms;

	struct {
		struct Ref camera0_target;
	} gpu_objects;

	struct Array_Any materials;
	struct Array_Any cameras;
	struct Array_Any entities;
} state;

static uint8_t const test111[] = "abcdefghigklmnopqrstuvwxyz\n0123456789\nABCDEFGHIGKLMNOPQRSTUVWXYZ";
static uint32_t const test111_length = sizeof(test111) / (sizeof(*test111)) - 1;

static struct mat4 camera_get_projection(enum Camera_Mode mode, float ncp, float fcp, float ortho, uint32_t camera_size_x, uint32_t camera_size_y) {
	switch (mode) {
		case CAMERA_MODE_NONE: // @note: basically normalized device coordinates
			// @note: is equivalent of `CAMERA_MODE_ASPECT_X` or `CAMERA_MODE_ASPECT_Y`
			//        with `camera_size_x == camera_size_y`
			return mat4_identity;

		case CAMERA_MODE_SCREEN:
			return mat4_set_projection(
				(struct vec2){2 / (float)camera_size_x, 2 / (float)camera_size_y},
				(struct vec2){-1, -1},
				ncp, fcp, ortho
			);

		case CAMERA_MODE_ASPECT_X:
			return mat4_set_projection(
				(struct vec2){1, (float)camera_size_x / (float)camera_size_y},
				(struct vec2){0, 0},
				ncp, fcp, ortho
			);

		case CAMERA_MODE_ASPECT_Y:
			return mat4_set_projection(
				(struct vec2){(float)camera_size_y / (float)camera_size_x, 1},
				(struct vec2){0, 0},
				ncp, fcp, ortho
			);
	}

	logger_to_console("unknown camera mode"); DEBUG_BREAK();
	return (struct mat4){0};
}

inline static void entity_get_rect(
	struct Transform_3D const * transform, struct Transform_Rect const * rect,
	uint32_t camera_size_x, uint32_t camera_size_y,
	struct vec2 * min, struct vec2 * max, struct vec2 * pivot
) {
	*min = (struct vec2){
		rect->min_relative.x * (float)camera_size_x + rect->min_absolute.x,
		rect->min_relative.y * (float)camera_size_y + rect->min_absolute.y,
	};
	*max = (struct vec2){
		rect->max_relative.x * (float)camera_size_x + rect->max_absolute.x,
		rect->max_relative.y * (float)camera_size_y + rect->max_absolute.y,
	};
	*pivot = (struct vec2){
		.x = lerp(min->x, max->x, rect->pivot.x) + transform->position.x,
		.y = lerp(min->y, max->y, rect->pivot.y) + transform->position.y,
	};
}

inline static struct uvec2 entity_get_content_size(
	struct Entity const * entity,
	uint32_t camera_size_x, uint32_t camera_size_y
) {
	struct Gfx_Material * material = array_any_at(&state.materials, entity->material);

	switch (entity->type) {
		case ENTITY_TYPE_MESH: return (struct uvec2){
			camera_size_x,
			camera_size_y
		};

		case ENTITY_TYPE_QUAD_2D: {
			// struct Entity_Quad const * quad = &entity->as.quad;

			struct Ref const * gpu_texture_ref = gfx_material_get_texture(material, state.uniforms.texture);

			uint32_t texture_size_x, texture_size_y;
			gpu_texture_get_size(*gpu_texture_ref, &texture_size_x, &texture_size_y);

			return (struct uvec2){
				texture_size_x,
				texture_size_y
			};
		} // break;

		case ENTITY_TYPE_TEXT_2D: {
			int32_t const rect[] = {
				(int32_t)floorf(entity->rect.min_relative.x * (float)camera_size_x + entity->rect.min_absolute.x),
				(int32_t)floorf(entity->rect.min_relative.y * (float)camera_size_y + entity->rect.min_absolute.y),
				(int32_t)ceilf(entity->rect.max_relative.x * (float)camera_size_x + entity->rect.max_absolute.x),
				(int32_t)ceilf(entity->rect.max_relative.y * (float)camera_size_y + entity->rect.max_absolute.y),
			};
			return (struct uvec2){ // @idea: invert negatives?
				(uint32_t)max_s32(rect[2] - rect[0], 0),
				(uint32_t)max_s32(rect[3] - rect[1], 0),
			};
		} // break;
	}

	logger_to_console("unknown entity type"); DEBUG_BREAK();
	return (struct uvec2){0};
}

static void game_init(void) {
	// init state
	{
		asset_system_init(&state.asset_system);
		state.batcher = batcher_2d_init();
		array_any_init(&state.materials, sizeof(struct Gfx_Material));
		array_any_init(&state.cameras, sizeof(struct Camera));
		array_any_init(&state.entities, sizeof(struct Entity));
	}

	// init asset system
	{ // state is expected to be inited
		// > map types
		asset_system_map_extension(&state.asset_system, "shader", "glsl");
		asset_system_map_extension(&state.asset_system, "model",  "obj");
		asset_system_map_extension(&state.asset_system, "model",  "fbx");
		asset_system_map_extension(&state.asset_system, "image",  "png");
		asset_system_map_extension(&state.asset_system, "font",   "ttf");
		asset_system_map_extension(&state.asset_system, "font",   "otf");
		asset_system_map_extension(&state.asset_system, "bytes",  "txt");

		// > register types
		asset_system_set_type(&state.asset_system, "shader", (struct Asset_Callbacks){
			.init = asset_shader_init,
			.free = asset_shader_free,
		}, sizeof(struct Asset_Shader));

		asset_system_set_type(&state.asset_system, "model", (struct Asset_Callbacks){
			.init = asset_model_init,
			.free = asset_model_free,
		}, sizeof(struct Asset_Model));

		asset_system_set_type(&state.asset_system, "image", (struct Asset_Callbacks){
			.init = asset_image_init,
			.free = asset_image_free,
		}, sizeof(struct Asset_Image));

		asset_system_set_type(&state.asset_system, "font", (struct Asset_Callbacks){
			.init = asset_font_init,
			.free = asset_font_free,
		}, sizeof(struct Asset_Font));

		asset_system_set_type(&state.asset_system, "bytes", (struct Asset_Callbacks){
			.init = asset_bytes_init,
			.free = asset_bytes_free,
		}, sizeof(struct Asset_Bytes));
	}

	// prefetch some assets
	{ // asset system is expected to be inited
		asset_system_aquire(&state.asset_system, "assets/shaders/test.glsl");
		asset_system_aquire(&state.asset_system, "assets/shaders/batcher_2d.glsl");
		asset_system_aquire(&state.asset_system, "assets/fonts/OpenSans-Regular.ttf");
		asset_system_aquire(&state.asset_system, "assets/fonts/JetBrainsMono-Regular.ttf");
		asset_system_aquire(&state.asset_system, "assets/sandbox/test.png");
		asset_system_aquire(&state.asset_system, "assets/sandbox/cube.obj");
		asset_system_aquire(&state.asset_system, "assets/sandbox/test.txt");
	}

	// prepare assets
	WEAK_PTR(struct Asset_Shader const) gpu_program_test = asset_system_find_instance(&state.asset_system, "assets/shaders/test.glsl");
	WEAK_PTR(struct Asset_Shader const) gpu_program_batcher = asset_system_find_instance(&state.asset_system, "assets/shaders/batcher_2d.glsl");
	WEAK_PTR(struct Asset_Font const) font_open_sans = asset_system_find_instance(&state.asset_system, "assets/fonts/OpenSans-Regular.ttf");
	WEAK_PTR(struct Asset_Image const) texture_test = asset_system_find_instance(&state.asset_system, "assets/sandbox/test.png");
	WEAK_PTR(struct Asset_Model const) mesh_cube = asset_system_find_instance(&state.asset_system, "assets/sandbox/cube.obj");
	WEAK_PTR(struct Asset_Bytes const) text_test = asset_system_find_instance(&state.asset_system, "assets/sandbox/test.txt");

	// init uniforms ids
	{
		state.uniforms.color = graphics_add_uniform("u_Color");
		state.uniforms.texture = graphics_add_uniform("u_Texture");
		state.uniforms.camera = graphics_add_uniform("u_Camera");
		state.uniforms.transform = graphics_add_uniform("u_Transform");
	}

	// prepare gpu targets
	{
		state.gpu_objects.camera0_target = gpu_target_init(
		320, 180,
		(struct Texture_Parameters[]){
			[0] = {
				.texture_type = TEXTURE_TYPE_COLOR,
				.data_type = DATA_TYPE_U8,
				.channels = 4,
				.flags = TEXTURE_FLAG_READ
			},
			[1] = {
					.texture_type = TEXTURE_TYPE_DEPTH,
					.data_type = DATA_TYPE_R32,
				},
			},
			2
		);
	}

	// init objects
	{ // @todo: introduce assets
		// > materials
		WEAK_PTR(struct Gfx_Material) material;

		array_any_push(&state.materials, &(struct Gfx_Material){0});
		material = array_any_at(&state.materials, state.materials.count - 1);
		gfx_material_init(material, gpu_program_test->gpu_ref);
		gfx_material_set_texture(material, state.uniforms.texture, 1, &texture_test->gpu_ref);
		gfx_material_set_float(material, state.uniforms.color, 4, &(struct vec4){0.2f, 0.6f, 1, 1}.x);

		array_any_push(&state.materials, &(struct Gfx_Material){0});
		material = array_any_at(&state.materials, state.materials.count - 1);
		gfx_material_init(material, gpu_program_batcher->gpu_ref);
		gfx_material_set_texture(material, state.uniforms.texture, 1, (struct Ref[]){
			gpu_target_get_texture_ref(state.gpu_objects.camera0_target, TEXTURE_TYPE_COLOR, 0),
		});
		gfx_material_set_float(material, state.uniforms.color, 4, &(struct vec4){1, 1, 1, 1}.x);

		array_any_push(&state.materials, &(struct Gfx_Material){0});
		material = array_any_at(&state.materials, state.materials.count - 1);
		gfx_material_init(material, gpu_program_batcher->gpu_ref);
		gfx_material_set_texture(material, state.uniforms.texture, 1, (struct Ref[]){
			font_open_sans->gpu_ref,
		});
		gfx_material_set_float(material, state.uniforms.color, 4, &(struct vec4){1, 1, 1, 1}.x);

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
			.gpu_target_ref = state.gpu_objects.camera0_target,
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
			.blend_mode = blend_mode_opaque,
			.depth_mode = {.enabled = true, .mask = true},
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
			.blend_mode = blend_mode_opaque,
			//
			.rect_mode = ENTITY_RECT_MODE_FIT,
			.type = ENTITY_TYPE_QUAD_2D,
			.as.quad = {0},
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
			.blend_mode = blend_mode_transparent,
			//
			.type = ENTITY_TYPE_TEXT_2D,
			.as.text = {
				.length = test111_length,
				.data = test111,
				.font = font_open_sans,
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
			.blend_mode = blend_mode_transparent,
			//
			.type = ENTITY_TYPE_TEXT_2D,
			.as.text = {
				.length = text_test->length,
				.data = text_test->data,
				.font = font_open_sans,
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
			.blend_mode = blend_mode_transparent,
			//
			.rect_mode = ENTITY_RECT_MODE_CONTENT,
			.type = ENTITY_TYPE_QUAD_2D,
			.as.quad = {0},
		});
	}
}

static void game_free(void) {
	asset_system_free(&state.asset_system);

	batcher_2d_free(state.batcher);

	for (uint32_t i = 0; i < state.materials.count; i++) {
		gfx_material_free(array_any_at(&state.materials, i));
	}

	array_any_free(&state.materials);
	array_any_free(&state.cameras);
	array_any_free(&state.entities);

	memset(&state, 0, sizeof(state));
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
		uint32_t camera_size_x = screen_size_x, camera_size_y = screen_size_y;
		if (camera->gpu_target_ref.id != 0) {
			gpu_target_get_size(camera->gpu_target_ref, &camera_size_x, &camera_size_y);
		}

		// rect behaviour
		struct uvec2 const entity_content_size = entity_get_content_size(entity, camera_size_x, camera_size_y);
		switch (entity->rect_mode) {
			case ENTITY_RECT_MODE_NONE: break;

			case ENTITY_RECT_MODE_FIT: {
				// @note: `(fit_size_N <= camera_size_N) == true`
				//        `(fit_offset_N >= 0) == true`
				//        alternatively `fit_axis_is_x` can be calculated as:
				//        `((float)texture_size_x / (float)camera_size_x > (float)texture_size_y / (float)camera_size_y)`
				bool const fit_axis_is_x = (entity_content_size.x * camera_size_y > entity_content_size.y * camera_size_x);
				uint32_t const fit_size_x = fit_axis_is_x ? camera_size_x : mul_div_u32(camera_size_y, entity_content_size.x, entity_content_size.y);
				uint32_t const fit_size_y = fit_axis_is_x ? mul_div_u32(camera_size_x, entity_content_size.y, entity_content_size.x) : camera_size_y;
				uint32_t const fit_offset_x = (camera_size_x - fit_size_x) / 2;
				uint32_t const fit_offset_y = (camera_size_y - fit_size_y) / 2;

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

		if (camera->clear_mask != TEXTURE_TYPE_NONE) {
			graphics_draw(&(struct Render_Pass){
				.screen_size_x = screen_size_x, .screen_size_y = screen_size_y,
				.gpu_target_ref = camera->gpu_target_ref,
				.blend_mode = {.mask = COLOR_CHANNEL_FULL},
				.depth_mode = {.enabled = true, .mask = true},
				//
				.clear_mask = camera->clear_mask,
				.clear_rgba = camera->clear_rgba,
			});
		}

		// prepare camera
		uint32_t camera_size_x = screen_size_x, camera_size_y = screen_size_y;
		if (camera->gpu_target_ref.id != 0) {
			gpu_target_get_size(camera->gpu_target_ref, &camera_size_x, &camera_size_y);
		}

		struct mat4 const mat4_camera = mat4_mul_mat(
			camera_get_projection(camera->mode, camera->ncp, camera->fcp, camera->ortho, camera_size_x, camera_size_y),
			mat4_set_inverse_transformation(camera->transform.position, camera->transform.scale, camera->transform.rotation)
		);

		// draw entities
		for (uint32_t entity_i = 0; entity_i < state.entities.count; entity_i++) {
			struct Entity * entity = array_any_at(&state.entities, entity_i);
			if (entity->camera != camera_i) { continue; }

			struct Gfx_Material * material = array_any_at(&state.materials, entity->material);

			struct vec2 entity_rect_min, entity_rect_max, entity_pivot;
			entity_get_rect(
				&entity->transform, &entity->rect,
				camera_size_x, camera_size_y,
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

			batcher_2d_push_matrix(state.batcher, mat4_mul_mat(mat4_camera, mat4_entity));
			batcher_2d_set_material(state.batcher, material);
			batcher_2d_set_blend_mode(state.batcher, entity->blend_mode);
			batcher_2d_set_depth_mode(state.batcher, entity->depth_mode);

			switch (entity->type) {
				case ENTITY_TYPE_MESH: {
					// @todo: make a draw commands buffer?
					// @note: flush the batcher before drawing a mesh
					batcher_2d_draw(state.batcher, screen_size_x, screen_size_y, camera->gpu_target_ref);

					//
					struct Entity_Mesh const * mesh = &entity->as.mesh;

					gfx_material_set_float(material, state.uniforms.camera, 4*4, &mat4_camera.x.x);
					gfx_material_set_float(material, state.uniforms.transform, 4*4, &mat4_entity.x.x);
					graphics_draw(&(struct Render_Pass){
						.screen_size_x = screen_size_x, .screen_size_y = screen_size_y,
						.gpu_target_ref = camera->gpu_target_ref,
						.blend_mode = entity->blend_mode,
						.depth_mode = entity->depth_mode,
						//
						.material = material,
						.gpu_mesh_ref = mesh->gpu_mesh_ref,
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

			batcher_2d_pop_matrix(state.batcher);
		}

		batcher_2d_draw(state.batcher, screen_size_x, screen_size_y, camera->gpu_target_ref);
	}
}

//

int main (int argc, char * argv[]) {
	(void)argc; (void)argv;
	application_run(&(struct Application_Config){
		.callbacks = {
			.init = game_init,
			.free = game_free,
			.fixed_update = game_fixed_update,
			.update = game_update,
			.render = game_render,
		},
		.size_x = 1280, .size_y = 720,
		.vsync = 1,
		.target_refresh_rate = 72,
		.fixed_refresh_rate = 50,
		.slow_frames_limit = 5,
	});
	return EXIT_SUCCESS;
}
