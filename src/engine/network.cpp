#ifdef WITH_FTP
#include "network.h"
#include "optional/gnutls.h"
#include "optional/openssl.h"
#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <netdb.h>
#include <netinet/tcp.h>
#endif
#include <SDL_log.h>
#include <format>

#ifndef _WIN32
#define closesocket(s) close(s)
#endif

addrinfo* resolveAddress(const char* host, uint16 port, int family) {
	addrinfo hints = {
		.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG | AI_NUMERICSERV,
		.ai_family = family,
		.ai_socktype = SOCK_STREAM,
		.ai_protocol = IPPROTO_TCP
	};
	addrinfo* addrv;
	if (int rc = getaddrinfo(host, toStr(port).data(), &hints, &addrv))
#ifdef _WIN32
		throw std::runtime_error(std::format("Failed to resolve address: {}", swtos(gai_strerrorW(rc))));
#else
		throw std::runtime_error(std::format("Failed to resolve address: {}", gai_strerror(rc)));
#endif
	return addrv;
}

// NET CONNECTION

NetConnection::NetConnection(const IpAddress& addr) {
#ifdef _WIN32
	if (!wsa)
		throw std::runtime_error("Winsock 2.2 isn't initialized");
#endif
	if (sock = socket(addr.g.sa_family, SOCK_STREAM, IPPROTO_TCP); sock == INVALID_SOCKET)
		throw std::runtime_error(std::format("Failed to create socket: {}", lastError()));
	if (connect(sock, &addr.g, addr.g.sa_family == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6))) {
		string err = lastError();
		closesocket(sock);
		throw std::runtime_error(std::format("Failed to connect: {}", err));
	}
}

IpAddress NetConnection::startConnection(const char* host, uint16 port, int family, uint timeout) {
#ifdef _WIN32
	if (!wsa)
		throw std::runtime_error("Winsock 2.2 isn't initialized");
#endif
	IpAddress addr;
	addrinfo* addrv = resolveAddress(host, port, family);
	for (addrinfo* ai = addrv; ai; ai = ai->ai_next) {
		if (sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol); sock == INVALID_SOCKET)
			continue;
		if (!connect(sock, ai->ai_addr, ai->ai_addrlen)) {
			memcpy(&addr, ai->ai_addr, ai->ai_addrlen);
			break;
		}
		closesocket(sock);
		sock = INVALID_SOCKET;
	}
	freeaddrinfo(addrv);
	if (sock == INVALID_SOCKET)
		throw std::runtime_error("Failed to connect");
	setNetTimeout(timeout);
	return addr;
}

