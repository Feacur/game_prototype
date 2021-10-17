#include "framework/logger.h"
#include "framework/graphics/types.h"
#include "framework/graphics/gpu_objects.h"

//
#include "material.h"

void gfx_material_init(
	struct Gfx_Material * material,
	struct Ref gpu_program_ref,
	struct Blend_Mode const * blend_mode,
	struct Depth_Mode const * depth_mode
) {
	material->gpu_program_ref = gpu_program_ref;
	material->blend_mode = (blend_mode != NULL) ? *blend_mode : blend_mode_opaque;
	material->depth_mode = (blend_mode != NULL) ? *depth_mode : (struct Depth_Mode){.enabled = true, .mask = true};

	array_any_init(&material->textures, sizeof(struct Ref));
	array_u32_init(&material->values_u32);
	array_s32_init(&material->values_s32);
	array_float_init(&material->values_float);

	uint32_t uniforms_count;
	struct Gpu_Program_Field const * uniforms;
	gpu_program_get_uniforms(gpu_program_ref, &uniforms_count, &uniforms);

	uint32_t unit_count = 0, u32_count = 0, s32_count = 0, float_count = 0;
	for (uint32_t i = 0; i < uniforms_count; i++) {
		uint32_t const elements_count = data_type_get_count(uniforms[i].type) * uniforms[i].array_size;
		switch (data_type_get_element_type(uniforms[i].type)) {
			default: logger_to_console("unknown data type\n"); DEBUG_BREAK(); break;

			case DATA_TYPE_UNIT: unit_count  += elements_count; break;
			case DATA_TYPE_U32:  u32_count   += elements_count; break;
			case DATA_TYPE_S32:  s32_count   += elements_count; break;
			case DATA_TYPE_R32:  float_count += elements_count; break;
		}
	}

	array_any_resize(&material->textures,       unit_count);
	array_u32_resize(&material->values_u32,     u32_count);
	array_s32_resize(&material->values_s32,     s32_count);
	array_float_resize(&material->values_float, float_count);

	array_any_resize(&material->textures,       unit_count);  common_memset(material->textures.data,     0, sizeof(struct Ref) * unit_count);
	array_u32_resize(&material->values_u32,     u32_count);   common_memset(material->values_u32.data,   0, sizeof(uint32_t) * u32_count);
	array_s32_resize(&material->values_s32,     s32_count);   common_memset(material->values_s32.data,   0, sizeof(int32_t) * s32_count);
	array_float_resize(&material->values_float, float_count); common_memset(material->values_float.data, 0, sizeof(float) * float_count);

	material->textures.count     = unit_count;
	material->values_u32.count   = u32_count;
	material->values_s32.count   = s32_count;
	material->values_float.count = float_count;
}

void gfx_material_free(struct Gfx_Material * material) {
	array_any_free(&material->textures);
	array_u32_free(&material->values_u32);
	array_s32_free(&material->values_s32);
	array_float_free(&material->values_float);
	common_memset(material, 0, sizeof(*material));
}

static void gfx_material_set_value(
	struct Gfx_Material * material, uint32_t uniform_id, enum Data_Type type,
	uint8_t * target, uint32_t value_size,
	uint32_t values_count, void const * value
);

void gfx_material_set_texture(struct Gfx_Material * material, uint32_t uniform_id, uint32_t count, struct Ref const * value) {
	gfx_material_set_value(
		material, uniform_id, DATA_TYPE_UNIT,
		(uint8_t *)material->textures.data, material->textures.value_size,
		count, value
	);
}

void gfx_material_set_u32(struct Gfx_Material * material, uint32_t uniform_id, uint32_t count, uint32_t const * value) {
	gfx_material_set_value(
		material, uniform_id, DATA_TYPE_U32,
		(uint8_t *)material->values_u32.data, sizeof(*material->values_u32.data),
		count, value
	);
}

