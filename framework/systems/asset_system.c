#include "framework/logger.h"

#include "framework/systems/string_system.h"

//
#include "asset_system.h"

static struct Asset_System {
	struct Sparseset meta;  // `struct Asset_Meta`
	struct Hashmap handles; // name string id : `struct Handle`
	struct Hashmap types;   // type string id : `struct Asset_Type`
	struct Hashmap map;     // extension string id : type string id
} gs_asset_system;

struct Asset_Meta {
	struct Handle inst_handle;
	uint32_t type_id;
	uint32_t name_id;
};

struct Asset_Type {
	struct Asset_Info info;
	struct Sparseset instances; // `struct Asset_Inst`
};

struct Asset_Inst {
	struct Asset_Inst_Header {
		struct Handle meta_handle;
	} header;
	uint8_t payload[FLEXIBLE_ARRAY];
};

static struct CString asset_system_name_to_extension(struct CString name);

void asset_system_init(void) {
	gs_asset_system = (struct Asset_System){
		.meta = sparseset_init(sizeof(struct Asset_Meta)),
		.handles = hashmap_init(&hash32, sizeof(uint32_t), sizeof(struct Handle)),
		.types = hashmap_init(&hash32, sizeof(uint32_t), sizeof(struct Asset_Type)),
		.map = hashmap_init(&hash32, sizeof(uint32_t), sizeof(uint32_t)),
	};
}

void asset_system_free(void) {
	FOR_HASHMAP (&gs_asset_system.types, it_type) {
		struct Asset_Type * type = it_type.value;
		FOR_SPARSESET (&type->instances, it_inst) {
			struct Asset_Inst * inst = it_inst.value;
			if (type->info.drop != NULL) {
				type->info.drop(inst->payload);
			}
		}
		sparseset_free(&type->instances);
	}
	sparseset_free(&gs_asset_system.meta);
	hashmap_free(&gs_asset_system.handles);
	hashmap_free(&gs_asset_system.types);
	hashmap_free(&gs_asset_system.map);
	// common_memset(&gs_asset_system, 0, sizeof(gs_asset_system));
}

void asset_system_map_extension(struct CString type, struct CString extension) {
	uint32_t const type_id = string_system_add(type);
	if (type_id == 0) { logger_to_console("empty type\n"); goto fail; }
	uint32_t const extension_id = string_system_add(extension);
	if (extension_id == 0) { logger_to_console("empty extension\n"); goto fail; }
	hashmap_set(&gs_asset_system.map, &extension_id, &type_id);

	return;
	fail:
	REPORT_CALLSTACK(); DEBUG_BREAK();
}

void asset_system_set_type(struct CString type, struct Asset_Info info) {
	uint32_t const type_id = string_system_add(type);
	hashmap_set(&gs_asset_system.types, &type_id, &(struct Asset_Type){
		.info = info,
		.instances = sparseset_init(SIZE_OF_MEMBER(struct Asset_Inst, header) + info.size),
	});
}

void asset_system_del_type(struct CString type_name) {
	uint32_t const type_id = string_system_find(type_name);
	struct Asset_Type * type = hashmap_get(&gs_asset_system.types, &type_id);
	if (type == NULL) { return; }

	FOR_SPARSESET (&type->instances, it_inst) {
		struct Asset_Inst * inst = it_inst.value;

		struct Asset_Meta const * meta = sparseset_get(&gs_asset_system.meta, inst->header.meta_handle);
		hashmap_del(&gs_asset_system.handles, &meta->name_id);
		sparseset_discard(&gs_asset_system.meta, inst->header.meta_handle);

		if (type->info.drop != NULL) {
			type->info.drop(inst->payload);
		}
	}

	sparseset_free(&type->instances);

	hashmap_del(&gs_asset_system.types, &type_id);
}

struct Handle asset_system_load(struct CString name) {
	uint32_t name_id = string_system_add(name);
	if (name_id == 0) { return (struct Handle){0}; }

	struct Handle const * meta_handle_ptr = hashmap_get(&gs_asset_system.handles, &name_id);
	if (meta_handle_ptr != NULL) { return *meta_handle_ptr; }

	//
	struct CString const extension = asset_system_name_to_extension(name);
	uint32_t const extension_id = string_system_find(extension);
	if (extension_id == 0) { return (struct Handle){0}; }

	uint32_t const * type_id_ptr = hashmap_get(&gs_asset_system.map, &extension_id);
	uint32_t const type_id = (type_id_ptr != NULL) ? *type_id_ptr : extension_id;

	//
	struct Asset_Type * type = hashmap_get(&gs_asset_system.types, &type_id);
	if (type == NULL) { return (struct Handle){0}; }

	//
	struct Handle const inst_handle = sparseset_aquire(&type->instances, NULL);
	struct Handle const meta_handle = sparseset_aquire(&gs_asset_system.meta, &(struct Asset_Meta){
		.inst_handle = inst_handle,
		.type_id = type_id,
		.name_id = name_id,
	});
	hashmap_set(&gs_asset_system.handles, &name_id, &meta_handle);

	struct Asset_Inst * inst = sparseset_get(&type->instances, inst_handle);
	inst->header = (struct Asset_Inst_Header) {
		.meta_handle = meta_handle,
	};

	if (type->info.load != NULL) {
		type->info.load(inst->payload, name);
	}

	return meta_handle;
}

void asset_system_drop(struct Handle handle) {
	struct Asset_Meta const * meta = sparseset_get(&gs_asset_system.meta, handle);
	if (meta == NULL) { return; }

	hashmap_del(&gs_asset_system.handles, &meta->name_id);
	sparseset_discard(&gs_asset_system.meta, handle);

	struct Asset_Type * type = hashmap_get(&gs_asset_system.types, &meta->type_id);
	if (type == NULL) { return; }

	struct Asset_Inst * inst = sparseset_get(&type->instances, meta->inst_handle);
	if (inst != NULL) { return; }

	if (type->info.drop != NULL) {
		type->info.drop(inst->payload);
	}
	sparseset_discard(&type->instances, meta->inst_handle);
}

void * asset_system_get(struct Handle handle) {
	struct Asset_Meta const * meta = sparseset_get(&gs_asset_system.meta, handle);
	if (meta == NULL) { return NULL; }

	struct Asset_Type * type = hashmap_get(&gs_asset_system.types, &meta->type_id);
	struct Asset_Inst * inst = (type != NULL)
		? sparseset_get(&type->instances, meta->inst_handle)
		: NULL;
	return (inst != NULL) ? inst->payload : NULL;
}

struct CString asset_system_get_type(struct Handle handle) {
	struct Asset_Meta const * meta = sparseset_get(&gs_asset_system.meta, handle);
	if (meta == NULL) { return (struct CString){0}; }
	return string_system_get(meta->type_id);
}

struct CString asset_system_get_name(struct Handle handle) {
	struct Asset_Meta const * meta = sparseset_get(&gs_asset_system.meta, handle);
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
