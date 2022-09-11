#pragma once

#include <string>
#include <memory>

#include "socket.h"
#include "addr.h"

template <typename Addr, typename Encrypt>
class sudp : private Encrypt {
	socket_t m_sock;
public:
	sudp() : m_sock(make_udp(Addr())) {}
	explicit sudp(const Addr &ad) : m_sock(make_udp<Addr>(ad)) {}
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

	size_t sendto(const void *buf, size_t len, const Addr &ad) {
		std::unique_ptr<uint8_t[]> cipher_buf(new uint8_t[len + Encrypt::min_cap]);
		uint8_t key[] = "12345612345678901";
		size_t n = Encrypt::encrypt(key, static_cast<const uint8_t*>(buf), len, cipher_buf.get(), len + Encrypt::min_cap);
		return send_to_udp<Addr>(m_sock, cipher_buf.get(), n, ad);
	}

	size_t recvfrom(void *buf, size_t len, Addr &ad) {
		std::unique_ptr<uint8_t[]> cipher_buf(new uint8_t[len + Encrypt::min_cap]);
		uint8_t key[] = "12345612345678901";
		size_t n = Encrypt::decrypt(key, static_cast<const uint8_t *>(buf), len, cipher_buf.get(), len + Encrypt::min_cap);
		return receive_from_udp<Addr>(m_sock, cipher_buf.get(), n, ad);
	}
	size_t recvfrom(void *buf, size_t len) const {
		return receive_from_udp(m_sock, buf, len);
	}
};

template <typename Encrypt> using sudp4 = sudp<addr_ipv4, Encrypt>;
template <typename Encrypt> using sudp6 = sudp<addr_ipv6, Encrypt>;
