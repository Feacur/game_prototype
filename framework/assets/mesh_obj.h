#if !defined(GAME_ASSET_MESH_OBJ)
#define GAME_ASSET_MESH_OBJ

#include "framework/containers/array_float.h"
#include "framework/containers/array_u32.h"

struct Mesh_Obj {
	struct Array_Float positions;
	struct Array_Float texcoords;
	struct Array_Float normals;
	struct Array_U32 triangles;
};

void mesh_obj_init(struct Mesh_Obj * obj, const char * text);
void mesh_obj_free(struct Mesh_Obj * obj);

#endif
