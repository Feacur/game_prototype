#include "framework/formatter.h"
#include "framework/containers/hashmap.h"
#include "framework/containers/sparseset.h"
#include "framework/systems/strings.h"


//
#include "assets.h"

struct Asset_Meta {
	struct Array dependencies; // meta `struct Handle`
	struct Handle inst_handle; // into `struct Asset_Type : instances`
	struct Handle sh_type;     // get `struct CString` via `system_strings_get`
	struct Handle sh_name;     // get `struct CString` via `system_strings_get`
	uint32_t ref_count;        // zero-based
};

struct Asset_Inst {
	struct Asset_Inst_Header {
		struct Handle ah_meta; // into `struct assets : meta`
	} header;
	uint8_t payload[FLEXIBLE_ARRAY];
};

struct Asset_Type {
	struct Asset_Info info;
	struct Sparseset instances; // `struct Asset_Inst`
};

static struct Assets {
	struct Sparseset meta;  // `struct Asset_Meta`
	struct Hashmap handles; // name `struct Handle` : meta `struct Handle`
	struct Hashmap types;   // type `struct Handle` : `struct Asset_Type`
	struct Hashmap map;     // extension `struct Handle` : type `struct Handle`
	struct Array stack;     // meta `struct Handle`
} gs_assets;

static HANDLE_ACTION(system_assets_add_dependency);
static void system_assets_report(struct CString tag, struct Handle handle);
static struct CString system_assets_name_to_extension(struct CString name);

