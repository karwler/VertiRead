#ifdef CAN_SECRET
#include "glib.h"
#include "secret.h"
#include "internal.h"

static LibType lib = nullptr;
static bool failed = false;
decltype(secret_service_get_sync)* secretServiceGetSync = nullptr;
decltype(secret_service_search_sync)* secretServiceSearchSync = nullptr;
decltype(secret_service_store_sync)* secretServiceStoreSync = nullptr;
decltype(secret_item_get_secret)* secretItemGetSecret = nullptr;
decltype(secret_item_load_secret_sync)* secretItemLoadSecretSync = nullptr;
decltype(secret_value_new)* secretValueNew = nullptr;
decltype(secret_value_get_text)* secretValueGetText = nullptr;

bool symLibsecret() noexcept {
	if (!(lib || failed || (symGlib() && (lib = libOpen("libsecret-1" LIB_EXT))
		&& (secretServiceGetSync = libSym<decltype(secretServiceGetSync)>(lib, "secret_service_get_sync"))
		&& (secretServiceSearchSync = libSym<decltype(secretServiceSearchSync)>(lib, "secret_service_search_sync"))
		&& (secretServiceStoreSync = libSym<decltype(secretServiceStoreSync)>(lib, "secret_service_store_sync"))
		&& (secretItemGetSecret = libSym<decltype(secretItemGetSecret)>(lib, "secret_item_get_secret"))
		&& (secretItemLoadSecretSync = libSym<decltype(secretItemLoadSecretSync)>(lib, "secret_item_load_secret_sync"))
		&& (secretValueNew = libSym<decltype(secretValueNew)>(lib, "secret_value_new"))
		&& (secretValueGetText = libSym<decltype(secretValueGetText)>(lib, "secret_value_get_text"))
	))) {
		libClose(lib);
		failed = true;
	}
	return lib;
}

void closeLibsecret() noexcept {
	libClose(lib);
	failed = false;
}
#endif
