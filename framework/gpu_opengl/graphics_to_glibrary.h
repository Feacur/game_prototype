#if !defined(GAME_GPU_GRAPHICS_TO_GLIBRARY)
#define GAME_GPU_GRAPHICS_TO_GLIBRARY

// interface from `graphics.c` to `glibrary_opengl.c`
// - opengl graphics initialization

void graphics_to_glibrary_init(void);
void graphics_to_glibrary_free(void);

#endif
