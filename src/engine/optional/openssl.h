#pragma once

#ifdef CAN_OPENSSL
#include <openssl/ssl.h>

extern decltype(SSLv23_method)* sslv23Method;
extern decltype(SSL_CTX_new)* sslCtxNew;
extern decltype(SSL_CTX_ctrl)* sslCtxCtrl;
extern decltype(SSL_CTX_set_cipher_list)* sslCtxSetCipherList;
extern decltype(SSL_CTX_set_options)* sslCtxSetOptions;
extern decltype(SSL_CTX_sess_set_new_cb)* sslCtxSessSetNewCb;
extern decltype(SSL_CTX_free)* sslCtxFree;
extern decltype(SSL_new)* sslNew;
extern decltype(SSL_get_ex_data)* sslGetExData;
extern decltype(SSL_set_ex_data)* sslSetExData;
extern decltype(SSL_set_session)* sslSetSession;
extern decltype(SSL_SESSION_free)* sslSessionFree;
extern decltype(SSL_set_fd)* sslSetFd;
extern decltype(SSL_connect)* sslConnect;
extern decltype(SSL_read)* sslRead;
extern decltype(SSL_write)* sslWrite;
extern decltype(SSL_shutdown)* sslShutdown;
extern decltype(SSL_free)* sslFree;
extern decltype(SSL_get_error)* sslGetError;

bool symOpenssl() noexcept;
void closeOpenssl() noexcept;
#endif
