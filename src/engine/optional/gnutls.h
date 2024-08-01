#pragma once

#ifdef CAN_GNUTLS
#include <gnutls/gnutls.h>

extern decltype(gnutls_global_init)* gnutlsGlobalInit;
extern decltype(gnutls_global_deinit)* gnutlsGlobalDeinit;
extern decltype(gnutls_init)* gnutlsInit;
extern decltype(gnutls_bye)* gnutlsBye;
extern decltype(gnutls_deinit)* gnutlsDeinit;
extern decltype(gnutls_certificate_allocate_credentials)* gnutlsCertificateAllocateCredentials;
extern decltype(gnutls_certificate_free_credentials)* gnutlsCertificateFreeCredentials;
extern decltype(gnutls_credentials_set)* gnutlsCredentialsSet;
extern decltype(gnutls_set_default_priority)* gnutlsSetDefaultPriority;
extern decltype(gnutls_session_get_data2)* gnutlsSessionGetData2;
extern decltype(gnutls_session_set_data)* gnutlsSessionSetData;
extern decltype(gnutls_session_get_ptr)* gnutlsSessionGetPtr;
extern decltype(gnutls_session_set_ptr)* gnutlsSessionSetPtr;
extern decltype(gnutls_transport_set_int2)* gnutlsTransportSetInt2;
extern decltype(gnutls_record_set_timeout)* gnutlsRecordSetTimeout;
extern decltype(gnutls_handshake_set_timeout)* gnutlsHandshakeSetTimeout;
extern decltype(gnutls_handshake_set_hook_function)* gnutlsHandshakeSetHookFunction;
extern decltype(gnutls_handshake)* gnutlsHandshake;
extern decltype(gnutls_record_recv)* gnutlsRecordRecv;
extern decltype(gnutls_record_send)* gnutlsRecordSend;
extern decltype(gnutls_free)* gnutlsFree;
extern decltype(gnutls_strerror)* gnutlsStrerror;

bool symGnutls() noexcept;
void closeGnutls() noexcept;
#endif
