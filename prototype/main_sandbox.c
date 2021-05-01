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
#include "framework/graphics/font_image.h"

#include "application/application.h"

#include "framework/containers/array_byte.h"
#include "framework/containers/ref_table.h"

#include "framework/assets/asset_mesh.h"
#include "framework/assets/asset_image.h"
#include "framework/assets/asset_font.h"

#include "application/application.h"
#include "components.h"
#include "batcher_2d.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static struct Game_Uniforms {
	uint32_t camera;
	uint32_t color;
	uint32_t texture;
	uint32_t transform;
} uniforms;

struct Game_Font {
	struct Font_Image * buffer;
	struct Gpu_Texture * gpu_texture;
};

static struct Game_Content {
	struct {
		struct Asset_Font * font_sans;
		struct Asset_Font * font_mono;
		struct Array_Byte text_test;
	} assets;
	//
	struct {
		struct Gpu_Program * program_test;
		struct Gpu_Program * program_batcher;
		struct Gpu_Texture * texture_test;
		struct Ref cube_gpu_mesh_ref;
	} gpu;
	//
	struct {
		struct Game_Font sans, mono;
	} fonts;
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
		content.assets.font_sans = asset_font_init("assets/fonts/OpenSans-Regular.ttf");
		content.assets.font_mono = asset_font_init("assets/fonts/JetBrainsMono-Regular.ttf");

		platform_file_read_entire("assets/sandbox/test.txt", &content.assets.text_test);
		content.assets.text_test.data[content.assets.text_test.count] = '\0';

		struct Array_Byte asset_shader_test;
		platform_file_read_entire("assets/shaders/test.glsl", &asset_shader_test);

		struct Array_Byte asset_shader_batcher;
		platform_file_read_entire("assets/shaders/batcher_2d.glsl", &asset_shader_batcher);

		struct Asset_Image asset_image_test;
		asset_image_init(&asset_image_test, "assets/sandbox/test.png");

		struct Asset_Mesh asset_mesh_cube;
		asset_mesh_init(&asset_mesh_cube, "assets/sandbox/cube.obj");

		content.gpu.program_test = gpu_program_init(&asset_shader_test);
		content.gpu.program_batcher = gpu_program_init(&asset_shader_batcher);
		content.gpu.texture_test = gpu_texture_init(&asset_image_test);
		content.gpu.cube_gpu_mesh_ref = gpu_mesh_init(&asset_mesh_cube);

		array_byte_free(&asset_shader_test);
		array_byte_free(&asset_shader_batcher);
		asset_image_free(&asset_image_test);
		asset_mesh_free(&asset_mesh_cube);

		struct Array_Byte asset_codepoints;
		platform_file_read_entire("assets/sandbox/additional_codepoints_french.txt", &asset_codepoints);
		asset_codepoints.data[asset_codepoints.count] = '\0';

		uint32_t codepoints_count = 0;
		uint32_t * codepoints = MEMORY_ALLOCATE_ARRAY(NULL, uint32_t, (asset_codepoints.count + 4) * 2 + 1);
		codepoints[codepoints_count++] = ' ';
		codepoints[codepoints_count++] = '~';
		codepoints[codepoints_count++] = 0x401;
		codepoints[codepoints_count++] = 0x401;
		codepoints[codepoints_count++] = 0x410;
		codepoints[codepoints_count++] = 0x44f;
		codepoints[codepoints_count++] = 0x451;
		codepoints[codepoints_count++] = 0x451;
		for (size_t i = 0; i < asset_codepoints.count;) {
			uint32_t const octets_count = utf8_length(asset_codepoints.data + i);
			if (octets_count == 0) { i++; continue; }

			uint32_t const codepoint = utf8_decode(asset_codepoints.data + i, octets_count);
			if (codepoint == CODEPOINT_EMPTY) { i++; continue; }

			i += octets_count;

			if (codepoint <= 0x7f) { continue; }
			codepoints[codepoints_count++] = codepoint;
			codepoints[codepoints_count++] = codepoint;
		}

		array_byte_free(&asset_codepoints);

		content.fonts.sans.buffer = font_image_init(content.assets.font_sans, 32);
		font_image_build(content.fonts.sans.buffer, codepoints_count / 2, codepoints);
		content.fonts.sans.gpu_texture = gpu_texture_init(font_image_get_asset(content.fonts.sans.buffer));

		MEMORY_FREE(NULL, codepoints);

		content.fonts.mono.buffer = font_image_init(content.assets.font_mono, 32);
		font_image_build(content.fonts.mono.buffer, 1, (uint32_t[]){' ', '~'});
		content.fonts.mono.gpu_texture = gpu_texture_init(font_image_get_asset(content.fonts.mono.buffer));

		//
		gfx_material_init(&content.materials.test, content.gpu.program_test);
		gfx_material_init(&content.materials.batcher, content.gpu.program_batcher);
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
	asset_font_free(content.assets.font_sans);
	asset_font_free(content.assets.font_mono);
	array_byte_free(&content.assets.text_test);

	gpu_program_free(content.gpu.program_test);
	gpu_program_free(content.gpu.program_batcher);
	gpu_texture_free(content.gpu.texture_test);
	gfx_material_free(&content.materials.test);
	gfx_material_free(&content.materials.batcher);

	font_image_free(content.fonts.sans.buffer);
	gpu_texture_free(content.fonts.sans.gpu_texture);

	font_image_free(content.fonts.mono.buffer);
	gpu_texture_free(content.fonts.mono.gpu_texture);

	gpu_target_free(target.gpu_target_ref);

	batcher_2d_free(batcher);

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

	gfx_material_set_texture(&content.materials.test, uniforms.texture, 1, &content.gpu.texture_test);
	gfx_material_set_float(&content.materials.test, uniforms.color, 4, &(struct vec4){0.2f, 0.6f, 1, 1}.x);

	struct mat4 const test_transform = mat4_set_transformation(state.object.transform.position, state.object.transform.scale, state.object.transform.rotation);
	gfx_material_set_float(&content.materials.test, uniforms.transform, 4*4, &test_transform.x.x);
	graphics_draw(&(struct Render_Pass){
		.target = target.gpu_target_ref,
		.blend_mode = {.mask = COLOR_CHANNEL_FULL},
		.depth_mode = {.enabled = true, .mask = true},
		//
		.material = &content.materials.test,
		.mesh = content.gpu.cube_gpu_mesh_ref,
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
	batcher_2d_set_texture(batcher, gpu_target_get_texture(target.gpu_target_ref, TEXTURE_TYPE_COLOR, 0));

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

	struct Game_Font * font = &content.fonts.sans;

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
	batcher_2d_set_texture(batcher, font->gpu_texture);

	batcher_2d_add_text(
		batcher,
		font->buffer,
		content.assets.text_test.count,
		content.assets.text_test.data,
		50, 200
	);

	struct Asset_Image const * font_image = font_image_get_asset(font->buffer);
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
