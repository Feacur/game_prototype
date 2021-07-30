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
	CAMERA_MODE_SCALE,
	CAMERA_MODE_SCREEN,
};

struct Camera {
	struct Transform_3D transform;
	enum Camera_Mode mode;
	struct vec2 offset_xy, scale_xy;
	float ncp, fcp;
	float ortho;
	struct Ref gpu_target_ref;
};

struct Entity {
	uint32_t camera;
	struct Transform_3D transform;
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

		// > cameras
		array_any_push(&state.cameras, &(struct Camera){
			.transform = {
				.position = (struct vec3){0, 3, -5},
				.scale = (struct vec3){1, 1, 1},
				.rotation = quat_set_radians((struct vec3){MATHS_TAU / 16, 0, 0}),
			},
			.mode = CAMERA_MODE_SCALE,
			.offset_xy = (struct vec2){0, 0},
			.scale_xy = (struct vec2){1, 1},
			.ncp = 0.1f,
			.fcp = 10,
			.ortho = 0,
			.gpu_target_ref = gpu_target_ref,
		});

		array_any_push(&state.cameras, &(struct Camera){
			.transform = {
				.scale = (struct vec3){1, 1, 1},
				.rotation = quat_identity,
			},
			.mode = CAMERA_MODE_SCREEN,
			.offset_xy = (struct vec2){0, 0},
			.scale_xy = (struct vec2){1, 1},
			.ncp = 0,
			.fcp = 1,
			.ortho = 1,
		});

		// > entities
		array_any_push(&state.entities, &(struct Entity){
			.camera = 0,
			.transform = {
				.scale = (struct vec3){1, 1, 1},
				.rotation = quat_identity,
			},
		});

		// @todo: display the render texture
		// array_any_push(&state.entities, &(struct Entity){
		// 	.camera = 1,
		// 	.transform = {
		// 		.scale = (struct vec3){1, 1, 1},
		// 		.rotation = quat_identity,
		// 	},
		// });

		// @todo: draw UI
		// array_any_push(&state.entities, &(struct Entity){
		// 	.camera = 2,
		// 	.transform = {
		// 		.scale = (struct vec3){1, 1, 1},
		// 		.rotation = quat_identity,
		// 	},
		// });
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

	if (input_mouse(MC_LEFT)) {
		int32_t x, y;
		input_mouse_delta(&x, &y);
		logger_to_console("delta: %d %d\n", x, y);
	}

	for (uint32_t entity_i = 0; entity_i < state.entities.count; entity_i++) {
		struct Entity * entity = array_any_at(&state.entities, entity_i);

		entity->transform.rotation = vec4_norm(quat_mul(entity->transform.rotation, quat_set_radians(
			(struct vec3){0 * delta_time, 1 * delta_time, 0 * delta_time}
		)));

		break;
	}
}

