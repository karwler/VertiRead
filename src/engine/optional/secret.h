#pragma once

#if CAN_SECRET
#include <libsecret/secret.h>

namespace LibSecret {

extern decltype(secret_service_get_sync)* secretServiceGetSync;
extern decltype(secret_service_search_sync)* secretServiceSearchSync;
extern decltype(secret_service_store_sync)* secretServiceStoreSync;
extern decltype(secret_item_get_secret)* secretItemGetSecret;
extern decltype(secret_item_load_secret_sync)* secretItemLoadSecretSync;
extern decltype(secret_value_new)* secretValueNew;
extern decltype(secret_value_get_text)* secretValueGetText;

extern decltype(g_object_unref)* gObjectUnref;

extern decltype(g_hash_table_new)* gHashTableNew;
extern decltype(g_hash_table_insert)* gHashTableInsert;
extern decltype(g_hash_table_remove)* gHashTableRemove;
extern decltype(g_hash_table_unref)* gHashTableUnref;
extern decltype(g_str_hash)* gStrHash;
extern decltype(g_list_free)* gListFree;
extern decltype(g_error_free)* gErrorFree;

bool symLibsecret();
void closeLibsecret();

}
#endif
