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
	//
	enum Texture_Type clear_mask;
	uint32_t clear_rgba;
};

enum Entity_Type {
	ENTITY_TYPE_MESH,
	ENTITY_TYPE_QUAD,
	ENTITY_TYPE_TEXT,
	ENTITY_TYPE_FONT,
};

struct Entity_Mesh {
	// @todo: an asset ref? a name?
	struct Ref gpu_mesh_ref;
};

struct Entity_Quad { // @note: a fulscreen quad
	struct Ref gpu_target_ref;
	enum Texture_Type type;
	uint32_t index;
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
	uint32_t camera;
	struct Transform_3D transform;
	//
	struct Gfx_Material material;
	//
	enum Entity_Type type;
	union {
		struct Entity_Mesh mesh;
		struct Entity_Quad quad;
		struct Entity_Text text;
		struct Entity_Font font;
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
		struct Gfx_Material test;
		struct Gfx_Material batcher;
	} materials;

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

static void game_init(void) {
	// init state
	{
		asset_system_init(&state.asset_system);
		state.batcher = batcher_2d_init();
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
		asset_system_aquire(&state.asset_system, "assets/sandbox/cube.obj");
		asset_system_aquire(&state.asset_system, "assets/sandbox/test.png");
		asset_system_aquire(&state.asset_system, "assets/sandbox/test.txt");
	}

	// init uniforms ids
	{
		state.uniforms.color = graphics_add_uniform("u_Color");
		state.uniforms.texture = graphics_add_uniform("u_Texture");
		state.uniforms.camera = graphics_add_uniform("u_Camera");
		state.uniforms.transform = graphics_add_uniform("u_Transform");
	}

	// prepare materials
	{
		// @todo: make material assets
		WEAK_PTR(struct Asset_Shader const) gpu_program_test = asset_system_find_instance(&state.asset_system, "assets/shaders/test.glsl");
		gfx_material_init(&state.materials.test, gpu_program_test->gpu_ref);

		WEAK_PTR(struct Asset_Image const) texture_test = asset_system_find_instance(&state.asset_system, "assets/sandbox/test.png");
		gfx_material_set_texture(&state.materials.test, state.uniforms.texture, 1, &texture_test->gpu_ref);
		gfx_material_set_float(&state.materials.test, state.uniforms.color, 4, &(struct vec4){0.2f, 0.6f, 1, 1}.x);

		//
		WEAK_PTR(struct Asset_Shader const) gpu_program_batcher = asset_system_find_instance(&state.asset_system, "assets/shaders/batcher_2d.glsl");
		gfx_material_init(&state.materials.batcher, gpu_program_batcher->gpu_ref);
	}

	// init objects
	{ // state is expected to be inited
		struct Ref const gpu_target_ref = gpu_target_init(
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

		WEAK_PTR(struct Asset_Model const) mesh_cube = asset_system_find_instance(&state.asset_system, "assets/sandbox/cube.obj");
		WEAK_PTR(struct Asset_Font const) font_open_sans = asset_system_find_instance(&state.asset_system, "assets/fonts/OpenSans-Regular.ttf");
		WEAK_PTR(struct Asset_Bytes const) text_test = asset_system_find_instance(&state.asset_system, "assets/sandbox/test.txt");

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
			.gpu_target_ref = gpu_target_ref,
			//
			.clear_mask = TEXTURE_TYPE_COLOR | TEXTURE_TYPE_DEPTH,
			.clear_rgba = 0x303030ff,
		});

		array_any_push(&state.cameras, &(struct Camera){
			.transform = {
				.scale = (struct vec3){1, 1, 1},
				.rotation = quat_identity,
			},
			//
			.mode = CAMERA_MODE_SCREEN,
			.ncp = 0, .fcp = 1, .ortho = 1,
			//
			.clear_mask = TEXTURE_TYPE_COLOR | TEXTURE_TYPE_DEPTH,
			.clear_rgba = 0x000000ff,
		});

		// > entities
		array_any_push(&state.entities, &(struct Entity){
			.camera = 0,
			.transform = {
				.scale = (struct vec3){1, 1, 1},
				.rotation = quat_identity,
			},
			//
			.material = state.materials.test,
			.type = ENTITY_TYPE_MESH,
			.as.mesh = {
				.gpu_mesh_ref = mesh_cube->gpu_ref,
			}
		});

		array_any_push(&state.entities, &(struct Entity){
			.camera = 1,
			.transform = {
				.scale = (struct vec3){1, 1, 1},
				.rotation = quat_identity,
			},
			//
			.material = state.materials.batcher,
			.type = ENTITY_TYPE_QUAD,
			.as.quad = {
				.gpu_target_ref = gpu_target_ref,
				.type = TEXTURE_TYPE_COLOR,
			}
		});

		array_any_push(&state.entities, &(struct Entity){
			.camera = 1,
			.transform = {
				.position = (struct vec3){50, 200, 0},
				.scale = (struct vec3){1, 1, 1},
				.rotation = quat_identity,
			},
			//
			.material = state.materials.batcher,
			.type = ENTITY_TYPE_TEXT,
			.as.text = {
				.length = test111_length,
				.data = test111,
				.font = font_open_sans,
			}
		});

		array_any_push(&state.entities, &(struct Entity){
			.camera = 1,
			.transform = {
				.position = (struct vec3){600, 200, 0},
				.scale = (struct vec3){1, 1, 1},
				.rotation = quat_identity,
			},
			//
			.material = state.materials.batcher,
			.type = ENTITY_TYPE_TEXT,
			.as.text = {
				.length = text_test->length,
				.data = text_test->data,
				.font = font_open_sans,
			}
		});

		array_any_push(&state.entities, &(struct Entity){
			.camera = 1,
			.transform = {
				.position = (struct vec3){0, 400, 0},
				.scale = (struct vec3){1, 1, 1},
				.rotation = quat_identity,
			},
			//
			.material = state.materials.batcher,
			.type = ENTITY_TYPE_FONT,
			.as.font = {
				.font = font_open_sans,
			}
		});
	}
}

