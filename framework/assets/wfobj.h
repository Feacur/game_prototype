#if !defined(FRAMEWORK_ASSETS_WFOBJ)
#define FRAMEWORK_ASSETS_WFOBJ

#include "framework/containers/array_flt.h"
#include "framework/containers/array_u32.h"

struct WFObj {
	struct Array_Flt positions;
	struct Array_Flt texcoords;
	struct Array_Flt normals;
	struct Array_U32 triangles;
};

struct WFObj wfobj_init(char const * text);
void wfobj_free(struct WFObj * obj);

#endif
