#include "framework/graphics/types.h"
#include "framework/graphics/gpu_objects.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//
#include "material.h"

void gfx_material_init(struct Gfx_Material * material, struct Gpu_Program * gpu_program) {
	material->program = gpu_program;
	array_any_init(&material->textures, sizeof(struct Ref));
	array_u32_init(&material->values_u32);
	array_s32_init(&material->values_s32);
	array_float_init(&material->values_float);

	uint32_t uniforms_count;
	struct Gpu_Program_Field const * uniforms;
	gpu_program_get_uniforms(gpu_program, &uniforms_count, &uniforms);

	uint32_t unit_count = 0, u32_count = 0, s32_count = 0, float_count = 0;
	for (uint32_t i = 0; i < uniforms_count; i++) {
		uint32_t const elements_count = data_type_get_count(uniforms[i].type) * uniforms[i].array_size;
		switch (data_type_get_element_type(uniforms[i].type)) {
			default: fprintf(stderr, "unknown data type\n"); DEBUG_BREAK(); break;

			case DATA_TYPE_UNIT: unit_count  += elements_count; break;
			case DATA_TYPE_U32:  u32_count   += elements_count; break;
			case DATA_TYPE_S32:  s32_count   += elements_count; break;
			case DATA_TYPE_R32:  float_count += elements_count; break;
		}
	}

	array_any_resize(&material->textures, unit_count);
	array_u32_resize(&material->values_u32, u32_count);
	array_s32_resize(&material->values_s32, s32_count);
	array_float_resize(&material->values_float, float_count);

	array_any_resize(&material->textures,       unit_count);  memset(material->textures.data,     0, sizeof(struct Ref) * unit_count);
	array_u32_resize(&material->values_u32,     u32_count);   memset(material->values_u32.data,   0, sizeof(uint32_t) * u32_count);
	array_s32_resize(&material->values_s32,     s32_count);   memset(material->values_s32.data,   0, sizeof(int32_t) * s32_count);
	array_float_resize(&material->values_float, float_count); memset(material->values_float.data, 0, sizeof(float) * float_count);
}

void gfx_material_free(struct Gfx_Material * material) {
	material->program = NULL;
	array_any_free(&material->textures);
	array_u32_free(&material->values_u32);
	array_s32_free(&material->values_s32);
	array_float_free(&material->values_float);
}

static void gfx_material_set_value(
	struct Gfx_Material * material, uint32_t uniform_id, enum Data_Type type,
	uint8_t * target,
	uint32_t values_count, uint32_t value_size, void const * value
);

void gfx_material_set_texture(struct Gfx_Material * material, uint32_t uniform_id, uint32_t count, struct Ref * value) {
	gfx_material_set_value(
		material, uniform_id, DATA_TYPE_UNIT,
		(uint8_t *)material->textures.data,
		count, sizeof(*material->textures.data), value
	);
}

void gfx_material_set_u32(struct Gfx_Material * material, uint32_t uniform_id, uint32_t count, uint32_t const * value) {
	gfx_material_set_value(
		material, uniform_id, DATA_TYPE_U32,
		(uint8_t *)material->values_u32.data,
		count, sizeof(*material->values_u32.data), value
	);
}

void gfx_material_set_s32(struct Gfx_Material * material, uint32_t uniform_id, uint32_t count, int32_t const * value) {
	gfx_material_set_value(
		material, uniform_id, DATA_TYPE_S32,
		(uint8_t *)material->values_s32.data,
		count, sizeof(*material->values_s32.data), value
	);
}

void gfx_material_set_float(struct Gfx_Material * material, uint32_t uniform_id, uint32_t count, float const * value) {
	gfx_material_set_value(
		material, uniform_id, DATA_TYPE_R32,
		(uint8_t *)material->values_float.data,
		count, sizeof(*material->values_float.data), value
	);
}

//

static void gfx_material_set_value(
	struct Gfx_Material * material, uint32_t uniform_id, enum Data_Type type,
	uint8_t * target,
	uint32_t values_count, uint32_t value_size, void const * value
) {
	uint32_t uniforms_count;
	struct Gpu_Program_Field const * uniforms;
	gpu_program_get_uniforms(material->program, &uniforms_count, &uniforms);

	uint32_t offset = 0;
	for (uint32_t i = 0; i < uniforms_count; i++) {
		if (data_type_get_element_type(uniforms[i].type) != type) { continue; }

		uint32_t const elements_count = data_type_get_count(uniforms[i].type) * uniforms[i].array_size;
		if (uniforms[i].id != uniform_id) { offset += elements_count; continue; }

		if (values_count != elements_count) {
			fprintf(stderr, "data is too large\n"); DEBUG_BREAK();
			return;
		}

		memcpy(target + offset * value_size, value, value_size * values_count);
		return;
	}

	fprintf(stderr, "material doesn't have such a property\n"); DEBUG_BREAK();
}
