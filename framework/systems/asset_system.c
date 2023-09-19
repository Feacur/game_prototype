#include "framework/logger.h"

#include "framework/systems/string_system.h"

//
#include "asset_system.h"

static struct Asset_System {
	struct Handle_Table meta; // `struct Asset_Meta`
	struct Hashtable reflex;  // name string id : `struct Handle`
	struct Hashtable types;   // type string id : `struct Asset_Type`
	struct Hashtable map;     // extension string id : type string id
} gs_asset_system;

struct Asset_Meta {
	uint32_t type_id;
	uint32_t name_id;
	struct Handle inst_handle;
};

struct Asset_Type {
	struct Asset_Callbacks callbacks;
	struct Handle_Table instances; // `struct Asset_Inst`, variable type sizes
};

struct Asset_Inst {
	struct Asset_Inst_Header {
		uint32_t name_id;
		struct Handle meta_handle;
	} header;
	uint8_t payload[FLEXIBLE_ARRAY];
};

static struct CString asset_system_name_to_extension(struct CString name);

void asset_system_init(void) {
	gs_asset_system = (struct Asset_System){
		.meta = handle_table_init(sizeof(struct Asset_Meta)),
		.reflex = hashtable_init(&hash32, sizeof(uint32_t), sizeof(struct Handle)),
		.types = hashtable_init(&hash32, sizeof(uint32_t), sizeof(struct Asset_Type)),
		.map = hashtable_init(&hash32, sizeof(uint32_t), sizeof(uint32_t)),
	};
}

void asset_system_free(void) {
	FOR_HASHTABLE (&gs_asset_system.types, it_type) {
		struct Asset_Type * type = it_type.value;
		FOR_HANDLE_TABLE (&type->instances, it_inst) {
			struct Asset_Inst * inst = it_inst.value;
			if (type->callbacks.free != NULL) {
				type->callbacks.free(inst->payload);
			}
		}
		handle_table_free(&type->instances);
		if (type->callbacks.type_free != NULL) {
			type->callbacks.type_free();
		}
	}
	handle_table_free(&gs_asset_system.meta);
	hashtable_free(&gs_asset_system.reflex);
	hashtable_free(&gs_asset_system.types);
	hashtable_free(&gs_asset_system.map);
	// common_memset(&gs_asset_system, 0, sizeof(gs_asset_system));
}

void asset_system_map_extension(struct CString type, struct CString extension) {
	uint32_t const type_id = string_system_add(type);
	if (type_id == 0) { logger_to_console("empty type\n"); goto fail; }
	uint32_t const extension_id = string_system_add(extension);
	if (extension_id == 0) { logger_to_console("empty extension\n"); goto fail; }
	hashtable_set(&gs_asset_system.map, &extension_id, &type_id);

	return;
	fail: DEBUG_BREAK();
}

bool asset_system_match_type(struct Handle handle, struct CString type_name) {
	struct Asset_Meta const * meta = handle_table_get(&gs_asset_system.meta, handle);
	if (meta == NULL) { return false; }
	struct CString const handle_type = string_system_get(meta->type_id);
	return cstring_equals(handle_type, type_name);
}

void asset_system_set_type(struct CString type, struct Asset_Callbacks callbacks, uint32_t value_size) {
	uint32_t const type_id = string_system_add(type);
	hashtable_set(&gs_asset_system.types, &type_id, &(struct Asset_Type){
		.callbacks = callbacks,
		.instances = handle_table_init(SIZE_OF_MEMBER(struct Asset_Inst, header) + value_size),
	});
	if (callbacks.type_init != NULL) {
		callbacks.type_init();
	}
}

void asset_system_del_type(struct CString type_name) {
	uint32_t const type_id = string_system_find(type_name);
	struct Asset_Type * type = hashtable_get(&gs_asset_system.types, &type_id);
	if (type == NULL) { return; }

	FOR_HANDLE_TABLE (&type->instances, it_inst) {
		struct Asset_Inst * inst = it_inst.value;

		hashtable_del(&gs_asset_system.reflex, &inst->header.name_id);
		handle_table_discard(&gs_asset_system.meta, inst->header.meta_handle);

		if (type->callbacks.free != NULL) {
			type->callbacks.free(inst->payload);
		}
	}

	handle_table_free(&type->instances);
	if (type->callbacks.type_free != NULL) {
		type->callbacks.type_free();
	}

	hashtable_del(&gs_asset_system.types, &type_id);
}

