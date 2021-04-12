#include "framework/assets/asset_mesh.h"
#include "framework/containers/array_float.h"
#include "framework/containers/array_u32.h"
#include "framework/containers/array_byte.h"
#include "framework/memory.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct Batch_Mesh_Generic {
	struct Asset_Mesh mesh;
	struct Array_Float vertices;
	struct Array_U32 indices;
	float * scratch; uint32_t scratch_capacity;
	uint32_t vertex_index, element_index;
};

//
#include "batch_mesh_generic.h"

struct Batch_Mesh_Generic * batch_mesh_generic_init(uint32_t attributes_count, uint32_t const * attributes) {
	struct Batch_Mesh_Generic * batch_mesh = MEMORY_ALLOCATE(NULL, struct Batch_Mesh_Generic);

	uint32_t const buffers_count = 2;
	*batch_mesh = (struct Batch_Mesh_Generic){
		.mesh = (struct Asset_Mesh){
			.count      = buffers_count,
			.buffers    = MEMORY_ALLOCATE_ARRAY(batch_mesh, struct Array_Byte, buffers_count),
			.parameters = MEMORY_ALLOCATE_ARRAY(batch_mesh, struct Mesh_Parameters, buffers_count),
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
		.flags = MESH_FLAG_INDEX | MESH_FLAG_MUTABLE | MESH_FLAG_WRITE | MESH_FLAG_FREQUENT,
	};
	memcpy(batch_mesh->mesh.parameters[0].attributes, attributes, attributes_count * 2 * sizeof(uint32_t));

	array_float_init(&batch_mesh->vertices);
	array_u32_init(&batch_mesh->indices);

	return batch_mesh;
}

void batch_mesh_generic_free(struct Batch_Mesh_Generic * batch_mesh) {
	MEMORY_FREE(batch_mesh, batch_mesh->mesh.buffers);
	MEMORY_FREE(batch_mesh, batch_mesh->mesh.parameters);
	MEMORY_FREE(batch_mesh, batch_mesh->scratch);

	array_float_free(&batch_mesh->vertices);
	array_u32_free(&batch_mesh->indices);

	memset(batch_mesh, 0, sizeof(*batch_mesh));
	MEMORY_FREE(batch_mesh, batch_mesh);
}

void batch_mesh_generic_clear(struct Batch_Mesh_Generic * batch_mesh) {
	batch_mesh->vertices.count = 0;
	batch_mesh->indices.count  = 0;
	batch_mesh->vertex_index   = 0;
	batch_mesh->element_index  = 0;
}

struct Asset_Mesh * batch_mesh_generic_get_asset(struct Batch_Mesh_Generic * batch_mesh) {
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

uint32_t batch_mesh_generic_get_offset(struct Batch_Mesh_Generic * batch_mesh) {
	return batch_mesh->element_index;
}

void batch_mesh_generic_add_quad_xy(
	struct Batch_Mesh_Generic * batch_mesh,
	float const * rect, float const * uv // left, bottom, right, top
) {

	uint32_t const attributes_count = batch_mesh->mesh.parameters[0].attributes_count;
	uint32_t const * attributes = batch_mesh->mesh.parameters[0].attributes;

	uint32_t vertex_size = 0;
	for (uint32_t i = 0; i < attributes_count; i++) {
		vertex_size += attributes[i * 2 + 1];
	}

	uint32_t const quad_size = vertex_size * 4;
	if (batch_mesh->scratch_capacity < quad_size) {
		batch_mesh->scratch_capacity = quad_size;
		batch_mesh->scratch = MEMORY_REALLOCATE_ARRAY(batch_mesh, batch_mesh->scratch, quad_size);
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

	uint32_t const vertex_index = batch_mesh->vertex_index;
	array_float_write_many(&batch_mesh->vertices, vertex_size * 4, vertices);
	array_u32_write_many(&batch_mesh->indices, 3 * 2, (uint32_t[]){
		vertex_index + 1, vertex_index + 0, vertex_index + 2,
		vertex_index + 1, vertex_index + 2, vertex_index + 3
	});
	batch_mesh->vertex_index += 4;
	batch_mesh->element_index += 3 * 2;
}
