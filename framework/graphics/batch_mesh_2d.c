#include "framework/assets/asset_mesh.h"
#include "framework/containers/array_float.h"
#include "framework/containers/array_u32.h"
#include "framework/containers/array_byte.h"
#include "framework/memory.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct Batch_Mesh_2D {
	struct Asset_Mesh mesh;
	struct Array_Float vertices;
	struct Array_U32 indices;
	uint32_t vertex_index, element_index;
};

//
#include "batch_mesh_2d.h"

struct Batch_Mesh_2D * batch_mesh_2d_init(void) {
	uint32_t const attributes[] = {
		ATTRIBUTE_TYPE_POSITION, 2,
		ATTRIBUTE_TYPE_TEXCOORD, 2,
		ATTRIBUTE_TYPE_MIXER, 2,
	};
	uint32_t const attributes_count = (sizeof(attributes) / sizeof(*attributes)) / 2;

	struct Batch_Mesh_2D * batch_mesh = MEMORY_ALLOCATE(NULL, struct Batch_Mesh_2D);

	uint32_t const buffers_count = 2;
	*batch_mesh = (struct Batch_Mesh_2D){
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

void batch_mesh_2d_free(struct Batch_Mesh_2D * batch_mesh) {
	MEMORY_FREE(batch_mesh, batch_mesh->mesh.buffers);
	MEMORY_FREE(batch_mesh, batch_mesh->mesh.parameters);

	array_float_free(&batch_mesh->vertices);
	array_u32_free(&batch_mesh->indices);

	memset(batch_mesh, 0, sizeof(*batch_mesh));
	MEMORY_FREE(batch_mesh, batch_mesh);
}

void batch_mesh_2d_clear(struct Batch_Mesh_2D * batch_mesh) {
	batch_mesh->vertices.count = 0;
	batch_mesh->indices.count  = 0;
	batch_mesh->vertex_index   = 0;
	batch_mesh->element_index  = 0;
}

struct Asset_Mesh * batch_mesh_2d_get_asset(struct Batch_Mesh_2D * batch_mesh) {
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

uint32_t batch_mesh_2d_get_offset(struct Batch_Mesh_2D * batch_mesh) {
	return batch_mesh->element_index;
}

void batch_mesh_2d_add_quad(
	struct Batch_Mesh_2D * batch_mesh,
	float const * rect, float const * uv, // left, bottom, right, top
	float const * mixer
) {
	uint32_t const vertex_index = batch_mesh->vertex_index;
	array_float_write_many(&batch_mesh->vertices, (2 + 2 + 2) * 4, (float[]){
		rect[0], rect[1], uv[0], uv[1], mixer[0], mixer[1],
		rect[0], rect[3], uv[0], uv[3], mixer[0], mixer[1],
		rect[2], rect[1], uv[2], uv[1], mixer[0], mixer[1],
		rect[2], rect[3], uv[2], uv[3], mixer[0], mixer[1],
	});
	array_u32_write_many(&batch_mesh->indices, 3 * 2, (uint32_t[]){
		vertex_index + 1, vertex_index + 0, vertex_index + 2,
		vertex_index + 1, vertex_index + 2, vertex_index + 3
	});
	batch_mesh->vertex_index += 4;
	batch_mesh->element_index += 3 * 2;
}
