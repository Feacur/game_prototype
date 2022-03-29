#include "framework/memory.h"
#include "framework/logger.h"
#include "framework/platform_file.h"
#include "framework/containers/buffer.h"
#include "framework/containers/array_flt.h"
#include "framework/containers/array_u32.h"
#include "wfobj.h"

//
#include "mesh.h"

static void wfobj_repack(
	struct WFObj const * wfobj,
	struct Array_Flt * vertices,
	struct Array_U32 * attributes,
	struct Array_U32 * indices
);

static struct Mesh mesh_construct(
	struct Array_Flt const * vertices,
	struct Array_U32 const * attributes,
	struct Array_U32 const * indices
);

struct Mesh mesh_init(struct Buffer const * buffer) {
	struct WFObj wfobj = wfobj_init((char const *)buffer->data);

	//
	struct Array_Flt vertices;
	struct Array_U32 attributes;
	struct Array_U32 indices;

	wfobj_repack(&wfobj, &vertices, &attributes, &indices);
	wfobj_free(&wfobj);

	struct Mesh mesh = mesh_construct(&vertices, &attributes, &indices);
	array_u32_free(&attributes);

	return mesh;
}

void mesh_free(struct Mesh * mesh) {
	for (uint32_t i = 0; i < mesh->capacity; i++) {
		buffer_free(mesh->buffers + i);
	}
	MEMORY_FREE(mesh, mesh->buffers);
	MEMORY_FREE(mesh, mesh->parameters);
	common_memset(mesh, 0, sizeof(*mesh));
}

//

static void wfobj_repack(
	struct WFObj const * wfobj,
	struct Array_Flt * vertices,
	struct Array_U32 * attributes,
	struct Array_U32 * indices
) {
	*vertices   = array_flt_init();
	*attributes = array_u32_init();
	*indices    = array_u32_init();

	uint32_t attributes_buffer[MAX_MESH_ATTRIBUTES];
	uint32_t attributes_count = 0;

	if (wfobj->positions.count > 0) { attributes_buffer[attributes_count++] = ATTRIBUTE_TYPE_POSITION; attributes_buffer[attributes_count++] = 3; }
	if (wfobj->texcoords.count > 0) { attributes_buffer[attributes_count++] = ATTRIBUTE_TYPE_TEXCOORD; attributes_buffer[attributes_count++] = 2; }
	if (wfobj->normals.count > 0)   { attributes_buffer[attributes_count++] = ATTRIBUTE_TYPE_NORMAL;   attributes_buffer[attributes_count++] = 3; }

	array_u32_push_many(attributes, attributes_count, attributes_buffer);

	uint32_t indices_count = wfobj->triangles.count / 3;
	for (uint32_t i = 0, vertex_id = 0; i < indices_count; i++) {
		uint32_t const vertex_index[] = {
			wfobj->triangles.data[i * 3 + 0],
			wfobj->triangles.data[i * 3 + 1],
			wfobj->triangles.data[i * 3 + 2],
		};

		// @todo: reuse matching vertices instead of copying them
		//        naive linear search would be quadratically slow,
		//        so a hash_table it is

		uint32_t attribute_index = 1;
		if (wfobj->positions.count > 0) { array_flt_push_many(vertices, 3, wfobj->positions.data + vertex_index[0] * attributes_buffer[attribute_index]); attribute_index += 2; }
		if (wfobj->texcoords.count > 0) { array_flt_push_many(vertices, 2, wfobj->texcoords.data + vertex_index[1] * attributes_buffer[attribute_index]); attribute_index += 2; }
		if (wfobj->normals.count > 0)   { array_flt_push_many(vertices, 3, wfobj->normals.data   + vertex_index[2] * attributes_buffer[attribute_index]); attribute_index += 2; }

		array_u32_push(indices, vertex_id);
		vertex_id++;
	}
}

static struct Mesh mesh_construct(
	struct Array_Flt const * vertices,
	struct Array_U32 const * attributes,
	struct Array_U32 const * indices
) {
	if (vertices->count == 0) { return (struct Mesh){0}; }
	if (indices->count == 0) { return (struct Mesh){0}; }

	uint32_t const count = 2;
	struct Mesh mesh = {
		.capacity = count,
		.count = count,
		.buffers    = MEMORY_ALLOCATE_ARRAY(NULL, struct Buffer, count),
		.parameters = MEMORY_ALLOCATE_ARRAY(NULL, struct Mesh_Parameters, count),
	};
	//
	mesh.buffers[0] = (struct Buffer){
		.data = (uint8_t *)vertices->data,
		.count = sizeof(float) * vertices->count,
		.capacity = sizeof(float) * vertices->capacity,
	};
	mesh.buffers[1] = (struct Buffer){
		.data = (uint8_t *)indices->data,
		.count = sizeof(uint32_t) * indices->count,
		.capacity = sizeof(uint32_t) * indices->capacity,
	};
	//
	mesh.parameters[0] = (struct Mesh_Parameters){
		.type = DATA_TYPE_R32,
		.attributes_count = attributes->count / 2,
	};
	mesh.parameters[1] = (struct Mesh_Parameters){
		.type = DATA_TYPE_U32,
		.flags = MESH_FLAG_INDEX,
	};
	common_memcpy(mesh.parameters[0].attributes, attributes->data, sizeof(*attributes->data) * attributes->count);
	return mesh;
}
