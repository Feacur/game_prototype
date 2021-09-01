#include "framework/containers/ref.h"

#include "framework/graphics/material.h"
#include "framework/graphics/gpu_misc.h"

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
		array_any_init(&state.materials, sizeof(struct Gfx_Material));
		array_any_init(&state.targets, sizeof(struct Ref));
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

	array_any_free(&state.materials);
	array_any_free(&state.targets);
	array_any_free(&state.cameras);
	array_any_free(&state.entities);

	memset(&state, 0, sizeof(state));
}
