#ifdef CAN_GNUTLS
#include "gnutls.h"
#include "internal.h"

static LibType lib = nullptr;
static bool failed = false;
decltype(gnutls_global_init)* gnutlsGlobalInit = nullptr;
decltype(gnutls_global_deinit)* gnutlsGlobalDeinit = nullptr;
decltype(gnutls_init)* gnutlsInit = nullptr;
decltype(gnutls_bye)* gnutlsBye = nullptr;
decltype(gnutls_deinit)* gnutlsDeinit = nullptr;
decltype(gnutls_certificate_allocate_credentials)* gnutlsCertificateAllocateCredentials = nullptr;
decltype(gnutls_certificate_free_credentials)* gnutlsCertificateFreeCredentials = nullptr;
decltype(gnutls_credentials_set)* gnutlsCredentialsSet = nullptr;
decltype(gnutls_set_default_priority)* gnutlsSetDefaultPriority = nullptr;
decltype(gnutls_session_get_data2)* gnutlsSessionGetData2 = nullptr;
decltype(gnutls_session_set_data)* gnutlsSessionSetData = nullptr;
decltype(gnutls_session_get_ptr)* gnutlsSessionGetPtr = nullptr;
decltype(gnutls_session_set_ptr)* gnutlsSessionSetPtr = nullptr;
decltype(gnutls_transport_set_int2)* gnutlsTransportSetInt2 = nullptr;
decltype(gnutls_record_set_timeout)* gnutlsRecordSetTimeout = nullptr;
decltype(gnutls_handshake_set_timeout)* gnutlsHandshakeSetTimeout = nullptr;
decltype(gnutls_handshake_set_hook_function)* gnutlsHandshakeSetHookFunction = nullptr;
decltype(gnutls_handshake)* gnutlsHandshake = nullptr;
decltype(gnutls_record_recv)* gnutlsRecordRecv = nullptr;
decltype(gnutls_record_send)* gnutlsRecordSend = nullptr;
decltype(gnutls_free)* gnutlsFree = nullptr;
decltype(gnutls_strerror)* gnutlsStrerror = nullptr;

bool symGnutls() noexcept {
	decltype(gnutls_check_version)* gnutlsCheckVersion;
	if (!(lib || failed || ((lib = libOpen("libgnutls" LIB_EXT))
		&& (gnutlsCheckVersion = libSym<decltype(gnutlsCheckVersion)>(lib, "gnutls_check_version"))
		&& gnutlsCheckVersion("3.6.10")
		&& (gnutlsGlobalInit = libSym<decltype(gnutlsGlobalInit)>(lib, "gnutls_global_init"))
		&& (gnutlsGlobalDeinit = libSym<decltype(gnutlsGlobalDeinit)>(lib, "gnutls_global_deinit"))
		&& (gnutlsInit = libSym<decltype(gnutlsInit)>(lib, "gnutls_init"))
		&& (gnutlsBye = libSym<decltype(gnutlsBye)>(lib, "gnutls_bye"))
		&& (gnutlsDeinit = libSym<decltype(gnutlsDeinit)>(lib, "gnutls_deinit"))
		&& (gnutlsCertificateAllocateCredentials = libSym<decltype(gnutlsCertificateAllocateCredentials)>(lib, "gnutls_certificate_allocate_credentials"))
		&& (gnutlsCertificateFreeCredentials = libSym<decltype(gnutlsCertificateFreeCredentials)>(lib, "gnutls_certificate_free_credentials"))
		&& (gnutlsCredentialsSet = libSym<decltype(gnutlsCredentialsSet)>(lib, "gnutls_credentials_set"))
		&& (gnutlsSetDefaultPriority = libSym<decltype(gnutlsSetDefaultPriority)>(lib, "gnutls_set_default_priority"))
		&& (gnutlsSessionGetData2 = libSym<decltype(gnutlsSessionGetData2)>(lib, "gnutls_session_get_data2"))
		&& (gnutlsSessionSetData = libSym<decltype(gnutlsSessionSetData)>(lib, "gnutls_session_set_data"))
		&& (gnutlsSessionGetPtr = libSym<decltype(gnutlsSessionGetPtr)>(lib, "gnutls_session_get_ptr"))
		&& (gnutlsSessionSetPtr = libSym<decltype(gnutlsSessionSetPtr)>(lib, "gnutls_session_set_ptr"))
		&& (gnutlsTransportSetInt2 = libSym<decltype(gnutlsTransportSetInt2)>(lib, "gnutls_transport_set_int2"))
		&& (gnutlsRecordSetTimeout = libSym<decltype(gnutlsRecordSetTimeout)>(lib, "gnutls_record_set_timeout"))
		&& (gnutlsHandshakeSetTimeout = libSym<decltype(gnutlsHandshakeSetTimeout)>(lib, "gnutls_handshake_set_timeout"))
		&& (gnutlsHandshakeSetHookFunction = libSym<decltype(gnutlsHandshakeSetHookFunction)>(lib, "gnutls_handshake_set_hook_function"))
		&& (gnutlsHandshake = libSym<decltype(gnutlsHandshake)>(lib, "gnutls_handshake"))
		&& (gnutlsRecordRecv = libSym<decltype(gnutlsRecordRecv)>(lib, "gnutls_record_recv"))
		&& (gnutlsRecordSend = libSym<decltype(gnutlsRecordSend)>(lib, "gnutls_record_send"))
		&& (gnutlsFree = libSym<decltype(gnutlsFree)>(lib, "gnutls_free"))
		&& (gnutlsStrerror = libSym<decltype(gnutlsStrerror)>(lib, "gnutls_strerror"))
	))) {
		libClose(lib);
		failed = true;
	}
	return lib;
}

void closeGnutls() noexcept {
	libClose(lib);
	failed = false;
}
#endif
