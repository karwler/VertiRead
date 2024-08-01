#pragma once

#if defined(CAN_SFTP) || defined(WITH_FTP)
#include "utils/utils.h"
#ifdef _WIN32
#include <winsock2.h>
#include <ws2ipdef.h>
#else
#include <netinet/in.h>
#endif
#ifdef CAN_GNUTLS
#include <gnutls/gnutls.h>
#endif

#ifdef _WIN32
using nsint = int;
#else
using nsint = ssize_t;
using SOCKET = int;

#define INVALID_SOCKET -1
#endif

struct addrinfo;
struct ssl_ctx_st;
struct ssl_st;
struct ssl_session_st;

union IpAddress {
	sockaddr g;
	sockaddr_in v4;
	sockaddr_in6 v6;
};

addrinfo* resolveAddress(const char* host, uint16 port, int family);

struct TlsData {
#ifdef CAN_GNUTLS
	gnutls_datum_t datum;
#endif
#ifdef CAN_OPENSSL
	ssl_session_st* sess;
#endif
};

class NetConnection {
private:
#ifdef _WIN32
	static inline bool wsa = false;
#endif
#ifdef CAN_GNUTLS
	static inline bool initedGnutls = false;
#endif
#ifdef CAN_OPENSSL
	static inline ssl_ctx_st* ctx = nullptr;
#endif
	static constexpr uint handshakeTimeout = 10;

#ifdef CAN_GNUTLS
	gnutls_session_int* sess = nullptr;
#endif
#ifdef CAN_OPENSSL
	ssl_st* ssl = nullptr;
#endif
	SOCKET sock = INVALID_SOCKET;

public:
	NetConnection() = default;
	NetConnection(const NetConnection&) = delete;
	NetConnection(NetConnection&&) = delete;
	NetConnection(const IpAddress& addr);
	~NetConnection() { cleanup(); }

	IpAddress startConnection(const char* host, uint16 port, int family, uint timeout);

	static bool initTlsLib() noexcept;
	static void closeTlsLib(TlsData& data) noexcept;
#ifdef _WIN32
	static void initWsa() noexcept;
	static void cleanupWsa() noexcept;
#endif
	operator bool() const { return sock != INVALID_SOCKET; }
	bool tls() const;
	void startTls(TlsData& data);
	void setTimeout(uint timeout) noexcept;
	nsint recv(void* buf, size_t len);
	void send(const void* buf, size_t len);
	void disconnect() noexcept;
private:
	void setNetTimeout(uint timeout) noexcept;
	void cleanup() noexcept;
#ifdef CAN_GNUTLS
	static int storeGnuSessionData(gnutls_session_t s, uint htype, uint when, uint incoming, const gnutls_datum_t* msg) noexcept;
#endif
#ifdef CAN_OPENSSL
	static int storeSslSessionData(ssl_st* s, ssl_session_st* se) noexcept;
#endif
#ifdef _WIN32
	static string lastError();
#else
	static const char* lastError();
#endif
};

inline bool NetConnection::tls() const {
#if defined(CAN_GNUTLS) && defined(CAN_OPENSSL)
	return sess || ssl;
#elif defined(CAN_GNUTLS)
	return sess;
#elif defined(CAN_OPENSSL)
	return ssl;
#else
	return false;
#endif
}

struct FtpReply {
	string cmd;
	string args;
	ushort code;
	bool cont;
	bool entry;

	bool operator==(ushort rc) const;
	bool isCont(ushort rc) const;
};

inline bool FtpReply::operator==(ushort rc) const {
	return code == rc && !cont && !entry;
}

inline bool FtpReply::isCont(ushort rc) const {
	return code == rc && cont && !entry;
}

class FtpReceiver {
private:
	static constexpr size_t lineStep = 256;
	static constexpr size_t dataStep = 4096;

public:
	uptr<char[]> buf;
	size_t pos = 0;	// position of unused data
	size_t end = 0;	// position of received data
	size_t rsv = 0;	// total buffer size

	FtpReply sendCmd(NetConnection& conn, string_view cmd);
	FtpReply sendCmd(NetConnection& conn, string_view cmd, string_view arg);
	FtpReply getReply(NetConnection& conn);
	bool getLine(NetConnection& conn, string_view& line);
	void advanceLine(size_t length);
	static Data getData(NetConnection& conn, size_t initRsv);

private:
	size_t findLineEnd(size_t ofs) const noexcept;
	template <IntEnum T> static void expandBuffer(uptr<T[]>& buf, size_t& rsv, size_t len, size_t step);
};

inline void FtpReceiver::advanceLine(size_t length) {
	pos += length + 2;
}
#endif
