#ifdef CAN_SFTP
#include "ssh2.h"
#include "internal.h"

static LibType lib = nullptr;
static bool failed = false;
decltype(libssh2_init)* sshInit = nullptr;
decltype(libssh2_exit)* sshExit = nullptr;
decltype(libssh2_session_init_ex)* sshSessionInitEx = nullptr;
decltype(libssh2_session_set_blocking)* sshSessionSetBlocking = nullptr;
decltype(libssh2_session_handshake)* sshSessionHandshake = nullptr;
decltype(libssh2_session_last_error)* sshSessionLastError = nullptr;
decltype(libssh2_userauth_list)* sshUserauthList = nullptr;
decltype(libssh2_userauth_password_ex)* sshUserauthPasswordEx = nullptr;
decltype(libssh2_userauth_authenticated)* sshUserauthAuthenticated = nullptr;
decltype(libssh2_session_disconnect_ex)* sshSessionDisconnectEx = nullptr;
decltype(libssh2_session_free)* sshSessionFree = nullptr;
decltype(libssh2_sftp_init)* sftpInit = nullptr;
decltype(libssh2_sftp_shutdown)* sftpShutdown = nullptr;
decltype(libssh2_sftp_open_ex)* sftpOpenEx = nullptr;
decltype(libssh2_sftp_read)* sftpRead = nullptr;
decltype(libssh2_sftp_write)* sftpWrite = nullptr;
decltype(libssh2_sftp_seek64)* sftpSeek64 = nullptr;
decltype(libssh2_sftp_tell64)* sftpTell64 = nullptr;
decltype(libssh2_sftp_readdir_ex)* sftpReaddirEx = nullptr;
decltype(libssh2_sftp_close_handle)* sftpClose = nullptr;
decltype(libssh2_sftp_stat_ex)* sftpStatEx = nullptr;
decltype(libssh2_sftp_fstat_ex)* sftpFstatEx = nullptr;
decltype(libssh2_sftp_unlink_ex)* sftpUnlinkEx = nullptr;
decltype(libssh2_sftp_rmdir_ex)* sftpRmdirEx = nullptr;
decltype(libssh2_sftp_rename_ex)* sftpRenameEx = nullptr;

bool symLibssh2() noexcept {
	if (!(lib || failed || ((lib = libOpen("libssh2" LIB_EXT))
		&& (sshInit = libSym<decltype(sshInit)>(lib, "libssh2_init"))
		&& (sshExit = libSym<decltype(sshExit)>(lib, "libssh2_exit"))
		&& (sshSessionInitEx = libSym<decltype(sshSessionInitEx)>(lib, "libssh2_session_init_ex"))
		&& (sshSessionSetBlocking = libSym<decltype(sshSessionSetBlocking)>(lib, "libssh2_session_set_blocking"))
		&& (sshSessionHandshake = libSym<decltype(sshSessionHandshake)>(lib, "libssh2_session_handshake"))
		&& (sshSessionLastError = libSym<decltype(sshSessionLastError)>(lib, "libssh2_session_last_error"))
		&& (sshUserauthList = libSym<decltype(sshUserauthList)>(lib, "libssh2_userauth_list"))
		&& (sshUserauthPasswordEx = libSym<decltype(sshUserauthPasswordEx)>(lib, "libssh2_userauth_password_ex"))
		&& (sshUserauthAuthenticated = libSym<decltype(sshUserauthAuthenticated)>(lib, "libssh2_userauth_authenticated"))
		&& (sshSessionDisconnectEx = libSym<decltype(sshSessionDisconnectEx)>(lib, "libssh2_session_disconnect_ex"))
		&& (sshSessionFree = libSym<decltype(sshSessionFree)>(lib, "libssh2_session_free"))
		&& (sftpInit = libSym<decltype(sftpInit)>(lib, "libssh2_sftp_init"))
		&& (sftpShutdown = libSym<decltype(sftpShutdown)>(lib, "libssh2_sftp_shutdown"))
		&& (sftpOpenEx = libSym<decltype(sftpOpenEx)>(lib, "libssh2_sftp_open_ex"))
		&& (sftpRead = libSym<decltype(sftpRead)>(lib, "libssh2_sftp_read"))
		&& (sftpWrite = libSym<decltype(sftpWrite)>(lib, "libssh2_sftp_write"))
		&& (sftpSeek64 = libSym<decltype(sftpSeek64)>(lib, "libssh2_sftp_seek64"))
		&& (sftpTell64 = libSym<decltype(sftpTell64)>(lib, "libssh2_sftp_tell64"))
		&& (sftpReaddirEx = libSym<decltype(sftpReaddirEx)>(lib, "libssh2_sftp_readdir_ex"))
		&& (sftpClose = libSym<decltype(sftpClose)>(lib, "libssh2_sftp_close_handle"))
		&& (sftpStatEx = libSym<decltype(sftpStatEx)>(lib, "libssh2_sftp_stat_ex"))
		&& (sftpFstatEx = libSym<decltype(sftpFstatEx)>(lib, "libssh2_sftp_fstat_ex"))
		&& (sftpUnlinkEx = libSym<decltype(sftpUnlinkEx)>(lib, "libssh2_sftp_unlink_ex"))
		&& (sftpRmdirEx = libSym<decltype(sftpRmdirEx)>(lib, "libssh2_sftp_rmdir_ex"))
		&& (sftpRenameEx = libSym<decltype(sftpRenameEx)>(lib, "libssh2_sftp_rename_ex"))
	))) {
		libClose(lib);
		failed = true;
	}
	return lib;
}

void closeLibssh2() noexcept {
	libClose(lib);
	failed = false;
}
#endif
