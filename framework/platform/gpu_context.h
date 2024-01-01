#if !defined(FRAMEWORK_PLATFORM_GPU_CONTEXT)
#define FRAMEWORK_PLATFORM_GPU_CONTEXT

#include "framework/common.h"

struct GPU_Context;

struct GPU_Context * gpu_context_init(void * surface);
void gpu_context_free(struct GPU_Context * gpu_context);

bool gpu_context_exists(struct GPU_Context const * gpu_context);

void gpu_context_start_frame(struct GPU_Context const * gpu_context, void * surface);
void gpu_context_end_frame(struct GPU_Context const * gpu_context);

int32_t gpu_context_get_vsync(struct GPU_Context const * gpu_context);
void gpu_context_set_vsync(struct GPU_Context * gpu_context, int32_t value);

#endif
