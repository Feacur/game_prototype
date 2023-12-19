#if !defined(FRAMEWORK_GPU_OPENGL_FUNCTIONS_TO_GPU_LIBRARY)
#define FRAMEWORK_GPU_OPENGL_FUNCTIONS_TO_GPU_LIBRARY

// @purpose: interface from `functions.c` to `gpu_library_opengl.c`
// - opengl functions initialization

#include "framework/common.h"

#define PROC_GETTER(func) void * (func)(struct CString name)
typedef PROC_GETTER(Proc_Getter);

void functions_to_gpu_library_init(Proc_Getter * get_proc);
void functions_to_gpu_library_free(void);

#endif
