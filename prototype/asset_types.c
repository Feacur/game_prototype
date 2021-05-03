#include "framework/platform_file.h"
#include "framework/containers/array_byte.h"
#include "framework/graphics/gpu_objects.h"

//
#include "asset_types.h"

void asset_program_init(void * instance, char const * name) {
	struct Array_Byte source;
	platform_file_read_entire(name, &source);

	struct Ref const gpu_ref = gpu_program_init(&source);
	array_byte_free(&source);

	struct Asset_Program * program = instance;
	*program = (struct Asset_Program){
		.gpu_ref = gpu_ref,
	};
}

void asset_program_free(void * instance) {
	struct Asset_Program * program = instance;
	gpu_program_free(program->gpu_ref);
}
