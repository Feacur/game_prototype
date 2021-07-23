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

struct Entity {
	struct Transform_3D transform;
};

static struct Game_State {
	struct Asset_System asset_system;

	struct {
		struct Gfx_Material test;
		struct Gfx_Material batcher;
	} materials;

	struct Batcher_2D * batcher;
	struct Ref gpu_target_ref;

	struct {
		uint32_t camera;
		uint32_t color;
		uint32_t texture;
		uint32_t transform;
	} uniforms;

	struct Entity camera;
	struct Entity object;
} state;

static void game_init(void) {
	// init asset system
	{
		asset_system_init(&state.asset_system);

		// map extensions
		asset_system_map_extension(&state.asset_system, "shader", "glsl");
		asset_system_map_extension(&state.asset_system, "model",  "obj");
		asset_system_map_extension(&state.asset_system, "model",  "fbx");
		asset_system_map_extension(&state.asset_system, "image",  "png");
		asset_system_map_extension(&state.asset_system, "font",   "ttf");
		asset_system_map_extension(&state.asset_system, "font",   "otf");
		asset_system_map_extension(&state.asset_system, "text",   "txt");

		// -- Asset gpu program part
		asset_system_set_type(&state.asset_system, "shader", (struct Asset_Callbacks){
			.init = asset_shader_init,
			.free = asset_shader_free,
		}, sizeof(struct Asset_Shader));

		// -- Asset mesh part
		asset_system_set_type(&state.asset_system, "model", (struct Asset_Callbacks){
			.init = asset_model_init,
			.free = asset_model_free,
		}, sizeof(struct Asset_Model));

		// -- Asset texture part
		asset_system_set_type(&state.asset_system, "image", (struct Asset_Callbacks){
			.init = asset_image_init,
			.free = asset_image_free,
		}, sizeof(struct Asset_Image));

		// -- Asset font part
		asset_system_set_type(&state.asset_system, "font", (struct Asset_Callbacks){
			.init = asset_font_init,
			.free = asset_font_free,
		}, sizeof(struct Asset_Font));

		// -- Asset text part
		asset_system_set_type(&state.asset_system, "text", (struct Asset_Callbacks){
			.init = asset_text_init,
			.free = asset_text_free,
		}, sizeof(struct Asset_Text));
	}

	// prefetch some assets
	{
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
		struct Asset_Shader const * gpu_program_test = asset_system_find_instance(&state.asset_system, "assets/shaders/test.glsl");
		gfx_material_init(&state.materials.test, gpu_program_test->gpu_ref);

		struct Asset_Shader const * gpu_program_batcher = asset_system_find_instance(&state.asset_system, "assets/shaders/batcher_2d.glsl");
		gfx_material_init(&state.materials.batcher, gpu_program_batcher->gpu_ref);
	}

	// init target
	{
		state.gpu_target_ref = gpu_target_init(
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

	// init batch mesh
	{
		state.batcher = batcher_2d_init();
	}

	// init objects
	{
		// @todo: make entity assets
		// @todo: make an entities system
		state.camera = (struct Entity){
			.transform = {
				.position = (struct vec3){0, 3, -5},
				.scale = (struct vec3){1, 1, 1},
				.rotation = quat_set_radians((struct vec3){MATHS_TAU / 16, 0, 0}),
			},
		};

		state.object = (struct Entity){
			.transform = {
				.position = (struct vec3){0, 0, 0},
				.scale = (struct vec3){1, 1, 1},
				.rotation = quat_identity,
			},
		};
	}
}

static void game_free(void) {
	asset_system_free(&state.asset_system);

	gfx_material_free(&state.materials.test);
	gfx_material_free(&state.materials.batcher);

	gpu_target_free(state.gpu_target_ref);
	batcher_2d_free(state.batcher);

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

	state.object.transform.rotation = vec4_norm(quat_mul(state.object.transform.rotation, quat_set_radians(
		(struct vec3){0 * delta_time, 1 * delta_time, 0 * delta_time}
	)));
}

static void game_render(uint32_t size_x, uint32_t size_y) {
	uint32_t target_size_x, target_size_y;
	gpu_target_get_size(state.gpu_target_ref, &target_size_x, &target_size_y);

	struct Asset_Model const * mesh_cube = asset_system_find_instance(&state.asset_system, "assets/sandbox/cube.obj");
	struct Asset_Image const * texture_test = asset_system_find_instance(&state.asset_system, "assets/sandbox/test.png");
	struct Asset_Font const * font_open_sans = asset_system_find_instance(&state.asset_system, "assets/fonts/OpenSans-Regular.ttf");
	struct Asset_Text const * text_test = asset_system_find_instance(&state.asset_system, "assets/sandbox/test.txt");

	// render to target
	graphics_draw(&(struct Render_Pass){
		.target = state.gpu_target_ref,
		.blend_mode = {.mask = COLOR_CHANNEL_FULL},
		.depth_mode = {.enabled = true, .mask = true},
		//
		.clear_mask = TEXTURE_TYPE_COLOR | TEXTURE_TYPE_DEPTH,
		.clear_rgba = 0x303030ff,
	});

	// --- map to camera coords; draw transformed
	struct mat4 const test_camera = mat4_mul_mat(
		mat4_set_projection((struct vec2){0, 0}, (struct vec2){1, (float)target_size_x / (float)target_size_y}, 0.1f, 10, 0),
		mat4_set_inverse_transformation(state.camera.transform.position, state.camera.transform.scale, state.camera.transform.rotation)
	);
	gfx_material_set_float(&state.materials.test, state.uniforms.camera, 4*4, &test_camera.x.x);

	gfx_material_set_texture(&state.materials.test, state.uniforms.texture, 1, &texture_test->gpu_ref);
	gfx_material_set_float(&state.materials.test, state.uniforms.color, 4, &(struct vec4){0.2f, 0.6f, 1, 1}.x);

	struct mat4 const test_transform = mat4_set_transformation(state.object.transform.position, state.object.transform.scale, state.object.transform.rotation);
	gfx_material_set_float(&state.materials.test, state.uniforms.transform, 4*4, &test_transform.x.x);
	graphics_draw(&(struct Render_Pass){
		.target = state.gpu_target_ref,
		.blend_mode = {.mask = COLOR_CHANNEL_FULL},
		.depth_mode = {.enabled = true, .mask = true},
		//
		.material = &state.materials.test,
		.mesh = mesh_cube->gpu_ref,
	});


	// batch a quad with target texture
	// --- fully fit to normalized device coords; draw at the farthest point
	float const target_scale_x   = (float)target_size_x / (float)size_x;
	float const target_scale_y   = (float)target_size_y / (float)size_y;
	float const target_scale_max = max_r32(target_scale_x, target_scale_y);
	batcher_2d_push_matrix(state.batcher, mat4_set_projection(
		(struct vec2){0, 0},
		(struct vec2){target_scale_x / target_scale_max, target_scale_y / target_scale_max},
		0, 1, 1
	));

	batcher_2d_set_blend_mode(state.batcher, (struct Blend_Mode){.mask = COLOR_CHANNEL_FULL});
	batcher_2d_set_depth_mode(state.batcher, (struct Depth_Mode){0});

	batcher_2d_set_material(state.batcher, &state.materials.batcher);
	batcher_2d_set_texture(state.batcher, gpu_target_get_texture_ref(state.gpu_target_ref, TEXTURE_TYPE_COLOR, 0));

	batcher_2d_add_quad(
		state.batcher,
		(float[]){-1, -1, 1, 1},
		(float[]){0,0,1,1}
	);
	batcher_2d_pop_matrix(state.batcher);

	// batch some text and a texture
	// --- map to screen buffer coords; draw at the nearest point
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

	batcher_2d_set_material(state.batcher, &state.materials.batcher);
	batcher_2d_set_texture(state.batcher, font_open_sans->gpu_ref);


	//
	char const test111[] = "abcdefghigklmnopqrstuvwxyz\n0123456789\nABCDEFGHIGKLMNOPQRSTUVWXYZ";

	font_image_add_glyphs_from_text(font_open_sans->buffer, text_test->length, text_test->data);
	font_image_add_glyphs_from_text(font_open_sans->buffer, sizeof(test111) / (sizeof(*test111)) - 1, (uint8_t const *)test111);
	font_image_add_kerning_all(font_open_sans->buffer);
	font_image_render(font_open_sans->buffer);

	batcher_2d_add_text(
		state.batcher,
		font_open_sans,
		text_test->length,
		text_test->data,
		50, 200
	);

	batcher_2d_add_text(
		state.batcher,
		font_open_sans,
		sizeof(test111) / (sizeof(*test111)) - 1,
		(uint8_t const *)test111,
		600, 200
	);

	gpu_texture_update(font_open_sans->gpu_ref, font_image_get_asset(font_open_sans->buffer));

	struct Image const * font_image = font_image_get_asset(font_open_sans->buffer);
	batcher_2d_add_quad(
		state.batcher,
		(float[]){0, (float)(size_y - font_image->size_y), (float)font_image->size_x, (float)size_y},
		(float[]){0,0,1,1}
	);
	batcher_2d_pop_matrix(state.batcher);


	// draw batches
	graphics_draw(&(struct Render_Pass){
		.size_x = size_x, .size_y = size_y,
		.blend_mode = {.mask = COLOR_CHANNEL_FULL},
		.depth_mode = {.enabled = true, .mask = true},
		//
		.clear_mask = TEXTURE_TYPE_COLOR | TEXTURE_TYPE_DEPTH,
		.clear_rgba = 0x303030ff,
	});
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
