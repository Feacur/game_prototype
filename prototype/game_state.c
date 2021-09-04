#include "framework/containers/ref.h"

#include "framework/graphics/material.h"
#include "framework/graphics/gpu_misc.h"
#include "framework/graphics/gpu_objects.h"

#include "batcher_2d.h"
#include "components.h"
#include "object_camera.h"
#include "object_entity.h"
#include "asset_types.h"

#include <string.h>

//
#include "game_state.h"

struct Game_State state;

void state_init(void) {

	// init state
	{
		state = (struct Game_State){
			.batcher = batcher_2d_init(),
		};
		asset_system_init(&state.asset_system);
		array_any_init(&state.targets, sizeof(struct Ref));
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
		asset_system_map_extension(&state.asset_system, "json",   "json");

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

		asset_system_set_type(&state.asset_system, "json", (struct Asset_Callbacks){
			.init = asset_json_init,
			.free = asset_json_free,
		}, sizeof(struct Asset_JSON));
	}
}

void state_free(void) {
	batcher_2d_free(state.batcher);

	asset_system_free(&state.asset_system);

	for (uint32_t i = 0; i < state.materials.count; i++) {
		gfx_material_free(array_any_at(&state.materials, i));
	}

	array_any_free(&state.targets);
	array_any_free(&state.materials);
	array_any_free(&state.cameras);
	array_any_free(&state.entities);

	memset(&state, 0, sizeof(state));
}

static void state_read_json_init(void);
static void state_read_json_targets(struct JSON const * targets_json);
static void state_read_json_materials(struct JSON const * targets_json);

void state_read_json(struct JSON const * json) {
	state_read_json_init();

	struct JSON const * targets_json = json_object_get(json, "targets");
	state_read_json_targets(targets_json);

	struct JSON const * materials_json = json_object_get(json, "materials");
	state_read_json_materials(materials_json);
}

//

static uint32_t id_color_rgba_u8;
static uint32_t id_color_depth_r32;
static uint32_t id_image;
static uint32_t id_target;
static uint32_t id_font;
static uint32_t id_opaque;
static uint32_t id_transparent;
static uint32_t id_color;

static uint32_t uniforms_color;
static uint32_t uniforms_texture;

static void state_read_json_init(void) {
	id_color_rgba_u8 = json_system_add_string_id("color_rgba_u8");
	id_color_depth_r32 = json_system_add_string_id("color_depth_r32");
	id_image = json_system_add_string_id("image");
	id_target = json_system_add_string_id("target");
	id_font = json_system_add_string_id("font");
	id_opaque = json_system_add_string_id("opaque");
	id_transparent = json_system_add_string_id("transparent");
	id_color = json_system_add_string_id("color");

	uniforms_color = graphics_add_uniform_id("u_Color");
	uniforms_texture = graphics_add_uniform_id("u_Texture");
}

static void state_read_json_target_buffer(struct JSON const * buffer_json, struct Array_Any * parameters) {
	if (buffer_json->type != JSON_OBJECT) { DEBUG_BREAK(); return; }

	uint32_t const buffer_type = json_as_string_id(json_object_get(buffer_json, "type"));
	bool const buffer_read = json_as_boolean(json_object_get(buffer_json, "read"), false);

	if (buffer_type == id_color_rgba_u8) {
		array_any_push(parameters, &(struct Texture_Parameters) {
			.texture_type = TEXTURE_TYPE_COLOR,
			.data_type = DATA_TYPE_U8,
			.channels = 4,
			.flags = buffer_read ? TEXTURE_FLAG_READ : TEXTURE_FLAG_NONE,
		});
		return;
	}

	if (buffer_type == id_color_depth_r32) {
		array_any_push(parameters, &(struct Texture_Parameters) {
			.texture_type = TEXTURE_TYPE_DEPTH,
			.data_type = DATA_TYPE_R32,
			.flags = buffer_read ? TEXTURE_FLAG_READ : TEXTURE_FLAG_NONE,
		});
		return;
	}
}

static void state_read_json_target(struct JSON const * target_json, struct Array_Any * parameters) {
	if (target_json->type != JSON_OBJECT) { DEBUG_BREAK(); return; }

	struct JSON const * buffers = json_object_get(target_json, "buffers");
	if (buffers->type != JSON_ARRAY) { DEBUG_BREAK(); return; }

	parameters->count = 0;
	uint32_t const buffers_count = json_array_count(buffers);
	for (uint32_t buffer_i = 0; buffer_i < buffers_count; buffer_i++) {
		struct JSON const * buffer_json = json_array_at(buffers, buffer_i);
		state_read_json_target_buffer(buffer_json, parameters);
	}

	if (parameters->count == 0) { return; }

	uint32_t const target_size_x = (uint32_t)json_as_number(json_object_get(target_json, "size_x"), 320);
	uint32_t const target_size_y = (uint32_t)json_as_number(json_object_get(target_json, "size_y"), 180);

	array_any_push(&state.targets, (struct Ref[]){
		gpu_target_init(target_size_x, target_size_y, array_any_at(parameters, 0), parameters->count)
	});
}

//

static void state_read_json_material_color(struct JSON const * uniform_json, struct Gfx_Material * material) {
	// if (uniform_json->type != JSON_ARRAY) { DEBUG_BREAK(); return; }

	float color[4] = {1, 1, 1, 1};
	// uint32_t const color_count = json_array_count(materials);
	for (uint32_t color_i = 0; color_i < 4; color_i++) {
		color[color_i] = json_as_number(json_array_at(uniform_json, color_i), 1);
	}
	gfx_material_set_float(material, uniforms_color, 4, color);
}

