#pragma once

#if defined(CAN_SECRET) || defined(CAN_PDF)
#include <glib-object.h>

extern decltype(g_object_unref)* gObjectUnref;

extern decltype(g_hash_table_new)* gHashTableNew;
extern decltype(g_hash_table_insert)* gHashTableInsert;
extern decltype(g_hash_table_remove)* gHashTableRemove;
extern decltype(g_hash_table_unref)* gHashTableUnref;
extern decltype(g_str_hash)* gStrHash;
extern decltype(g_list_free)* gListFree;
extern decltype(g_bytes_new_static)* gBytesNewStatic;
extern decltype(g_bytes_unref)* gBytesUnref;
extern decltype(g_error_free)* gErrorFree;

bool symGlib();
void closeGlib();
#endif
