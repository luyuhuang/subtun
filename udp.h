#pragma once

#include <string>
#include <memory>

#include "socket.h"
#include "addr.h"

template <typename Addr>
class udp {
	socket_t m_sock;
public:
	udp() : m_sock(make_udp(Addr())) {}
	explicit udp(const Addr &ad) : m_sock(make_udp<Addr>(ad)) {}
	udp(const udp &) = delete;
	udp &operator=(const udp &) = delete;

	udp(udp &&u) : m_sock(u.m_sock) {
		u.m_sock = socket_invalid;
	}
	udp &operator=(udp &&u) {
		if (&u == this) return *this;
		close_udp(m_sock);
		m_sock = u.m_sock;
		u.m_sock = socket_invalid;
		return *this;
	}
	~udp() {
		close_udp(m_sock);
	}

	void connect(const Addr &ad) {
		connect_socket<Addr>(m_sock, ad);
	}

	size_t sendto(const void *buf, size_t len, const Addr &ad) {
		return send_to_udp<Addr>(m_sock, buf, len, ad);
	}

	size_t recvfrom(void *buf, size_t len, Addr &ad) {
		return receive_from_udp<Addr>(m_sock, buf, len, ad);
	}
	size_t recvfrom(void *buf, size_t len) {
		return receive_from_udp(m_sock, buf, len);
	}
};

template <typename Addr, typename Encrypt>
class sudp : public udp<Addr>, private Encrypt {
public:
	sudp() {}
	explicit sudp(const Addr &ad) : udp<Addr>(ad) {}

	size_t sendto(const void *buf, size_t len, const Addr &ad) {
		std::unique_ptr<uint8_t[]> cipher_buf(new uint8_t[len + Encrypt::min_cap]);
		uint8_t key[] = "12345612345678901234561234567890";
		size_t n = Encrypt::encrypt(key, static_cast<const uint8_t*>(buf), len, cipher_buf.get(), len + Encrypt::min_cap);
		return udp<Addr>::sendto(cipher_buf.get(), n, ad);
	}

	size_t recvfrom(void *buf, size_t len, Addr &ad) {
		std::unique_ptr<uint8_t[]> cipher_buf(new uint8_t[len + Encrypt::min_cap]);
		uint8_t key[] = "12345612345678901234561234567890";
		size_t n = udp<Addr>::recvfrom(cipher_buf.get(), len - Encrypt::min_cap, ad);
		return Encrypt::decrypt(key, cipher_buf.get(), n, static_cast<uint8_t *>(buf), len);
	}
	size_t recvfrom(void *buf, size_t len) {
		std::unique_ptr<uint8_t[]> cipher_buf(new uint8_t[len + Encrypt::min_cap]);
		uint8_t key[] = "12345612345678901234561234567890";
		size_t n = udp<Addr>::recvfrom(cipher_buf.get(), len - Encrypt::min_cap);
		return Encrypt::decrypt(key, cipher_buf.get(), n, static_cast<uint8_t *>(buf), len);
	}
};

using udp4 = udp<addr_ipv4>;
using udp6 = udp<addr_ipv6>;

template <typename Encrypt> using sudp4 = sudp<addr_ipv4, Encrypt>;
template <typename Encrypt> using sudp6 = sudp<addr_ipv6, Encrypt>;
