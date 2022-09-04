#include <stdexcept>
#include <string>

#include <winsock2.h>
#include <ws2ipdef.h>

#include "../socket.h"

using std::runtime_error;
using std::string;

void set_sockaddr_in(SOCKADDR_IN &saddr, const addr &ad) {
	saddr.sin_family = AF_INET;
	ad.copy_ip(&saddr.sin_addr.s_addr);
	saddr.sin_port = htons(ad.port());
}

void set_sockaddr_in6(SOCKADDR_IN6 &saddr, const addr &ad) {
	saddr.sin6_family = AF_INET6;
	ad.copy_ip(&saddr.sin6_addr.s6_addr);
	saddr.sin6_port = htons(ad.port());
	//saddr.sin6_scope_id
}

socket_t make_udp(const addr &ad) {
	SOCKET fd;
	if (ad.type() == addr::ipv4) {
		fd = socket(PF_INET, SOCK_DGRAM, 0);
	} else {
		fd = socket(PF_INET6, SOCK_DGRAM, 0);
	}

	if (!ad.is_zero()) {
		if (ad.type() == addr::ipv4) {
			SOCKADDR_IN saddr {};
			set_sockaddr_in(saddr, ad);
			bind(fd, reinterpret_cast<SOCKADDR *>(&saddr), sizeof(saddr));
		} else {
			SOCKADDR_IN6 saddr {};
			set_sockaddr_in6(saddr, ad);
			bind(fd, reinterpret_cast<SOCKADDR *>(&saddr), sizeof(saddr));
		}
	}
	return fd;
}

size_t receive_from_udp(const socket_t &sock, void *buf, size_t len, addr &ad) {
	int size;
	char *p = reinterpret_cast<char *>(buf);
	if (ad.type() == addr::ipv4) {
		SOCKADDR_IN saddr {};
		int as = sizeof(saddr);
		if (size = recvfrom(sock, p, len, 0, reinterpret_cast<SOCKADDR *>(&saddr), &as); size < 0)
			throw runtime_error(string("receive_from_udp: recvfrom returns err ") + strerror(errno));
		if (saddr.sin_family != AF_INET) throw runtime_error("receive_from_udp: src is not ipv4");
		ad.set_ip(&saddr.sin_addr.s_addr);
		ad.set_port(ntohs(saddr.sin_port));
	} else {
		SOCKADDR_IN6 saddr{};
		int as = sizeof(saddr);
		if (size = recvfrom(sock, p, len, 0, reinterpret_cast<SOCKADDR *>(&saddr), &as); size < 0)
			throw runtime_error(string("receive_from_udp: recvfrom returns err ") + strerror(errno));
		if (saddr.sin6_family != AF_INET6) throw runtime_error("receive_from_udp: src is not ipv4");
		ad.set_ip(&saddr.sin6_addr.s6_addr);
		ad.set_port(ntohs(saddr.sin6_port));
	}
	return size;
}

size_t receive_from_udp(const socket_t &sock, void *buf, size_t len) {
	int size;
	if (size = recvfrom(sock, reinterpret_cast<char *>(buf), len, 0, nullptr, nullptr); size < 0)
		throw runtime_error(string("receive_from_udp: recvfrom returns err ") + strerror(errno));
	return size;
}

size_t send_to_udp(const socket_t &sock, const void *buf, size_t len, const addr &ad) {
	int size;
	const char *p = reinterpret_cast<const char *>(buf);
	if (ad.type() == addr::ipv4) {
		SOCKADDR_IN saddr {};
		set_sockaddr_in(saddr, ad);
		if (size = sendto(sock, p, len, 0, reinterpret_cast<SOCKADDR *>(&saddr), sizeof(saddr)); size < 0)
			throw runtime_error(string("receive_from_udp: sendto returns err ") + strerror(errno));
	} else {
		SOCKADDR_IN6 saddr{};
		set_sockaddr_in6(saddr, ad);
		if (size = sendto(sock, p, len, 0, reinterpret_cast<SOCKADDR *>(&saddr), sizeof(saddr)); size < 0)
			throw runtime_error(string("receive_from_udp: sendto returns err ") + strerror(errno));
	}
	return size;
}

socket_t make_tcp(addr &ad) {
	return 0;
}
