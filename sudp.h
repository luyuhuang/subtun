#pragma once

#include <string>

#include "socket.h"
#include "addr.h"

template <typename ADDR>
class sudp {
	socket_t m_sock;
public:
	sudp() : m_sock(make_udp(ADDR())) {}
	explicit sudp(const ADDR &ad) : m_sock(make_udp(ad)) {}
	sudp(const sudp &) = delete;
	sudp &operator=(const sudp &) = delete;
	
	sudp(sudp &&u) : m_sock(u.m_sock) {
		u.m_sock = socket_invalid;
	}
	sudp &operator=(sudp &&u) {
		if (&u == this) return *this;
		close_udp(m_sock);
		m_sock = u.m_sock;
		u.m_sock = socket_invalid;
		return *this;
	}
	~sudp() {
		close_udp(m_sock);
	}

	size_t sendto(const void *buf, size_t len, const ADDR &ad) const {
		return send_to_udp(m_sock, buf, len, ad);
	}

	size_t recvfrom(void *buf, size_t len, ADDR &ad) const {
		return receive_from_udp(m_sock, buf, len, ad);
	}
	size_t recvfrom(void *buf, size_t len) const {
		return receive_from_udp(m_sock, buf, len);
	}
};

using sudp4 = sudp<addr_ipv4>;
using sudp6 = sudp<addr_ipv6>;
