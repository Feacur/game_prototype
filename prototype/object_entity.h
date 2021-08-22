#if !defined(GAME_PROTOTYPE_OBJECT_ENTITY)
#define GAME_PROTOTYPE_OBJECT_ENTITY

#include "framework/containers/ref.h"
#include "framework/graphics/types.h"

#include "components.h"

enum Entity_Rect_Mode {
	ENTITY_RECT_MODE_NONE,
	ENTITY_RECT_MODE_FIT,
	ENTITY_RECT_MODE_CONTENT,
};

enum Entity_Type {
	ENTITY_TYPE_MESH,
	ENTITY_TYPE_QUAD_2D,
	ENTITY_TYPE_TEXT_2D,
};

struct Entity_Mesh {
	struct Ref gpu_mesh_ref;
};

struct Entity_Quad {
	// @idea: a gpu ref?
	uint32_t texture_uniform;
};

struct Entity_Text {
	// @todo: a separate type for Asset_Bytes?
	uint32_t visible_length;
	uint32_t length;
	uint8_t const * data;
	// @todo: an asset ref
	struct Asset_Font const * font;
};

struct Entity {
	uint32_t material;
	uint32_t camera;
	struct Transform_3D transform;
	struct Transform_Rect rect;
	//
	struct Blend_Mode blend_mode;
	struct Depth_Mode depth_mode;
	//
	enum Entity_Rect_Mode rect_mode;
	enum Entity_Type type;
	union {
		struct Entity_Mesh mesh;
		struct Entity_Quad quad;
		struct Entity_Text text;
	} as;
};

//

bool entity_get_is_batched(struct Entity const * entity);

void entity_get_rect(
	struct Entity const * entity,
	uint32_t camera_size_x, uint32_t camera_size_y,
	struct vec2 * min, struct vec2 * max, struct vec2 * pivot
);

struct uvec2 entity_get_content_size(
	struct Entity const * entity,
	uint32_t camera_size_x, uint32_t camera_size_y
);

#endif