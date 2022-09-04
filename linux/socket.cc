#include <stdexcept>
#include <string>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

#include "../socket.h"

using std::runtime_error;
using std::string;

const socket_t socket_invalid = -1;

void set_sockaddr_in(struct sockaddr_in &saddr, const addr &ad) {
	saddr.sin_family = AF_INET;
	ad.copy_ip(&saddr.sin_addr.s_addr);
	saddr.sin_port = htons(ad.port());
}

void set_sockaddr_in6(struct sockaddr_in6 &saddr, const addr &ad) {
	saddr.sin6_family = AF_INET6;
	ad.copy_ip(&saddr.sin6_addr.s6_addr);
	saddr.sin6_port = htons(ad.port());
	//saddr.sin6_scope_id
}

socket_t make_udp(const addr &ad) {
	int fd;
	if (ad.type() == addr::ipv4) {
		fd = socket(PF_INET, SOCK_DGRAM, 0);
	} else {
		fd = socket(PF_INET6, SOCK_DGRAM, 0);
	}

	if (!ad.is_zero()) {
		if (ad.type() == addr::ipv4) {
			struct sockaddr_in saddr {};
			set_sockaddr_in(saddr, ad);
			bind(fd, reinterpret_cast<struct sockaddr*>(&saddr), sizeof(saddr));
		} else {
			struct sockaddr_in6 saddr {};
			set_sockaddr_in6(saddr, ad);
			bind(fd, reinterpret_cast<struct sockaddr *>(&saddr), sizeof(saddr));
		}
	}
	return fd;
}

size_t receive_from_udp(const socket_t &sock, void *buf, size_t len, addr &ad) {
	ssize_t size;
	if (ad.type() == addr::ipv4) {
		struct sockaddr_in saddr {};
		socklen_t as = sizeof(saddr);
		if (size = recvfrom(sock, buf, len, 0, reinterpret_cast<struct sockaddr *>(&saddr), &as); size < 0)
			throw runtime_error(string("receive_from_udp: recvfrom returns err ") + strerror(errno));
		if (saddr.sin_family != AF_INET) throw runtime_error("receive_from_udp: src is not ipv4");
		ad.set_ip(&saddr.sin_addr.s_addr);
		ad.set_port(ntohs(saddr.sin_port));
	} else {
		struct sockaddr_in6 saddr {};
		socklen_t as = sizeof(saddr);
		if (size = recvfrom(sock, buf, len, 0, reinterpret_cast<struct sockaddr *>(&saddr), &as); size < 0) 
			throw runtime_error(string("receive_from_udp: recvfrom returns err ") + strerror(errno));
		if (saddr.sin6_family != AF_INET6) throw runtime_error("receive_from_udp: src is not ipv4");
		ad.set_ip(&saddr.sin6_addr.s6_addr);
		ad.set_port(ntohs(saddr.sin6_port));
	}
	return size;
}

size_t receive_from_udp(const socket_t &sock, void *buf, size_t len) {
	ssize_t size;
	if (size = recvfrom(sock, buf, len, 0, nullptr, nullptr); size < 0)
		throw runtime_error(string("receive_from_udp: recvfrom returns err ") + strerror(errno));
	return size;
}

size_t send_to_udp(const socket_t &sock, const void *buf, size_t len, const addr &ad) {
	ssize_t size;
	if (ad.type() == addr::ipv4) {
		struct sockaddr_in saddr {};
		set_sockaddr_in(saddr, ad);
		if (size = sendto(sock, buf, len, 0, reinterpret_cast<struct sockaddr *>(&saddr), sizeof(saddr)); size < 0)
			throw runtime_error(string("receive_from_udp: sendto returns err ") + strerror(errno));
	} else {
		struct sockaddr_in6 saddr {};
		set_sockaddr_in6(saddr, ad);
		if (size = sendto(sock, buf, len, 0, reinterpret_cast<struct sockaddr *>(&saddr), sizeof(saddr)); size < 0)
			throw runtime_error(string("receive_from_udp: sendto returns err ") + strerror(errno));
	}
	return size;
}

void close_udp(socket_t &sock) {
	if (sock == socket_invalid) return;
	close(sock);
	sock = socket_invalid;
}

socket_t make_tcp(addr &ad) {
	return 0;
}
