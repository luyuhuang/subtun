#pragma once

#include "socket.h"
#include "addr.h"
#include "ring_buffer.h"
#include "cipher.h"
#include "poller.h"

#include <iterator>
#include <algorithm>
#include <memory>
#include <functional>

template <typename Addr>
class tcp_listener;

template <typename Addr>
class tcp_conn {
	static constexpr size_t buffer_cap = 4096;

	socket_obj m_sock;
	Addr m_addr;
	ring_buffer m_write_buffer{ buffer_cap };

	friend class tcp_listener<Addr>;

	tcp_conn() = default;

	void write_to_buffer(const uint8_t *buf, size_t len) {
		if (buffer_cap - m_write_buffer.size() < len)
			throw std::runtime_error("write buffer overflow");
		m_write_buffer.append(buf, len);
	}

	void write(const uint8_t *buf, size_t len) {
		if (!m_write_buffer.empty()) {
			write_to_buffer(buf, len);
		} else {
			size_t n = send_socket(m_sock, buf, len);
			if (len -= n) {
				write_to_buffer(buf + n, len);
			}
		}
	}
public:
	tcp_conn(const tcp_conn &) = delete;
	tcp_conn &operator=(const tcp_conn &) = delete;
	tcp_conn(tcp_conn &&u) = default;
	tcp_conn &operator=(tcp_conn &&u) = default;

	socket_t get_socket() const {
		return m_sock;
	}

	size_t send(const void *buf, size_t len) {
		return write(buf, len), len;
	}

	size_t recv(void *buf, size_t len) {
		return receive_socket(m_sock, buf, len);
	}

	bool on_writable() {
		m_write_buffer.poll([this](const void *buf, size_t len) {
			return send_socket(m_sock, buf, len);
		});
		return need_to_wait_write();
	}

	bool need_to_wait_write() const {
		return !m_write_buffer.empty();
	}
};

using tcp4_conn = tcp_conn<addr_ipv4>;
using tcp6_conn = tcp_conn<addr_ipv6>;

template <typename Addr>
class tcp_listener {
	socket_obj m_sock;
public:
	explicit tcp_listener(const Addr &ad) : m_sock(make_tcp<Addr>(ad)) {}
	tcp_listener(const tcp_listener &) = delete;
	tcp_listener &operator=(const tcp_listener &) = delete;
	tcp_listener(tcp_listener &&u) = default;
	tcp_listener &operator=(tcp_listener &&u) = default;

	void listen() {
		listen_tcp(m_sock);
	}

	tcp_conn<Addr> accept() {
		tcp_conn<Addr> conn;
		conn.m_sock = accept_tcp<Addr>(m_sock, conn.m_addr);
		return conn;
	}
};

using tcp4_listener = tcp_listener<addr_ipv4>;
using tcp6_listener = tcp_listener<addr_ipv6>;

template <typename Addr, typename Encrypt>
class stcp_conn : public tcp_conn<Addr>, private Encrypt {
	static constexpr size_t buffer_cap = 4096;
	static constexpr size_t head_size = 2 + Encrypt::tag_size;
	static_assert(Encrypt::padding_size == 0);

	ring_buffer m_read_buffer{ buffer_cap };
	size_t m_body_size = 0;
	bool m_send_flag = false, m_recv_flag = false;

	bool read(uint8_t *buf, size_t len) {
		size_t m = m_read_buffer.size(), n = std::min(len, m);
		if (len -= n) {
			size_t k = tcp_conn<Addr>::recv(buf + n, len);
			if (k < len) {
				m_read_buffer.append(buf + n, k);
				return false;
			}
		} else {
			m_read_buffer.poll(buf, n);
		}
		return true;
	}

public:
	// stcp_conn(const tcp_conn<Addr> &c, const uint8_t *key) : tcp_conn<Addr>(c) {
	// 	Encrypt::init(key, nullptr, nullptr);
	// }
	stcp_conn(tcp_conn<Addr> &&c, const uint8_t *key) : tcp_conn<Addr>(std::move(c)) {
		Encrypt::init(key, nullptr, nullptr);
	}

	size_t send(const void *buf, size_t len) {
		if (len > 0x3FFF)
			throw std::range_error("send length is up to 0x3FFF");

		if (!m_send_flag) {
			uint8_t iv[Encrypt::iv_size];
			RAND_bytes(iv, sizeof(iv));
			Encrypt::init(nullptr, iv, nullptr);
			tcp_conn<Addr>::send(iv, sizeof(iv));
			m_send_flag = true;
		}

		uint16_t l = htons(static_cast<uint16_t>(len));
		uint8_t head[head_size] = {};
		size_t n = Encrypt::encrypt(reinterpret_cast<const uint8_t*>(&l), sizeof(l), head, sizeof(head));
		assert(n == sizeof(head));
		tcp_conn<Addr>::send(head, n);

		std::unique_ptr<uint8_t[]> cipher_buf(new uint8_t[len + Encrypt::tag_size]);
		n = Encrypt::encrypt(static_cast<const uint8_t *>(buf), len, cipher_buf.get(), len + Encrypt::tag_size);
		assert(n <= len + Encrypt::tag_size);
		tcp_conn<Addr>::send(cipher_buf.get(), n);

		return len;
	}

	size_t recv(void *buf, size_t len) {
		if (!m_recv_flag) {
			uint8_t iv[Encrypt::iv_size];
			if (!read(iv, sizeof(iv)))
				return 0;
			Encrypt::init(nullptr, nullptr, iv);
			m_recv_flag = true;
		}

		if (!m_body_size) {
			uint8_t head[head_size];
			if (!read(head, head_size))
				return 0;

			uint16_t l;
			size_t n = Encrypt::decrypt(head, head_size, reinterpret_cast<uint8_t *>(&l), sizeof(l));
			assert(n == sizeof(l));
			m_body_size = static_cast<size_t>(ntohs(l));
		}

		size_t size = m_body_size + Encrypt::tag_size;
		std::unique_ptr<uint8_t[]> cipher_buf(new uint8_t[size]);
		if (!read(cipher_buf.get(), size))
			return 0;

		m_body_size = 0;
		return Encrypt::decrypt(cipher_buf.get(), size, static_cast<uint8_t *>(buf), len);
	}
};

template <typename Encrypt> using stcp4_conn = stcp_conn<addr_ipv4, Encrypt>;
template <typename Encrypt> using stcp6_conn = stcp_conn<addr_ipv6, Encrypt>;