struct Handle asset_system_aquire(struct CString name) {
	uint32_t name_id = string_system_add(name);
	if (name_id == 0) { return (struct Handle){0}; }

	struct Handle const * meta_handle_ptr = hashtable_get(&gs_asset_system.reflex, &name_id);
	if (meta_handle_ptr != NULL) { return *meta_handle_ptr; }

	//
	struct CString const extension = asset_system_name_to_extension(name);
	uint32_t const extension_id = string_system_find(extension);
	if (extension_id == 0) { return (struct Handle){0}; }

	uint32_t const * type_id_ptr = hashtable_get(&gs_asset_system.map, &extension_id);
	uint32_t const type_id = (type_id_ptr != NULL) ? *type_id_ptr : extension_id;

	//
	struct Asset_Type * type = hashtable_get(&gs_asset_system.types, &type_id);
	if (type == NULL) { return (struct Handle){0}; }

	//
	struct Handle const inst_handle = handle_table_aquire(&type->instances, NULL);
	struct Handle const meta_handle = handle_table_aquire(&gs_asset_system.meta, &(struct Asset_Meta){
		.type_id = type_id,
		.name_id = name_id,
		.inst_handle = inst_handle,
	});
	hashtable_set(&gs_asset_system.reflex, &name_id, &meta_handle);

	struct Asset_Inst * inst = handle_table_get(&type->instances, inst_handle);
	inst->header = (struct Asset_Inst_Header) {
		.name_id = name_id,
		.meta_handle = meta_handle,
	};

	if (type->callbacks.init != NULL) {
		type->callbacks.init(inst->payload, name);
	}

	return meta_handle;
}

void asset_system_discard(struct Handle handle) {
	struct Asset_Meta const * meta = handle_table_get(&gs_asset_system.meta, handle);
	if (meta == NULL) { return; }

	hashtable_del(&gs_asset_system.reflex, &meta->name_id);
	handle_table_discard(&gs_asset_system.meta, handle);

	struct Asset_Type * type = hashtable_get(&gs_asset_system.types, &meta->type_id);
	if (type == NULL) { return; }

	struct Asset_Inst * inst = handle_table_get(&type->instances, meta->inst_handle);
	if (inst != NULL) { return; }

	if (type->callbacks.free != NULL) {
		type->callbacks.free(inst->payload);
	}
	handle_table_discard(&type->instances, meta->inst_handle);
}

void * asset_system_take(struct Handle handle) {
	struct Asset_Meta const * meta = handle_table_get(&gs_asset_system.meta, handle);
	if (meta == NULL) { return NULL; }

	struct Asset_Type * type = hashtable_get(&gs_asset_system.types, &meta->type_id);
	struct Asset_Inst * inst = (type != NULL)
		? handle_table_get(&type->instances, meta->inst_handle)
		: NULL;
	return (inst != NULL) ? inst->payload : NULL;
}

void * asset_system_aquire_instance(struct CString name) {
	struct Handle const handle = asset_system_aquire(name);
	return asset_system_take(handle);
}

struct CString asset_system_get_name(struct Handle handle) {
	struct Asset_Meta const * meta = handle_table_get(&gs_asset_system.meta, handle);
	if (meta == NULL) { return (struct CString){0}; }
	return string_system_get(meta->name_id);
}

//

static struct CString asset_system_name_to_extension(struct CString name) {
	for (uint32_t extension_length = 0; extension_length < name.length; extension_length++) {
		// @todo: make it unicode-aware?
		char const symbol = name.data[name.length - extension_length - 1];
		if (symbol == '.') {
			return (struct CString){
				.length = extension_length,
				.data = name.data + (name.length - extension_length),
			};
		}
		if (symbol == '/') { break; }
	}
	return (struct CString){0};
}
