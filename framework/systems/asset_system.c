#include "framework/logger.h"

#include "framework/systems/string_system.h"

//
#include "asset_system.h"

static struct Asset_System {
	struct Hash_Table_U32 handles; // name string id : `struct Handle`
	struct Hash_Table_U32 types;   // type string id : `struct Asset_Type`
	struct Hash_Table_U32 map;     // extension string id : type string id
} gs_asset_system;

struct Asset_Type {
	struct Asset_Callbacks callbacks;
	struct Handle_Table instances; // `struct Asset_Entry`, variable type sizes
};

struct Asset_Entry {
	struct {
		uint32_t name_id;
	} header;
	uint8_t payload[FLEXIBLE_ARRAY];
};

static void asset_system_del_type_internal(struct Asset_Type * asset_type);
static struct CString asset_system_name_to_extension(struct CString name);

void asset_system_init(void) {
	gs_asset_system = (struct Asset_System){
		.types = hash_table_u32_init(sizeof(struct Asset_Type)),
		.handles = hash_table_u32_init(sizeof(struct Handle)),
		.map = hash_table_u32_init(sizeof(uint32_t)),
	};
}

void asset_system_free(void) {
	FOR_HASH_TABLE_U32 (&gs_asset_system.types, it) {
		asset_system_del_type_internal(it.value);
	}
	hash_table_u32_free(&gs_asset_system.types);
	hash_table_u32_free(&gs_asset_system.handles);
	hash_table_u32_free(&gs_asset_system.map);
	// common_memset(&gs_asset_system, 0, sizeof(gs_asset_system));
}

void asset_system_map_extension(struct CString type, struct CString extension) {
	uint32_t const type_id = string_system_add(type);
	if (type_id == 0) { logger_to_console("empty type\n"); goto fail; }
	uint32_t const extension_id = string_system_add(extension);
	if (extension_id == 0) { logger_to_console("empty extension\n"); goto fail; }
	hash_table_u32_set(&gs_asset_system.map, extension_id, &type_id);

	return;
	fail: DEBUG_BREAK();
}

bool asset_system_match_type(struct Asset_Handle handle, struct CString type) {
	struct CString const asset_type = string_system_get(handle.type_id);
	return cstring_equals(asset_type, type);
}

void asset_system_set_type(struct CString type, struct Asset_Callbacks callbacks, uint32_t value_size) {
	uint32_t const type_id = string_system_add(type);
	hash_table_u32_set(&gs_asset_system.types, type_id, &(struct Asset_Type){
		.callbacks = callbacks,
		.instances = handle_table_init(SIZE_OF_MEMBER(struct Asset_Entry, header) + value_size),
	});
	if (callbacks.type_init != NULL) {
		callbacks.type_init();
	}
}

void asset_system_del_type(struct CString type) {
	uint32_t const type_id = string_system_find(type);
	struct Asset_Type * asset_type = hash_table_u32_get(&gs_asset_system.types, type_id);
	if (asset_type != NULL) {
		asset_system_del_type_internal(asset_type);
		hash_table_u32_del(&gs_asset_system.types, type_id);
	}
}

struct Asset_Handle asset_system_aquire(struct CString name) {
	uint32_t name_id = string_system_add(name);
	if (name_id == 0) { return (struct Asset_Handle){0}; }

	//
	struct CString const extension = asset_system_name_to_extension(name);
	uint32_t const extension_id = string_system_find(extension);
	if (extension_id == 0) { return (struct Asset_Handle){0}; }

	uint32_t const * type_id_ptr = hash_table_u32_get(&gs_asset_system.map, extension_id);
	uint32_t const type_id = (type_id_ptr != NULL) ? *type_id_ptr : extension_id;

	//
	struct Asset_Type * asset_type = hash_table_u32_get(&gs_asset_system.types, type_id);
	if (asset_type == NULL) { return (struct Asset_Handle){0}; }

	//
	struct Handle const * instance_handle_ptr = hash_table_u32_get(&gs_asset_system.handles, name_id);
	if (instance_handle_ptr != NULL) {
		return (struct Asset_Handle){
			.instance_handle = *instance_handle_ptr,
			.type_id = type_id,
			.name_id = name_id,
		};
	}

	//
	struct Handle const instance_handle = handle_table_aquire(&asset_type->instances, NULL);
	hash_table_u32_set(&gs_asset_system.handles, name_id, &instance_handle);

	struct Asset_Entry * entry = handle_table_get(&asset_type->instances, instance_handle);
	entry->header.name_id = name_id;

	if (asset_type->callbacks.init != NULL) {
		asset_type->callbacks.init(entry->payload, name);
	}

	return (struct Asset_Handle){
		.instance_handle = instance_handle,
		.type_id = type_id,
		.name_id = name_id,
	};
}

void asset_system_discard(struct Asset_Handle handle) {
	hash_table_u32_del(&gs_asset_system.handles, handle.name_id);

	struct Asset_Type * asset_type = hash_table_u32_get(&gs_asset_system.types, handle.type_id);
	if (asset_type == NULL) { return; }

	struct Asset_Entry * entry = handle_table_get(&asset_type->instances, handle.instance_handle);
	if (entry != NULL) { return; }

	if (asset_type->callbacks.free != NULL) {
		asset_type->callbacks.free(entry->payload);
	}
	handle_table_discard(&asset_type->instances, handle.instance_handle);
}

void * asset_system_take(struct Asset_Handle handle) {
	struct Asset_Type * asset_type = hash_table_u32_get(&gs_asset_system.types, handle.type_id);
	struct Asset_Entry * entry = (asset_type != NULL)
		? handle_table_get(&asset_type->instances, handle.instance_handle)
		: NULL;
	return (entry != NULL) ? entry->payload : NULL;
}

void * asset_system_aquire_instance(struct CString name) {
	struct Asset_Handle const handle = asset_system_aquire(name);
	return asset_system_take(handle);
}

struct CString asset_system_get_name(struct Asset_Handle handle) {
	return string_system_get(handle.name_id);
}

//

static void asset_system_del_type_internal(struct Asset_Type * asset_type) {
	FOR_HANDLE_TABLE (&asset_type->instances, it) {
		struct Asset_Entry * entry = it.value;
		hash_table_u32_del(&gs_asset_system.handles, entry->header.name_id);
		if (asset_type->callbacks.free != NULL) {
			asset_type->callbacks.free(entry->payload);
		}
	}

	handle_table_free(&asset_type->instances);
	if (asset_type->callbacks.type_free != NULL) {
		asset_type->callbacks.type_free();
	}
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