static void game_free(void) {
	asset_system_free(&state.asset_system);

	batcher_2d_free(state.batcher);

	array_any_free(&state.cameras);
	array_any_free(&state.entities);

	gfx_material_free(&state.materials.test);
	gfx_material_free(&state.materials.batcher);

	memset(&state, 0, sizeof(state));
}

static void game_fixed_update(uint64_t elapsed, uint64_t per_second) {
	float const delta_time = (float)((double)elapsed / (double)per_second);
	(void)delta_time;
}

static void game_update(uint64_t elapsed, uint64_t per_second) {
	float const delta_time = (float)((double)elapsed / (double)per_second);

	// if (input_mouse(MC_LEFT)) {
	// 	int32_t x, y;
	// 	input_mouse_delta(&x, &y);
	// 	logger_to_console("delta: %d %d\n", x, y);
	// }

	for (uint32_t entity_i = 0; entity_i < state.entities.count; entity_i++) {
		struct Entity * entity = array_any_at(&state.entities, entity_i);

		switch (entity->type) {
			case ENTITY_TYPE_MESH: {
				// struct Entity_Mesh * mesh = &entity->as.mesh;

				entity->transform.rotation = vec4_norm(quat_mul(entity->transform.rotation, quat_set_radians(
					(struct vec3){0 * delta_time, 1 * delta_time, 0 * delta_time}
				)));
			} break;

			case ENTITY_TYPE_TEXT: {
				struct Entity_Text * text = &entity->as.text;

				text->visible_length = (text->visible_length + 1) % text->length;
			} break;

			default: break;
		}
		if (entity->type != ENTITY_TYPE_MESH) { continue; }


	}
}

