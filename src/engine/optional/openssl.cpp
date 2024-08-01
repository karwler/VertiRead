#ifdef CAN_OPENSSL
#include "openssl.h"
#include "internal.h"

static LibType lib = nullptr;
static bool failed = false;
decltype(SSLv23_method)* sslv23Method = nullptr;
decltype(SSL_CTX_new)* sslCtxNew = nullptr;
decltype(SSL_CTX_ctrl)* sslCtxCtrl = nullptr;
decltype(SSL_CTX_set_cipher_list)* sslCtxSetCipherList = nullptr;
decltype(SSL_CTX_set_options)* sslCtxSetOptions = nullptr;
decltype(SSL_CTX_sess_set_new_cb)* sslCtxSessSetNewCb = nullptr;
decltype(SSL_CTX_free)* sslCtxFree = nullptr;
decltype(SSL_new)* sslNew = nullptr;
decltype(SSL_get_ex_data)* sslGetExData = nullptr;
decltype(SSL_set_ex_data)* sslSetExData = nullptr;
decltype(SSL_set_session)* sslSetSession = nullptr;
decltype(SSL_SESSION_free)* sslSessionFree = nullptr;
decltype(SSL_set_fd)* sslSetFd = nullptr;
decltype(SSL_connect)* sslConnect = nullptr;
decltype(SSL_read)* sslRead = nullptr;
decltype(SSL_write)* sslWrite = nullptr;
decltype(SSL_shutdown)* sslShutdown = nullptr;
decltype(SSL_free)* sslFree = nullptr;
decltype(SSL_get_error)* sslGetError = nullptr;

bool symOpenssl() noexcept {
	if (!(lib || failed || ((lib = libOpen("libssl" LIB_EXT))
		&& (sslv23Method = libSym<decltype(sslv23Method)>(lib, "SSLv23_method"))
		&& (sslCtxNew = libSym<decltype(sslCtxNew)>(lib, "SSL_CTX_new"))
		&& (sslCtxCtrl = libSym<decltype(sslCtxCtrl)>(lib, "SSL_CTX_ctrl"))
		&& (sslCtxSetCipherList = libSym<decltype(sslCtxSetCipherList)>(lib, "SSL_CTX_set_cipher_list"))
		&& (sslCtxSetOptions = libSym<decltype(sslCtxSetOptions)>(lib, "SSL_CTX_set_options"))
		&& (sslCtxSessSetNewCb = libSym<decltype(sslCtxSessSetNewCb)>(lib, "SSL_CTX_sess_set_new_cb"))
		&& (sslCtxFree = libSym<decltype(sslCtxFree)>(lib, "SSL_CTX_free"))
		&& (sslNew = libSym<decltype(sslNew)>(lib, "SSL_new"))
		&& (sslGetExData = libSym<decltype(sslGetExData)>(lib, "SSL_get_ex_data"))
		&& (sslSetExData = libSym<decltype(sslSetExData)>(lib, "SSL_set_ex_data"))
		&& (sslSetSession = libSym<decltype(sslSetSession)>(lib, "SSL_set_session"))
		&& (sslSessionFree = libSym<decltype(sslSessionFree)>(lib, "SSL_SESSION_free"))
		&& (sslSetFd = libSym<decltype(sslSetFd)>(lib, "SSL_set_fd"))
		&& (sslConnect = libSym<decltype(sslConnect)>(lib, "SSL_connect"))
		&& (sslRead = libSym<decltype(sslRead)>(lib, "SSL_read"))
		&& (sslWrite = libSym<decltype(sslWrite)>(lib, "SSL_write"))
		&& (sslShutdown = libSym<decltype(sslShutdown)>(lib, "SSL_shutdown"))
		&& (sslFree = libSym<decltype(sslFree)>(lib, "SSL_free"))
		&& (sslGetError = libSym<decltype(sslGetError)>(lib, "SSL_get_error"))
	))) {
		libClose(lib);
		failed = true;
	}
	return lib;
}

void closeOpenssl() noexcept {
	libClose(lib);
	failed = false;
}
#endif
