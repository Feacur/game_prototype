#include "framework/platform_file.h"
#include "framework/containers/array_byte.h"
#include "framework/graphics/gpu_objects.h"
#include "framework/assets/font.h"

//
#include "asset_types.h"

// -- Asset program part
void asset_gpu_program_init(void * instance, char const * name) {
	struct Array_Byte source;
	platform_file_read_entire(name, &source);

	struct Ref const gpu_ref = gpu_program_init(&source);
	array_byte_free(&source);

	struct Asset_Gpu_Program * asset = instance;
	*asset = (struct Asset_Gpu_Program){
		.gpu_ref = gpu_ref,
	};
}

void asset_gpu_program_free(void * instance) {
	struct Asset_Gpu_Program * asset = instance;
	gpu_program_free(asset->gpu_ref);
}

// -- Asset font part
void asset_font_init(void * instance, char const * name) {
	struct Asset_Font * asset = instance;
	*asset = (struct Asset_Font){
		.font = font_init(name),
	};
}

void asset_font_free(void * instance) {
	struct Asset_Font * asset = instance;
	font_free(asset->font);
}
