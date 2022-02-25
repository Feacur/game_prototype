#if !defined(GAME_PROTOTYPE_OBJECT_ENTITY)
#define GAME_PROTOTYPE_OBJECT_ENTITY

#include "framework/systems/asset_ref.h"
#include "framework/graphics/material.h"
#include "framework/graphics/types.h"

#include "components.h"

enum Entity_Type {
	ENTITY_TYPE_NONE,
	ENTITY_TYPE_MESH,
	ENTITY_TYPE_QUAD_2D,
	ENTITY_TYPE_TEXT_2D,
};

struct Entity_Mesh {
	struct Asset_Ref mesh;
};

struct Entity_Quad {
	uint32_t texture_uniform;
	bool fit;
};

struct Entity_Text {
	struct Asset_Ref font;
	struct Asset_Ref message;
	uint32_t visible_length;
};

struct Entity {
	uint32_t camera;
	struct Asset_Ref material;
	//
	struct Transform_3D transform;
	struct Transform_Rect rect; // @todo: make it completely optional?
	//
	enum Entity_Type type;
	union {
		struct Entity_Mesh mesh;
		struct Entity_Quad quad;
		struct Entity_Text text;
	} as;
};

//

bool entity_get_is_screen_space(struct Entity const * entity);
bool entity_get_is_batched(struct Entity const * entity);

void entity_get_rect(
	struct Entity const * entity,
	uint32_t viewport_size_x, uint32_t viewport_size_y,
	struct vec2 * min, struct vec2 * max, struct vec2 * pivot
);

struct uvec2 entity_get_content_size(
	struct Entity const * entity, struct Gfx_Material const * material,
	uint32_t viewport_size_x, uint32_t viewport_size_y
);

#endif
