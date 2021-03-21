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
	uint32_t index_offset;
};

//
#include "batch_mesh.h"

struct Batch_Mesh * batch_mesh_init(uint32_t attributes_count, uint32_t const * attributes) {
	struct Batch_Mesh * batch_mesh = MEMORY_ALLOCATE(struct Batch_Mesh);

	uint32_t const buffers_count = 2;
	batch_mesh->mesh = (struct Asset_Mesh){
		.count = buffers_count,
		.buffers = MEMORY_ALLOCATE_ARRAY(struct Array_Byte, buffers_count),
		.settings = MEMORY_ALLOCATE_ARRAY(struct Mesh_Settings, buffers_count),
	};
	array_byte_init(batch_mesh->mesh.buffers + 0);
	array_byte_init(batch_mesh->mesh.buffers + 1);
	batch_mesh->mesh.settings[0] = (struct Mesh_Settings){
		.type = DATA_TYPE_R32,
		.frequency = MESH_FREQUENCY_STREAM,
		.access = MESH_ACCESS_DRAW,
		.attributes_count = attributes_count,
	};
	batch_mesh->mesh.settings[1] = (struct Mesh_Settings){
		.type = DATA_TYPE_U32,
		.frequency = MESH_FREQUENCY_STREAM,
		.access = MESH_ACCESS_DRAW,
		.is_index = true,
	};
	memcpy(batch_mesh->mesh.settings[0].attributes, attributes, attributes_count * 2 * sizeof(uint32_t));

	array_float_init(&batch_mesh->vertices);
	array_u32_init(&batch_mesh->indices);
	batch_mesh->index_offset = 0;

	return batch_mesh;
}

void batch_mesh_free(struct Batch_Mesh * batch_mesh) {
	MEMORY_FREE(batch_mesh->mesh.buffers);
	MEMORY_FREE(batch_mesh->mesh.settings);

	array_float_free(&batch_mesh->vertices);
	array_u32_free(&batch_mesh->indices);

	memset(batch_mesh, 0, sizeof(*batch_mesh));
	MEMORY_FREE(batch_mesh);
}

void batch_mesh_clear(struct Batch_Mesh * batch_mesh) {
	batch_mesh->vertices.count = 0;
	batch_mesh->indices.count = 0;
	batch_mesh->index_offset = 0;
}

struct Asset_Mesh * batch_mesh_get_asset(struct Batch_Mesh * batch_mesh) {
	batch_mesh->mesh.buffers[0] = (struct Array_Byte){
		.data = (uint8_t *)batch_mesh->vertices.data,
		.count = batch_mesh->vertices.count * sizeof(float),
	};
	batch_mesh->mesh.buffers[1] = (struct Array_Byte){
		.data = (uint8_t *)batch_mesh->indices.data,
		.count = batch_mesh->indices.count * sizeof(uint32_t),
	};
	return &batch_mesh->mesh;
}

void batch_mesh_add(
	struct Batch_Mesh * batch_mesh,
	uint32_t vertices_count, float * vertices,
	uint32_t indices_count, uint32_t * indices
) {
	array_float_write_many(&batch_mesh->vertices, vertices_count, vertices);
	array_u32_write_many(&batch_mesh->indices, indices_count, indices);
}

void batch_mesh_add_quad(
	struct Batch_Mesh * batch_mesh,
	float const * rect, float const * uv
) {
	batch_mesh_add(
		batch_mesh,
		(3 + 2) * 4, (float[]){
			rect[0], rect[1], 0, uv[0], uv[1],
			rect[0], rect[3], 0, uv[0], uv[3],
			rect[2], rect[1], 0, uv[2], uv[1],
			rect[2], rect[3], 0, uv[2], uv[3],
		},
		3 * 2, (uint32_t[]){
			batch_mesh->index_offset + 1, batch_mesh->index_offset + 0, batch_mesh->index_offset + 2,
			batch_mesh->index_offset + 1, batch_mesh->index_offset + 2, batch_mesh->index_offset + 3
		}
	);
	batch_mesh->index_offset += 4;
}
