#if !defined(FRAMEWORK_GPU_OPENGL_FUNCTIONS_TO_GPU_LIBRARY)
#define FRAMEWORK_GPU_OPENGL_FUNCTIONS_TO_GPU_LIBRARY

// @purpose: interface from `functions.c` to `gpu_library_opengl.c`
// - opengl functions initialization

#include "framework/common.h"

void functions_to_gpu_library_init(void * (* get)(struct CString name));
void functions_to_gpu_library_free(void);

#endif