void NetConnection::startTls(TlsData& data) {
#ifdef CAN_GNUTLS
	if (initedGnutls) {
		gnutls_certificate_credentials_st* cred;
		if (int rc = gnutlsCertificateAllocateCredentials(&cred))
			throw std::runtime_error(std::format("Failed to allocate certificate credentials: {}", gnutlsStrerror(rc)));
		try {
			if (int rc = gnutlsInit(&sess, GNUTLS_CLIENT))
				throw std::runtime_error(std::format("Failed to init TLS session: {}", gnutlsStrerror(rc)));
			if (data.datum.data)
				if (int rc = gnutlsSessionSetData(sess, data.datum.data, data.datum.size))
					SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to set session data: %s", gnutlsStrerror(rc));
			if (int rc = gnutlsCredentialsSet(sess, GNUTLS_CRD_CERTIFICATE, cred))
				throw std::runtime_error(std::format("Failed to set credentials: {}", gnutlsStrerror(rc)));
			if (int rc = gnutlsSetDefaultPriority(sess))
				throw std::runtime_error(std::format("Failed to set priority: {}", gnutlsStrerror(rc)));

			gnutlsSessionSetPtr(sess, &data);
			gnutlsTransportSetInt2(sess, sock, sock);
			gnutlsHandshakeSetTimeout(sess, handshakeTimeout * 1000);
			gnutlsHandshakeSetHookFunction(sess, GNUTLS_HANDSHAKE_NEW_SESSION_TICKET, GNUTLS_HOOK_POST, storeGnuSessionData);
			for (int rc; (rc = gnutlsHandshake(sess));)
				if (rc != GNUTLS_E_AGAIN)
					throw std::runtime_error(std::format("Handshake failed: {}", gnutlsStrerror(rc)));
			gnutlsCertificateFreeCredentials(cred);
		} catch (const std::runtime_error&) {
			if (sess) {
				gnutlsDeinit(sess);
				sess = nullptr;
			}
			gnutlsCertificateFreeCredentials(cred);
			throw;
		}
		return;
	}
#endif
#ifdef CAN_OPENSSL
	if (ctx) {
		if (ssl = sslNew(ctx); !ssl)
			throw std::runtime_error("Failed to create TLS session");
		try {
			if (int rc = sslSetExData(ssl, 0, &data); rc != 1)
				throw std::runtime_error(std::format("Failed to set TLS data: {}", rc));
			if (data.sess)
				if (int rc = sslSetSession(ssl, data.sess); rc != 1)
					SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to set session data: %d", rc);
			if (int rc = sslSetFd(ssl, sock); rc != 1)
				throw std::runtime_error(std::format("Failed to set socket: {}", rc));
			setNetTimeout(handshakeTimeout);	// TODO: check if this works
			if (int rc = sslConnect(ssl); rc != 1)
				throw std::runtime_error(std::format("Handshake failed: {}", rc));
		} catch (const std::runtime_error&) {
			sslFree(ssl);
			ssl = nullptr;
			throw;
		}
	}
#endif
}

void NetConnection::setTimeout(uint timeout) noexcept {
#ifdef CAN_GNUTLS
	if (sess) {
		gnutlsRecordSetTimeout(sess, timeout ? timeout * 1000 : GNUTLS_INDEFINITE_TIMEOUT);
		return;
	}
#endif
	setNetTimeout(timeout);
}

void NetConnection::setNetTimeout(uint timeout) noexcept {
#ifdef _WIN32
	DWORD tv = timeout * 1000;
#else
	timeval tv = { .tv_sec = timeout };
#endif
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&tv), sizeof(tv)))
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to set timeout: %s", lastError());
}

nsint NetConnection::recv(void* buf, size_t len) {
	nsint ret;
#ifdef CAN_GNUTLS
	if (sess) {
		while ((ret = gnutlsRecordRecv(sess, buf, len)) < 0)
			if (ret != GNUTLS_E_AGAIN && ret != GNUTLS_E_REHANDSHAKE)
				throw std::runtime_error(std::format("Failed to receive data: {}", gnutlsStrerror(ret)));
	} else
#endif
#ifdef CAN_OPENSSL
	if (ssl) {
		if (ret = sslRead(ssl, buf, len); ret < 0)
			throw std::runtime_error(std::format("Failed to receive data: {}", sslGetError(ssl, ret)));
	} else
#endif
	if (ret = ::recv(sock, static_cast<char*>(buf), len, 0); ret < 0)
		throw std::runtime_error(std::format("Failed to receive data: {}", lastError()));
	return ret;
}

void NetConnection::send(const void* buf, size_t len) {
#ifdef CAN_GNUTLS
	if (sess) {
		if (nsint rc = gnutlsRecordSend(sess, buf, len); rc < 0 || size_t(rc) != len)
			throw std::runtime_error(std::format("Failed to send data: {}", gnutlsStrerror(rc)));
	} else
#endif
#ifdef CAN_OPENSSL
	if (ssl) {
		if (int rc = sslWrite(ssl, buf, len); size_t(rc) != len)
			throw std::runtime_error(std::format("Failed to send data: {}", sslGetError(ssl, rc)));
	} else
#endif
	if (nsint rc = ::send(sock, static_cast<const char*>(buf), len, 0); rc < 0 || size_t(rc) != len)
		throw std::runtime_error(std::format("Failed to send data: {}", lastError()));
}

