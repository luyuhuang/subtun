#pragma once

#include <cstddef>

#include "addr.h"

#if defined (_WIN32)
	#include <winsock2.h>
	using socket_t = SOCKET;
#else
	using socket_t = int;
#endif

extern const socket_t socket_invalid;

socket_t make_udp4(const addr_ipv4 &ad);
socket_t make_udp6(const addr_ipv6 &ad);
size_t receive_from_socket4(const socket_t &sock, void *buf, size_t len, addr_ipv4 &ad);
size_t receive_from_socket6(const socket_t &sock, void *buf, size_t len, addr_ipv6 &ad);
size_t receive_from_socket(const socket_t &sock, void *buf, size_t len);
size_t send_to_socket4(const socket_t &sock, const void *buf, size_t len, const addr_ipv4 &ad);
size_t send_to_socket6(const socket_t &sock, const void *buf, size_t len, const addr_ipv6 &ad);

void connect4(const size_t &sock, const addr_ipv4 &ad);
void connect6(const size_t &sock, const addr_ipv6 &ad);

socket_t make_tcp4(const addr_ipv4 &ad);
socket_t make_tcp6(const addr_ipv6 &ad);

void listen_tcp(const socket_t &sock);

socket_t accept_tcp4(const socket_t &sock, addr_ipv4 &ad);
socket_t accept_tcp6(const socket_t &sock, addr_ipv6 &ad);

size_t receive_socket(const socket_t &sock, void *buf, size_t len);
size_t send_socket(const socket_t &sock, const void *buf, size_t len);

void close_socket(socket_t &sock);


template <typename Addr>
inline socket_t make_udp(const Addr &ad) = delete;

template <> 
inline socket_t make_udp<addr_ipv4>(const addr_ipv4 &ad) {
	return make_udp4(ad);
};
template <> 
inline socket_t make_udp<addr_ipv6>(const addr_ipv6 &ad) {
	return make_udp6(ad);
};

template <typename Addr>
inline size_t receive_from_socket(const socket_t &sock, void *buf, size_t len, Addr &ad) = delete;

template <>
inline size_t receive_from_socket<addr_ipv4>(const socket_t &sock, void *buf, size_t len, addr_ipv4 &ad) {
	return receive_from_socket4(sock, buf, len, ad);
}
template <>
inline size_t receive_from_socket<addr_ipv6>(const socket_t &sock, void *buf, size_t len, addr_ipv6 &ad) {
	return receive_from_socket6(sock, buf, len, ad);
}

template <typename Addr>
inline size_t send_to_socket(const socket_t &sock, const void *buf, size_t len, const Addr &ad) = delete;

template <>
inline size_t send_to_socket<addr_ipv4>(const socket_t &sock, const void *buf, size_t len, const addr_ipv4 &ad) {
	return send_to_socket4(sock, buf, len, ad);
}
template <>
inline size_t send_to_socket<addr_ipv6>(const socket_t &sock, const void *buf, size_t len, const addr_ipv6 &ad) {
	return send_to_socket6(sock, buf, len, ad);
}

template <typename Addr>
inline void connect_socket(const size_t &sock, const Addr &ad) = delete;

template <>
inline void connect_socket<addr_ipv4>(const size_t &sock, const addr_ipv4 &ad) {
	connect4(sock, ad);
}

template <>
inline void connect_socket<addr_ipv6>(const size_t &sock, const addr_ipv6 &ad) {
	connect6(sock, ad);
}

template <typename Addr>
inline socket_t make_tcp(const Addr &ad) = delete;

template <>
inline socket_t make_tcp<addr_ipv4>(const addr_ipv4 &ad) {
	return make_tcp4(ad);
};
template <>
inline socket_t make_tcp<addr_ipv6>(const addr_ipv6 &ad) {
	return make_tcp6(ad);
};

template <typename Addr>
inline socket_t accept_tcp(const socket_t &sock, Addr &ad) = delete;

template <>
inline socket_t accept_tcp<addr_ipv4>(const socket_t &sock, addr_ipv4 &ad) {
	return accept_tcp4(sock, ad);
};
template <>
inline socket_t accept_tcp<addr_ipv6>(const socket_t &sock, addr_ipv6 &ad) {
	return accept_tcp6(sock, ad);
};


class socket_obj {
	socket_t m_sock;
public:
	socket_obj() : m_sock(socket_invalid) {}
	socket_obj(const socket_t &sock) : m_sock(sock) {}
	socket_obj(const socket_obj &) = delete;
	socket_obj &operator=(const socket_obj &) = delete;

	socket_obj(socket_obj &&o) : m_sock(o.m_sock) {
		o.m_sock = socket_invalid;
	}
	socket_obj &operator=(socket_obj &&o) {
		if (&o == this) return *this;
		close_socket(m_sock);
		m_sock = o.m_sock;
		o.m_sock = socket_invalid;
		return *this;
	}
	~socket_obj() {
		close_socket(m_sock);
	}

	operator socket_t() const {
		return m_sock;
	}
};
