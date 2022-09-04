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

socket_t make_udp(const addr &ad);
size_t receive_from_udp(const socket_t &sock, void *buf, size_t len, addr &ad);
size_t receive_from_udp(const socket_t &sock, void *buf, size_t len);
size_t send_to_udp(const socket_t &sock, const void *buf, size_t len, const addr &ad);
void close_udp(socket_t &sock);

socket_t make_tcp(addr &ad);
