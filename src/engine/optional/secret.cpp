#ifdef CAN_SECRET
#include "secret.h"
#include <iostream>
#include <dlfcn.h>

#define GLIB_NAME "glib-2.0"
#define OLIB_NAME "gobject-2.0"
#define SLIB_NAME "libsecret-1"

namespace LibSecret {

static void* libs = nullptr;
static void* libo = nullptr;
static void* libg = nullptr;
static bool failed = false;
decltype(secret_service_get_sync)* secretServiceGetSync = nullptr;
decltype(secret_service_search_sync)* secretServiceSearchSync = nullptr;
decltype(secret_service_store_sync)* secretServiceStoreSync = nullptr;
decltype(secret_item_get_secret)* secretItemGetSecret = nullptr;
decltype(secret_item_load_secret_sync)* secretItemLoadSecretSync = nullptr;
decltype(secret_value_new)* secretValueNew = nullptr;
decltype(secret_value_get_text)* secretValueGetText = nullptr;
decltype(g_object_unref)* gObjectUnref = nullptr;
decltype(g_hash_table_new)* gHashTableNew = nullptr;
decltype(g_hash_table_insert)* gHashTableInsert = nullptr;
decltype(g_hash_table_remove)* gHashTableRemove = nullptr;
decltype(g_hash_table_unref)* gHashTableUnref = nullptr;
decltype(g_str_hash)* gStrHash = nullptr;
decltype(g_list_free)* gListFree = nullptr;
decltype(g_error_free)* gErrorFree = nullptr;

bool symLibsecret() {
	if (libs || failed)
		return libs;
	try {
		if (libg = dlopen("lib" GLIB_NAME ".so", RTLD_LAZY | RTLD_GLOBAL); !libg)
			throw "Failed to open " GLIB_NAME;
		if (libo = dlopen("lib" OLIB_NAME ".so", RTLD_LAZY | RTLD_GLOBAL); !libo)
			throw "Failed to open " OLIB_NAME;
		if (libs = dlopen(SLIB_NAME ".so", RTLD_LAZY | RTLD_LOCAL); !libs)
			throw "Failed to open " SLIB_NAME;
	} catch (const char* msg) {
		const char* err = dlerror();
		std::cerr << (err ? err : msg) << std::endl;
		if (libo)
			dlclose(libo);
		if (libg)
			dlclose(libg);
		libo = libg = nullptr;
		failed = true;
		return false;
	}

	try {
		gHashTableNew = reinterpret_cast<decltype(gHashTableNew)>(dlsym(libg, "g_hash_table_new"));
		gHashTableInsert = reinterpret_cast<decltype(gHashTableInsert)>(dlsym(libg, "g_hash_table_insert"));
		gHashTableRemove = reinterpret_cast<decltype(gHashTableRemove)>(dlsym(libg, "g_hash_table_remove"));
		gHashTableUnref = reinterpret_cast<decltype(gHashTableUnref)>(dlsym(libg, "g_hash_table_unref"));
		gStrHash = reinterpret_cast<decltype(gStrHash)>(dlsym(libg, "g_str_hash"));
		gListFree = reinterpret_cast<decltype(gListFree)>(dlsym(libg, "g_list_free"));
		gErrorFree = reinterpret_cast<decltype(gErrorFree)>(dlsym(libg, "g_error_free"));
		if (!(gHashTableNew && gHashTableInsert && gHashTableRemove && gHashTableUnref && gStrHash && gListFree && gErrorFree))
			throw "Failed to find " GLIB_NAME " functions";

		gObjectUnref = reinterpret_cast<decltype(gObjectUnref)>(dlsym(libo, "g_object_unref"));
		if (!gObjectUnref)
			throw "Failed to find " OLIB_NAME " functions";

		secretServiceGetSync = reinterpret_cast<decltype(secretServiceGetSync)>(dlsym(libs, "secret_service_get_sync"));
		secretServiceSearchSync = reinterpret_cast<decltype(secretServiceSearchSync)>(dlsym(libs, "secret_service_search_sync"));
		secretServiceStoreSync = reinterpret_cast<decltype(secretServiceStoreSync)>(dlsym(libs, "secret_service_store_sync"));
		secretItemGetSecret = reinterpret_cast<decltype(secretItemGetSecret)>(dlsym(libs, "secret_item_get_secret"));
		secretItemLoadSecretSync = reinterpret_cast<decltype(secretItemLoadSecretSync)>(dlsym(libs, "secret_item_load_secret_sync"));
		secretValueNew = reinterpret_cast<decltype(secretValueNew)>(dlsym(libs, "secret_value_new"));
		secretValueGetText = reinterpret_cast<decltype(secretValueGetText)>(dlsym(libs, "secret_value_get_text"));
		if (!(secretServiceGetSync && secretServiceSearchSync && secretServiceStoreSync && secretItemGetSecret && secretItemLoadSecretSync && secretValueNew && secretValueGetText))
			throw "Failed to find " SLIB_NAME " functions";
	} catch (const char* err) {
		std::cerr << err << std::endl;
		dlclose(libs);
		dlclose(libo);
		dlclose(libg);
		libs = libo = libg = nullptr;
		failed = true;
		return false;
	}
	return true;
}

void closeLibsecret() {
	if (libs) {
		dlclose(libs);
		dlclose(libo);
		dlclose(libg);
		libs = libo = libg = nullptr;
	}
	failed = false;
}

}
#endif
