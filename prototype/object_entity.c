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
	*min = (struct vec2){
		entity->rect.min_relative.x * (float)viewport_size_x + entity->rect.min_absolute.x,
		entity->rect.min_relative.y * (float)viewport_size_y + entity->rect.min_absolute.y,
	};
	*max = (struct vec2){
		entity->rect.max_relative.x * (float)viewport_size_x + entity->rect.max_absolute.x,
		entity->rect.max_relative.y * (float)viewport_size_y + entity->rect.max_absolute.y,
	};
	*pivot = (struct vec2){
		.x = lerp(min->x, max->x, entity->rect.pivot.x) + entity->transform.position.x,
		.y = lerp(min->y, max->y, entity->rect.pivot.y) + entity->transform.position.y,
	};
}

struct uvec2 entity_get_content_size(
	struct Entity const * entity,
	uint32_t viewport_size_x, uint32_t viewport_size_y
) {
	struct Gfx_Material * material = array_any_at(&gs_game.materials, entity->material);

	switch (entity->type) {
		case ENTITY_TYPE_NONE: return (struct uvec2){0, 0};

		case ENTITY_TYPE_MESH: return (struct uvec2){
			viewport_size_x,
			viewport_size_y
		};

		case ENTITY_TYPE_QUAD_2D: {
			struct Entity_Quad const * quad = &entity->as.quad;

			struct Ref const * gpu_texture_ref = gfx_material_get_texture(material, quad->texture_uniform);

			uint32_t texture_size_x, texture_size_y;
			gpu_texture_get_size(*gpu_texture_ref, &texture_size_x, &texture_size_y);

			return (struct uvec2){
				texture_size_x,
				texture_size_y
			};
		} // break;

		case ENTITY_TYPE_TEXT_2D: {
			int32_t const rect[] = {
				(int32_t)maths_floor(entity->rect.min_relative.x * (float)viewport_size_x + entity->rect.min_absolute.x),
				(int32_t)maths_floor(entity->rect.min_relative.y * (float)viewport_size_y + entity->rect.min_absolute.y),
				(int32_t)maths_ceil (entity->rect.max_relative.x * (float)viewport_size_x + entity->rect.max_absolute.x),
				(int32_t)maths_ceil (entity->rect.max_relative.y * (float)viewport_size_y + entity->rect.max_absolute.y),
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
