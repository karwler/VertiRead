#ifdef CAN_SFTP
#include "ssh2.h"
#include <iostream>
#include <dlfcn.h>

#define LIB_NAME "libssh2"

namespace LibSsh2 {

static void* lib = nullptr;
static bool failed = false;
decltype(libssh2_init)* sshInit = nullptr;
decltype(libssh2_exit)* sshExit = nullptr;
decltype(libssh2_session_init_ex)* sshSessionInitEx = nullptr;
decltype(libssh2_session_set_blocking)* sshSessionSetBlocking = nullptr;
decltype(libssh2_session_handshake)* sshSessionHandshake = nullptr;
decltype(libssh2_userauth_list)* sshUserauthList = nullptr;
decltype(libssh2_userauth_password_ex)* sshUserauthPasswordEx = nullptr;
decltype(libssh2_userauth_publickey_fromfile_ex)* sshUserauthPublickeyFromfileEx = nullptr;
decltype(libssh2_session_disconnect_ex)* sshSessionDisconnectEx = nullptr;
decltype(libssh2_session_free)* sshSessionFree = nullptr;
decltype(libssh2_sftp_init)* sftpInit = nullptr;
decltype(libssh2_sftp_shutdown)* sftpShutdown = nullptr;
decltype(libssh2_sftp_open_ex)* sftpOpenEx = nullptr;
decltype(libssh2_sftp_read)* sftpRead = nullptr;
decltype(libssh2_sftp_readdir_ex)* sftpReaddirEx = nullptr;
decltype(libssh2_sftp_close_handle)* sftpClose = nullptr;
decltype(libssh2_sftp_stat_ex)* sftpStatEx = nullptr;
decltype(libssh2_sftp_fstat_ex)* sftpFstatEx = nullptr;
decltype(libssh2_sftp_unlink_ex)* sftpUnlinkEx = nullptr;
decltype(libssh2_sftp_rmdir_ex)* sftpRmdirEx = nullptr;
decltype(libssh2_sftp_rename_ex)* sftpRenameEx = nullptr;

bool symLibssh2() {
	if (lib || failed)
		return lib;
	if (lib = dlopen(LIB_NAME ".so", RTLD_LAZY | RTLD_LOCAL); !lib) {
		const char* err = dlerror();
		std::cerr << (err ? err : "Failed to open " LIB_NAME) << std::endl;
		failed = true;
		return false;
	}

	if (!((sshInit = reinterpret_cast<decltype(sshInit)>(dlsym(lib, "libssh2_init")))
		&& (sshExit = reinterpret_cast<decltype(sshExit)>(dlsym(lib, "libssh2_exit")))
		&& (sshSessionInitEx = reinterpret_cast<decltype(sshSessionInitEx)>(dlsym(lib, "libssh2_session_init_ex")))
		&& (sshSessionSetBlocking = reinterpret_cast<decltype(sshSessionSetBlocking)>(dlsym(lib, "libssh2_session_set_blocking")))
		&& (sshSessionHandshake = reinterpret_cast<decltype(sshSessionHandshake)>(dlsym(lib, "libssh2_session_handshake")))
		&& (sshSessionDisconnectEx = reinterpret_cast<decltype(sshSessionDisconnectEx)>(dlsym(lib, "libssh2_session_disconnect_ex")))
		&& (sshSessionFree = reinterpret_cast<decltype(sshSessionFree)>(dlsym(lib, "libssh2_session_free")))
		&& (sftpInit = reinterpret_cast<decltype(sftpInit)>(dlsym(lib, "libssh2_sftp_init")))
		&& (sftpShutdown = reinterpret_cast<decltype(sftpShutdown)>(dlsym(lib, "libssh2_sftp_shutdown")))
		&& (sftpOpenEx = reinterpret_cast<decltype(sftpOpenEx)>(dlsym(lib, "libssh2_sftp_open_ex")))
		&& (sftpRead = reinterpret_cast<decltype(sftpRead)>(dlsym(lib, "libssh2_sftp_read")))
		&& (sftpReaddirEx = reinterpret_cast<decltype(sftpReaddirEx)>(dlsym(lib, "libssh2_sftp_readdir_ex")))
		&& (sftpClose = reinterpret_cast<decltype(sftpClose)>(dlsym(lib, "libssh2_sftp_close_handle")))
		&& (sftpStatEx = reinterpret_cast<decltype(sftpStatEx)>(dlsym(lib, "libssh2_sftp_stat_ex")))
		&& (sftpFstatEx = reinterpret_cast<decltype(sftpFstatEx)>(dlsym(lib, "libssh2_sftp_fstat_ex")))
	)) {
		std::cerr << "Failed to find " LIB_NAME " functions" << std::endl;
		dlclose(lib);
		lib = nullptr;
		failed = true;
		return false;
	}
	sshUserauthList = reinterpret_cast<decltype(sshUserauthList)>(dlsym(lib, "libssh2_userauth_list"));
	sshUserauthPasswordEx = reinterpret_cast<decltype(sshUserauthPasswordEx)>(dlsym(lib, "libssh2_userauth_password_ex"));
	sshUserauthPublickeyFromfileEx = reinterpret_cast<decltype(sshUserauthPublickeyFromfileEx)>(dlsym(lib, "libssh2_userauth_publickey_fromfile_ex"));
	sftpUnlinkEx = reinterpret_cast<decltype(sftpUnlinkEx)>(dlsym(lib, "libssh2_sftp_unlink_ex"));
	sftpRmdirEx = reinterpret_cast<decltype(sftpRmdirEx)>(dlsym(lib, "libssh2_sftp_rmdir_ex"));
	sftpRenameEx = reinterpret_cast<decltype(sftpRenameEx)>(dlsym(lib, "libssh2_sftp_ename_ex"));
	return true;
}

void closeLibssh2() {
	if (lib) {
		dlclose(lib);
		lib = nullptr;
	}
	failed = false;
}

}
#endif
