#include "framework/memory.h"
#include "framework/unicode.h"
#include "framework/platform_file.h"

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

#include "application/application.h"
#include "components.h"
#include "batcher_2d.h"
#include "asset_system.h"
#include "asset_types.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static struct Game_Uniforms {
	uint32_t camera;
	uint32_t color;
	uint32_t texture;
	uint32_t transform;
} uniforms;

static struct Game_Content {
	struct Asset_System asset_system;

	struct {
		struct Array_Byte text_test;
	} assets;
	//
	struct {
		struct Gfx_Material test;
		struct Gfx_Material batcher;
	} materials;
} content;

static struct Game_Target {
	struct Ref gpu_target_ref;
} target;

static struct Batcher_2D * batcher = NULL;

struct Entity {
	struct Transform_3D transform;
};

static struct Game_State {
	struct Entity camera;
	struct Entity object;
} state;

static void game_init(void) {
	{
		asset_system_init(&content.asset_system);

		// -- Asset gpu program part
		asset_system_set_type(&content.asset_system, "glsl", (struct Asset_Callbacks){
			.init = asset_shader_init,
			.free = asset_shader_free,
		}, sizeof(struct Asset_Shader));

		asset_system_aquire(&content.asset_system, "assets/shaders/test.glsl");
		asset_system_aquire(&content.asset_system, "assets/shaders/batcher_2d.glsl");

		// -- Asset font part
		asset_system_set_type(&content.asset_system, "ttf", (struct Asset_Callbacks){
			.init = asset_font_init,
			.free = asset_font_free,
		}, sizeof(struct Asset_Font));

		asset_system_aquire(&content.asset_system, "assets/fonts/OpenSans-Regular.ttf");
		asset_system_aquire(&content.asset_system, "assets/fonts/JetBrainsMono-Regular.ttf");

		// -- Asset mesh part
		asset_system_set_type(&content.asset_system, "obj", (struct Asset_Callbacks){
			.init = asset_model_init,
			.free = asset_model_free,
		}, sizeof(struct Asset_Model));

		asset_system_aquire(&content.asset_system, "assets/sandbox/cube.obj");

		// -- Asset texture part
		asset_system_set_type(&content.asset_system, "png", (struct Asset_Callbacks){
			.init = asset_image_init,
			.free = asset_image_free,
		}, sizeof(struct Asset_Image));

		asset_system_aquire(&content.asset_system, "assets/sandbox/test.png");
	}

	{
		struct Ref_Table ref_table;
		ref_table_init(&ref_table, sizeof(float));
		struct Ref refs[20];
		for (uint32_t i = 0; i < 20; i++) {
			float value = (float)((i + 1) * 10 + i + 1);
			refs[i] = ref_table_aquire(&ref_table, &value);
		}
		ref_table_discard(&ref_table, refs[2]);
		ref_table_discard(&ref_table, refs[4]);
		ref_table_discard(&ref_table, refs[6]);
		ref_table_aquire(&ref_table, &(float){500});
		ref_table_aquire(&ref_table, &(float){600});
		ref_table_discard(&ref_table, refs[8]);
		for (uint32_t i = 0; i < ref_table_get_count(&ref_table); i++) {
			printf("%f\n", (double)*(float *)ref_table_value_at(&ref_table, i));
		}
		ref_table_free(&ref_table);
	}

	// init uniforms ids
	uniforms.color = graphics_add_uniform("u_Color");
	uniforms.texture = graphics_add_uniform("u_Texture");
	uniforms.camera = graphics_add_uniform("u_Camera");
	uniforms.transform = graphics_add_uniform("u_Transform");

	// load content
	{
		platform_file_read_entire("assets/sandbox/test.txt", &content.assets.text_test);
		content.assets.text_test.data[content.assets.text_test.count] = '\0';

		//
		struct Asset_Shader const * gpu_program_test = asset_system_get_instance(&content.asset_system, asset_system_aquire(&content.asset_system, "assets/shaders/test.glsl"));
		gfx_material_init(&content.materials.test, gpu_program_test->gpu_ref);

		struct Asset_Shader const * gpu_program_batcher = asset_system_get_instance(&content.asset_system, asset_system_aquire(&content.asset_system, "assets/shaders/batcher_2d.glsl"));
		gfx_material_init(&content.materials.batcher, gpu_program_batcher->gpu_ref);
	}

	// init target
	{
		target.gpu_target_ref = gpu_target_init(
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
	batcher = batcher_2d_init();

	// init state
	{
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
	array_byte_free(&content.assets.text_test);

	gfx_material_free(&content.materials.test);
	gfx_material_free(&content.materials.batcher);

	gpu_target_free(target.gpu_target_ref);

	batcher_2d_free(batcher);

	asset_system_free(&content.asset_system);

	batcher = NULL;
	memset(&uniforms, 0, sizeof(uniforms));
	memset(&content, 0, sizeof(content));
	memset(&target, 0, sizeof(target));
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
		printf("delta: %d %d\n", x, y);
	}

	state.object.transform.rotation = vec4_norm(quat_mul(state.object.transform.rotation, quat_set_radians(
		(struct vec3){0 * delta_time, 1 * delta_time, 0 * delta_time}
	)));
}

static void game_render(uint32_t size_x, uint32_t size_y) {
	uint32_t target_size_x, target_size_y;
	gpu_target_get_size(target.gpu_target_ref, &target_size_x, &target_size_y);

	struct Asset_Model const * mesh_cube = asset_system_get_instance(&content.asset_system, asset_system_aquire(&content.asset_system, "assets/sandbox/cube.obj"));
	struct Asset_Image const * texture_test = asset_system_get_instance(&content.asset_system, asset_system_aquire(&content.asset_system, "assets/sandbox/test.png"));
	struct Asset_Font const * font = asset_system_get_instance(&content.asset_system, asset_system_aquire(&content.asset_system, "assets/fonts/OpenSans-Regular.ttf"));

	// render to target
	graphics_draw(&(struct Render_Pass){
		.target = target.gpu_target_ref,
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
	gfx_material_set_float(&content.materials.test, uniforms.camera, 4*4, &test_camera.x.x);

	gfx_material_set_texture(&content.materials.test, uniforms.texture, 1, &texture_test->gpu_ref);
	gfx_material_set_float(&content.materials.test, uniforms.color, 4, &(struct vec4){0.2f, 0.6f, 1, 1}.x);

	struct mat4 const test_transform = mat4_set_transformation(state.object.transform.position, state.object.transform.scale, state.object.transform.rotation);
	gfx_material_set_float(&content.materials.test, uniforms.transform, 4*4, &test_transform.x.x);
	graphics_draw(&(struct Render_Pass){
		.target = target.gpu_target_ref,
		.blend_mode = {.mask = COLOR_CHANNEL_FULL},
		.depth_mode = {.enabled = true, .mask = true},
		//
		.material = &content.materials.test,
		.mesh = mesh_cube->gpu_ref,
	});


	// batch a quad with target texture
	// --- fully fit to normalized device coords; draw at the farthest point
	float const target_scale_x   = (float)target_size_x / (float)size_x;
	float const target_scale_y   = (float)target_size_y / (float)size_y;
	float const target_scale_max = max_r32(target_scale_x, target_scale_y);
	batcher_2d_push_matrix(batcher, mat4_set_projection(
		(struct vec2){0, 0},
		(struct vec2){target_scale_x / target_scale_max, target_scale_y / target_scale_max},
		0, 1, 1
	));

	batcher_2d_set_blend_mode(batcher, (struct Blend_Mode){.mask = COLOR_CHANNEL_FULL});
	batcher_2d_set_depth_mode(batcher, (struct Depth_Mode){0});

	batcher_2d_set_material(batcher, &content.materials.batcher);
	batcher_2d_set_texture(batcher, gpu_target_get_texture_ref(target.gpu_target_ref, TEXTURE_TYPE_COLOR, 0));

	batcher_2d_add_quad(
		batcher,
		(float[]){-1, -1, 1, 1},
		(float[]){0,0,1,1}
	);
	batcher_2d_pop_matrix(batcher);

	// batch some text and a texture
	// --- map to screen buffer coords; draw at the nearest point
	batcher_2d_push_matrix(batcher, mat4_set_projection(
		(struct vec2){-1, -1},
		(struct vec2){2 / (float)size_x, 2 / (float)size_y},
		0, 1, 1
	));

	batcher_2d_set_blend_mode(batcher, (struct Blend_Mode){
		.rgb = {
			.op = BLEND_OP_ADD,
			.src = BLEND_FACTOR_SRC_ALPHA,
			.dst = BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		},
		.mask = COLOR_CHANNEL_FULL
	});
	batcher_2d_set_depth_mode(batcher, (struct Depth_Mode){0});

	batcher_2d_set_material(batcher, &content.materials.batcher);
	batcher_2d_set_texture(batcher, font->gpu_ref);

	batcher_2d_add_text(
		batcher,
		font->buffer,
		content.assets.text_test.count,
		content.assets.text_test.data,
		50, 200
	);

	struct Image const * font_image = font_image_get_asset(font->buffer);
	batcher_2d_add_quad(
		batcher,
		(float[]){0, (float)(size_y - font_image->size_y), (float)font_image->size_x, (float)size_y},
		(float[]){0,0,1,1}
	);
	batcher_2d_pop_matrix(batcher);


	// draw batches
	graphics_draw(&(struct Render_Pass){
		.size_x = size_x, .size_y = size_y,
		.blend_mode = {.mask = COLOR_CHANNEL_FULL},
		.depth_mode = {.enabled = true, .mask = true},
		//
		.clear_mask = TEXTURE_TYPE_COLOR | TEXTURE_TYPE_DEPTH,
		.clear_rgba = 0x303030ff,
	});
	batcher_2d_draw(batcher, size_x, size_y, (struct Ref){0});
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