static void state_read_json_uniform_image(struct JSON const * uniform_json, struct Gfx_Material * material) {
	if (uniform_json->type != JSON_OBJECT) { DEBUG_BREAK(); return; }

	char const * image_path = json_as_string(json_object_get(uniform_json, "path"), NULL);
	if (image_path == NULL) { DEBUG_BREAK(); return; }

	struct Asset_Image const * asset_image = asset_system_find_instance(&state.asset_system, image_path);
	if (asset_image == NULL) { DEBUG_BREAK(); return; }

	gfx_material_set_texture(material, uniforms_texture, 1, &asset_image->gpu_ref);
}

static void state_read_json_uniform_target(struct JSON const * uniform_json, struct Gfx_Material * material) {
	if (uniform_json->type != JSON_OBJECT) { DEBUG_BREAK(); return; }

	uint32_t const id = (uint32_t)json_as_number(json_object_get(uniform_json, "id"), UINT16_MAX);
	if (id == UINT16_MAX) { DEBUG_BREAK(); return; }

	struct Ref const * gpu_target = array_any_at(&state.targets, id);
	if (gpu_target == NULL) { DEBUG_BREAK(); return; }

	uint32_t const target_type = json_as_string_id(json_object_get(uniform_json, "type"));

	struct Ref texture_ref;
	if (target_type == id_color) {
		texture_ref = gpu_target_get_texture_ref(*gpu_target, TEXTURE_TYPE_COLOR, 0);
	}
	else {
		return;
	}

	gfx_material_set_texture(material, uniforms_texture, 1, &texture_ref);
}

static void state_read_json_uniform_font(struct JSON const * uniform_json, struct Gfx_Material * material) {
	if (uniform_json->type != JSON_OBJECT) { DEBUG_BREAK(); return; }

	char const * font_path = json_as_string(json_object_get(uniform_json, "path"), NULL);
	if (font_path == NULL) { DEBUG_BREAK(); return; }

	struct Asset_Font const * asset_font = asset_system_find_instance(&state.asset_system, font_path);
	if (asset_font == NULL) { DEBUG_BREAK(); return; }

	gfx_material_set_texture(material, uniforms_texture, 1, &asset_font->gpu_ref);
}

static void state_read_json_uniform_texture(struct JSON const * uniform_json, struct Gfx_Material * material) {
	if (uniform_json->type != JSON_OBJECT) { DEBUG_BREAK(); return; }

	uint32_t const source_type = json_as_string_id(json_object_get(uniform_json, "source_type"));

	if (source_type == id_image) {
		state_read_json_uniform_image(uniform_json, material);
		return;
	}

	if (source_type == id_target) {
		state_read_json_uniform_target(uniform_json, material);
		return;
	}

	if (source_type == id_font) {
		state_read_json_uniform_font(uniform_json, material);
		return;
	}
}

static void state_read_json_material(struct JSON const * material_json) {
	if (material_json->type != JSON_OBJECT) { DEBUG_BREAK(); return; }

	char const * shader_path = json_as_string(json_object_get(material_json, "shader"), NULL);
	if (shader_path == NULL) { DEBUG_BREAK(); return; }

	struct Asset_Shader const * asset_shader = asset_system_find_instance(&state.asset_system, shader_path);
	if (asset_shader == NULL) { DEBUG_BREAK(); return; }

	array_any_push(&state.materials, &(struct Gfx_Material){0});
	struct Gfx_Material * material = array_any_at(&state.materials, state.materials.count - 1);

	uint32_t const blend_mode_json_value = json_as_string_id(json_object_get(material_json, "mode"));

	struct Blend_Mode blend_mode;
	if (blend_mode_json_value == id_opaque) {
		blend_mode = blend_mode_opaque;
	}
	else if (blend_mode_json_value == id_transparent) {
		blend_mode = blend_mode_transparent;
	}
	else {
		blend_mode = blend_mode_opaque;
	}

	bool const depth_mode_json_value = json_as_boolean(json_object_get(material_json, "depth"), true);

	struct Depth_Mode depth_mode;
	if (depth_mode_json_value) {
		depth_mode = (struct Depth_Mode){.enabled = true, .mask = true};
	}
	else {
		depth_mode = (struct Depth_Mode){0};
	}

	gfx_material_init(
		material, asset_shader->gpu_ref,
		&blend_mode, &depth_mode
	);

	// @todo: cycle through shader uniforms
	{
		struct JSON const * uniform_json = json_object_get(material_json, "u_Color");
		state_read_json_material_color(uniform_json, material);
	}

	{
		struct JSON const * uniform_json = json_object_get(material_json, "u_Texture");
		state_read_json_uniform_texture(uniform_json, material);
	}
}

//

static void state_read_json_targets(struct JSON const * targets_json) {
	if (targets_json->type != JSON_ARRAY) { DEBUG_BREAK(); return; }

	struct Array_Any parameters;
	array_any_init(&parameters, sizeof(struct Texture_Parameters));

	uint32_t const targets_count = json_array_count(targets_json);
	for (uint32_t target_i = 0; target_i < targets_count; target_i++) {
		struct JSON const * target_json = json_array_at(targets_json, target_i);
		state_read_json_target(target_json, &parameters);
	}

	array_any_free(&parameters);
}

static void state_read_json_materials(struct JSON const * materials_json) {
	if (materials_json->type != JSON_ARRAY) { DEBUG_BREAK(); return; }

	uint32_t const materials_count = json_array_count(materials_json);
	for (uint32_t material_i = 0; material_i < materials_count; material_i++) {
		struct JSON const * material_json = json_array_at(materials_json, material_i);
		state_read_json_material(material_json);
	}
}
