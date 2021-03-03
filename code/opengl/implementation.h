#if !defined(GAME_GRAPHICS_IMPLEMENTATION)
#define GAME_GRAPHICS_IMPLEMENTATION

// -- graphics library part
uint32_t glibrary_get_uniform_id(char const * name);

// -- GPU program part
struct Gpu_Program;

struct Gpu_Program * gpu_program_init(char const * text, uint32_t text_size);
void gpu_program_free(struct Gpu_Program * gpu_program);

void gpu_program_select(struct Gpu_Program * gpu_program);

void gpu_program_set_uniform_unit(struct Gpu_Program * gpu_program, uint32_t location, uint32_t value);

// -- GPU texture part
struct Gpu_Texture;

struct Gpu_Texture * gpu_texture_init(uint8_t const * data, uint32_t size_x, uint32_t size_y, uint32_t channels);
void gpu_texture_free(struct Gpu_Texture * gpu_texture);

void gpu_texture_select(struct Gpu_Texture * gpu_texture, uint32_t unit);

// -- GPU mesh part
struct Gpu_Mesh;

struct Gpu_Mesh * gpu_mesh_init(float const * vertices, uint32_t vertices_count, uint32_t const * indices, uint32_t indices_count);
void gpu_mesh_free(struct Gpu_Mesh * gpu_mesh);

void gpu_mesh_select(struct Gpu_Mesh * gpu_mesh);

#endif
