#include <stdexcept>
#include <string>

#include <winsock2.h>
#include <ws2ipdef.h>

#include "err.h"
#include "../socket.h"

using std::runtime_error;
using std::string;

const socket_t socket_invalid = 0;

static void set_sockaddr_in(SOCKADDR_IN &saddr, const addr_ipv4 &ad) {
	saddr.sin_family = AF_INET;
	ad.copy_ip(&saddr.sin_addr.s_addr);
	saddr.sin_port = htons(ad.port());
}

static void set_sockaddr_in6(SOCKADDR_IN6 &saddr, const addr_ipv6 &ad) {
	saddr.sin6_family = AF_INET6;
	ad.copy_ip(&saddr.sin6_addr.s6_addr);
	saddr.sin6_port = htons(ad.port());
	//saddr.sin6_scope_id
}

socket_t make_udp4(const addr_ipv4 &ad) {
	SOCKET fd = socket(PF_INET, SOCK_DGRAM, 0);

	if (!ad.is_zero()) {
		SOCKADDR_IN saddr {};
		set_sockaddr_in(saddr, ad);
		bind(fd, reinterpret_cast<SOCKADDR *>(&saddr), sizeof(saddr));
	}
	return fd;
}

socket_t make_udp6(const addr_ipv6 &ad) {
	SOCKET fd = socket(PF_INET6, SOCK_DGRAM, 0);

	if (!ad.is_zero()) {
		SOCKADDR_IN6 saddr{};
		set_sockaddr_in6(saddr, ad);
		bind(fd, reinterpret_cast<SOCKADDR *>(&saddr), sizeof(saddr));
	}
	return fd;
}

size_t receive_from_udp4(const socket_t &sock, void *buf, size_t len, addr_ipv4 &ad) {
	int size;
	char *p = reinterpret_cast<char *>(buf);
	SOCKADDR_IN saddr {};
	int as = sizeof(saddr);
	if (size = recvfrom(sock, p, len, 0, reinterpret_cast<SOCKADDR *>(&saddr), &as); size < 0)
		throw runtime_error("receive_from_udp: recvfrom returns err " + last_error_str());
	if (saddr.sin_family != AF_INET) throw runtime_error("receive_from_udp: src is not ipv4");
	ad.set_ip(&saddr.sin_addr.s_addr);
	ad.set_port(ntohs(saddr.sin_port));
	return size;
}

size_t receive_from_udp6(const socket_t &sock, void *buf, size_t len, addr_ipv6 &ad) {
	int size;
	char *p = reinterpret_cast<char *>(buf);
	SOCKADDR_IN6 saddr{};
	int as = sizeof(saddr);
	if (size = recvfrom(sock, p, len, 0, reinterpret_cast<SOCKADDR *>(&saddr), &as); size < 0)
		throw runtime_error("receive_from_udp: recvfrom returns err " + last_error_str());
	if (saddr.sin6_family != AF_INET6) throw runtime_error("receive_from_udp: src is not ipv4");
	ad.set_ip(&saddr.sin6_addr.s6_addr);
	ad.set_port(ntohs(saddr.sin6_port));
	return size;
}

size_t receive_from_udp(const socket_t &sock, void *buf, size_t len) {
	int size;
	if (size = recvfrom(sock, reinterpret_cast<char *>(buf), len, 0, nullptr, nullptr); size < 0)
		throw runtime_error("receive_from_udp: recvfrom returns err " + last_error_str());
	return size;
}

size_t send_to_udp4(const socket_t &sock, const void *buf, size_t len, const addr_ipv4 &ad) {
	int size;
	const char *p = reinterpret_cast<const char *>(buf);
	SOCKADDR_IN saddr{};
	set_sockaddr_in(saddr, ad);
	if (size = sendto(sock, p, len, 0, reinterpret_cast<SOCKADDR *>(&saddr), sizeof(saddr)); size < 0)
		throw runtime_error("receive_from_udp: sendto returns err " + last_error_str());
	return size;
}

size_t send_to_udp6(const socket_t &sock, const void *buf, size_t len, const addr_ipv6 &ad) {
	int size;
	const char *p = reinterpret_cast<const char *>(buf);
	SOCKADDR_IN6 saddr{};
	set_sockaddr_in6(saddr, ad);
	if (size = sendto(sock, p, len, 0, reinterpret_cast<SOCKADDR *>(&saddr), sizeof(saddr)); size < 0)
		throw runtime_error("receive_from_udp: sendto returns err " + last_error_str());
	return size;
}

void close_udp(socket_t &sock) {
	if (sock == socket_invalid) return;
	closesocket(sock);
	sock = socket_invalid;
}

void connect4(const size_t &sock, const addr_ipv4 &ad) {
	SOCKADDR_IN saddr{};
	set_sockaddr_in(saddr, ad);
	if (connect(sock, reinterpret_cast<SOCKADDR *>(&saddr), sizeof(saddr)) != 0)
		throw runtime_error("connect4: fail to connect. err " + last_error_str());
}

void connect6(const size_t &sock, const addr_ipv6 &ad) {
	SOCKADDR_IN6 saddr{};
	set_sockaddr_in6(saddr, ad);
	if (connect(sock, reinterpret_cast<SOCKADDR *>(&saddr), sizeof(saddr)) != 0)
		throw runtime_error("connect4: fail to connect. err " + last_error_str());
}
