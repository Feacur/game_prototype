#include "framework/logger.h"
#include "framework/containers/hashmap.h"
#include "framework/containers/sparseset.h"
#include "framework/systems/string_system.h"

//
#include "asset_system.h"

static struct Asset_System {
	struct Sparseset meta;  // `struct Asset_Meta`
	struct Hashmap handles; // name string id : `struct Handle`
	struct Hashmap types;   // type string id : `struct Asset_Type`
	struct Hashmap map;     // extension string id : type string id
	uint32_t indent;
} gs_asset_system;

struct Asset_Meta {
	struct Array dependencies; // `struct Handle`
	struct Handle inst_handle;
	uint32_t ref_count; // zero-based
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

static void asset_system_report(struct Handle handle, struct CString tag);
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
	uint32_t dropped_count = 0;
	FOR_HASHMAP (&gs_asset_system.types, it_type) {
		struct Asset_Type * type = it_type.value;
		dropped_count += sparseset_get_count(&type->instances);
		FOR_SPARSESET (&type->instances, it_inst) {
			struct Asset_Inst * inst = it_inst.value;
			asset_system_report(inst->header.meta_handle, S_("[free]"));
			if (type->info.drop != NULL) {
				type->info.drop(inst->header.meta_handle);
			}
		}
		sparseset_free(&type->instances);
	}
	FOR_SPARSESET (&gs_asset_system.meta, it) {
		struct Asset_Meta * meta = it.value;
		array_free(&meta->dependencies);
	}
	sparseset_free(&gs_asset_system.meta);
	hashmap_free(&gs_asset_system.handles);
	hashmap_free(&gs_asset_system.types);
	hashmap_free(&gs_asset_system.map);
	// common_memset(&gs_asset_system, 0, sizeof(gs_asset_system));
	if (dropped_count > 0) { DEBUG_BREAK(); }
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

	uint32_t const inst_count = sparseset_get_count(&type->instances);

	logger_to_console("[type] %.*s\n", type_name.length, type_name.data);
	gs_asset_system.indent++;
	FOR_SPARSESET (&type->instances, it) {
		struct Asset_Inst * inst = it.value;
		struct Handle const handle = inst->header.meta_handle;

		asset_system_report(handle, S_(""));
		gs_asset_system.indent++;
		if (type->info.drop != NULL) {
			type->info.drop(handle);
		}
		gs_asset_system.indent--;

		struct Asset_Meta * meta = sparseset_get(&gs_asset_system.meta, handle);
		if (meta == NULL) { logger_to_console("inst w/o meta\n"); DEBUG_BREAK(); goto cleanup; }

		gs_asset_system.indent++;
		FOR_ARRAY(&meta->dependencies, it_dpdc) {
			struct Handle const * other = it_dpdc.value;
			asset_system_drop(*other);
		}
		array_free(&meta->dependencies);
		gs_asset_system.indent--;

		hashmap_del(&gs_asset_system.handles, &meta->name_id);
		cleanup: sparseset_discard(&gs_asset_system.meta, handle);
	}
	gs_asset_system.indent--;

	sparseset_free(&type->instances);
	hashmap_del(&gs_asset_system.types, &type_id);

	if (inst_count > 0) { DEBUG_BREAK(); }
}

struct Handle asset_system_load(struct CString name) {
	uint32_t name_id = string_system_add(name);
	if (name_id == 0) { return (struct Handle){0}; }

	struct Handle const * meta_handle_ptr = hashmap_get(&gs_asset_system.handles, &name_id);
	if (meta_handle_ptr != NULL) {
		struct Handle const handle = *meta_handle_ptr;
		struct Asset_Meta * meta = sparseset_get(&gs_asset_system.meta, handle);
		meta->ref_count++; asset_system_report(handle, S_("[refc]"));
		return handle;
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
	struct Handle const meta_handle = sparseset_aquire(&gs_asset_system.meta, &(struct Asset_Meta){
		.dependencies = array_init(sizeof(struct Handle)),
		.inst_handle = inst_handle,
		.type_id = type_id,
		.name_id = name_id,
	});
	hashmap_set(&gs_asset_system.handles, &name_id, &meta_handle);

	struct Asset_Inst * inst = sparseset_get(&type->instances, inst_handle);
	inst->header = (struct Asset_Inst_Header) {
		.meta_handle = meta_handle,
	};

	asset_system_report(meta_handle, S_("[load]"));
	gs_asset_system.indent++;
	if (type->info.load != NULL) {
		type->info.load(inst->header.meta_handle);
	}
	gs_asset_system.indent--;

	return meta_handle;
}

void asset_system_drop(struct Handle handle) {
	struct Asset_Meta * meta = sparseset_get(&gs_asset_system.meta, handle);
	if (meta == NULL) { return; }
	if (meta->ref_count > 0) {
		meta->ref_count--; asset_system_report(handle, S_("[unrf]"));
		return;
	}

	struct Asset_Type * type = hashmap_get(&gs_asset_system.types, &meta->type_id);
	if (type == NULL) { logger_to_console("meta w/o type\n"); DEBUG_BREAK(); goto cleanup; }

	struct Asset_Inst * inst = sparseset_get(&type->instances, meta->inst_handle);
	if (inst == NULL) { logger_to_console("meta w/o inst\n"); DEBUG_BREAK(); goto cleanup; }

	asset_system_report(handle, S_("[drop]"));
	gs_asset_system.indent++;
	if (type->info.drop != NULL) {
		type->info.drop(inst->header.meta_handle);
	}
	gs_asset_system.indent--;
	sparseset_discard(&type->instances, meta->inst_handle);

	cleanup:
	gs_asset_system.indent++;
	FOR_ARRAY(&meta->dependencies, it) {
		struct Handle const * other = it.value;
		asset_system_drop(*other);
	}
	array_free(&meta->dependencies);
	gs_asset_system.indent--;

	hashmap_del(&gs_asset_system.handles, &meta->name_id);
	sparseset_discard(&gs_asset_system.meta, handle);
}

void asset_system_add_dependency(struct Handle handle, struct Handle other) {
	struct Asset_Meta * meta = sparseset_get(&gs_asset_system.meta, handle);
	if (meta == NULL) { return; }
	array_push_many(&meta->dependencies, 1, &other);

	// gs_asset_system.indent++;
	// asset_system_report(handle, S_("[deps]"));
	// asset_system_report(other, S_("[deps] child"));
	// gs_asset_system.indent--;
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
	struct Handle const * meta_handle_ptr = hashmap_get(&gs_asset_system.handles, &name_id);
	return (meta_handle_ptr != NULL) ? *meta_handle_ptr : (struct Handle){0};
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

static void asset_system_report(struct Handle handle, struct CString tag) {
	struct Asset_Meta const * meta = sparseset_get(&gs_asset_system.meta, handle);
	struct CString const name = asset_system_get_name(handle);
	logger_to_console(
		"%*s" // @note: indentation
		"%.*s {%u:%u} (%u) %.*s\n"
		, gs_asset_system.indent * 4, ""
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
