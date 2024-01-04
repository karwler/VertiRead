#ifdef CAN_SMB
#include "smbclient.h"
#include <iostream>
#include <dlfcn.h>

#define LIB_NAME "smbclient"

namespace LibSmbclient {

static void* lib = nullptr;
static bool failed = false;
decltype(smbc_new_context)* smbcNewContext = nullptr;
decltype(smbc_init_context)* smbcInitContext = nullptr;
decltype(smbc_set_context)* smbcSetContext = nullptr;
decltype(smbc_free_context)* smbcFreeContext = nullptr;
decltype(smbc_setDebug)* smbcSetDebug = nullptr;
decltype(smbc_setLogCallback)* smbcSetLogCallback = nullptr;
decltype(smbc_getFunctionOpen)* smbcGetFunctionOpen = nullptr;
decltype(smbc_getFunctionRead)* smbcGetFunctionRead = nullptr;
decltype(smbc_getFunctionClose)* smbcGetFunctionClose = nullptr;
decltype(smbc_getFunctionStat)* smbcGetFunctionStat = nullptr;
decltype(smbc_getFunctionFstat)* smbcGetFunctionFstat = nullptr;
decltype(smbc_getFunctionOpendir)* smbcGetFunctionOpendir = nullptr;
decltype(smbc_getFunctionReaddir)* smbcGetFunctionReaddir = nullptr;
decltype(smbc_getFunctionClosedir)* smbcGetFunctionClosedir = nullptr;
decltype(smbc_getFunctionUnlink)* smbcGetFunctionUnlink = nullptr;
decltype(smbc_getFunctionRmdir)* smbcGetFunctionRmdir = nullptr;
decltype(smbc_getFunctionRename)* smbcGetFunctionRename = nullptr;
decltype(smbc_getFunctionNotify)* smbcGetFunctionNotify = nullptr;
decltype(smbc_setFunctionAuthDataWithContext)* smbcSetFunctionAuthDataWithContext = nullptr;
decltype(smbc_getUser)* smbcGetUser = nullptr;
decltype(smbc_setUser)* smbcSetUser = nullptr;
decltype(smbc_getWorkgroup)* smbcGetWorkgroup = nullptr;
decltype(smbc_setWorkgroup)* smbcSetWorkgroup = nullptr;
decltype(smbc_getPort)* smbcGetPort = nullptr;
decltype(smbc_setPort)* smbcSetPort = nullptr;
decltype(smbc_getOptionUserData)* smbcGetOptionUserData = nullptr;
decltype(smbc_setOptionUserData)* smbcSetOptionUserData = nullptr;

bool symSmbclient() {
	if (lib || failed)
		return lib;
	if (lib = dlopen("lib" LIB_NAME ".so", RTLD_LAZY | RTLD_LOCAL); !lib) {
		const char* err = dlerror();
		std::cerr << (err ? err : "Failed to open " LIB_NAME) << std::endl;
		failed = true;
		return false;
	}

	if (!((smbcNewContext = reinterpret_cast<decltype(smbcNewContext)>(dlsym(lib, "smbc_new_context")))
		&& (smbcInitContext = reinterpret_cast<decltype(smbcInitContext)>(dlsym(lib, "smbc_init_context")))
		&& (smbcSetContext = reinterpret_cast<decltype(smbcSetContext)>(dlsym(lib, "smbc_set_context")))
		&& (smbcFreeContext = reinterpret_cast<decltype(smbcFreeContext)>(dlsym(lib, "smbc_free_context")))
		&& (smbcGetFunctionOpen = reinterpret_cast<decltype(smbcGetFunctionOpen)>(dlsym(lib, "smbc_getFunctionOpen")))
		&& (smbcGetFunctionRead = reinterpret_cast<decltype(smbcGetFunctionRead)>(dlsym(lib, "smbc_getFunctionRead")))
		&& (smbcGetFunctionClose = reinterpret_cast<decltype(smbcGetFunctionClose)>(dlsym(lib, "smbc_getFunctionClose")))
		&& (smbcGetFunctionStat = reinterpret_cast<decltype(smbcGetFunctionStat)>(dlsym(lib, "smbc_getFunctionStat")))
		&& (smbcGetFunctionFstat = reinterpret_cast<decltype(smbcGetFunctionFstat)>(dlsym(lib, "smbc_getFunctionFstat")))
		&& (smbcGetFunctionOpendir = reinterpret_cast<decltype(smbcGetFunctionOpendir)>(dlsym(lib, "smbc_getFunctionOpendir")))
		&& (smbcGetFunctionReaddir = reinterpret_cast<decltype(smbcGetFunctionReaddir)>(dlsym(lib, "smbc_getFunctionReaddir")))
		&& (smbcGetFunctionClosedir = reinterpret_cast<decltype(smbcGetFunctionClosedir)>(dlsym(lib, "smbc_getFunctionClosedir")))
		&& (smbcSetFunctionAuthDataWithContext = reinterpret_cast<decltype(smbcSetFunctionAuthDataWithContext)>(dlsym(lib, "smbc_setFunctionAuthDataWithContext")))
		&& (smbcGetUser = reinterpret_cast<decltype(smbcGetUser)>(dlsym(lib, "smbc_getUser")))
		&& (smbcSetUser = reinterpret_cast<decltype(smbcSetUser)>(dlsym(lib, "smbc_setUser")))
		&& (smbcGetWorkgroup = reinterpret_cast<decltype(smbcGetWorkgroup)>(dlsym(lib, "smbc_getWorkgroup")))
		&& (smbcSetWorkgroup = reinterpret_cast<decltype(smbcSetWorkgroup)>(dlsym(lib, "smbc_setWorkgroup")))
		&& (smbcGetPort = reinterpret_cast<decltype(smbcGetPort)>(dlsym(lib, "smbc_getPort")))
		&& (smbcSetPort = reinterpret_cast<decltype(smbcSetPort)>(dlsym(lib, "smbc_setPort")))
		&& (smbcGetOptionUserData = reinterpret_cast<decltype(smbcGetOptionUserData)>(dlsym(lib, "smbc_getOptionUserData")))
		&& (smbcSetOptionUserData = reinterpret_cast<decltype(smbcSetOptionUserData)>(dlsym(lib, "smbc_setOptionUserData")))
	)) {
		std::cerr << "Failed to find " LIB_NAME " functions" << std::endl;
		dlclose(lib);
		lib = nullptr;
		failed = true;
		return false;
	}
	smbcSetDebug = reinterpret_cast<decltype(smbcSetDebug)>(dlsym(lib, "smbc_setDebug"));
	smbcSetLogCallback = reinterpret_cast<decltype(smbcSetLogCallback)>(dlsym(lib, "smbc_setLogCallback"));
	smbcGetFunctionUnlink = reinterpret_cast<decltype(smbcGetFunctionUnlink)>(dlsym(lib, "smbc_getFunctionUnlink"));
	smbcGetFunctionRmdir = reinterpret_cast<decltype(smbcGetFunctionRmdir)>(dlsym(lib, "smbc_getFunctionRmdir"));
	smbcGetFunctionRename = reinterpret_cast<decltype(smbcGetFunctionRename)>(dlsym(lib, "smbc_getFunctionRename"));
	smbcGetFunctionNotify = reinterpret_cast<decltype(smbcGetFunctionNotify)>(dlsym(lib, "smbc_getFunctionNotify"));
	return true;
}

void closeSmbclient() {
	if (lib) {
		dlclose(lib);
		lib = nullptr;
	}
	failed = false;
}

}
#endif
