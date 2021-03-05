#include "memory.h"
#include "platform_file.h"
#include "array_byte.h"
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
}

void asset_mesh_free(struct Asset_Mesh * asset_mesh) {
	array_float_free(&asset_mesh->vertices);
	array_u32_free(&asset_mesh->sizes);
	array_u32_free(&asset_mesh->locations);
	array_u32_free(&asset_mesh->indices);
	memset(asset_mesh, 0, sizeof(*asset_mesh));
}
