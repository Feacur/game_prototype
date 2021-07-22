#include "framework/memory.h"
#include "framework/platform_file.h"
#include "framework/containers/array_byte.h"
#include "framework/graphics/gpu_objects.h"

#include "framework/assets/font.h"
#include "framework/assets/mesh.h"
#include "framework/assets/image.h"

//
#include "asset_types.h"

// -- Asset shader part
void asset_shader_init(void * instance, char const * name) {
	struct Array_Byte source;
	platform_file_read_entire(name, &source);

	struct Ref const gpu_ref = gpu_program_init(&source);
	array_byte_free(&source);

	struct Asset_Shader * asset = instance;
	asset->gpu_ref = gpu_ref;
}

void asset_shader_free(void * instance) {
	struct Asset_Shader * asset = instance;
	gpu_program_free(asset->gpu_ref);
}

// -- Asset model part
void asset_model_init(void * instance, char const * name) {
	struct Mesh mesh;
	mesh_init(&mesh, name);

	struct Ref const gpu_ref = gpu_mesh_init(&mesh);
	mesh_free(&mesh);

	struct Asset_Model * asset = instance;
	asset->gpu_ref = gpu_ref;
}

void asset_model_free(void * instance) {
	struct Asset_Model * asset = instance;
	gpu_mesh_free(asset->gpu_ref);
}

// -- Asset image part
void asset_image_init(void * instance, char const * name) {
	struct Image image;
	image_init(&image, name);

	struct Ref const gpu_ref = gpu_texture_init(&image);
	image_free(&image);

	struct Asset_Image * asset = instance;
	asset->gpu_ref = gpu_ref;
}

void asset_image_free(void * instance) {
	struct Asset_Image * asset = instance;
	gpu_texture_free(asset->gpu_ref);
}

// -- Asset font part
void asset_font_init(void * instance, char const * name) {
	struct Font * font = font_init(name);

	// @todo: allow variable-sized fonts
	// @todo: dynamically generate glyphs
	// @todo: multiple buffers and gpu-textures?
	struct Font_Image * buffer = font_image_init(font, 32);

	struct Asset_Font * asset = instance;
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

// -- Asset text part
void asset_text_init(void * instance, char const * name) {
	struct Array_Byte buffer;
	platform_file_read_entire(name, &buffer);
	buffer.data[buffer.count] = '\0';

	// @note: memory ownership transfer
	struct Asset_Text * asset = instance;
	asset->data = buffer.data;
	asset->length = (uint32_t)buffer.count;
}

void asset_text_free(void * instance) {
	struct Asset_Text * asset = instance;
	MEMORY_FREE(instance, asset->data);
}
