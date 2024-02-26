#ifdef CAN_SMB
#include "smbclient.h"
#include "internal.h"

static LibType lib = nullptr;
static bool failed = false;
decltype(smbc_new_context)* smbcNewContext = nullptr;
decltype(smbc_init_context)* smbcInitContext = nullptr;
decltype(smbc_set_context)* smbcSetContext = nullptr;
decltype(smbc_free_context)* smbcFreeContext = nullptr;
decltype(smbc_setDebug)* smbcSetDebug = nullptr;
decltype(smbc_setLogCallback)* smbcSetLogCallback = nullptr;
decltype(smbc_getFunctionOpen)* smbcGetFunctionOpen = nullptr;
decltype(smbc_getFunctionRead)* smbcGetFunctionRead = nullptr;
decltype(smbc_getFunctionWrite)* smbcGetFunctionWrite = nullptr;
decltype(smbc_getFunctionLseek)* smbcGetFunctionLseek = nullptr;
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
	if (!(lib || failed || ((lib = libOpen("libsmbclient" LIB_EXT))
		&& (smbcNewContext = libSym<decltype(smbcNewContext)>(lib, "smbc_new_context"))
		&& (smbcInitContext = libSym<decltype(smbcInitContext)>(lib, "smbc_init_context"))
		&& (smbcSetContext = libSym<decltype(smbcSetContext)>(lib, "smbc_set_context"))
		&& (smbcFreeContext = libSym<decltype(smbcFreeContext)>(lib, "smbc_free_context"))
		&& (smbcSetDebug = libSym<decltype(smbcSetDebug)>(lib, "smbc_setDebug"))
		&& (smbcSetLogCallback = libSym<decltype(smbcSetLogCallback)>(lib, "smbc_setLogCallback"))
		&& (smbcGetFunctionOpen = libSym<decltype(smbcGetFunctionOpen)>(lib, "smbc_getFunctionOpen"))
		&& (smbcGetFunctionRead = libSym<decltype(smbcGetFunctionRead)>(lib, "smbc_getFunctionRead"))
		&& (smbcGetFunctionWrite = libSym<decltype(smbcGetFunctionWrite)>(lib, "smbc_getFunctionWrite"))
		&& (smbcGetFunctionLseek = libSym<decltype(smbcGetFunctionLseek)>(lib, "smbc_getFunctionLseek"))
		&& (smbcGetFunctionClose = libSym<decltype(smbcGetFunctionClose)>(lib, "smbc_getFunctionClose"))
		&& (smbcGetFunctionStat = libSym<decltype(smbcGetFunctionStat)>(lib, "smbc_getFunctionStat"))
		&& (smbcGetFunctionFstat = libSym<decltype(smbcGetFunctionFstat)>(lib, "smbc_getFunctionFstat"))
		&& (smbcGetFunctionOpendir = libSym<decltype(smbcGetFunctionOpendir)>(lib, "smbc_getFunctionOpendir"))
		&& (smbcGetFunctionReaddir = libSym<decltype(smbcGetFunctionReaddir)>(lib, "smbc_getFunctionReaddir"))
		&& (smbcGetFunctionClosedir = libSym<decltype(smbcGetFunctionClosedir)>(lib, "smbc_getFunctionClosedir"))
		&& (smbcGetFunctionUnlink = libSym<decltype(smbcGetFunctionUnlink)>(lib, "smbc_getFunctionUnlink"))
		&& (smbcGetFunctionRmdir = libSym<decltype(smbcGetFunctionRmdir)>(lib, "smbc_getFunctionRmdir"))
		&& (smbcGetFunctionRename = libSym<decltype(smbcGetFunctionRename)>(lib, "smbc_getFunctionRename"))
		&& (smbcGetFunctionNotify = libSym<decltype(smbcGetFunctionNotify)>(lib, "smbc_getFunctionNotify"))
		&& (smbcSetFunctionAuthDataWithContext = libSym<decltype(smbcSetFunctionAuthDataWithContext)>(lib, "smbc_setFunctionAuthDataWithContext"))
		&& (smbcGetUser = libSym<decltype(smbcGetUser)>(lib, "smbc_getUser"))
		&& (smbcSetUser = libSym<decltype(smbcSetUser)>(lib, "smbc_setUser"))
		&& (smbcGetWorkgroup = libSym<decltype(smbcGetWorkgroup)>(lib, "smbc_getWorkgroup"))
		&& (smbcSetWorkgroup = libSym<decltype(smbcSetWorkgroup)>(lib, "smbc_setWorkgroup"))
		&& (smbcGetPort = libSym<decltype(smbcGetPort)>(lib, "smbc_getPort"))
		&& (smbcSetPort = libSym<decltype(smbcSetPort)>(lib, "smbc_setPort"))
		&& (smbcGetOptionUserData = libSym<decltype(smbcGetOptionUserData)>(lib, "smbc_getOptionUserData"))
		&& (smbcSetOptionUserData = libSym<decltype(smbcSetOptionUserData)>(lib, "smbc_setOptionUserData"))
	))) {
		libClose(lib);
		failed = true;
	}
	return lib;
}

void closeSmbclient() {
	libClose(lib);
	failed = false;
}
#endif
