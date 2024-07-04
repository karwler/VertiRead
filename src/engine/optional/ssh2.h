#pragma once

#ifdef CAN_SFTP
#include <libssh2.h>
#include <libssh2_sftp.h>

extern decltype(libssh2_init)* sshInit;
extern decltype(libssh2_exit)* sshExit;
extern decltype(libssh2_session_init_ex)* sshSessionInitEx;
extern decltype(libssh2_session_set_blocking)* sshSessionSetBlocking;
extern decltype(libssh2_session_handshake)* sshSessionHandshake;
extern decltype(libssh2_session_last_error)* sshSessionLastError;
extern decltype(libssh2_userauth_list)* sshUserauthList;
extern decltype(libssh2_userauth_password_ex)* sshUserauthPasswordEx;
extern decltype(libssh2_session_disconnect_ex)* sshSessionDisconnectEx;
extern decltype(libssh2_session_free)* sshSessionFree;
extern decltype(libssh2_sftp_init)* sftpInit;
extern decltype(libssh2_sftp_shutdown)* sftpShutdown;
extern decltype(libssh2_sftp_open_ex)* sftpOpenEx;
extern decltype(libssh2_sftp_read)* sftpRead;
extern decltype(libssh2_sftp_write)* sftpWrite;
extern decltype(libssh2_sftp_seek64)* sftpSeek64;
extern decltype(libssh2_sftp_tell64)* sftpTell64;
extern decltype(libssh2_sftp_readdir_ex)* sftpReaddirEx;
extern decltype(libssh2_sftp_close_handle)* sftpClose;
extern decltype(libssh2_sftp_stat_ex)* sftpStatEx;
extern decltype(libssh2_sftp_fstat_ex)* sftpFstatEx;
extern decltype(libssh2_sftp_unlink_ex)* sftpUnlinkEx;
extern decltype(libssh2_sftp_rmdir_ex)* sftpRmdirEx;
extern decltype(libssh2_sftp_rename_ex)* sftpRenameEx;

bool symLibssh2() noexcept;
void closeLibssh2() noexcept;
#endif
