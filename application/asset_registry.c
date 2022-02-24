#include "framework/memory.h"
#include "framework/logger.h"
#include "framework/maths.h"
#include "framework/platform_file.h"
#include "framework/containers/buffer.h"
#include "framework/containers/strings.h"
#include "framework/systems/asset_system.h"
#include "framework/graphics/gpu_objects.h"
#include "framework/graphics/gpu_misc.h"
#include "framework/graphics/font_image.h"

#include "framework/assets/font.h"
#include "framework/assets/mesh.h"
#include "framework/assets/image.h"
#include "framework/assets/json.h"

#include "asset_parser.h"

//
#include "asset_types.h"

// ----- ----- ----- ----- -----
//     Asset bytes part
// ----- ----- ----- ----- -----

static void asset_bytes_init(struct Asset_System * system, void * instance, struct CString name) {
	struct Asset_Bytes * asset = instance;
	(void)system;

	struct Buffer buffer;
	bool const read_success = platform_file_read_entire(name, &buffer);
	if (!read_success) { DEBUG_BREAK(); }

	// @note: memory ownership transfer
	asset->data = buffer.data;
	asset->length = (uint32_t)buffer.count;
}

static void asset_bytes_free(struct Asset_System * system, void * instance) {
	struct Asset_Bytes * asset = instance;
	(void)system;
	MEMORY_FREE(instance, asset->data);
}

// ----- ----- ----- ----- -----
//     Asset json part
// ----- ----- ----- ----- -----

static struct Strings gs_asset_json_strings;

static void asset_json_type_init(void) {
	strings_init(&gs_asset_json_strings);
}

static void asset_json_type_free(void) {
	strings_free(&gs_asset_json_strings);
}

static void asset_json_init(struct Asset_System * system, void * instance, struct CString name) {
	struct Asset_JSON * asset = instance;
	(void)system;

	struct Buffer buffer;
	bool const read_success = platform_file_read_entire(name, &buffer);
	if (!read_success || buffer.count == 0) { DEBUG_BREAK(); }

	json_init(&asset->value, &gs_asset_json_strings, (char const *)buffer.data);

	buffer_free(&buffer);
}

static void asset_json_free(struct Asset_System * system, void * instance) {
	struct Asset_JSON * asset = instance;
	(void)system;
	json_free(&asset->value);
}

// ----- ----- ----- ----- -----
//     Asset shader part
// ----- ----- ----- ----- -----

static void asset_shader_init(struct Asset_System * system, void * instance, struct CString name) {
	struct Asset_Shader * asset = instance;
	(void)system;

	struct Buffer buffer;
	bool const read_success = platform_file_read_entire(name, &buffer);
	if (!read_success || buffer.count == 0) { DEBUG_BREAK(); return; }
	// @todo: return error shader?

	struct Ref const gpu_ref = gpu_program_init(&buffer);
	buffer_free(&buffer);

	asset->gpu_ref = gpu_ref;
}

static void asset_shader_free(struct Asset_System * system, void * instance) {
	struct Asset_Shader * asset = instance;
	(void)system;
	gpu_program_free(asset->gpu_ref);
}

// ----- ----- ----- ----- -----
//     Asset image part
// ----- ----- ----- ----- -----

static void asset_image_init(struct Asset_System * system, void * instance, struct CString name) {
	struct Asset_Image * asset = instance;
	(void)system;

	struct Image image;
	image_init(&image, name);

	struct Ref const gpu_ref = gpu_texture_init(&image);
	image_free(&image);

	asset->gpu_ref = gpu_ref;
}

static void asset_image_free(struct Asset_System * system, void * instance) {
	struct Asset_Image * asset = instance;
	(void)system;
	gpu_texture_free(asset->gpu_ref);
}

// ----- ----- ----- ----- -----
//     Asset font part
// ----- ----- ----- ----- -----

static void asset_font_init(struct Asset_System * system, void * instance, struct CString name) {
	struct Asset_Font * asset = instance;
	(void)system;

	struct Font * font = font_init(name);
	struct Font_Image * buffer = font_image_init(font, 32); // @todo: remove hardcode

	asset->font = font;
	asset->buffer = buffer;
	asset->gpu_ref = gpu_texture_init(font_image_get_asset(buffer));
}

static void asset_font_free(struct Asset_System * system, void * instance) {
	struct Asset_Font * asset = instance;
	(void)system;
	font_free(asset->font);
	font_image_free(asset->buffer);
	gpu_texture_free(asset->gpu_ref);
}

// ----- ----- ----- ----- -----
// -- Asset target part
// ----- ----- ----- ----- -----

