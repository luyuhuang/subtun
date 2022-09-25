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

static void set_sockaddr_in(struct sockaddr_in &saddr, const addr_ipv4 &ad) {
	saddr.sin_family = AF_INET;
	ad.copy_ip(&saddr.sin_addr.s_addr);
	saddr.sin_port = htons(ad.port());
}

static void set_sockaddr_in6(struct sockaddr_in6 &saddr, const addr_ipv6 &ad) {
	saddr.sin6_family = AF_INET6;
	ad.copy_ip(&saddr.sin6_addr.s6_addr);
	saddr.sin6_port = htons(ad.port());
	//saddr.sin6_scope_id
}

socket_t make_udp4(const addr_ipv4 &ad) {
	int fd = socket(PF_INET, SOCK_DGRAM, 0);

	if (!ad.is_zero()) {
		struct sockaddr_in saddr {};
		set_sockaddr_in(saddr, ad);
		bind(fd, reinterpret_cast<struct sockaddr*>(&saddr), sizeof(saddr));
	}
	return fd;
}

socket_t make_udp6(const addr_ipv6 &ad) {
	int fd = socket(PF_INET6, SOCK_DGRAM, 0);

	if (!ad.is_zero()) {
		struct sockaddr_in6 saddr {};
		set_sockaddr_in6(saddr, ad);
		bind(fd, reinterpret_cast<struct sockaddr *>(&saddr), sizeof(saddr));
	}
	return fd;
}

size_t receive_from_socket4(const socket_t &sock, void *buf, size_t len, addr_ipv4 &ad) {
	ssize_t size;
	struct sockaddr_in saddr {};
	socklen_t as = sizeof(saddr);
	if (size = recvfrom(sock, buf, len, 0, reinterpret_cast<struct sockaddr *>(&saddr), &as); size < 0)
		throw runtime_error(string("receive_from_socket: recvfrom returns err ") + strerror(errno));
	if (saddr.sin_family != AF_INET) throw runtime_error("receive_from_socket: src is not ipv4");
	ad.set_ip(&saddr.sin_addr.s_addr);
	ad.set_port(ntohs(saddr.sin_port));
	return size;
}

size_t receive_from_socket6(const socket_t &sock, void *buf, size_t len, addr_ipv6 &ad) {
	ssize_t size;
	struct sockaddr_in6 saddr {};
	socklen_t as = sizeof(saddr);
	if (size = recvfrom(sock, buf, len, 0, reinterpret_cast<struct sockaddr *>(&saddr), &as); size < 0)
		throw runtime_error(string("receive_from_socket: recvfrom returns err ") + strerror(errno));
	if (saddr.sin6_family != AF_INET6) throw runtime_error("receive_from_socket: src is not ipv4");
	ad.set_ip(&saddr.sin6_addr.s6_addr);
	ad.set_port(ntohs(saddr.sin6_port));
	return size;
}

size_t receive_from_socket(const socket_t &sock, void *buf, size_t len) {
	ssize_t size;
	if (size = recvfrom(sock, buf, len, 0, nullptr, nullptr); size < 0)
		throw runtime_error(string("receive_from_socket: recvfrom returns err ") + strerror(errno));
	return size;
}

size_t send_to_socket4(const socket_t &sock, const void *buf, size_t len, const addr_ipv4 &ad) {
	ssize_t size;
	struct sockaddr_in saddr {};
	set_sockaddr_in(saddr, ad);
	if (size = sendto(sock, buf, len, 0, reinterpret_cast<struct sockaddr *>(&saddr), sizeof(saddr)); size < 0)
		throw runtime_error(string("receive_from_socket: sendto returns err ") + strerror(errno));
	return size;
}

