#include "framework/assets/asset_mesh.h"
#include "framework/containers/array_float.h"
#include "framework/containers/array_u32.h"
#include "framework/memory.h"

#include <stdlib.h>
#include <string.h>

struct Batch_Mesh {
	struct Asset_Mesh mesh;
	struct Array_Float vertices;
	struct Array_U32 indices;
};

//
#include "batch_mesh.h"

struct Batch_Mesh * batch_mesh_init(uint32_t attributes_count, uint32_t const * sizes, uint32_t const * locations) {
	struct Batch_Mesh * batch = MEMORY_ALLOCATE(struct Batch_Mesh);

	uint32_t const buffers_count = 2;
	batch->mesh.count = buffers_count;
	batch->mesh.buffers = MEMORY_ALLOCATE_ARRAY(struct Array_Byte, buffers_count);
	batch->mesh.settings = MEMORY_ALLOCATE_ARRAY(struct Mesh_Settings, buffers_count);

	batch->mesh.settings[0] = (struct Mesh_Settings){
		.type = DATA_TYPE_R32,
		.frequency = MESH_FREQUENCY_STREAM,
		.access = MESH_ACCESS_DRAW,
		.count = attributes_count,
	};
	batch->mesh.settings[1] = (struct Mesh_Settings){
		.type = DATA_TYPE_U32,
		.frequency = MESH_FREQUENCY_STREAM,
		.access = MESH_ACCESS_DRAW,
		.is_index = true,
	};
	memcpy(batch->mesh.settings[0].sizes, sizes, attributes_count * sizeof(uint32_t));
	memcpy(batch->mesh.settings[0].locations, locations, attributes_count * sizeof(uint32_t));

	array_float_init(&batch->vertices);
	array_u32_init(&batch->indices);

	array_float_resize(&batch->vertices, 1024);
	array_u32_resize(&batch->indices, 256);

	return batch;
}

void batch_mesh_free(struct Batch_Mesh * batch) {
	memset(batch, 0, sizeof(*batch));
	MEMORY_FREE(batch);
}

struct Asset_Mesh * batch_mesh_get_mesh(struct Batch_Mesh * batch) {
	batch->mesh.buffers[0] = (struct Array_Byte){
		.data = (uint8_t *)batch->vertices.data,
		.count = batch->vertices.count * sizeof(float),
	};
	batch->mesh.buffers[1] = (struct Array_Byte){
		.data = (uint8_t *)batch->indices.data,
		.count = batch->indices.count * sizeof(uint32_t),
	};

	return &batch->mesh;
}