static void asset_target_init(struct Asset_System * system, void * instance, struct CString name) {
	struct Asset_Target * asset = instance;
	(void)system;

	struct Buffer buffer;
	bool const read_success = platform_file_read_entire(name, &buffer);
	if (!read_success || buffer.count == 0) { DEBUG_BREAK(); }

	struct Strings strings;
	strings_init(&strings);

	struct JSON json;
	json_init(&json, &strings, (char const *)buffer.data);
	buffer_free(&buffer);

	state_read_json_target(&json, &asset->gpu_ref);

	json_free(&json);
	strings_free(&strings);
}

static void asset_target_free(struct Asset_System * system, void * instance) {
	struct Asset_Target * asset = instance;
	(void)system;
	gpu_target_free(asset->gpu_ref);
}

// ----- ----- ----- ----- -----
//     Asset model part
// ----- ----- ----- ----- -----

static void asset_model_init(struct Asset_System * system, void * instance, struct CString name) {
	struct Asset_Model * asset = instance;
	(void)system;

	struct Mesh mesh;
	mesh_init(&mesh, name);

	struct Ref const gpu_ref = gpu_mesh_init(&mesh);
	mesh_free(&mesh);

	asset->gpu_ref = gpu_ref;
}

static void asset_model_free(struct Asset_System * system, void * instance) {
	struct Asset_Model * asset = instance;
	(void)system;
	gpu_mesh_free(asset->gpu_ref);
}

// ----- ----- ----- ----- -----
// -- Asset material part
// ----- ----- ----- ----- -----

static void asset_material_init(struct Asset_System * system, void * instance, struct CString name) {
	struct Asset_Material * asset = instance;
	(void)system;

	struct Buffer buffer;
	bool const read_success = platform_file_read_entire(name, &buffer);
	if (!read_success || buffer.count == 0) { DEBUG_BREAK(); }

	struct Strings strings;
	strings_init(&strings);

	struct JSON json;
	json_init(&json, &strings, (char const *)buffer.data);
	buffer_free(&buffer);

	state_read_json_material(system, &json, &asset->value);

	json_free(&json);
	strings_free(&strings);
}

static void asset_material_free(struct Asset_System * system, void * instance) {
	struct Asset_Material * asset = instance;
	(void)system;
	gfx_material_free(&asset->value);
}

//
#include "asset_registry.h"

void asset_types_init(struct Asset_System * system) {
	asset_system_map_extension(system, S_("bytes"),    S_("txt"));
	asset_system_map_extension(system, S_("json"),     S_("json"));
	asset_system_map_extension(system, S_("shader"),   S_("glsl"));
	asset_system_map_extension(system, S_("image"),    S_("png"));
	asset_system_map_extension(system, S_("font"),     S_("ttf"));
	asset_system_map_extension(system, S_("font"),     S_("otf"));
	asset_system_map_extension(system, S_("target"),   S_("rt"));
	asset_system_map_extension(system, S_("model"),    S_("obj"));
	asset_system_map_extension(system, S_("model"),    S_("fbx"));
	asset_system_map_extension(system, S_("material"), S_("mat"));

	asset_system_set_type(system, S_("bytes"), (struct Asset_Callbacks){
		.init = asset_bytes_init,
		.free = asset_bytes_free,
	}, sizeof(struct Asset_Bytes));

	asset_system_set_type(system, S_("json"), (struct Asset_Callbacks){
		.type_init = asset_json_type_init,
		.type_free = asset_json_type_free,
		.init = asset_json_init,
		.free = asset_json_free,
	}, sizeof(struct Asset_JSON));

	asset_system_set_type(system, S_("shader"), (struct Asset_Callbacks){
		.init = asset_shader_init,
		.free = asset_shader_free,
	}, sizeof(struct Asset_Shader));

	asset_system_set_type(system, S_("image"), (struct Asset_Callbacks){
		.init = asset_image_init,
		.free = asset_image_free,
	}, sizeof(struct Asset_Image));

	asset_system_set_type(system, S_("font"), (struct Asset_Callbacks){
		.init = asset_font_init,
		.free = asset_font_free,
	}, sizeof(struct Asset_Font));

	asset_system_set_type(system, S_("target"), (struct Asset_Callbacks){
		.init = asset_target_init,
		.free = asset_target_free,
	}, sizeof(struct Asset_Target));

	asset_system_set_type(system, S_("model"), (struct Asset_Callbacks){
		.init = asset_model_init,
		.free = asset_model_free,
	}, sizeof(struct Asset_Model));

	asset_system_set_type(system, S_("material"), (struct Asset_Callbacks){
		.init = asset_material_init,
		.free = asset_material_free,
	}, sizeof(struct Asset_Material));
}

void asset_types_free(struct Asset_System * system) {
	(void)system;
}
