#include "framework/memory.h"
#include "framework/platform_file.h"
#include "framework/containers/array_byte.h"
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
	array_float_init(&asset_mesh->vertices);
	array_u32_init(&asset_mesh->sizes);
	array_u32_init(&asset_mesh->locations);
	array_u32_init(&asset_mesh->indices);

	if (obj.positions.count > 0) { array_u32_write(&asset_mesh->sizes, 3); }
	if (obj.texcoords.count > 0) { array_u32_write(&asset_mesh->sizes, 2); }
	if (obj.normals.count > 0) { array_u32_write(&asset_mesh->sizes, 3); }

	if (obj.positions.count > 0) { array_u32_write(&asset_mesh->locations, 0); }
	if (obj.texcoords.count > 0) { array_u32_write(&asset_mesh->locations, 1); }
	if (obj.normals.count > 0) { array_u32_write(&asset_mesh->locations, 2); }

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

		if (obj.positions.count > 0) { array_float_write_many(&asset_mesh->vertices, 3, obj.positions.data + vertex_index[0] * 3); }
		if (obj.texcoords.count > 0) { array_float_write_many(&asset_mesh->vertices, 2, obj.texcoords.data + vertex_index[1] * 2); }
		if (obj.normals.count > 0) { array_float_write_many(&asset_mesh->vertices, 3, obj.normals.data + vertex_index[2] * 3); }

		array_u32_write(&asset_mesh->indices, vertex_id);
		vertex_id++;
	}

	asset_mesh_obj_free(&obj);
}

void asset_mesh_free(struct Asset_Mesh * asset_mesh) {
	array_float_free(&asset_mesh->vertices);
	array_u32_free(&asset_mesh->sizes);
	array_u32_free(&asset_mesh->locations);
	array_u32_free(&asset_mesh->indices);
	memset(asset_mesh, 0, sizeof(*asset_mesh));
}
