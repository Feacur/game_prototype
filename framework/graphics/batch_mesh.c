#include "framework/assets/asset_mesh.h"
#include "framework/containers/array_float.h"
#include "framework/containers/array_u32.h"
#include "framework/containers/array_byte.h"
#include "framework/memory.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct Batch_Mesh {
	struct Asset_Mesh mesh;
	struct Array_Float vertices;
	struct Array_U32 indices;
	void * scratch; uint32_t scratch_capacity;
	uint32_t index_offset;
};

//
#include "batch_mesh.h"

struct Batch_Mesh * batch_mesh_init(uint32_t attributes_count, uint32_t const * attributes) {
	struct Batch_Mesh * batch_mesh = MEMORY_ALLOCATE(struct Batch_Mesh);

	uint32_t const buffers_count = 2;
	*batch_mesh = (struct Batch_Mesh){
		.mesh = (struct Asset_Mesh){
			.count = buffers_count,
			.buffers = MEMORY_ALLOCATE_ARRAY(struct Array_Byte, buffers_count),
			.parameters = MEMORY_ALLOCATE_ARRAY(struct Mesh_Parameters, buffers_count),
		},
	};

	array_byte_init(batch_mesh->mesh.buffers + 0);
	array_byte_init(batch_mesh->mesh.buffers + 1);
	batch_mesh->mesh.parameters[0] = (struct Mesh_Parameters){
		.type = DATA_TYPE_R32,
		.flags = MESH_FLAG_MUTABLE | MESH_FLAG_WRITE | MESH_FLAG_FREQUENT,
		.attributes_count = attributes_count,
	};
	batch_mesh->mesh.parameters[1] = (struct Mesh_Parameters){
		.type = DATA_TYPE_U32,
		.flags = MESH_FLAG_MUTABLE | MESH_FLAG_WRITE | MESH_FLAG_FREQUENT,
		.is_index = true,
	};
	memcpy(batch_mesh->mesh.parameters[0].attributes, attributes, attributes_count * 2 * sizeof(uint32_t));

	array_float_init(&batch_mesh->vertices);
	array_u32_init(&batch_mesh->indices);

	return batch_mesh;
}

void batch_mesh_free(struct Batch_Mesh * batch_mesh) {
	MEMORY_FREE(batch_mesh->mesh.buffers);
	MEMORY_FREE(batch_mesh->mesh.parameters);
	MEMORY_FREE(batch_mesh->scratch);

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

void batch_mesh_add_quad_xy(
	struct Batch_Mesh * batch_mesh,
	float const * rect, float const * uv // left, bottom, right, top
) {

	uint32_t const attributes_count = batch_mesh->mesh.parameters[0].attributes_count;
	uint32_t const * attributes = batch_mesh->mesh.parameters[0].attributes;

	uint32_t vertex_size = 0;
	for (uint32_t i = 0; i < attributes_count; i++) {
		vertex_size += attributes[i * 2 + 1];
	}

	uint32_t const vertex_size_bytes = vertex_size * 4 * sizeof(float);
	if (batch_mesh->scratch_capacity < vertex_size_bytes) {
		batch_mesh->scratch_capacity = vertex_size_bytes;
		batch_mesh->scratch = memory_reallocate(batch_mesh->scratch, vertex_size_bytes);
	}
	float * vertices = batch_mesh->scratch;

	uint32_t vertex_offset = 0;
	for (uint32_t i = 0; i < attributes_count; i++) {
		float const * source = NULL;
		switch (attributes[i * 2 + 0]) {
			default: fprintf(stderr, "this attribute is not supported\n"); DEBUG_BREAK(); break;
			case 0: source = rect; break;
			case 1: source = uv;   break;
		}
		uint32_t const size = attributes[i * 2 + 1];
		if (source != NULL) {
			memcpy(vertices + 0 * vertex_size + vertex_offset, (float[3]){source[0], source[1], 0}, size * sizeof(*vertices));
			memcpy(vertices + 1 * vertex_size + vertex_offset, (float[3]){source[0], source[3], 0}, size * sizeof(*vertices));
			memcpy(vertices + 2 * vertex_size + vertex_offset, (float[3]){source[2], source[1], 0}, size * sizeof(*vertices));
			memcpy(vertices + 3 * vertex_size + vertex_offset, (float[3]){source[2], source[3], 0}, size * sizeof(*vertices));
		}
		vertex_offset += size;
	}

	uint32_t const index_offset = batch_mesh->index_offset;
	array_float_write_many(&batch_mesh->vertices, vertex_size * 4, vertices);
	array_u32_write_many(&batch_mesh->indices, 3 * 2, (uint32_t[]){
		index_offset + 1, index_offset + 0, index_offset + 2,
		index_offset + 1, index_offset + 2, index_offset + 3
	});
	batch_mesh->index_offset += 4;
}
