#if !defined(GAME_GRAPHICS_IMPLEMENTATION)
#define GAME_GRAPHICS_IMPLEMENTATION

// -- GPU program part
struct Gpu_Program;

struct Gpu_Program * gpu_program_init(char const * text, uint32_t text_size);
void gpu_program_free(struct Gpu_Program * gpu_program);

void gpu_program_select(struct Gpu_Program * gpu_program);

// -- GPU texture part
struct Gpu_Texture;

struct Gpu_Texture * gpu_texture_init(uint8_t const * data, uint32_t asset_image_size_x, uint32_t asset_image_size_y, uint32_t asset_image_channels);
void gpu_texture_free(struct Gpu_Texture * gpu_texture);

// -- GPU mesh part
struct Gpu_Mesh;

struct Gpu_Mesh * gpu_mesh_init(float const * vertices, uint32_t vertices_count, uint32_t const * indices, uint32_t indices_count);
void gpu_mesh_free(struct Gpu_Mesh * gpu_mesh);

void gpu_mesh_select(struct Gpu_Mesh * gpu_mesh);

#endif
