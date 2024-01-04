#if defined(CAN_SECRET) || defined(CAN_PDF)
#include "glib.h"
#include <iostream>
#include <dlfcn.h>

#define GLIB_NAME "glib-2.0"
#define OLIB_NAME "gobject-2.0"

namespace LibGlib {

static void* libo = nullptr;
static void* libg = nullptr;
static bool failed = false;
decltype(g_object_unref)* gObjectUnref = nullptr;
decltype(g_hash_table_new)* gHashTableNew = nullptr;
decltype(g_hash_table_insert)* gHashTableInsert = nullptr;
decltype(g_hash_table_remove)* gHashTableRemove = nullptr;
decltype(g_hash_table_unref)* gHashTableUnref = nullptr;
decltype(g_str_hash)* gStrHash = nullptr;
decltype(g_list_free)* gListFree = nullptr;
decltype(g_bytes_new_static)* gBytesNewStatic = nullptr;
decltype(g_bytes_unref)* gBytesUnref = nullptr;
decltype(g_error_free)* gErrorFree = nullptr;

bool symGlib() {
	if (libo || failed)
		return libo;
	try {
		if (libg = dlopen("lib" GLIB_NAME ".so", RTLD_LAZY | RTLD_GLOBAL); !libg)
			throw "Failed to open " GLIB_NAME;
		if (libo = dlopen("lib" OLIB_NAME ".so", RTLD_LAZY | RTLD_GLOBAL); !libo)
			throw "Failed to open " OLIB_NAME;
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
		if (!((gHashTableNew = reinterpret_cast<decltype(gHashTableNew)>(dlsym(libg, "g_hash_table_new")))
			&& (gHashTableInsert = reinterpret_cast<decltype(gHashTableInsert)>(dlsym(libg, "g_hash_table_insert")))
			&& (gHashTableRemove = reinterpret_cast<decltype(gHashTableRemove)>(dlsym(libg, "g_hash_table_remove")))
			&& (gHashTableUnref = reinterpret_cast<decltype(gHashTableUnref)>(dlsym(libg, "g_hash_table_unref")))
			&& (gStrHash = reinterpret_cast<decltype(gStrHash)>(dlsym(libg, "g_str_hash")))
			&& (gListFree = reinterpret_cast<decltype(gListFree)>(dlsym(libg, "g_list_free")))
			&& (gBytesNewStatic = reinterpret_cast<decltype(gBytesNewStatic)>(dlsym(libg, "g_bytes_new_static")))
			&& (gBytesUnref = reinterpret_cast<decltype(gBytesUnref)>(dlsym(libg, "g_bytes_unref")))
			&& (gErrorFree = reinterpret_cast<decltype(gErrorFree)>(dlsym(libg, "g_error_free")))
		))
			throw "Failed to find " GLIB_NAME " functions";

		if (gObjectUnref = reinterpret_cast<decltype(gObjectUnref)>(dlsym(libo, "g_object_unref")); !gObjectUnref)
			throw "Failed to find " OLIB_NAME " functions";
	} catch (const char* msg) {
		std::cerr << msg << std::endl;
		dlclose(libo);
		dlclose(libg);
		libo = libg = nullptr;
		failed = true;
		return false;
	}
	return true;
}

void closeGlib() {
	if (libo) {
		dlclose(libo);
		dlclose(libg);
		libo = libg = nullptr;
	}
	failed = false;
}

}
#endif
