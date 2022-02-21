#if !defined(GAME_PLATFORM_GPU_LIBRARY_TO_SYSTEM)
#define GAME_PLATFORM_GPU_LIBRARY_TO_SYSTEM

// interface from `gpu_library_*.c` to `system.c`
// - graphics library initialization

bool gpu_library_to_system_init(void);
void gpu_library_to_system_free(void);

#endif