void gfx_material_set_s32(struct Gfx_Material * material, uint32_t uniform_id, uint32_t count, int32_t const * value) {
	gfx_material_set_value(
		material, uniform_id, DATA_TYPE_S32,
		(uint8_t *)material->values_s32.data, sizeof(*material->values_s32.data),
		count, value
	);
}

void gfx_material_set_float(struct Gfx_Material * material, uint32_t uniform_id, uint32_t count, float const * value) {
	gfx_material_set_value(
		material, uniform_id, DATA_TYPE_R32,
		(uint8_t *)material->values_float.data, sizeof(*material->values_float.data),
		count, value
	);
}

static void * gfx_material_get_value(
	struct Gfx_Material const * material, uint32_t uniform_id, enum Data_Type type,
	uint8_t * source, uint32_t value_size
);

struct Ref * gfx_material_get_texture(struct Gfx_Material const * material, uint32_t uniform_id) {
	return gfx_material_get_value(
		material, uniform_id, DATA_TYPE_UNIT,
		(uint8_t *)material->textures.data, sizeof(struct Ref)
	);
}

uint32_t * gfx_material_get_u32(struct Gfx_Material const * material, uint32_t uniform_id) {
	return gfx_material_get_value(
		material, uniform_id, DATA_TYPE_U32,
		(uint8_t *)material->values_u32.data, sizeof(*material->values_u32.data)
	);
}

int32_t * gfx_material_get_s32(struct Gfx_Material const * material, uint32_t uniform_id) {
	return gfx_material_get_value(
		material, uniform_id, DATA_TYPE_S32,
		(uint8_t *)material->values_s32.data, sizeof(*material->values_s32.data)
	);
}

float * gfx_material_get_float(struct Gfx_Material const * material, uint32_t uniform_id) {
	return gfx_material_get_value(
		material, uniform_id, DATA_TYPE_R32,
		(uint8_t *)material->values_float.data, sizeof(*material->values_float.data)
	);
}

//

static void * gfx_material_get_value(
	struct Gfx_Material const * material, uint32_t uniform_id, enum Data_Type type,
	uint8_t * source, uint32_t value_size
) {
	uint32_t uniforms_count;
	struct Gpu_Program_Field const * uniforms;
	gpu_program_get_uniforms(material->gpu_program_ref, &uniforms_count, &uniforms);

	uint32_t offset = 0;
	for (uint32_t i = 0; i < uniforms_count; i++) {
		if (data_type_get_element_type(uniforms[i].type) != type) { continue; }

		uint32_t const elements_count = data_type_get_count(uniforms[i].type) * uniforms[i].array_size;
		if (uniforms[i].id != uniform_id) { offset += elements_count; continue; }

		return source + offset * value_size;
	}

	logger_to_console("material doesn't have such a property\n"); DEBUG_BREAK();
	return NULL;
}

static void gfx_material_set_value(
	struct Gfx_Material * material, uint32_t uniform_id, enum Data_Type type,
	uint8_t * target, uint32_t value_size,
	uint32_t values_count, void const * value
) {
	uint32_t uniforms_count;
	struct Gpu_Program_Field const * uniforms;
	gpu_program_get_uniforms(material->gpu_program_ref, &uniforms_count, &uniforms);

	uint32_t offset = 0;
	for (uint32_t i = 0; i < uniforms_count; i++) {
		if (data_type_get_element_type(uniforms[i].type) != type) { continue; }

		uint32_t const elements_count = data_type_get_count(uniforms[i].type) * uniforms[i].array_size;
		if (uniforms[i].id != uniform_id) { offset += elements_count; continue; }

		if (values_count != elements_count) {
			logger_to_console("data is too large\n"); DEBUG_BREAK();
			return;
		}

		common_memcpy(target + offset * value_size, value, value_size * values_count);
		return;
	}

	logger_to_console("material doesn't have such a property\n"); DEBUG_BREAK();
}
