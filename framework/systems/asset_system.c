#include "framework/logger.h"

//
#include "asset_system.h"

static struct Asset_System {
	struct Strings strings; // extensions, types, names
	struct Hash_Table_U32 handles; // name string id : instance handle
	struct Hash_Table_U32 types; // type string id : callbacks and instances
	struct Hash_Table_U32 map; // extension string id : type string id
} gs_asset_system;

struct Asset_Type {
	struct Asset_Callbacks callbacks;
	struct Handle_Table instances;
};

struct Asset_Entry {
	struct {
		uint32_t name_id;
	} header;
	uint8_t payload[FLEXIBLE_ARRAY];
};

static struct Asset_Type * asset_system_get_type(uint32_t id);
static void asset_system_del_type_internal(struct Asset_Type * asset_type);
static struct CString asset_system_name_to_extension(struct CString name);

void asset_system_init(void) {
	gs_asset_system = (struct Asset_System){
		.strings = strings_init(),
		.types = hash_table_u32_init(sizeof(struct Asset_Type)),
		.handles = hash_table_u32_init(sizeof(struct Handle)),
		.map = hash_table_u32_init(sizeof(uint32_t)),
	};
}

void asset_system_free(void) {
	FOR_HASH_TABLE_U32 (&gs_asset_system.types, it) {
		asset_system_del_type_internal(it.value);
	}
	strings_free(&gs_asset_system.strings);
	hash_table_u32_free(&gs_asset_system.types);
	hash_table_u32_free(&gs_asset_system.handles);
	hash_table_u32_free(&gs_asset_system.map);
	// common_memset(&gs_asset_system, 0, sizeof(gs_asset_system));
}

void asset_system_map_extension(struct CString type, struct CString extension) {
	uint32_t const type_id = strings_add(&gs_asset_system.strings, type);
	if (type_id == 0) { logger_to_console("empty type\n"); goto fail; }
	uint32_t const extension_id = strings_add(&gs_asset_system.strings, extension);
	if (extension_id == 0) { logger_to_console("empty extension\n"); goto fail; }
	hash_table_u32_set(&gs_asset_system.map, extension_id, &type_id);

	return;
	fail: DEBUG_BREAK();
}

bool asset_system_match_type(struct Asset_Handle handle, struct CString type) {
	struct CString const asset_type = strings_get(&gs_asset_system.strings, handle.type_id);
	return cstring_equals(asset_type, type);
}

void asset_system_set_type(struct CString type, struct Asset_Callbacks callbacks, uint32_t value_size) {
	uint32_t const type_id = strings_add(&gs_asset_system.strings, type);
	if (type_id == 0) { logger_to_console("empty type\n"); goto fail; }

	hash_table_u32_set(&gs_asset_system.types, type_id, &(struct Asset_Type){
		.callbacks = callbacks,
		.instances = handle_table_init(SIZE_OF_MEMBER(struct Asset_Entry, header) + value_size),
	});

	if (callbacks.type_init != NULL) {
		callbacks.type_init();
	}

	return;
	fail: DEBUG_BREAK();
}

void asset_system_del_type(struct CString type) {
	uint32_t const type_id = strings_find(&gs_asset_system.strings, type);
	if (type_id == 0) { logger_to_console("unknown type: %.*s\n", type.length, type.data); goto fail; }

	struct Asset_Type * asset_type = asset_system_get_type(type_id);
	if (asset_type != NULL) {
		asset_system_del_type_internal(asset_type);
		hash_table_u32_del(&gs_asset_system.types, type_id);
	}

	return;
	fail: DEBUG_BREAK();
}

struct Asset_Handle asset_system_aquire(struct CString name) {
	uint32_t name_id = strings_add(&gs_asset_system.strings, name);
	if (name_id == 0) { logger_to_console("empty name\n"); goto fail; }

	//
	struct CString const extension = asset_system_name_to_extension(name);
	uint32_t const extension_id = strings_find(&gs_asset_system.strings, extension);
	if (extension_id == 0) { logger_to_console("unknown extension: %.*s\n", extension.length, extension.data); goto fail; }

	uint32_t const * type_id_ptr = hash_table_u32_get(&gs_asset_system.map, extension_id);
	uint32_t const type_id = (type_id_ptr != NULL) ? *type_id_ptr : extension_id;

	//
	struct Asset_Type * asset_type = asset_system_get_type(type_id);
	if (asset_type == NULL) { goto fail; }

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

	// process errors
	fail: DEBUG_BREAK();
	return (struct Asset_Handle){0};
}

void asset_system_discard(struct Asset_Handle handle) {
	struct Asset_Type * asset_type = asset_system_get_type(handle.type_id);
	if (asset_type == NULL) { goto fail; }

	//
	hash_table_u32_del(&gs_asset_system.handles, handle.name_id);
	struct Asset_Entry * entry = handle_table_get(&asset_type->instances, handle.instance_handle);
	if (entry != NULL) {
		if (asset_type->callbacks.free != NULL) {
			asset_type->callbacks.free(entry->payload);
		}
		handle_table_discard(&asset_type->instances, handle.instance_handle);
	}

	return;
	fail: DEBUG_BREAK();
}

void * asset_system_take(struct Asset_Handle handle) {
	struct Asset_Type * asset_type = asset_system_get_type(handle.type_id);
	if (asset_type == NULL) { goto fail; }

	//
	struct Asset_Entry * entry = handle_table_get(&asset_type->instances, handle.instance_handle);
	return (entry != NULL) ? entry->payload : NULL;

	fail: DEBUG_BREAK();
	return NULL;
}

void * asset_system_aquire_instance(struct CString name) {
	struct Asset_Handle const handle = asset_system_aquire(name);
	return asset_system_take(handle);
}

struct CString asset_system_get_name(struct Asset_Handle handle) {
	return strings_get(&gs_asset_system.strings, handle.name_id);
}

//

static struct Asset_Type * asset_system_get_type(uint32_t id) {
	struct Asset_Type * type = hash_table_u32_get(&gs_asset_system.types, id);
	if (type == NULL) {
		struct CString const type_string = strings_get(&gs_asset_system.strings, id);
		logger_to_console("unknown type: %.*s\n", type_string.length, type_string.data); DEBUG_BREAK();
	}
	return type;
}

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
