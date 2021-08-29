#if !defined(GAME_ASSETS_MESH_OBJ)
#define GAME_ASSETS_MESH_OBJ

#include "framework/containers/array_float.h"
#include "framework/containers/array_u32.h"

struct WFObj {
	struct Array_Float positions;
	struct Array_Float texcoords;
	struct Array_Float normals;
	struct Array_U32 triangles;
};

void wfobj_init(struct WFObj * obj, char const * text);
void wfobj_free(struct WFObj * obj);

#endif
