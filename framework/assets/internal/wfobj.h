#if !defined(FRAMEWORK_system_assets_WFOBJ)
#define FRAMEWORK_system_assets_WFOBJ

#include "framework/containers/array.h"
#include "framework/containers/array.h"

// ----- ----- ----- ----- -----
//     personal
// ----- ----- ----- ----- -----

struct WFObj {
	struct Array positions; // float
	struct Array texcoords; // float
	struct Array normals;   // float
	struct Array triangles; // uint32_t
};

struct WFObj wfobj_init(void);
void wfobj_free(struct WFObj * obj);

// ----- ----- ----- ----- -----
//     parsing
// ----- ----- ----- ----- -----

struct WFObj wfobj_parse(struct CString text);

#endif
