#ifdef CAN_SECRET
#include "glib.h"
#include "secret.h"
#include <iostream>
#include <dlfcn.h>

#define LIB_NAME "libsecret-1"

namespace LibSecret {

static void* lib = nullptr;
static bool failed = false;
decltype(secret_service_get_sync)* secretServiceGetSync = nullptr;
decltype(secret_service_search_sync)* secretServiceSearchSync = nullptr;
decltype(secret_service_store_sync)* secretServiceStoreSync = nullptr;
decltype(secret_item_get_secret)* secretItemGetSecret = nullptr;
decltype(secret_item_load_secret_sync)* secretItemLoadSecretSync = nullptr;
decltype(secret_value_new)* secretValueNew = nullptr;
decltype(secret_value_get_text)* secretValueGetText = nullptr;

bool symLibsecret() {
	if (lib || failed)
		return lib;
	if (!LibGlib::symGlib()) {
		failed = true;
		return false;
	}
	if (lib = dlopen(LIB_NAME ".so", RTLD_LAZY | RTLD_LOCAL); !lib) {
		const char* err = dlerror();
		std::cerr << (err ? err : "Failed to open " LIB_NAME) << std::endl;
		failed = true;
		return false;
	}

	secretServiceGetSync = reinterpret_cast<decltype(secretServiceGetSync)>(dlsym(lib, "secret_service_get_sync"));
	secretServiceSearchSync = reinterpret_cast<decltype(secretServiceSearchSync)>(dlsym(lib, "secret_service_search_sync"));
	secretServiceStoreSync = reinterpret_cast<decltype(secretServiceStoreSync)>(dlsym(lib, "secret_service_store_sync"));
	secretItemGetSecret = reinterpret_cast<decltype(secretItemGetSecret)>(dlsym(lib, "secret_item_get_secret"));
	secretItemLoadSecretSync = reinterpret_cast<decltype(secretItemLoadSecretSync)>(dlsym(lib, "secret_item_load_secret_sync"));
	secretValueNew = reinterpret_cast<decltype(secretValueNew)>(dlsym(lib, "secret_value_new"));
	secretValueGetText = reinterpret_cast<decltype(secretValueGetText)>(dlsym(lib, "secret_value_get_text"));
	if (!(secretServiceGetSync && secretServiceSearchSync && secretServiceStoreSync && secretItemGetSecret && secretItemLoadSecretSync && secretValueNew && secretValueGetText)) {
		std::cerr << "Failed to find " LIB_NAME " functions" << std::endl;
		dlclose(lib);
		lib = nullptr;
		failed = true;
		return false;
	}
	return true;
}

void closeLibsecret() {
	if (lib) {
		dlclose(lib);
		lib = nullptr;
	}
	failed = false;
}

}
#endif
