#include "framework/memory.h"
#include "framework/platform_file.h"
#include "framework/containers/array_float.h"
#include "framework/containers/array_u32.h"
#include "asset_mesh_obj.h"

#include <string.h>

//
#include "asset_mesh.h"

static void asset_mesh_obj_repack(
	struct Asset_Mesh_Obj const * obj,
	struct Array_Float * vertices,
	struct Array_U32 * attributes,
	struct Array_U32 * indices
);

static void asset_mesh_fill(
	struct Array_Float const * vertices,
	struct Array_U32 const * attributes,
	struct Array_U32 const * indices,
	struct Asset_Mesh * asset_mesh
);

void asset_mesh_init(struct Asset_Mesh * asset_mesh, char const * path) {
	struct Array_Byte file;
	platform_file_read_entire(path, &file);
	array_byte_write(&file, '\0');

	struct Asset_Mesh_Obj obj;
	asset_mesh_obj_init(&obj, (char const *)file.data);

	array_byte_free(&file);

	//
	struct Array_Float vertices;
	struct Array_U32 attributes;
	struct Array_U32 indices;

	asset_mesh_obj_repack(
		&obj,
		&vertices,
		&attributes,
		&indices
	);

	asset_mesh_obj_free(&obj);

	//
	asset_mesh_fill(
		&vertices,
		&attributes,
		&indices,
		asset_mesh
	);

	array_u32_free(&attributes);
}

void asset_mesh_free(struct Asset_Mesh * asset_mesh) {
	for (uint32_t i = 0; i < asset_mesh->count; i++) {
		array_byte_free(asset_mesh->buffers + i);
	}
	if (asset_mesh->capacity != 0) {
		MEMORY_FREE(asset_mesh, asset_mesh->buffers);
		MEMORY_FREE(asset_mesh, asset_mesh->parameters);
	}
	memset(asset_mesh, 0, sizeof(*asset_mesh));
}

//

static void asset_mesh_obj_repack(
	struct Asset_Mesh_Obj const * obj,
	struct Array_Float * vertices,
	struct Array_U32 * attributes,
	struct Array_U32 * indices
) {
	array_float_init(vertices);
	array_u32_init(attributes);
	array_u32_init(indices);

	uint32_t attributes_buffer[MAX_MESH_ATTRIBUTES];
	uint32_t attributes_count = 0;

	if (obj->positions.count > 0) { attributes_buffer[attributes_count++] = ATTRIBUTE_TYPE_POSITION; attributes_buffer[attributes_count++] = 3; }
	if (obj->texcoords.count > 0) { attributes_buffer[attributes_count++] = ATTRIBUTE_TYPE_TEXCOORD; attributes_buffer[attributes_count++] = 2; }
	if (obj->normals.count > 0)   { attributes_buffer[attributes_count++] = ATTRIBUTE_TYPE_NORMAL;   attributes_buffer[attributes_count++] = 3; }

	array_u32_write_many(attributes, attributes_count, attributes_buffer);

	uint32_t indices_count = obj->triangles.count / 3;
	for (uint32_t i = 0, vertex_id = 0; i < indices_count; i++) {
		uint32_t const vertex_index[] = {
			obj->triangles.data[i * 3 + 0],
			obj->triangles.data[i * 3 + 1],
			obj->triangles.data[i * 3 + 2],
		};

		// @todo: reuse matching vertices instead of copying them
		//        naive linear search would be quadrativally slow,
		//        so a hashset it is

		uint32_t attribute_index = 1;
		if (obj->positions.count > 0) { array_float_write_many(vertices, 3, obj->positions.data + vertex_index[0] * attributes_buffer[attribute_index]); attribute_index += 2; }
		if (obj->texcoords.count > 0) { array_float_write_many(vertices, 2, obj->texcoords.data + vertex_index[1] * attributes_buffer[attribute_index]); attribute_index += 2; }
		if (obj->normals.count > 0)   { array_float_write_many(vertices, 3, obj->normals.data   + vertex_index[2] * attributes_buffer[attribute_index]); attribute_index += 2; }

		array_u32_write(indices, vertex_id);
		vertex_id++;
	}
}

static void asset_mesh_fill(
	struct Array_Float const * vertices,
	struct Array_U32 const * attributes,
	struct Array_U32 const * indices,
	struct Asset_Mesh * asset_mesh
) {
	if (vertices->count == 0) { return; }
	if (indices->count == 0) { return; }

	uint32_t const count = 2;
	asset_mesh->capacity = count;
	asset_mesh->count = count;
	asset_mesh->buffers    = MEMORY_ALLOCATE_ARRAY(asset_mesh, struct Array_Byte, count);
	asset_mesh->parameters = MEMORY_ALLOCATE_ARRAY(asset_mesh, struct Mesh_Parameters, count);

	asset_mesh->buffers[0] = (struct Array_Byte){
		.data = (uint8_t *)vertices->data,
		.count = sizeof(float) * vertices->count,
		.capacity = sizeof(float) * vertices->capacity,
	};
	asset_mesh->buffers[1] = (struct Array_Byte){
		.data = (uint8_t *)indices->data,
		.count = sizeof(uint32_t) * indices->count,
		.capacity = sizeof(uint32_t) * indices->capacity,
	};

	asset_mesh->parameters[0] = (struct Mesh_Parameters){
		.type = DATA_TYPE_R32,
		.attributes_count = attributes->count / 2,
	};
	asset_mesh->parameters[1] = (struct Mesh_Parameters){
		.type = DATA_TYPE_U32,
		.flags = MESH_FLAG_INDEX,
	};
	memcpy(asset_mesh->parameters[0].attributes, attributes->data, sizeof(*attributes->data) * attributes->count);
}
