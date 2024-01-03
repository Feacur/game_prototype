#include "framework/containers/hashmap.h"
#include "framework/graphics/gfx_types.h"
#include "framework/graphics/gfx_objects.h"
#include "framework/systems/strings.h"


//
#include "gfx_material.h"

// ----- ----- ----- ----- -----
//     uniforms
// ----- ----- ----- ----- -----

struct Gfx_Uniforms gfx_uniforms_init(void) {
	return (struct Gfx_Uniforms){
		.headers = array_init(sizeof(struct Gfx_Uniforms_Entry)),
		.payload = buffer_init(),
	};
}

void gfx_uniforms_free(struct Gfx_Uniforms * uniforms) {
	array_free(&uniforms->headers);
	buffer_free(&uniforms->payload);
	// zero_out(AMP_(uniforms));
}

void gfx_uniforms_clear(struct Gfx_Uniforms * uniforms) {
	array_clear(&uniforms->headers, false);
	buffer_clear(&uniforms->payload, false);
}

struct CArray_Mut gfx_uniforms_get(struct Gfx_Uniforms const * uniforms, struct CString name, uint32_t offset) {
	struct Handle const sh_name = system_strings_find(name);
	return gfx_uniforms_id_get(uniforms, sh_name, offset);
}

void gfx_uniforms_push(struct Gfx_Uniforms * uniforms, struct CString name, struct CArray value) {
	struct Handle const sh_name = system_strings_add(name);
	gfx_uniforms_id_push(uniforms, sh_name, value);
}

struct CArray_Mut gfx_uniforms_id_get(struct Gfx_Uniforms const * uniforms, struct Handle id, uint32_t offset) {
	if (handle_is_null(id)) { goto null; }
	for (uint32_t i = offset; i < uniforms->headers.count; i++) {
		struct Gfx_Uniforms_Entry const * entry = array_at(&uniforms->headers, i);
		if (!handle_equals(entry->id, id)) { continue; }
		return (struct CArray_Mut){
			.data = (uint8_t *)uniforms->payload.data + entry->offset,
			.size = entry->size,
		};
	}
	null: return (struct CArray_Mut){0};
}

void gfx_uniforms_id_push(struct Gfx_Uniforms * uniforms, struct Handle sh_name, struct CArray value) {
	if (handle_is_null(sh_name)) { return; }
	array_push_many(&uniforms->headers, 1, &(struct Gfx_Uniforms_Entry){
		.id = sh_name,
		.size = (uint32_t)value.size,
		.offset = (uint32_t)uniforms->payload.size,
	});
	buffer_push_many(&uniforms->payload, value.size, value.data);
}

bool gfx_uniforms_iterate(struct Gfx_Uniforms const * uniforms, struct Gfx_Uniforms_Iterator * iterator) {
	while (iterator->next < uniforms->headers.count) {
		uint32_t const index = iterator->next++;
		iterator->current = index;
		//
		struct Gfx_Uniforms_Entry const * entry = array_at(&uniforms->headers, index);
		iterator->id = entry->id;
		iterator->size = entry->size;
		iterator->value = (uint8_t *)uniforms->payload.data + entry->offset;
		return true;
	}
	return false;
}

// ----- ----- ----- ----- -----
//     material
// ----- ----- ----- ----- -----

struct Gfx_Material gfx_material_init(void) {
	return (struct Gfx_Material){
		.uniforms = gfx_uniforms_init(),
	};
}

void gfx_material_free(struct Gfx_Material * material) {
	gfx_uniforms_free(&material->uniforms);
	zero_out(AMP_(material));
}

void gfx_material_set_shader(struct Gfx_Material * material, struct Handle gh_program) {
	struct CString const property_prefix = S_("p_");

	material->gh_program = gh_program;
	struct GPU_Program const * program = gpu_program_get(gh_program);
	if (program == NULL) { return; }

	uint32_t payload_bytes = 0, properties_count = 0;
	FOR_HASHMAP(&program->uniforms, it) {
		struct Handle const * sh_name = it.key;
		struct CString const name = system_strings_get(*sh_name);
		if (!cstring_starts(name, property_prefix)) { continue; }

		struct GPU_Uniform const * uniform = it.value;
		payload_bytes += gfx_type_get_size(uniform->type) * uniform->array_size;
		properties_count++;
	}

	gfx_uniforms_clear(&material->uniforms);
	array_resize(&material->uniforms.headers, properties_count);
	buffer_resize(&material->uniforms.payload, payload_bytes);

	FOR_HASHMAP(&program->uniforms, it) {
		struct Handle const * sh_name = it.key;
		struct CString const name = system_strings_get(*sh_name);
		if (!cstring_starts(name, property_prefix)) { continue; }

		struct GPU_Uniform const * uniform = it.value;
		gfx_uniforms_id_push(&material->uniforms, *sh_name, (struct CArray){
			.size = gfx_type_get_size(uniform->type) * uniform->array_size,
		});
	}
	zero_out((struct CArray_Mut){
		.size = payload_bytes,
		.data = material->uniforms.payload.data,
	});
}
