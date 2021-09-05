#include "framework/containers/ref.h"

#include "framework/containers/strings.h"
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
			.type_init = asset_json_type_init,
			.type_free = asset_json_type_free,
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

static void state_read_json_init(struct Strings * strings);
static void state_read_json_targets(struct JSON const * json, struct Strings * strings);
static void state_read_json_materials(struct JSON const * json, struct Strings * strings);

void state_read_json(struct JSON const * json, struct Strings * strings) {
	state_read_json_init(strings);

	struct JSON const * targets_json = json_object_get(json, strings, "targets");
	state_read_json_targets(targets_json, strings);

	struct JSON const * materials_json = json_object_get(json, strings, "materials");
	state_read_json_materials(materials_json, strings);
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

static uint32_t json_system_add_string_id(struct Strings * strings, char const * name) {
	return strings_add(strings, (uint32_t)strlen(name), name);
}

static void state_read_json_init(struct Strings * strings) {
	id_color_rgba_u8 = json_system_add_string_id(strings, "color_rgba_u8");
	id_color_depth_r32 = json_system_add_string_id(strings, "color_depth_r32");
	id_image = json_system_add_string_id(strings, "image");
	id_target = json_system_add_string_id(strings, "target");
	id_font = json_system_add_string_id(strings, "font");
	id_opaque = json_system_add_string_id(strings, "opaque");
	id_transparent = json_system_add_string_id(strings, "transparent");
	id_color = json_system_add_string_id(strings, "color");

	uniforms_color = graphics_add_uniform_id("u_Color");
	uniforms_texture = graphics_add_uniform_id("u_Texture");
}

static void state_read_json_target_buffer(struct JSON const * json, struct Strings * strings, struct Array_Any * parameters) {
	if (json->type != JSON_OBJECT) { DEBUG_BREAK(); return; }

	uint32_t const type_id = json_as_string_id(json_object_get(json, strings, "type"));
	if (type_id == INDEX_EMPTY) { DEBUG_BREAK(); return; }

	bool const buffer_read = json_as_boolean(json_object_get(json, strings, "read"), false);

	if (type_id == id_color_rgba_u8) {
		array_any_push(parameters, &(struct Texture_Parameters) {
			.texture_type = TEXTURE_TYPE_COLOR,
			.data_type = DATA_TYPE_U8,
			.channels = 4,
			.flags = buffer_read ? TEXTURE_FLAG_READ : TEXTURE_FLAG_NONE,
		});
		return;
	}

	if (type_id == id_color_depth_r32) {
		array_any_push(parameters, &(struct Texture_Parameters) {
			.texture_type = TEXTURE_TYPE_DEPTH,
			.data_type = DATA_TYPE_R32,
			.flags = buffer_read ? TEXTURE_FLAG_READ : TEXTURE_FLAG_NONE,
		});
		return;
	}
}

static void state_read_json_target(struct JSON const * json, struct Strings * strings, struct Array_Any * parameters) {
	if (json->type != JSON_OBJECT) { DEBUG_BREAK(); return; }

	struct JSON const * buffers_json = json_object_get(json, strings, "buffers");
	if (buffers_json->type != JSON_ARRAY) { DEBUG_BREAK(); return; }

	parameters->count = 0;
	uint32_t const buffers_count = json_array_count(buffers_json);
	for (uint32_t i = 0; i < buffers_count; i++) {
		struct JSON const * buffer_json = json_array_at(buffers_json, i);
		state_read_json_target_buffer(buffer_json, strings, parameters);
	}

	if (parameters->count == 0) { DEBUG_BREAK(); return; }

	uint32_t const size_x = (uint32_t)json_as_number(json_object_get(json, strings, "size_x"), 320);
	uint32_t const size_y = (uint32_t)json_as_number(json_object_get(json, strings, "size_y"), 180);

	array_any_push(&state.targets, (struct Ref[]){
		gpu_target_init(size_x, size_y, array_any_at(parameters, 0), parameters->count)
	});
}

//

static void state_read_json_material_color(struct JSON const * json, struct Strings * strings, struct Gfx_Material * material) {
	(void)strings;
	// if (uniform_json->type != JSON_ARRAY) { DEBUG_BREAK(); return; }

	float const color[4] = {
		json_as_number(json_array_at(json, 0), 1),
		json_as_number(json_array_at(json, 1), 1),
		json_as_number(json_array_at(json, 2), 1),
		json_as_number(json_array_at(json, 3), 1),
	};
	gfx_material_set_float(material, uniforms_color, 4, color);
}

static void state_read_json_uniform_image(struct JSON const * json, struct Strings * strings, struct Gfx_Material * material) {
	if (json->type != JSON_OBJECT) { DEBUG_BREAK(); return; }

	char const * path = json_as_string(json_object_get(json, strings, "path"), strings, NULL);
	if (path == NULL) { DEBUG_BREAK(); return; }

	struct Asset_Image const * asset = asset_system_find_instance(&state.asset_system, path);
	if (asset == NULL) { DEBUG_BREAK(); return; }

	gfx_material_set_texture(material, uniforms_texture, 1, &asset->gpu_ref);
}

static void state_read_json_uniform_target(struct JSON const * json, struct Strings * strings, struct Gfx_Material * material) {
	if (json->type != JSON_OBJECT) { DEBUG_BREAK(); return; }

	uint32_t const id = (uint32_t)json_as_number(json_object_get(json, strings, "id"), UINT16_MAX);
	if (id == UINT16_MAX) { DEBUG_BREAK(); return; }

	struct Ref const * gpu_target = array_any_at(&state.targets, id);
	if (gpu_target == NULL) { DEBUG_BREAK(); return; }

	uint32_t const buffer_type_id = json_as_string_id(json_object_get(json, strings, "buffer_type"));
	if (buffer_type_id == INDEX_EMPTY) { DEBUG_BREAK(); return; }

	if (buffer_type_id == id_color) {
		struct Ref const texture_ref = gpu_target_get_texture_ref(*gpu_target, TEXTURE_TYPE_COLOR, 0);
		if (texture_ref.id == ref_empty.id) { DEBUG_BREAK(); }

		gfx_material_set_texture(material, uniforms_texture, 1, &texture_ref);
		return;
	}

	DEBUG_BREAK();
}

static void state_read_json_uniform_font(struct JSON const * json, struct Strings * strings, struct Gfx_Material * material) {
	if (json->type != JSON_OBJECT) { DEBUG_BREAK(); return; }

	char const * path = json_as_string(json_object_get(json, strings, "path"), strings, NULL);
	if (path == NULL) { DEBUG_BREAK(); return; }

	struct Asset_Font const * asset = asset_system_find_instance(&state.asset_system, path);
	if (asset == NULL) { DEBUG_BREAK(); return; }

	gfx_material_set_texture(material, uniforms_texture, 1, &asset->gpu_ref);
}

static void state_read_json_uniform_texture(struct JSON const * json, struct Strings * strings, struct Gfx_Material * material) {
	if (json->type != JSON_OBJECT) { DEBUG_BREAK(); return; }

	uint32_t const type_id = json_as_string_id(json_object_get(json, strings, "type"));
	if (type_id == INDEX_EMPTY) { DEBUG_BREAK(); return; }

	if (type_id == id_image) {
		state_read_json_uniform_image(json, strings, material);
		return;
	}

	if (type_id == id_target) {
		state_read_json_uniform_target(json, strings, material);
		return;
	}

	if (type_id == id_font) {
		state_read_json_uniform_font(json, strings, material);
		return;
	}
}

static void state_read_json_material(struct JSON const * json, struct Strings * strings) {
	if (json->type != JSON_OBJECT) { DEBUG_BREAK(); return; }

	char const * path = json_as_string(json_object_get(json, strings, "shader"), strings, NULL);
	if (path == NULL) { DEBUG_BREAK(); return; }

	struct Asset_Shader const * asset = asset_system_find_instance(&state.asset_system, path);
	if (asset == NULL) { DEBUG_BREAK(); return; }

	array_any_push(&state.materials, &(struct Gfx_Material){0});
	struct Gfx_Material * material = array_any_at(&state.materials, state.materials.count - 1);

	uint32_t const mode_id = json_as_string_id(json_object_get(json, strings, "mode"));
	if (mode_id == INDEX_EMPTY) { DEBUG_BREAK(); return; }

	struct Blend_Mode blend_mode;
	if (mode_id == id_opaque) {
		blend_mode = blend_mode_opaque;
	}
	else if (mode_id == id_transparent) {
		blend_mode = blend_mode_transparent;
	}
	else {
		blend_mode = blend_mode_opaque;
	}

	bool const depth_write = json_as_boolean(json_object_get(json, strings, "depth"), true);

	struct Depth_Mode depth_mode;
	if (depth_write) {
		depth_mode = (struct Depth_Mode){.enabled = true, .mask = true};
	}
	else {
		depth_mode = (struct Depth_Mode){0};
	}

	gfx_material_init(
		material, asset->gpu_ref,
		&blend_mode, &depth_mode
	);

	// @todo: cycle through shader uniforms
	{
		struct JSON const * uniform_json = json_object_get(json, strings, "u_Color");
		state_read_json_material_color(uniform_json, strings, material);
	}

	{
		struct JSON const * uniform_json = json_object_get(json, strings, "u_Texture");
		state_read_json_uniform_texture(uniform_json, strings, material);
	}
}

//

static void state_read_json_targets(struct JSON const * json, struct Strings * strings) {
	if (json->type != JSON_ARRAY) { DEBUG_BREAK(); return; }

	struct Array_Any parameters;
	array_any_init(&parameters, sizeof(struct Texture_Parameters));

	uint32_t const targets_count = json_array_count(json);
	for (uint32_t i = 0; i < targets_count; i++) {
		struct JSON const * target_json = json_array_at(json, i);
		state_read_json_target(target_json, strings, &parameters);
	}

	array_any_free(&parameters);
}

static void state_read_json_materials(struct JSON const * json, struct Strings * strings) {
	if (json->type != JSON_ARRAY) { DEBUG_BREAK(); return; }

	uint32_t const materials_count = json_array_count(json);
	for (uint32_t i = 0; i < materials_count; i++) {
		struct JSON const * material_json = json_array_at(json, i);
		state_read_json_material(material_json, strings);
	}
}
