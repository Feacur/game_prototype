#if !defined(GAME_PLATFORM_GRAPHICS_LIBRARY)
#define GAME_PLATFORM_GRAPHICS_LIBRARY

// -- library part
void graphics_library_init(void);
void graphics_library_free(void);

// -- graphics part
struct Window;
struct Graphics;

struct Graphics * graphics_init(struct Window * window);
void graphics_free(struct Graphics * context);

#endif