size_t send_to_socket6(const socket_t &sock, const void *buf, size_t len, const addr_ipv6 &ad) {
	ssize_t size;
	struct sockaddr_in6 saddr {};
	set_sockaddr_in6(saddr, ad);
	if (size = sendto(sock, buf, len, 0, reinterpret_cast<struct sockaddr *>(&saddr), sizeof(saddr)); size < 0)
		throw runtime_error(string("receive_from_socket: sendto returns err ") + strerror(errno));
	return size;
}

void close_socket(socket_t &sock) {
	if (sock == socket_invalid) return;
	close(sock);
	sock = socket_invalid;
}

void connect4(const size_t &sock, const addr_ipv4 &ad) {
	struct sockaddr_in saddr{};
	set_sockaddr_in(saddr, ad);
	if (connect(sock, reinterpret_cast<struct sockaddr *>(&saddr), sizeof(saddr)) != 0)
		throw runtime_error("connect4: fail to connect. err ");
}

void connect6(const size_t &sock, const addr_ipv6 &ad) {
	struct sockaddr_in6 saddr{};
	set_sockaddr_in6(saddr, ad);
	if (connect(sock, reinterpret_cast<struct sockaddr *>(&saddr), sizeof(saddr)) != 0)
		throw runtime_error("connect4: fail to connect. err ");
}

socket_t make_tcp4(const addr_ipv4 &ad) {
	int fd = socket(PF_INET, SOCK_STREAM, 0);

	if (!ad.is_zero()) {
		struct sockaddr_in saddr {};
		set_sockaddr_in(saddr, ad);
		bind(fd, reinterpret_cast<struct sockaddr *>(&saddr), sizeof(saddr));
	}
	return fd;
}
socket_t make_tcp6(const addr_ipv6 &ad) {
	int fd = socket(PF_INET6, SOCK_STREAM, 0);

	if (!ad.is_zero()) {
		struct sockaddr_in6 saddr {};
		set_sockaddr_in6(saddr, ad);
		bind(fd, reinterpret_cast<struct sockaddr *>(&saddr), sizeof(saddr));
	}
	return fd;
}

void listen_tcp(const socket_t &sock) {
	if (listen(sock, 1024) != 0)
		throw runtime_error("fail to listen");
}

socket_t accept_tcp4(const socket_t &sock, addr_ipv4 &ad) {
	struct sockaddr_in saddr {};
	socklen_t as = sizeof(saddr);
	int fd = accept(fd, reinterpret_cast<struct sockaddr *>(&saddr), &as);
	if (fd < 0)
		throw runtime_error("fail to accept");
	if (saddr.sin_family != AF_INET)
		throw runtime_error("accept_tcp4: src is not ipv4");
	ad.set_ip(&saddr.sin_addr.s_addr);
	ad.set_port(ntohs(saddr.sin_port));
	return fd;
}
socket_t accept_tcp6(const socket_t &sock, addr_ipv6 &ad) {
	struct sockaddr_in6 saddr {};
	socklen_t as = sizeof(saddr);
	int fd = accept(fd, reinterpret_cast<struct sockaddr *>(&saddr), &as);
	if (fd < 0)
		throw runtime_error("fail to accept");
	if (saddr.sin6_family != AF_INET6)
		throw runtime_error("accept_tcp6: src is not ipv6");
	ad.set_ip(&saddr.sin6_addr.s6_addr);
	ad.set_port(ntohs(saddr.sin6_port));
	return fd;
}

size_t receive_socket(const socket_t &sock, void *buf, size_t len) {
	ssize_t size;
	if (size = recv(sock, buf, len, 0); size <= 0) {
		if (size == 0)
			throw runtime_error("socket disconnected");
		if (errno != EAGAIN)
			throw runtime_error("fail to receive socket");
	}
	return size;
}
size_t send_socket(const socket_t &sock, const void *buf, size_t len) {
	ssize_t size;
	if (size = send(sock, buf, len, 0); size < 0) {
		if (errno != EAGAIN)
			throw runtime_error("fail to send socket");
		return 0;
	}
	return size;
}
