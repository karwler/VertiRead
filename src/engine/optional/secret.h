#pragma once

#ifdef CAN_SECRET
#include <libsecret/secret.h>

extern decltype(secret_service_get_sync)* secretServiceGetSync;
extern decltype(secret_service_search_sync)* secretServiceSearchSync;
extern decltype(secret_service_store_sync)* secretServiceStoreSync;
extern decltype(secret_item_get_secret)* secretItemGetSecret;
extern decltype(secret_item_load_secret_sync)* secretItemLoadSecretSync;
extern decltype(secret_value_new)* secretValueNew;
extern decltype(secret_value_get_text)* secretValueGetText;

bool symLibsecret();	// loads glib because of dependency
void closeLibsecret();
#endif
