#if defined(CAN_SECRET) || defined(CAN_POPPLER)
#include "glib.h"
#include "internal.h"

static LibType libo = nullptr;
static LibType libg = nullptr;
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

bool symGlib() noexcept {
	if (!(libo || failed || ((libg = libOpen<true>("libglib-2.0" LIB_EXT)) && (libo = libOpen<true>("libgobject-2.0" LIB_EXT))
		&& (gHashTableNew = libSym<decltype(gHashTableNew)>(libg, "g_hash_table_new"))
		&& (gHashTableInsert = libSym<decltype(gHashTableInsert)>(libg, "g_hash_table_insert"))
		&& (gHashTableRemove = libSym<decltype(gHashTableRemove)>(libg, "g_hash_table_remove"))
		&& (gHashTableUnref = libSym<decltype(gHashTableUnref)>(libg, "g_hash_table_unref"))
		&& (gStrHash = libSym<decltype(gStrHash)>(libg, "g_str_hash"))
		&& (gListFree = libSym<decltype(gListFree)>(libg, "g_list_free"))
		&& (gBytesNewStatic = libSym<decltype(gBytesNewStatic)>(libg, "g_bytes_new_static"))
		&& (gBytesUnref = libSym<decltype(gBytesUnref)>(libg, "g_bytes_unref"))
		&& (gErrorFree = libSym<decltype(gErrorFree)>(libg, "g_error_free"))
		&& (gObjectUnref = libSym<decltype(gObjectUnref)>(libo, "g_object_unref"))
	))) {
		libClose(libo);
		libClose(libg);
		failed = true;
	}
	return libo;
}

void closeGlib() noexcept {
	libClose(libo);
	libClose(libg);
	failed = false;
}
#endif
