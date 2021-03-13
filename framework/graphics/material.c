#include "framework/graphics/types.h"
#include "framework/graphics/gpu_objects.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
#include "material.h"

void material_init(struct Material * material, struct Gpu_Program * gpu_program) {
	material->program = gpu_program;
	array_pointer_init(&material->textures);
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

	array_pointer_resize(&material->textures, unit_count);
	array_u32_resize(&material->values_u32, u32_count);
	array_s32_resize(&material->values_s32, s32_count);
	array_float_resize(&material->values_float, float_count);

	array_pointer_write_many_zeroes(&material->textures, unit_count);
	array_u32_write_many_zeroes(&material->values_u32, u32_count);
	array_s32_write_many_zeroes(&material->values_s32, s32_count);
	array_float_write_many_zeroes(&material->values_float, float_count);
}

void material_free(struct Material * material) {
	material->program = NULL;
	array_pointer_free(&material->textures);
	array_u32_free(&material->values_u32);
	array_s32_free(&material->values_s32);
	array_float_free(&material->values_float);
}

void material_set_texture(struct Material * material, uint32_t uniform_id, uint32_t count, struct Gpu_Texture ** value) {
	uint32_t uniforms_count;
	struct Gpu_Program_Field const * uniforms;
	gpu_program_get_uniforms(material->program, &uniforms_count, &uniforms);

	uint32_t offset = 0;
	for (uint32_t i = 0; i < uniforms_count; i++) {
		if (data_type_get_element_type(uniforms[i].type) != DATA_TYPE_UNIT) { continue; }

		uint32_t const elements_count = data_type_get_count(uniforms[i].type) * uniforms[i].array_size;
		if (uniforms[i].id != uniform_id) { offset += elements_count; continue; }

		memcpy(material->textures.data + offset, value, count * sizeof(*value));
		break;
	}
}

void material_set_u32(struct Material * material, uint32_t uniform_id, uint32_t count, uint32_t const * value) {
	uint32_t uniforms_count;
	struct Gpu_Program_Field const * uniforms;
	gpu_program_get_uniforms(material->program, &uniforms_count, &uniforms);

	uint32_t offset = 0;
	for (uint32_t i = 0; i < uniforms_count; i++) {
		if (data_type_get_element_type(uniforms[i].type) != DATA_TYPE_U32) { continue; }

		uint32_t const elements_count = data_type_get_count(uniforms[i].type) * uniforms[i].array_size;
		if (uniforms[i].id != uniform_id) { offset += elements_count; continue; }

		memcpy(material->values_u32.data + offset, value, count * sizeof(*value));
		break;
	}
}

void material_set_s32(struct Material * material, uint32_t uniform_id, uint32_t count, int32_t const * value) {
	uint32_t uniforms_count;
	struct Gpu_Program_Field const * uniforms;
	gpu_program_get_uniforms(material->program, &uniforms_count, &uniforms);

	uint32_t offset = 0;
	for (uint32_t i = 0; i < uniforms_count; i++) {
		if (data_type_get_element_type(uniforms[i].type) != DATA_TYPE_S32) { continue; }

		uint32_t const elements_count = data_type_get_count(uniforms[i].type) * uniforms[i].array_size;
		if (uniforms[i].id != uniform_id) { offset += elements_count; continue; }

		memcpy(material->values_s32.data + offset, value, count * sizeof(*value));
		break;
	}
}

void material_set_float(struct Material * material, uint32_t uniform_id, uint32_t count, float const * value) {
	uint32_t uniforms_count;
	struct Gpu_Program_Field const * uniforms;
	gpu_program_get_uniforms(material->program, &uniforms_count, &uniforms);

	uint32_t offset = 0;
	for (uint32_t i = 0; i < uniforms_count; i++) {
		if (data_type_get_element_type(uniforms[i].type) != DATA_TYPE_R32) { continue; }

		uint32_t const elements_count = data_type_get_count(uniforms[i].type) * uniforms[i].array_size;
		if (uniforms[i].id != uniform_id) { offset += elements_count; continue; }

		memcpy(material->values_float.data + offset, value, count * sizeof(*value));
		break;
	}
}
