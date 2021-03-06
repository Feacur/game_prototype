#if !defined(GAME_ASSET_MESH_OBJ)
#define GAME_ASSET_MESH_OBJ

#include "array_float.h"
#include "array_s32.h"

struct Asset_Mesh_Obj {
	struct Array_Float positions;
	struct Array_Float texcoords;
	struct Array_Float normals;
	struct Array_S32 faces;
};

void asset_mesh_obj_init(struct Asset_Mesh_Obj * obj, const char * text);
void asset_mesh_obj_free(struct Asset_Mesh_Obj * obj);

#endif