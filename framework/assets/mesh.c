#include "framework/memory.h"
#include "framework/logger.h"
#include "framework/platform_file.h"
#include "framework/containers/buffer.h"
#include "framework/containers/array.h"
#include "framework/containers/array.h"
#include "internal/wfobj.h"

//
#include "mesh.h"

static void wfobj_repack(
	struct WFObj const * wfobj,
	struct Array * vertices,   // float
	struct Array * attributes, // uint32_t
	struct Array * indices     // uint32_t
);

static struct Mesh mesh_construct(
	struct Array const * vertices,   // float
	struct Array const * attributes, // uint32_t
	struct Array const * indices     // uint32_t
);

struct Mesh mesh_init(struct Buffer const * buffer) {
	struct WFObj wfobj = wfobj_parse((char const *)buffer->data);

	//
	struct Array vertices;
	struct Array attributes;
	struct Array indices;

	wfobj_repack(&wfobj, &vertices, &attributes, &indices);
	wfobj_free(&wfobj);

	array_push_many(&attributes, 2, (uint32_t[]){0, 0});
	struct Mesh mesh = mesh_construct(&vertices, &attributes, &indices);
	array_free(&attributes);

	return mesh;
}

void mesh_free(struct Mesh * mesh) {
	for (uint32_t i = 0; i < mesh->capacity; i++) {
		buffer_free(mesh->buffers + i);
	}
	MEMORY_FREE(mesh->buffers);
	MEMORY_FREE(mesh->parameters);
	common_memset(mesh, 0, sizeof(*mesh));
}

//

static void wfobj_repack(
	struct WFObj const * wfobj,
	struct Array * vertices,   // float
	struct Array * attributes, // uint32_t
	struct Array * indices     // uint32_t
) {
	*vertices   = array_init(sizeof(float));
	*attributes = array_init(sizeof(uint32_t));
	*indices    = array_init(sizeof(uint32_t));

	uint32_t attributes_buffer[MESH_ATTRIBUTES_CAPACITY];
	uint32_t attributes_count = 0;

	if (wfobj->positions.count > 0) { attributes_buffer[attributes_count++] = ATTRIBUTE_TYPE_POSITION; attributes_buffer[attributes_count++] = 3; }
	if (wfobj->texcoords.count > 0) { attributes_buffer[attributes_count++] = ATTRIBUTE_TYPE_TEXCOORD; attributes_buffer[attributes_count++] = 2; }
	if (wfobj->normals.count   > 0) { attributes_buffer[attributes_count++] = ATTRIBUTE_TYPE_NORMAL;   attributes_buffer[attributes_count++] = 3; }

	array_push_many(attributes, attributes_count, attributes_buffer);

	uint32_t indices_count = wfobj->triangles.count / 3;
	for (uint32_t i = 0, vertex_id = 0; i < indices_count; i++) {
		uint32_t const * data = array_at(&wfobj->triangles, i * 3);
		uint32_t const vertex_index[] = {data[0], data[1], data[2]};

		// @todo: reuse matching vertices instead of copying them
		//        naive linear search would be quadratically slow,
		//        so a hash_table it is

		uint32_t attribute_index = 1;
		if (wfobj->positions.count > 0) { array_push_many(vertices, 3, array_at(&wfobj->positions, vertex_index[0] * attributes_buffer[attribute_index])); attribute_index += 2; }
		if (wfobj->texcoords.count > 0) { array_push_many(vertices, 2, array_at(&wfobj->texcoords, vertex_index[1] * attributes_buffer[attribute_index])); attribute_index += 2; }
		if (wfobj->normals.count > 0)   { array_push_many(vertices, 3, array_at(&wfobj->normals,   vertex_index[2] * attributes_buffer[attribute_index])); attribute_index += 2; }

		array_push_many(indices, 1, &vertex_id);
		vertex_id++;
	}
}

static struct Mesh mesh_construct(
	struct Array const * vertices,   // float
	struct Array const * attributes, // uint32_t
	struct Array const * indices     // uint32_t
) {
	if (vertices->count == 0) { return (struct Mesh){0}; }
	if (indices->count == 0) { return (struct Mesh){0}; }

	uint32_t const count = 2;
	struct Mesh mesh = {
		.capacity = count,
		.count = count,
		.buffers    = MEMORY_ALLOCATE_ARRAY(struct Buffer, count),
		.parameters = MEMORY_ALLOCATE_ARRAY(struct Mesh_Parameters, count),
	};
	//
	mesh.buffers[0] = (struct Buffer){
		.data = vertices->data,
		.capacity = sizeof(float) * vertices->capacity,
		.size = sizeof(float) * vertices->count,
	};
	mesh.buffers[1] = (struct Buffer){
		.data = indices->data,
		.capacity = sizeof(uint32_t) * indices->capacity,
		.size = sizeof(uint32_t) * indices->count,
	};
	//
	mesh.parameters[0] = (struct Mesh_Parameters){
		.type = DATA_TYPE_R32_F,
	};
	mesh.parameters[1] = (struct Mesh_Parameters){
		.type = DATA_TYPE_R32_U,
		.flags = MESH_FLAG_INDEX,
	};

	size_t const attribute_size = SIZE_OF_MEMBER(struct Mesh_Parameters, attributes[0]);
	common_memcpy(mesh.parameters[0].attributes, attributes->data, attribute_size * attributes->count);
	common_memset(mesh.parameters[0].attributes + attributes->count, 0, attribute_size * (MESH_ATTRIBUTES_CAPACITY - attributes->count));

	return mesh;
}
