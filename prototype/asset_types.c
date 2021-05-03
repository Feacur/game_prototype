#include "framework/platform_file.h"
#include "framework/containers/array_byte.h"
#include "framework/graphics/gpu_objects.h"

#include "framework/assets/font.h"
#include "framework/assets/mesh.h"

//
#include "asset_types.h"

// -- Asset program part
void asset_gpu_program_init(void * instance, char const * name) {
	struct Array_Byte source;
	platform_file_read_entire(name, &source);

	struct Ref const gpu_ref = gpu_program_init(&source);
	array_byte_free(&source);

	struct Asset_Gpu_Program * asset = instance;
	asset->gpu_ref = gpu_ref;
}

void asset_gpu_program_free(void * instance) {
	struct Asset_Gpu_Program * asset = instance;
	gpu_program_free(asset->gpu_ref);
}

// -- Asset font part
void asset_font_init(void * instance, char const * name) {
	struct Font * font = font_init(name);

	struct Asset_Font * asset = instance;
	asset->font = font;
}

void asset_font_free(void * instance) {
	struct Asset_Font * asset = instance;
	font_free(asset->font);
}

// -- Asset mesh part
void asset_mesh_init(void * instance, char const * name) {
	struct Mesh mesh;
	mesh_init(&mesh, name);

	struct Ref const gpu_ref = gpu_mesh_init(&mesh);
	mesh_free(&mesh);

	struct Asset_Mesh * asset = instance;
	asset->gpu_ref = gpu_ref;
}

void asset_mesh_free(void * instance) {
	struct Asset_Mesh * asset = instance;
	gpu_mesh_free(asset->gpu_ref);
}