static void game_render(uint32_t screen_size_x, uint32_t screen_size_y) {
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
				.target = camera->gpu_target_ref,
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

			struct mat4 const mat4_entity = mat4_set_transformation(entity->transform.position, entity->transform.scale, entity->transform.rotation);

			switch (entity->type) {
				// --- camera, world coords, transformed
				case ENTITY_TYPE_MESH: {
					batcher_2d_draw(state.batcher, screen_size_x, screen_size_y, camera->gpu_target_ref);

					//
					struct Entity_Mesh const * mesh = &entity->as.mesh;

					gfx_material_set_float(&entity->material, state.uniforms.camera, 4*4, &mat4_camera.x.x);
					gfx_material_set_float(&entity->material, state.uniforms.transform, 4*4, &mat4_entity.x.x);
					graphics_draw(&(struct Render_Pass){
						.screen_size_x = screen_size_x, .screen_size_y = screen_size_y,
						.target = camera->gpu_target_ref,
						.blend_mode = {.mask = COLOR_CHANNEL_FULL},
						.depth_mode = {.enabled = true, .mask = true},
						//
						.material = &entity->material,
						.mesh = mesh->gpu_mesh_ref,
					});
				} break;

				// --- overlay, normalized coords, farthest
				case ENTITY_TYPE_QUAD: {
					struct Entity_Quad const * quad = &entity->as.quad;

					struct Ref texture_ref = gpu_target_get_texture_ref(quad->gpu_target_ref, quad->type, quad->index);

					uint32_t texture_size_x, texture_size_y;
					gpu_texture_get_size(texture_ref, &texture_size_x, &texture_size_y);

					// @note: `(fit_size_N <= camera_size_N) == true`
					//        `(fit_offset_N >= 0) == true`
					//        alternatively `fit_axis_is_x` can be calculated as:
					//        `((float)texture_size_x / (float)camera_size_x > (float)texture_size_y / (float)camera_size_y)`
					bool const fit_axis_is_x = (texture_size_x * camera_size_y > texture_size_y * camera_size_x);
					uint32_t const fit_size_x = fit_axis_is_x ? camera_size_x : mul_div_u32(camera_size_y, texture_size_x, texture_size_y);
					uint32_t const fit_size_y = fit_axis_is_x ? mul_div_u32(camera_size_x, texture_size_y, texture_size_x) : camera_size_y;
					uint32_t const fit_offset_x = (camera_size_x - fit_size_x) / 2;
					uint32_t const fit_offset_y = (camera_size_y - fit_size_y) / 2;

					batcher_2d_push_matrix(state.batcher, mat4_mul_mat(mat4_camera, mat4_entity));

					batcher_2d_set_blend_mode(state.batcher, (struct Blend_Mode){.mask = COLOR_CHANNEL_FULL});
					batcher_2d_set_depth_mode(state.batcher, (struct Depth_Mode){0});

					// > the buffer
					batcher_2d_set_material(state.batcher, &entity->material);
					batcher_2d_set_texture(state.batcher, texture_ref);
					batcher_2d_add_quad(
						state.batcher,
						(float[]){(float)fit_offset_x, (float)fit_offset_y, (float)(fit_offset_x + fit_size_x), (float)(fit_offset_y + fit_size_y)},
						(float[]){0,0,1,1}
					);

					//
					batcher_2d_pop_matrix(state.batcher);

				} break;

				// --- overlay, screen coords, nearest
				case ENTITY_TYPE_TEXT: {
					struct Entity_Text const * text = &entity->as.text;

					//
					batcher_2d_push_matrix(state.batcher, mat4_mul_mat(mat4_camera, mat4_entity));

					batcher_2d_set_blend_mode(state.batcher, (struct Blend_Mode){
						.rgb = {
							.op = BLEND_OP_ADD,
							.src = BLEND_FACTOR_SRC_ALPHA,
							.dst = BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
						},
						.mask = COLOR_CHANNEL_FULL
					});
					batcher_2d_set_depth_mode(state.batcher, (struct Depth_Mode){0});

					// > text
					batcher_2d_set_material(state.batcher, &entity->material);
					batcher_2d_set_texture(state.batcher, text->font->gpu_ref);
					batcher_2d_add_text(
						state.batcher,
						text->font,
						text->visible_length,
						text->data
					);

					batcher_2d_pop_matrix(state.batcher);
				} break;

				// --- overlay, screen coords, nearest
				case ENTITY_TYPE_FONT: {
					struct Entity_Font const * font = &entity->as.font;

					//
					batcher_2d_push_matrix(state.batcher, mat4_mul_mat(mat4_camera, mat4_entity));

					batcher_2d_set_blend_mode(state.batcher, (struct Blend_Mode){
						.rgb = {
							.op = BLEND_OP_ADD,
							.src = BLEND_FACTOR_SRC_ALPHA,
							.dst = BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
						},
						.mask = COLOR_CHANNEL_FULL
					});
					batcher_2d_set_depth_mode(state.batcher, (struct Depth_Mode){0});

					// > text
					struct Image const * font_image = font_image_get_asset(font->font->buffer);

					batcher_2d_set_material(state.batcher, &entity->material);
					batcher_2d_set_texture(state.batcher, font->font->gpu_ref);
					batcher_2d_add_quad(
						state.batcher,
						(float[]){
							0,
							0,
							(float)font_image->size_x,
							(float)font_image->size_y
						},
						(float[]){0,0,1,1}
					);

					batcher_2d_pop_matrix(state.batcher);
				} break;
			}
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