void NetConnection::disconnect() noexcept {
	cleanup();
#ifdef CAN_GNUTLS
	sess = nullptr;
#endif
#ifdef CAN_OPENSSL
	ssl = nullptr;
#endif
	sock = INVALID_SOCKET;
}

void NetConnection::cleanup() noexcept {
#ifdef CAN_GNUTLS
	if (sess) {
		gnutlsBye(sess, GNUTLS_SHUT_RDWR);
		gnutlsDeinit(sess);
	}
#endif
#ifdef CAN_OPENSSL
	if (ssl) {
		sslShutdown(ssl);
		sslFree(ssl);
	}
#endif
	if (sock != INVALID_SOCKET)
		closesocket(sock);
}

bool NetConnection::initTlsLib() noexcept {
#ifdef CAN_GNUTLS
	if (symGnutls()) {
		if (initedGnutls = !gnutlsGlobalInit(); initedGnutls)
			return true;
		closeGnutls();
	}
#endif
#ifdef CAN_OPENSSL
	if (symOpenssl()) {
		if (const SSL_METHOD* method = sslv23Method())
			if (ctx = sslCtxNew(method); ctx) {
				sslCtxSetOptions(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION);
				if (sslCtxSetCipherList(ctx, "HIGH:!aNULL:!kRSA:!PSK:!SRP:!MD5:!RC4") == 1) {
					sslCtxCtrl(ctx, SSL_CTRL_MODE, SSL_MODE_AUTO_RETRY, nullptr);
					sslCtxCtrl(ctx, SSL_CTRL_SET_SESS_CACHE_MODE, SSL_SESS_CACHE_CLIENT, nullptr);
					sslCtxSessSetNewCb(ctx, storeSslSessionData);
					return true;
				}
				sslCtxFree(ctx);
				ctx = nullptr;
			}
		closeOpenssl();
	}
#endif
	return false;
}

void NetConnection::closeTlsLib(TlsData& data) noexcept {
#ifdef CAN_GNUTLS
	if (initedGnutls) {
		if (data.datum.data)
			(*gnutlsFree)(data.datum.data);
		gnutlsGlobalDeinit();
		initedGnutls = false;
	}
	closeGnutls();
#endif
#ifdef CAN_OPENSSL
	if (ctx) {
		if (data.sess)
			sslSessionFree(data.sess);
		sslCtxFree(NetConnection::ctx);
		ctx = nullptr;
	}
	closeOpenssl();
#endif
	data = {};
}

#ifdef CAN_GNUTLS
int NetConnection::storeGnuSessionData(gnutls_session_t s, uint, uint, uint, const gnutls_datum_t*) noexcept {
	auto td = static_cast<TlsData*>(gnutlsSessionGetPtr(s));
	if (td->datum.data) {
		(*gnutlsFree)(td->datum.data);
		td->datum.data = nullptr;
	}
	return gnutlsSessionGetData2(s, &td->datum);
}
#endif
#ifdef CAN_OPENSSL
int NetConnection::storeSslSessionData(SSL* s, SSL_SESSION* se) noexcept {
	auto td = static_cast<TlsData*>(sslGetExData(s, 0));
	if (!td) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get TLS data");
		return 0;
	}
	if (td->sess)
		sslSessionFree(td->sess);
	td->sess = se;
	return 1;
}
#endif

#ifdef _WIN32
void NetConnection::initWsa() noexcept {
	if (!wsa) {
		WSADATA wsad;
		wsa = !WSAStartup(MAKEWORD(2, 2), &wsad);
	}
}

void NetConnection::cleanupWsa() noexcept {
	if (wsa) {
		WSACleanup();
		wsa = false;
	}
}

string NetConnection::lastError() {
	LPSTR buff;
	DWORD len = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, WSAGetLastError(), 0, reinterpret_cast<LPSTR>(&buff), 0, nullptr);
	if (!len)
		return string();
	string ret(buff, len);
	LocalFree(buff);
	return ret;
}
#else
const char* NetConnection::lastError() {
	return strerror(errno);
}
#endif

