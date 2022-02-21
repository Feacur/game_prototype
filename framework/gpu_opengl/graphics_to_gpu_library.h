#if !defined(GAME_GPU_GRAPHICS_TO_GPU_LIBRARY)
#define GAME_GPU_GRAPHICS_TO_GPU_LIBRARY

// interface from `graphics.c` to `gpu_library_opengl.c`
// - opengl graphics initialization

void graphics_to_gpu_library_init(void);
void graphics_to_gpu_library_free(void);

#endif
