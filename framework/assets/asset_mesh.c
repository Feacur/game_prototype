#include "framework/memory.h"
#include "framework/platform_file.h"
#include "framework/containers/array_float.h"
#include "framework/containers/array_u32.h"
#include "asset_mesh_obj.h"

#include <string.h>

//
#include "asset_mesh.h"

void asset_mesh_init(struct Asset_Mesh * asset_mesh, char const * path) {
	struct Array_Byte file;
	platform_file_init(&file, path);
	array_byte_write(&file, '\0');

	struct Asset_Mesh_Obj obj;
	asset_mesh_obj_init(&obj, (char const *)file.data);

	array_byte_free(&file);

	//

	struct Array_Float vertices;
	struct Array_U32 sizes, locations;
	struct Array_U32 indices;

	array_float_init(&vertices);
	array_u32_init(&sizes);
	array_u32_init(&locations);
	array_u32_init(&indices);

	if (obj.positions.count > 0) { array_u32_write(&sizes, 3); }
	if (obj.texcoords.count > 0) { array_u32_write(&sizes, 2); }
	if (obj.normals.count > 0) { array_u32_write(&sizes, 3); }

	if (obj.positions.count > 0) { array_u32_write(&locations, 0); }
	if (obj.texcoords.count > 0) { array_u32_write(&locations, 1); }
	if (obj.normals.count > 0) { array_u32_write(&locations, 2); }

	uint32_t indices_count = obj.triangles.count / 3;
	for (uint32_t i = 0, vertex_id = 0; i < indices_count; i++) {
		uint32_t const vertex_index[] = {
			obj.triangles.data[i * 3 + 0],
			obj.triangles.data[i * 3 + 1],
			obj.triangles.data[i * 3 + 2],
		};

		// @todo: reuse matching vertices instead of copying them
		//        naive linear search would be quadrativally slow,
		//        so a hashset it is

		if (obj.positions.count > 0) { array_float_write_many(&vertices, 3, obj.positions.data + vertex_index[0] * 3); }
		if (obj.texcoords.count > 0) { array_float_write_many(&vertices, 2, obj.texcoords.data + vertex_index[1] * 2); }
		if (obj.normals.count > 0) { array_float_write_many(&vertices, 3, obj.normals.data + vertex_index[2] * 3); }

		array_u32_write(&indices, vertex_id);
		vertex_id++;
	}

	asset_mesh_obj_free(&obj);

	//
	uint32_t const count = 2;
	asset_mesh->capacity = count;
	asset_mesh->count = count;
	asset_mesh->buffers = MEMORY_ALLOCATE_ARRAY(struct Array_Byte, count);
	asset_mesh->settings = MEMORY_ALLOCATE_ARRAY(struct Mesh_Settings, count);

	asset_mesh->buffers[0] = (struct Array_Byte){
		.data = (uint8_t *)vertices.data,
		.count = vertices.count * sizeof(float),
		.capacity = vertices.capacity * sizeof(float),
	};
	asset_mesh->buffers[1] = (struct Array_Byte){
		.data = (uint8_t *)indices.data,
		.count = indices.count * sizeof(uint32_t),
		.capacity = indices.capacity * sizeof(uint32_t),
	};

	asset_mesh->settings[0] = (struct Mesh_Settings){
		.type = DATA_TYPE_R32,
		.frequency = MESH_FREQUENCY_STATIC,
		.access = MESH_ACCESS_DRAW,
		.count = locations.count,
	};
	asset_mesh->settings[1] = (struct Mesh_Settings){
		.type = DATA_TYPE_U32,
		.frequency = MESH_FREQUENCY_STATIC,
		.access = MESH_ACCESS_DRAW,
		.is_index = true,
	};
	memcpy(asset_mesh->settings[0].sizes, sizes.data, sizes.count * sizeof(uint32_t));
	memcpy(asset_mesh->settings[0].locations, locations.data, locations.count * sizeof(uint32_t));
	
	array_u32_free(&sizes);
	array_u32_free(&locations);
}

void asset_mesh_free(struct Asset_Mesh * asset_mesh) {
	for (uint32_t i = 0; i < asset_mesh->count; i++) {
		array_byte_free(asset_mesh->buffers + i);
	}
	if (asset_mesh->capacity != 0) {
		MEMORY_FREE(asset_mesh->buffers);
		MEMORY_FREE(asset_mesh->settings);
	}
	memset(asset_mesh, 0, sizeof(*asset_mesh));
}
