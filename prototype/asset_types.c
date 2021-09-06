#include "framework/memory.h"
#include "framework/logger.h"
#include "framework/platform_file.h"
#include "framework/containers/array_byte.h"
#include "framework/containers/strings.h"
#include "framework/graphics/gpu_objects.h"

#include "framework/assets/font.h"
#include "framework/assets/mesh.h"
#include "framework/assets/image.h"
#include "framework/assets/json.h"

//
#include "asset_types.h"

// ----- ----- ----- ----- -----
//     Asset shader part
// ----- ----- ----- ----- -----
void asset_shader_init(void * instance, char const * name) {
	struct Asset_Shader * asset = instance;

	struct Array_Byte buffer;
	bool const read_success = platform_file_read_entire(name, &buffer);
	if (!read_success || buffer.count == 0) { DEBUG_BREAK(); return; }
	// @todo: return error shader?

	struct Ref const gpu_ref = gpu_program_init(&buffer);
	array_byte_free(&buffer);

	asset->gpu_ref = gpu_ref;
}

void asset_shader_free(void * instance) {
	struct Asset_Shader * asset = instance;
	gpu_program_free(asset->gpu_ref);
}

// ----- ----- ----- ----- -----
//     Asset model part
// ----- ----- ----- ----- -----
void asset_model_init(void * instance, char const * name) {
	struct Asset_Model * asset = instance;

	struct Mesh mesh;
	mesh_init(&mesh, name);

	struct Ref const gpu_ref = gpu_mesh_init(&mesh);
	mesh_free(&mesh);

	asset->gpu_ref = gpu_ref;
}

void asset_model_free(void * instance) {
	struct Asset_Model * asset = instance;
	gpu_mesh_free(asset->gpu_ref);
}

// ----- ----- ----- ----- -----
//     Asset image part
// ----- ----- ----- ----- -----
void asset_image_init(void * instance, char const * name) {
	struct Asset_Image * asset = instance;

	struct Image image;
	image_init(&image, name);

	struct Ref const gpu_ref = gpu_texture_init(&image);
	image_free(&image);

	asset->gpu_ref = gpu_ref;
}

void asset_image_free(void * instance) {
	struct Asset_Image * asset = instance;
	gpu_texture_free(asset->gpu_ref);
}

// ----- ----- ----- ----- -----
//     Asset font part
// ----- ----- ----- ----- -----
void asset_font_init(void * instance, char const * name) {
	struct Asset_Font * asset = instance;

	struct Font * font = font_init(name);
	struct Font_Image * buffer = font_image_init(font, 32);

	asset->font = font;
	asset->buffer = buffer;
	asset->gpu_ref = gpu_texture_init(font_image_get_asset(buffer));
}

void asset_font_free(void * instance) {
	struct Asset_Font * asset = instance;
	font_free(asset->font);
	font_image_free(asset->buffer);
	gpu_texture_free(asset->gpu_ref);
}

// ----- ----- ----- ----- -----
//     Asset bytes part
// ----- ----- ----- ----- -----
void asset_bytes_init(void * instance, char const * name) {
	struct Asset_Bytes * asset = instance;

	struct Array_Byte buffer;
	bool const read_success = platform_file_read_entire(name, &buffer);
	if (!read_success) { DEBUG_BREAK(); }

	// @note: memory ownership transfer
	asset->data = buffer.data;
	asset->length = (uint32_t)buffer.count;
}

void asset_bytes_free(void * instance) {
	struct Asset_Bytes * asset = instance;
	MEMORY_FREE(instance, asset->data);
}

// ----- ----- ----- ----- -----
//     Asset json part
// ----- ----- ----- ----- -----
static struct Strings asset_json_strings; // @note: global state

void asset_json_type_init(void) {
	strings_init(&asset_json_strings);
}

void asset_json_type_free(void) {
	strings_free(&asset_json_strings);
}

void asset_json_init(void * instance, char const * name) {
	struct Asset_JSON * asset = instance;

	struct Array_Byte buffer;
	bool const read_success = platform_file_read_entire(name, &buffer);
	if (!read_success || buffer.count == 0) { DEBUG_BREAK(); }

	json_init(&asset->value, &asset_json_strings, (char const *)buffer.data);
	asset->strings = &asset_json_strings;

	array_byte_free(&buffer);
}

void asset_json_free(void * instance) {
	struct Asset_JSON * asset = instance;
	json_free(&asset->value);
}
