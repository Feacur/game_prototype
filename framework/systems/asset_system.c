#include "framework/formatter.h"
#include "framework/containers/hashmap.h"
#include "framework/containers/sparseset.h"
#include "framework/systems/string_system.h"

//
#include "asset_system.h"

struct Asset_Meta {
	struct Array dependencies; // `struct Handle`
	struct Handle inst_handle;
	uint32_t ref_count; // zero-based
	uint32_t type_id;
	uint32_t name_id;
};

struct Asset_Inst {
	struct Asset_Inst_Header {
		struct Handle ah_meta;
	} header;
	uint8_t payload[FLEXIBLE_ARRAY];
};

struct Asset_Type {
	struct Asset_Info info;
	struct Sparseset instances; // `struct Asset_Inst`
};

static struct Asset_System {
	struct Sparseset meta;  // `struct Asset_Meta`
	struct Hashmap handles; // name string id : `struct Handle`
	struct Hashmap types;   // type string id : `struct Asset_Type`
	struct Hashmap map;     // extension string id : type string id
	struct Array stack;     // `struct Handle`
} gs_asset_system = {
	.meta = {
		.packed = {
			.value_size = sizeof(struct Asset_Meta),
		},
		.sparse = {
			.value_size = sizeof(struct Handle),
		},
		.ids = {
			.value_size = sizeof(uint32_t),
		},
	},
	.handles = {
		.get_hash = hash32,
		.key_size = sizeof(uint32_t),
		.value_size = sizeof(struct Handle),
	},
	.types = {
		.get_hash = hash32,
		.key_size = sizeof(uint32_t),
		.value_size = sizeof(struct Asset_Type),
	},
	.map = {
		.get_hash = hash32,
		.key_size = sizeof(uint32_t),
		.value_size = sizeof(uint32_t),
	},
	.stack = {
		.value_size = sizeof(struct Handle),
	},
};

static void asset_system_add_dependency(struct Handle handle);
static void asset_system_report(struct CString tag, struct Handle handle);
static struct CString asset_system_name_to_extension(struct CString name);

void asset_system_clear(bool deallocate) {
	// dependecies
	uint32_t dropped_count = 0;
	FOR_HASHMAP (&gs_asset_system.types, it_type) {
		struct Asset_Type * type = it_type.value;
		dropped_count += sparseset_get_count(&type->instances);
		FOR_SPARSESET (&type->instances, it_inst) {
			struct Asset_Inst * inst = it_inst.value;
			asset_system_report(S_("[free]"), inst->header.ah_meta);
			if (type->info.drop != NULL) {
				type->info.drop(inst->header.ah_meta);
			}
		}
		sparseset_free(&type->instances);
	}
	if (dropped_count > 0) { DEBUG_BREAK(); }
	FOR_SPARSESET (&gs_asset_system.meta, it) {
		struct Asset_Meta * meta = it.value;
		array_free(&meta->dependencies);
	}
	// personal
	sparseset_clear(&gs_asset_system.meta, deallocate);
	hashmap_clear(&gs_asset_system.handles, deallocate);
	hashmap_clear(&gs_asset_system.types, deallocate);
	hashmap_clear(&gs_asset_system.map, deallocate);
	array_clear(&gs_asset_system.stack, deallocate);
}

void asset_system_type_map(struct CString type, struct CString extension) {
	uint32_t const type_id = string_system_add(type);
	if (type_id == 0) { WRN("empty type"); goto fail; }
	uint32_t const extension_id = string_system_add(extension);
	if (extension_id == 0) { WRN("empty extension"); goto fail; }
	hashmap_set(&gs_asset_system.map, &extension_id, &type_id);

	return;
	fail:
	REPORT_CALLSTACK(); DEBUG_BREAK();
}

void asset_system_type_set(struct CString type_name, struct Asset_Info info) {
	uint32_t const type_id = string_system_add(type_name);
	hashmap_set(&gs_asset_system.types, &type_id, &(struct Asset_Type){
		.info = info,
		.instances = sparseset_init(SIZE_OF_MEMBER(struct Asset_Inst, header) + info.size),
	});
}

void asset_system_type_del(struct CString type_name) {
	uint32_t const type_id = string_system_find(type_name);
	struct Asset_Type * type = hashmap_get(&gs_asset_system.types, &type_id);
	if (type == NULL) { return; }

	uint32_t const inst_count = sparseset_get_count(&type->instances);

	LOG("[type] %.*s\n", type_name.length, type_name.data);
	array_push_many(&gs_asset_system.stack, 1, &(struct Handle){0});
	FOR_SPARSESET (&type->instances, it) {
		struct Asset_Inst * inst = it.value;
		struct Handle const ah_meta = inst->header.ah_meta;

		asset_system_report(S_(""), ah_meta);
		array_push_many(&gs_asset_system.stack, 1, &ah_meta);
		if (type->info.drop != NULL) {
			type->info.drop(ah_meta);
		}
		array_pop(&gs_asset_system.stack, 1);

		struct Asset_Meta * meta = sparseset_get(&gs_asset_system.meta, ah_meta);
		if (meta == NULL) { WRN("inst w/o meta"); DEBUG_BREAK(); goto cleanup; }

		array_push_many(&gs_asset_system.stack, 1, &ah_meta);
		FOR_ARRAY(&meta->dependencies, it_dpdc) {
			struct Handle const * ah_meta_child = it_dpdc.value;
			asset_system_drop(*ah_meta_child);
		}
		array_pop(&gs_asset_system.stack, 1);
		array_free(&meta->dependencies);

		hashmap_del(&gs_asset_system.handles, &meta->name_id);
		cleanup: sparseset_discard(&gs_asset_system.meta, ah_meta);
	}
	array_pop(&gs_asset_system.stack, 1);

	sparseset_free(&type->instances);
	hashmap_del(&gs_asset_system.types, &type_id);

	if (inst_count > 0) { DEBUG_BREAK(); }
}