static void game_render(uint32_t size_x, uint32_t size_y) {
	WEAK_PTR(struct Asset_Model const) mesh_cube = asset_system_find_instance(&state.asset_system, "assets/sandbox/cube.obj");
	WEAK_PTR(struct Asset_Font const) font_open_sans = asset_system_find_instance(&state.asset_system, "assets/fonts/OpenSans-Regular.ttf");
	WEAK_PTR(struct Asset_Bytes const) text_test = asset_system_find_instance(&state.asset_system, "assets/sandbox/test.txt");


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

		// clear
		graphics_draw(&(struct Render_Pass){
			.target = camera->gpu_target_ref,
			.size_x = size_x, .size_y = size_y,
			.blend_mode = {.mask = COLOR_CHANNEL_FULL},
			.depth_mode = {.enabled = true, .mask = true},
			//
			.clear_mask = TEXTURE_TYPE_COLOR | TEXTURE_TYPE_DEPTH,
			.clear_rgba = 0x303030ff,
		});

		// prepare camera
		uint32_t camera_size_x = size_x, camera_size_y = size_y;
		if (camera->gpu_target_ref.id != 0) {
			gpu_target_get_size(camera->gpu_target_ref, &camera_size_x, &camera_size_y);
		}

		float const aspect_x = 1;
		float const aspect_y = (float)camera_size_x / (float)camera_size_y;

		struct mat4 const mat4_camera = mat4_mul_mat(
			mat4_set_projection(
				camera->offset_xy,
				(struct vec2){aspect_x * camera->scale_xy.x, aspect_y * camera->scale_xy.y},
				camera->ncp, camera->fcp,
				camera->ortho
			),
			mat4_set_inverse_transformation(camera->transform.position, camera->transform.scale, camera->transform.rotation)
		);

		// draw entities
		for (uint32_t entity_i = 0; entity_i < state.entities.count; entity_i++) {
			struct Entity const * entity = array_any_at(&state.entities, entity_i);
			if (entity->camera != camera_i) { continue; }

			struct mat4 const mat4_entity = mat4_set_transformation(entity->transform.position, entity->transform.scale, entity->transform.rotation);

			gfx_material_set_float(&state.materials.test, state.uniforms.camera, 4*4, &mat4_camera.x.x);
			gfx_material_set_float(&state.materials.test, state.uniforms.transform, 4*4, &mat4_entity.x.x);
			graphics_draw(&(struct Render_Pass){
				.target = camera->gpu_target_ref,
				.size_x = size_x, .size_y = size_y,
				.blend_mode = {.mask = COLOR_CHANNEL_FULL},
				.depth_mode = {.enabled = true, .mask = true},
				//
				.material = &state.materials.test,
				.mesh = mesh_cube->gpu_ref,
			});
		}
	}


	//
	uint32_t target_size_x = 0, target_size_y = 0;
	struct Ref gpu_target_ref = {0};
	for (uint32_t camera_i = 0; camera_i < state.cameras.count; camera_i++) {
		struct Camera const * camera = array_any_at(&state.cameras, camera_i);

		if (camera->gpu_target_ref.id != 0) {
			gpu_target_ref = camera->gpu_target_ref;
			gpu_target_get_size(camera->gpu_target_ref, &target_size_x, &target_size_y);
			break;
		}
	}


	//
	uint8_t const test111[] = "abcdefghigklmnopqrstuvwxyz\n0123456789\nABCDEFGHIGKLMNOPQRSTUVWXYZ";
	uint32_t const test111_length = sizeof(test111) / (sizeof(*test111)) - 1;

	static uint32_t text_length_dynamic = 0;
	text_length_dynamic = (text_length_dynamic + 1) % text_test->length;

	static uint32_t test111_length_dynamic = 0;
	test111_length_dynamic = (test111_length_dynamic + 1) % test111_length;


	// RENDER to the screen
	// @todo: move into the render loop
	//        (clearing happens there already)

	// --- overlay, normalized coords, farthest
	{ // CAMERA_MODE_NONE
		// @todo: transform into screen coords
		float const target_scale_x   = (float)target_size_x / (float)size_x;
		float const target_scale_y   = (float)target_size_y / (float)size_y;
		float const target_scale_max = max_r32(target_scale_x, target_scale_y);
		float const scale_x = target_scale_x / target_scale_max; (void)scale_x;
		float const scale_y = target_scale_y / target_scale_max; (void)scale_y;

		batcher_2d_push_matrix(state.batcher, mat4_set_projection(
			(struct vec2){0, 0},
			(struct vec2){1, 1},
			0, 1, 1
		));

		batcher_2d_set_blend_mode(state.batcher, (struct Blend_Mode){.mask = COLOR_CHANNEL_FULL});
		batcher_2d_set_depth_mode(state.batcher, (struct Depth_Mode){0});

		// > the buffer
		batcher_2d_set_material(state.batcher, &state.materials.batcher);
		batcher_2d_set_texture(state.batcher, gpu_target_get_texture_ref(gpu_target_ref, TEXTURE_TYPE_COLOR, 0));
		batcher_2d_add_quad(
			state.batcher,
			(float[]){-scale_x, -scale_y, scale_x, scale_y},
			(float[]){0,0,1,1}
		);

		//
		batcher_2d_pop_matrix(state.batcher);
	}

	// --- overlay, screen coords, nearest
	{ // CAMERA_MODE_SCREEN
		batcher_2d_push_matrix(state.batcher, mat4_set_projection(
			(struct vec2){-1, -1},
			(struct vec2){2 / (float)size_x, 2 / (float)size_y},
			0, 1, 1
		));

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
		batcher_2d_set_material(state.batcher, &state.materials.batcher);
		batcher_2d_set_texture(state.batcher, font_open_sans->gpu_ref);
		batcher_2d_add_text(
			state.batcher,
			font_open_sans,
			text_length_dynamic,
			text_test->data,
			50, 200
		);

		// > text
		batcher_2d_set_material(state.batcher, &state.materials.batcher);
		batcher_2d_set_texture(state.batcher, font_open_sans->gpu_ref);
		batcher_2d_add_text(
			state.batcher,
			font_open_sans,
			test111_length_dynamic,
			test111,
			600, 200
		);

		// > glyphs texture
		struct Image const * font_image = font_image_get_asset(font_open_sans->buffer);

		batcher_2d_set_material(state.batcher, &state.materials.batcher);
		batcher_2d_set_texture(state.batcher, font_open_sans->gpu_ref);
		batcher_2d_add_quad(
			state.batcher,
			(float[]){0, (float)(size_y - font_image->size_y), (float)font_image->size_x, (float)size_y},
			(float[]){0,0,1,1}
		);

		//
		batcher_2d_pop_matrix(state.batcher);
	}

	//
	batcher_2d_draw(state.batcher, size_x, size_y, (struct Ref){0});
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
