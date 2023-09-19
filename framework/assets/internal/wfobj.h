#if !defined(FRAMEWORK_ASSETS_WFOBJ)
#define FRAMEWORK_ASSETS_WFOBJ

#include "framework/containers/array.h"
#include "framework/containers/array.h"

struct WFObj {
	struct Array positions; // float
	struct Array texcoords; // float
	struct Array normals;   // float
	struct Array triangles; // uint32_t
};

struct WFObj wfobj_init(void);
void wfobj_free(struct WFObj * obj);

struct WFObj wfobj_parse(char const * text);

#endif
