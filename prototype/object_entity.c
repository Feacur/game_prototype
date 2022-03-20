#include "framework/logger.h"
#include "framework/maths.h"

#include "framework/containers/array_any.h"
#include "framework/graphics/material.h"
#include "framework/graphics/gpu_objects.h"

#include "game_state.h"

//
#include "object_entity.h"

bool entity_get_is_screen_space(struct Entity const * entity) {
	switch (entity->type) {
		case ENTITY_TYPE_NONE: return false;

		case ENTITY_TYPE_MESH:
			return false;

		case ENTITY_TYPE_QUAD_2D:
		case ENTITY_TYPE_TEXT_2D:
			return true;
	}

	logger_to_console("unknown entity type"); DEBUG_BREAK();
	return false;
}

bool entity_get_is_batched(struct Entity const * entity) {
	switch (entity->type) {
		case ENTITY_TYPE_NONE: return false;

		case ENTITY_TYPE_MESH:
			return false;

		case ENTITY_TYPE_QUAD_2D:
		case ENTITY_TYPE_TEXT_2D:
			return true;
	}

	logger_to_console("unknown entity type"); DEBUG_BREAK();
	return false;
}

void entity_get_rect(
	struct Entity const * entity,
	uint32_t viewport_size_x, uint32_t viewport_size_y,
	struct vec2 * min, struct vec2 * max, struct vec2 * pivot
) {
	struct vec2 const offset_min = {
		.x = entity->rect.offset.x - entity->rect.extents.x * entity->rect.pivot.x,
		.y = entity->rect.offset.y - entity->rect.extents.y * entity->rect.pivot.y,
	};

	struct vec2 const offset_max = {
		.x = entity->rect.offset.x + entity->rect.extents.x * (1 - entity->rect.pivot.x),
		.y = entity->rect.offset.y + entity->rect.extents.y * (1 - entity->rect.pivot.y),
	};

	*min = (struct vec2){
		entity->rect.anchor_min.x * (float)viewport_size_x + offset_min.x,
		entity->rect.anchor_min.y * (float)viewport_size_y + offset_min.y,
	};
	*max = (struct vec2){
		entity->rect.anchor_max.x * (float)viewport_size_x + offset_max.x,
		entity->rect.anchor_max.y * (float)viewport_size_y + offset_max.y,
	};
	*pivot = (struct vec2){
		.x = lerp(min->x, max->x, entity->rect.pivot.x) + entity->transform.position.x,
		.y = lerp(min->y, max->y, entity->rect.pivot.y) + entity->transform.position.y,
	};
}

struct uvec2 entity_get_content_size(
	struct Entity const * entity, struct Gfx_Material const * material,
	uint32_t viewport_size_x, uint32_t viewport_size_y
) {
	switch (entity->type) {
		case ENTITY_TYPE_NONE: return (struct uvec2){0, 0};

		case ENTITY_TYPE_MESH: return (struct uvec2){
			viewport_size_x,
			viewport_size_y
		};

		case ENTITY_TYPE_QUAD_2D: {
			struct Entity_Quad const * quad = &entity->as.quad;

			struct Gfx_Uniform_Out const value = gfx_uniforms_get(&material->uniforms, quad->texture_uniform);
			struct Ref const * gpu_texture_ref = value.data;

			uint32_t texture_size_x, texture_size_y;
			gpu_texture_get_size(*gpu_texture_ref, &texture_size_x, &texture_size_y);

			return (struct uvec2){
				texture_size_x,
				texture_size_y
			};
		} // break;

		case ENTITY_TYPE_TEXT_2D: {
			int32_t const rect[] = {
				(int32_t)maths_floor(entity->rect.anchor_min.x * (float)viewport_size_x + entity->rect.extents.x),
				(int32_t)maths_floor(entity->rect.anchor_min.y * (float)viewport_size_y + entity->rect.extents.y),
				(int32_t)maths_ceil (entity->rect.anchor_max.x * (float)viewport_size_x + entity->rect.extents.x),
				(int32_t)maths_ceil (entity->rect.anchor_max.y * (float)viewport_size_y + entity->rect.extents.y),
			};
			return (struct uvec2){
				(uint32_t)max_s32(rect[2] - rect[0], rect[0] - rect[2]),
				(uint32_t)max_s32(rect[3] - rect[1], rect[1] - rect[3]),
			};
		} // break;
	}

	logger_to_console("unknown entity type"); DEBUG_BREAK();
	return (struct uvec2){0};
}