void system_assets_init(void) {
	gs_assets = (struct Assets){
		.meta = {
			.payload = {
				.value_size = sizeof(struct Asset_Meta),
			},
			.sparse = {
				.value_size = sizeof(struct Handle),
			},
			.packed = {
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
}

void system_assets_free(void) {
	uint32_t dropped_count = 0;
	FOR_HASHMAP(&gs_assets.types, it_type) {
		struct Asset_Type * type = it_type.value;
		dropped_count += sparseset_get_count(&type->instances);
		FOR_SPARSESET(&type->instances, it_inst) {
			struct Asset_Inst * inst = it_inst.value;
			system_assets_report(S_("[free]"), inst->header.ah_meta);
			if (type->info.drop != NULL) {
				type->info.drop(inst->header.ah_meta);
			}
		}
		sparseset_free(&type->instances);
	}
	if (dropped_count > 0) { DEBUG_BREAK(); }
	FOR_SPARSESET(&gs_assets.meta, it) {
		struct Asset_Meta * meta = it.value;
		array_free(&meta->dependencies);
	}
	sparseset_free(&gs_assets.meta);
	hashmap_free(&gs_assets.handles);
	hashmap_free(&gs_assets.types);
	hashmap_free(&gs_assets.map);
	array_free(&gs_assets.stack);
}

void system_assets_type_map(struct CString type, struct CString extension) {
	struct Handle const sh_type = system_strings_add(type);
	if (handle_is_null(sh_type)) { WRN("empty type"); goto fail; }
	struct Handle const sh_extension = system_strings_add(extension);
	if (handle_is_null(sh_extension)) { WRN("empty extension"); goto fail; }
	hashmap_set(&gs_assets.map, &sh_extension, &sh_type);

	return;
	fail:
	REPORT_CALLSTACK(); DEBUG_BREAK();
}

void system_assets_type_set(struct CString type_name, struct Asset_Info info) {
	struct Handle const sh_type = system_strings_add(type_name);
	hashmap_set(&gs_assets.types, &sh_type, &(struct Asset_Type){
		.info = info,
		.instances = sparseset_init(SIZE_OF_MEMBER(struct Asset_Inst, header) + info.size),
	});
}

void system_assets_type_del(struct CString type_name) {
	struct Handle const sh_type = system_strings_find(type_name);
	struct Asset_Type * type = hashmap_get(&gs_assets.types, &sh_type);
	if (type == NULL) { return; }

	uint32_t const inst_count = sparseset_get_count(&type->instances);

	LOG("[type] %.*s\n", type_name.length, type_name.data);
	array_push_many(&gs_assets.stack, 1, &(struct Handle){0});
	FOR_SPARSESET(&type->instances, it) {
		struct Asset_Inst * inst = it.value;
		struct Handle const ah_meta = inst->header.ah_meta;

		system_assets_report(S_(""), ah_meta);
		array_push_many(&gs_assets.stack, 1, &ah_meta);
		if (type->info.drop != NULL) {
			type->info.drop(ah_meta);
		}
		array_pop(&gs_assets.stack, 1);

		struct Asset_Meta * meta = sparseset_get(&gs_assets.meta, ah_meta);
		if (meta == NULL) { WRN("inst w/o meta"); DEBUG_BREAK(); goto cleanup; }

		array_push_many(&gs_assets.stack, 1, &ah_meta);
		FOR_ARRAY(&meta->dependencies, it_dpdc) {
			struct Handle const * ah_meta_child = it_dpdc.value;
			system_assets_drop(*ah_meta_child);
		}
		array_pop(&gs_assets.stack, 1);
		array_free(&meta->dependencies);

		hashmap_del(&gs_assets.handles, &meta->sh_name);
		cleanup: sparseset_discard(&gs_assets.meta, ah_meta);
	}
	array_pop(&gs_assets.stack, 1);

	sparseset_free(&type->instances);
	hashmap_del(&gs_assets.types, &sh_type);

	if (inst_count > 0) { DEBUG_BREAK(); }
}

struct Handle system_assets_load(struct CString name) {
	struct Handle const sh_name = system_strings_add(name);
	if (handle_is_null(sh_name)) { return (struct Handle){0}; }

	struct Handle const * ah_meta = hashmap_get(&gs_assets.handles, &sh_name);
	if (ah_meta != NULL) {
		system_assets_add_dependency(*ah_meta);
		struct Asset_Meta * meta = sparseset_get(&gs_assets.meta, *ah_meta);
		meta->ref_count++; system_assets_report(S_("[refc]"), *ah_meta);
		return *ah_meta;
	}

	//
	struct CString const extension = system_assets_name_to_extension(name);
	struct Handle const sh_extension = system_strings_find(extension);
	if (handle_is_null(sh_extension)) { return (struct Handle){0}; }

	struct Handle const * sh_type_ptr = hashmap_get(&gs_assets.map, &sh_extension);
	struct Handle const sh_type = (sh_type_ptr != NULL) ? *sh_type_ptr : sh_extension;

	//
	struct Asset_Type * type = hashmap_get(&gs_assets.types, &sh_type);
	if (type == NULL) { return (struct Handle){0}; }

	//
	struct Handle const inst_handle = sparseset_aquire(&type->instances, NULL);
	struct Handle const ah_meta_new = sparseset_aquire(&gs_assets.meta, &(struct Asset_Meta){
		.dependencies = array_init(sizeof(struct Handle)),
		.inst_handle = inst_handle,
		.sh_type = sh_type,
		.sh_name = sh_name,
	});
	hashmap_set(&gs_assets.handles, &sh_name, &ah_meta_new);
	system_assets_add_dependency(ah_meta_new);

	struct Asset_Inst * inst = sparseset_get(&type->instances, inst_handle);
	inst->header = (struct Asset_Inst_Header) {
		.ah_meta = ah_meta_new,
	};

	system_assets_report(S_("[load]"), ah_meta_new);
	array_push_many(&gs_assets.stack, 1, &ah_meta_new);
	if (type->info.load != NULL) {
		type->info.load(ah_meta_new);
	}
	array_pop(&gs_assets.stack, 1);

	return ah_meta_new;
}

HANDLE_ACTION(system_assets_drop) {
	struct Asset_Meta * meta = sparseset_get(&gs_assets.meta, handle);
	if (meta == NULL) { return; }
	if (meta->ref_count > 0) {
		system_assets_report(S_("[unrf]"), handle);
		meta->ref_count--; return;
	}

	struct Asset_Type * type = hashmap_get(&gs_assets.types, &meta->sh_type);
	if (type == NULL) { WRN("meta w/o type"); DEBUG_BREAK(); goto cleanup; }

	struct Asset_Inst * inst = sparseset_get(&type->instances, meta->inst_handle);
	if (inst == NULL) { WRN("meta w/o inst"); DEBUG_BREAK(); goto cleanup; }

	system_assets_report(S_("[drop]"), handle);
	array_push_many(&gs_assets.stack, 1, &handle);
	if (type->info.drop != NULL) {
		type->info.drop(inst->header.ah_meta);
	}
	array_pop(&gs_assets.stack, 1);
	sparseset_discard(&type->instances, meta->inst_handle);

	cleanup:
	array_push_many(&gs_assets.stack, 1, &handle);
	FOR_ARRAY(&meta->dependencies, it) {
		struct Handle const * ah_meta_child = it.value;
		system_assets_drop(*ah_meta_child);
	}
	array_pop(&gs_assets.stack, 1);
	array_free(&meta->dependencies);

	hashmap_del(&gs_assets.handles, &meta->sh_name);
	sparseset_discard(&gs_assets.meta, handle);
}

void * system_assets_get(struct Handle handle) {
	struct Asset_Meta const * meta = sparseset_get(&gs_assets.meta, handle);
	if (meta == NULL) { return NULL; }

	struct Asset_Type * type = hashmap_get(&gs_assets.types, &meta->sh_type);
	struct Asset_Inst * inst = (type != NULL)
		? sparseset_get(&type->instances, meta->inst_handle)
		: NULL;
	return (inst != NULL) ? inst->payload : NULL;
}

struct Handle system_assets_find(struct CString name) {
	struct Handle const sh_name = system_strings_add(name);
	if (handle_is_null(sh_name)) { return (struct Handle){0}; }
	struct Handle const * ah_meta = hashmap_get(&gs_assets.handles, &sh_name);
	return (ah_meta != NULL) ? *ah_meta : (struct Handle){0};
}

struct CString system_assets_get_type(struct Handle handle) {
	struct Asset_Meta const * meta = sparseset_get(&gs_assets.meta, handle);
	if (meta == NULL) { return (struct CString){0}; }
	return system_strings_get(meta->sh_type);
}

struct CString system_assets_get_name(struct Handle handle) {
	struct Asset_Meta const * meta = sparseset_get(&gs_assets.meta, handle);
	if (meta == NULL) { return (struct CString){0}; }
	return system_strings_get(meta->sh_name);
}

//

static HANDLE_ACTION(system_assets_add_dependency) {
	if (gs_assets.stack.count == 0) { return; }
	struct Handle const * ah_meta_parent = array_peek(&gs_assets.stack, 0);
	struct Asset_Meta * meta = sparseset_get(&gs_assets.meta, *ah_meta_parent);
	if (meta == NULL) { return; }
	array_push_many(&meta->dependencies, 1, &handle);
}

static void system_assets_report(struct CString tag, struct Handle handle) {
	struct Asset_Meta const * meta = sparseset_get(&gs_assets.meta, handle);
	struct CString const name = system_assets_get_name(handle);
	uint32_t const indent = gs_assets.stack.count * 4;
	LOG("%*s" "%.*s {%u:%u} (%u) %.*s\n"
		, indent, ""
		, tag.length, tag.data
		, handle.id, handle.gen
		, meta->ref_count + 1
		, name.length, name.data
	);
}

static struct CString system_assets_name_to_extension(struct CString name) {
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
