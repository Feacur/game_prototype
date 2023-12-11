#if !defined(FRAMEWORK_GPU_CONTEXT)
#define FRAMEWORK_GPU_CONTEXT

#include "framework/common.h"

struct Gpu_Context;

struct Gpu_Context * gpu_context_init(void * device);
void gpu_context_free(struct Gpu_Context * gpu_context);

void gpu_context_start_frame(struct Gpu_Context const * gpu_context, void * device);
void gpu_context_draw_frame(struct Gpu_Context const * gpu_context);
void gpu_context_end_frame(struct Gpu_Context const * gpu_context);

int32_t gpu_context_get_vsync(struct Gpu_Context const * gpu_context);
void gpu_context_set_vsync(struct Gpu_Context * gpu_context, int32_t value);

#endif