struct Handle asset_system_load(struct CString name) {
	uint32_t name_id = string_system_add(name);
	if (name_id == 0) { return (struct Handle){0}; }

	struct Handle const * ah_meta = hashmap_get(&gs_asset_system.handles, &name_id);
	if (ah_meta != NULL) {
		asset_system_add_dependency(*ah_meta);
		struct Asset_Meta * meta = sparseset_get(&gs_asset_system.meta, *ah_meta);
		meta->ref_count++; asset_system_report(S_("[refc]"), *ah_meta);
		return *ah_meta;
	}

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
	struct Handle const ah_meta_new = sparseset_aquire(&gs_asset_system.meta, &(struct Asset_Meta){
		.dependencies = array_init(sizeof(struct Handle)),
		.inst_handle = inst_handle,
		.type_id = type_id,
		.name_id = name_id,
	});
	hashmap_set(&gs_asset_system.handles, &name_id, &ah_meta_new);
	asset_system_add_dependency(ah_meta_new);

	struct Asset_Inst * inst = sparseset_get(&type->instances, inst_handle);
	inst->header = (struct Asset_Inst_Header) {
		.ah_meta = ah_meta_new,
	};

	asset_system_report(S_("[load]"), ah_meta_new);
	array_push_many(&gs_asset_system.stack, 1, &ah_meta_new);
	if (type->info.load != NULL) {
		type->info.load(ah_meta_new);
	}
	array_pop(&gs_asset_system.stack, 1);

	return ah_meta_new;
}

void asset_system_drop(struct Handle handle) {
	struct Asset_Meta * meta = sparseset_get(&gs_asset_system.meta, handle);
	if (meta == NULL) { return; }
	if (meta->ref_count > 0) {
		asset_system_report(S_("[unrf]"), handle);
		meta->ref_count--; return;
	}

	struct Asset_Type * type = hashmap_get(&gs_asset_system.types, &meta->type_id);
	if (type == NULL) { WRN("meta w/o type"); DEBUG_BREAK(); goto cleanup; }

	struct Asset_Inst * inst = sparseset_get(&type->instances, meta->inst_handle);
	if (inst == NULL) { WRN("meta w/o inst"); DEBUG_BREAK(); goto cleanup; }

	asset_system_report(S_("[drop]"), handle);
	array_push_many(&gs_asset_system.stack, 1, &handle);
	if (type->info.drop != NULL) {
		type->info.drop(inst->header.ah_meta);
	}
	array_pop(&gs_asset_system.stack, 1);
	sparseset_discard(&type->instances, meta->inst_handle);

	cleanup:
	array_push_many(&gs_asset_system.stack, 1, &handle);
	FOR_ARRAY(&meta->dependencies, it) {
		struct Handle const * ah_meta_child = it.value;
		asset_system_drop(*ah_meta_child);
	}
	array_pop(&gs_asset_system.stack, 1);
	array_free(&meta->dependencies);

	hashmap_del(&gs_asset_system.handles, &meta->name_id);
	sparseset_discard(&gs_asset_system.meta, handle);
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

struct Handle asset_system_find(struct CString name) {
	uint32_t name_id = string_system_add(name);
	if (name_id == 0) { return (struct Handle){0}; }
	struct Handle const * ah_meta = hashmap_get(&gs_asset_system.handles, &name_id);
	return (ah_meta != NULL) ? *ah_meta : (struct Handle){0};
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

static void asset_system_add_dependency(struct Handle handle) {
	if (gs_asset_system.stack.count == 0) { return; }
	struct Handle const * ah_meta_parent = array_peek(&gs_asset_system.stack, 0);
	struct Asset_Meta * meta = sparseset_get(&gs_asset_system.meta, *ah_meta_parent);
	if (meta == NULL) { return; }
	array_push_many(&meta->dependencies, 1, &handle);
}

static void asset_system_report(struct CString tag, struct Handle handle) {
	struct Asset_Meta const * meta = sparseset_get(&gs_asset_system.meta, handle);
	struct CString const name = asset_system_get_name(handle);
	uint32_t const indent = gs_asset_system.stack.count * 4;
	LOG("%*s" "%.*s {%u:%u} (%u) %.*s\n"
		, indent, ""
		, tag.length, tag.data
		, handle.id, handle.gen
		, meta->ref_count + 1
		, name.length, name.data
	);
}

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