// FTP RECEIVER

FtpReply FtpReceiver::sendCmd(NetConnection& conn, string_view cmd) {
	string txt = std::format("{}\r\n", cmd);
	conn.send(txt.data(), txt.length());
	return getReply(conn);
}

FtpReply FtpReceiver::sendCmd(NetConnection& conn, string_view cmd, string_view arg) {
	string txt = std::format("{} {}\r\n", cmd, arg);
	conn.send(txt.data(), txt.length());
	return getReply(conn);
}

FtpReply FtpReceiver::getReply(NetConnection& conn) {
	string_view line;
	if (!getLine(conn, line))
		throw std::runtime_error("Connection unexpectedly closed");
	if (line.empty())
		throw std::runtime_error("Empty reply");

	bool entry = line[0] == ' ';
	string_view::iterator cmdPos = line.begin() + entry;
	string_view::iterator cmdEnd = std::find_if(cmdPos, line.end(), [](char ch) -> bool { return ch == ' ' || ch == '-'; });
	ushort rcode;
	std::from_chars_result crs = cmdEnd - cmdPos == 3 ? std::from_chars(std::to_address(cmdPos), std::to_address(cmdEnd), rcode, 10) : std::from_chars_result{ .ec = std::errc::invalid_argument };
	FtpReply reply = {
		.cmd = string(cmdPos, cmdEnd),
		.args = cmdEnd != line.end() ? string(cmdEnd + 1, line.end()) : string(),
		.code = crs.ptr == std::to_address(cmdEnd) && crs.ec == std::errc() ? rcode : ushort(0),
		.cont = cmdEnd != line.end() && *cmdEnd == '-',
		.entry = entry
	};
	advanceLine(line.length());
	return reply;
}

bool FtpReceiver::getLine(NetConnection& conn, string_view& line) {
	size_t lend;
	for (size_t ofs = pos; (lend = findLineEnd(ofs)) >= end;) {
		if (end < rsv) {
			nsint len = conn.recv(buf.get() + end, rsv - end);
			if (!len) {
				line = string_view(buf.get() + pos, end - pos);
				return false;
			}
			ofs = end ? end - 1 : 0;
			end += len;
		} else if (!pos) {	// no more space to receive data
			expandBuffer(buf, rsv, end, lineStep);
			ofs = end ? end - 1 : 0;
		} else {			// can make space without reallocating
			std::copy_backward(buf.get() + pos, buf.get() + end, buf.get() + rsv - pos);
			end -= pos;
			pos = 0;
			ofs = end ? end - 1 : 0;
		}
	}
	line = string_view(buf.get() + pos, lend - pos);
	return true;
}

size_t FtpReceiver::findLineEnd(size_t ofs) const noexcept {
	size_t i;
	for (i = ofs; i < end; ++i)
		if (buf[i] == '\r' && i + 1 < end && buf[i + 1] == '\n')
			break;
	return i;
}

Data FtpReceiver::getData(NetConnection& conn, size_t initRsv) {
	size_t dlen = 0;
	size_t drsv = coalesce(initRsv, dataStep);
	uptr<byte_t[]> data = std::make_unique_for_overwrite<byte_t[]>(drsv);
	for (nsint len; (len = conn.recv(data.get() + dlen, drsv - dlen));)
		if (dlen += len; dlen >= drsv)
			expandBuffer(data, drsv, dlen, dataStep);
	return Data(std::move(data), dlen);
}

template <IntEnum T>
void FtpReceiver::expandBuffer(uptr<T[]>& buf, size_t& rsv, size_t len, size_t step) {
	rsv += step;
	uptr<T[]> dst = std::make_unique_for_overwrite<T[]>(rsv);
	std::copy_n(buf.get(), len, dst.get());
	buf = std::move(dst);
}
#endif
