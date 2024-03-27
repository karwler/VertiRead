#pragma once

#ifdef CAN_SMB
#include <libsmbclient.h>

extern decltype(smbc_new_context)* smbcNewContext;
extern decltype(smbc_init_context)* smbcInitContext;
extern decltype(smbc_set_context)* smbcSetContext;
extern decltype(smbc_free_context)* smbcFreeContext;
extern decltype(smbc_setDebug)* smbcSetDebug;
extern decltype(smbc_setLogCallback)* smbcSetLogCallback;
extern decltype(smbc_getFunctionOpen)* smbcGetFunctionOpen;
extern decltype(smbc_getFunctionRead)* smbcGetFunctionRead;
extern decltype(smbc_getFunctionWrite)* smbcGetFunctionWrite;
extern decltype(smbc_getFunctionLseek)* smbcGetFunctionLseek;
extern decltype(smbc_getFunctionClose)* smbcGetFunctionClose;
extern decltype(smbc_getFunctionStat)* smbcGetFunctionStat;
extern decltype(smbc_getFunctionFstat)* smbcGetFunctionFstat;
extern decltype(smbc_getFunctionOpendir)* smbcGetFunctionOpendir;
extern decltype(smbc_getFunctionReaddir)* smbcGetFunctionReaddir;
extern decltype(smbc_getFunctionClosedir)* smbcGetFunctionClosedir;
extern decltype(smbc_getFunctionUnlink)* smbcGetFunctionUnlink;
extern decltype(smbc_getFunctionRmdir)* smbcGetFunctionRmdir;
extern decltype(smbc_getFunctionRename)* smbcGetFunctionRename;
extern decltype(smbc_getFunctionNotify)* smbcGetFunctionNotify;
extern decltype(smbc_setFunctionAuthDataWithContext)* smbcSetFunctionAuthDataWithContext;
extern decltype(smbc_getUser)* smbcGetUser;
extern decltype(smbc_setUser)* smbcSetUser;
extern decltype(smbc_getWorkgroup)* smbcGetWorkgroup;
extern decltype(smbc_setWorkgroup)* smbcSetWorkgroup;
extern decltype(smbc_getPort)* smbcGetPort;
extern decltype(smbc_setPort)* smbcSetPort;
extern decltype(smbc_getOptionUserData)* smbcGetOptionUserData;
extern decltype(smbc_setOptionUserData)* smbcSetOptionUserData;

bool symSmbclient() noexcept;
void closeSmbclient() noexcept;
#endif
