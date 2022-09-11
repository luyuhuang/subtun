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
size_t receive_from_udp4(const socket_t &sock, void *buf, size_t len, addr_ipv4 &ad);
size_t receive_from_udp6(const socket_t &sock, void *buf, size_t len, addr_ipv6 &ad);
size_t receive_from_udp(const socket_t &sock, void *buf, size_t len);
size_t send_to_udp4(const socket_t &sock, const void *buf, size_t len, const addr_ipv4 &ad);
size_t send_to_udp6(const socket_t &sock, const void *buf, size_t len, const addr_ipv6 &ad);
void close_udp(socket_t &sock);

template <typename ADDR>
inline socket_t make_udp(const ADDR &ad) = delete;

template <> 
inline socket_t make_udp<addr_ipv4>(const addr_ipv4 &ad) {
	return make_udp4(ad);
};
template <> 
inline socket_t make_udp<addr_ipv6>(const addr_ipv6 &ad) {
	return make_udp6(ad);
};

template <typename ADDR>
inline size_t receive_from_udp(const socket_t &sock, void *buf, size_t len, ADDR &ad) = delete;

template <>
inline size_t receive_from_udp<addr_ipv4>(const socket_t &sock, void *buf, size_t len, addr_ipv4 &ad) {
	return receive_from_udp4(sock, buf, len, ad);
}
template <>
inline size_t receive_from_udp<addr_ipv6>(const socket_t &sock, void *buf, size_t len, addr_ipv6 &ad) {
	return receive_from_udp6(sock, buf, len, ad);
}

template <typename ADDR>
inline size_t send_to_udp(const socket_t &sock, const void *buf, size_t len, const ADDR &ad) = delete;

template <>
inline size_t send_to_udp<addr_ipv4>(const socket_t &sock, const void *buf, size_t len, const addr_ipv4 &ad) {
	return send_to_udp4(sock, buf, len, ad);
}
template <>
inline size_t send_to_udp<addr_ipv6>(const socket_t &sock, const void *buf, size_t len, const addr_ipv6 &ad) {
	return send_to_udp6(sock, buf, len, ad);
}