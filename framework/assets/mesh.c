#include "framework/formatter.h"

#include "framework/platform/file.h"
#include "framework/platform/memory.h"
#include "framework/containers/buffer.h"
#include "framework/containers/array.h"
#include "framework/containers/array.h"

#include "framework/graphics/gfx_types.h"

#include "internal/wfobj.h"


//
#include "mesh.h"

static void wfobj_repack(
	struct WFObj * wfobj,
	struct Array * vertices,  // float
	struct Array * indices,   // uint32_t
	struct Array * attributes // uint32_t
);

static struct Mesh mesh_construct(
	struct Array * vertices,  // float
	struct Array * indices,   // uint32_t
	struct Array * attributes // uint32_t
);

struct Mesh mesh_init(struct Buffer const * source) {
	struct WFObj wfobj = wfobj_parse((struct CString){
		.length = (uint32_t)source->size,
		.data = source->data,
	});

	struct Array vertices, indices, attributes;
	wfobj_repack(&wfobj, &vertices, &indices, &attributes);
	struct Mesh mesh = mesh_construct(&vertices, &indices, &attributes);

	return mesh;
}

void mesh_free(struct Mesh * mesh) {
	for (uint32_t i = 0; i < mesh->capacity; i++) {
		buffer_free(mesh->buffers + i);
	}
	FREE(mesh->buffers);
	FREE(mesh->parameters);
	common_memset(mesh, 0, sizeof(*mesh));
}

//

static void wfobj_repack(
	struct WFObj * wfobj,
	struct Array * vertices,  // float
	struct Array * indices,   // uint32_t
	struct Array * attributes // uint32_t
) {
	*vertices   = array_init(sizeof(float));
	*indices    = array_init(sizeof(uint32_t));
	*attributes = array_init(sizeof(uint32_t));

	uint32_t attributes_buffer[MESH_ATTRIBUTES_CAPACITY];
	uint32_t attributes_count = 0;

	if (wfobj->positions.count > 0) { attributes_buffer[attributes_count++] = ATTRIBUTE_TYPE_POSITION; attributes_buffer[attributes_count++] = 3; }
	if (wfobj->texcoords.count > 0) { attributes_buffer[attributes_count++] = ATTRIBUTE_TYPE_TEXCOORD; attributes_buffer[attributes_count++] = 2; }
	if (wfobj->normals.count   > 0) { attributes_buffer[attributes_count++] = ATTRIBUTE_TYPE_NORMAL;   attributes_buffer[attributes_count++] = 3; }

	array_push_many(attributes, attributes_count, attributes_buffer);
	array_push_many(attributes, 2, (uint32_t[]){0, 0});

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

	wfobj_free(wfobj);
}

static struct Mesh mesh_construct(
	struct Array * vertices,  // float
	struct Array * indices,   // uint32_t
	struct Array * attributes // uint32_t
) {
	uint32_t const count = 2;
	struct Mesh mesh = {
		.capacity = count,
		.count = count,
		.buffers    = ALLOCATE_ARRAY(struct Buffer, count),
		.parameters = ALLOCATE_ARRAY(struct Mesh_Parameters, count),
	};

	mesh.buffers[0] = (struct Buffer){
		.allocate = vertices->allocate,
		.data     = vertices->data,
		.capacity = vertices->capacity * sizeof(float),
		.size     = vertices->count * sizeof(float),
	}; *vertices = (struct Array){0};

	mesh.buffers[1] = (struct Buffer){
		.allocate = indices->allocate,
		.data     = indices->data,
		.capacity = indices->capacity * sizeof(uint32_t),
		.size     = indices->count * sizeof(uint32_t),
	}; *indices = (struct Array){0};

	mesh.parameters[0] = (struct Mesh_Parameters){
		.type = DATA_TYPE_R32_F,
	};
	mesh.parameters[1] = (struct Mesh_Parameters){
		.mode = MESH_MODE_TRIANGLES,
		.type = DATA_TYPE_R32_U,
		.flags = MESH_FLAG_INDEX,
	};

	size_t const attribute_size = SIZE_OF_MEMBER(struct Mesh_Parameters, attributes[0]);
	common_memcpy(mesh.parameters[0].attributes, attributes->data, attribute_size * attributes->count);
	common_memset(mesh.parameters[0].attributes + attributes->count, 0, attribute_size * (MESH_ATTRIBUTES_CAPACITY - attributes->count));
	array_free(attributes);

	return mesh;
}
