#include "memory.h"
#include "code/array_byte.h"

#include <stdio.h>
#include <stdlib.h>

//
#include "platform_file.h"

void platform_file_init(struct Array_Byte * buffer, char const * path) {
	FILE * file = fopen(path, "rb");
	if (file == NULL) { fprintf(stderr, "'fopen' failed\n"); DEBUG_BREAK(); exit(1); }

	fseek(file, 0L, SEEK_END);
	size_t file_size = (size_t)ftell(file);
	rewind(file);

	uint8_t * data = MEMORY_ALLOCATE_ARRAY(uint8_t, file_size + 1);
	if (data == NULL) { fprintf(stderr, "'malloc' failed\n"); DEBUG_BREAK(); exit(1); }

	size_t bytes_read = (size_t)fread(data, sizeof(uint8_t), file_size, file);
	if (bytes_read < file_size) { fprintf(stderr, "'fread' failed\n"); DEBUG_BREAK(); exit(1); }

	fclose(file);

	*buffer = (struct Array_Byte){
		.capacity = (uint32_t)(file_size + 1),
		.count = (uint32_t)file_size,
		.data = data,
	};
}

void platform_file_free(struct Array_Byte * buffer) {
	array_byte_free(buffer);
}
