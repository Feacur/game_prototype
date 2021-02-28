#if !defined(GAME_GRAPHICS_IMPLEMENTATION)
#define GAME_GRAPHICS_IMPLEMENTATION

// -- GPU program part
struct Gpu_Program;

struct Gpu_Program * gpu_program_init(char const * text);
void gpu_program_free(struct Gpu_Program * gpu_program);

// -- GPU texture part
struct Gpu_Texture;

struct Gpu_Texture * gpu_texture_init(void);
void gpu_texture_free(struct Gpu_Texture * gpu_texture);

// -- GPU mesh part
struct Gpu_Mesh;

struct Gpu_Mesh * gpu_mesh_init(void);
void gpu_mesh_free(struct Gpu_Mesh * gpu_mesh);

#endif
