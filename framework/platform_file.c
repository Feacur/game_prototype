#include "memory.h"
#include "framework/containers/array_byte.h"

#include <stdio.h>
#include <stdlib.h>

//
#include "framework/platform_file.h"

bool platform_file_read(struct Array_Byte * buffer, char const * path) {
	FILE * file = fopen(path, "rb");
	if (file == NULL) { fprintf(stderr, "'fopen' failed\n"); DEBUG_BREAK(); return false; }

	fseek(file, 0L, SEEK_END);
	long const file_size_from_api = ftell(file);
	if (file_size_from_api == -1) { fprintf(stderr, "'ftell' failed\n"); DEBUG_BREAK(); return false; }

	// @todo: support large files?
	size_t const file_size = (size_t)file_size_from_api;
	uint8_t * data = MEMORY_ALLOCATE_ARRAY(uint8_t, file_size + 1);

	rewind(file);
	size_t const bytes_read = fread(data, sizeof(uint8_t), file_size, file);
	if (bytes_read < file_size) { fprintf(stderr, "'fread' failed\n"); DEBUG_BREAK(); return false; }

	fclose(file);

	*buffer = (struct Array_Byte){
		.capacity = (uint32_t)(file_size + 1),
		.count = (uint32_t)file_size,
		.data = data,
	};
	return true;
}
