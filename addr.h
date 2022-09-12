#pragma once

#include <string>
#include <cstddef>
#include <stdexcept>
#include <algorithm>

#include <arpa/inet.h>

class IPv4 {
	union {
		uint8_t c[4];
		uint32_t i;
	} m_data;

public:
	IPv4() : m_data{} {}
	explicit IPv4(const std::string &ip) {
		set(ip);
	}

	void set(const std::string &ip) {
		if (int ret = inet_pton(AF_INET, ip.c_str(), m_data.c); ret == 0) {
			throw std::runtime_error("`" + ip + "' is an invalid ipv4 address");
		} else if (ret < 0) {
			throw std::logic_error("inet_pton returns -1");
		}
	}

	void set(const void *src) {
		const uint8_t *p = reinterpret_cast<const uint8_t *>(src);
		std::copy(p, p + sizeof(m_data), m_data.c);
	}

	void copy_to(void *dst) const {
		std::copy(std::cbegin(m_data.c), std::cend(m_data.c), reinterpret_cast<uint8_t *>(dst));
	}

	std::string to_string() const {
		thread_local static char buf[16];
		return inet_ntop(AF_INET, m_data.c, buf, sizeof(buf) - 1);
	}

	bool is_zero() const {
		return m_data.i == 0;
	}

	struct less {
		bool operator()(const IPv4 &a, const IPv4 &b) const {
			return a.m_data.i < b.m_data.i;
		}
	};
};

class IPv6 {
	union {
		uint8_t c[16];
		uint64_t l[2];
	} m_data;

public:
	IPv6() : m_data{} {}
	explicit IPv6(const std::string &ip) {
		set(ip);
	}

	void set(const std::string &ip) {
		if (int ret = inet_pton(AF_INET6, ip.c_str(), m_data.c); ret == 0) {
			throw std::runtime_error("`" + ip + "' is an invalid ipv4 address");
		} else if (ret < 0) {
			throw std::logic_error("inet_pton returns -1");
		}
	}

	void set(const void *src) {
		const uint8_t *p = reinterpret_cast<const uint8_t *>(src);
		std::copy(p, p + sizeof(m_data), m_data.c);
	}

	void copy_to(void *dst) const {
		std::copy(std::cbegin(m_data.c), std::cend(m_data.c), reinterpret_cast<uint8_t *>(dst));
	}

	std::string to_string() const {
		thread_local static char buf[64];
		return inet_ntop(AF_INET6, m_data.c, buf, sizeof(buf) - 1);
	}

	bool is_zero() const {
		return m_data.l[0] == 0 && m_data.l[1] == 0;
	}

	struct less {
		bool operator()(const IPv6 &a, const IPv6 &b) const {
			if (a.m_data.l[0] != b.m_data.l[0]) return a.m_data.l[0] < b.m_data.l[0];
			return a.m_data.l[1] < b.m_data.l[1];
		}
	};
};

template <typename IP_T> 
inline uint16_t parse_addr(const std::string &saddr, std::string &ipaddr) = delete;

template <>
inline uint16_t parse_addr<IPv4>(const std::string &saddr, std::string &ipaddr) {
	size_t c = saddr.find(':');
	if (c == saddr.npos) {
		throw std::runtime_error("addr `" + saddr + "' does not contain a port");
	}

	auto i = saddr.begin() + c;
	ipaddr.assign(saddr.begin(), i);

	++i;
	if (i == saddr.end()) throw std::runtime_error("addr `" + saddr + "' is not the correct format");

	uint16_t port = 0;
	while (i != saddr.end()) {
		port *= 10;
		port += *i++ - '0';
	}
	return port;
}

template <>
inline uint16_t parse_addr<IPv6>(const std::string &saddr, std::string &ipaddr) {
	auto i = saddr.begin();
	if (*i != '[') throw std::runtime_error("addr `" + saddr + "' is not the correct format");
	auto j = ++i;
	while (j != saddr.end() && *j != ']') ++j;
	if (j == saddr.end()) throw std::runtime_error("addr `" + saddr + "' is not the correct format");
	ipaddr.assign(i, j);

	++j;
	if (j == saddr.end() || *j != ':') throw std::runtime_error("addr `" + saddr + "' is not the correct format");
	++j;
	if (j == saddr.end()) throw std::runtime_error("addr `" + saddr + "' is not the correct format");

	uint16_t port = 0;
	while (j != saddr.end()) {
		port *= 10;
		port += *j++ - '0';
	}
}


template <typename IP_T>
class address {
	IP_T m_ip;
	uint16_t m_port;

public:
	using ip_type = IP_T;

	address() : m_ip{}, m_port(0) {};
	explicit address(const std::string &saddr) {
		std::string ipaddr;
		m_port = parse_addr<IP_T>(saddr, ipaddr);
		m_ip.set(ipaddr);
	}
	
	address(const std::string &ipaddr, uint16_t port) : m_ip(ipaddr), m_port(port) {}

	void copy_ip(void *dst) const {
		return m_ip.copy_to(dst);
	}

	std::string to_string() const {
		return ip() + ':' + std::to_string(m_port);
	}

	std::string ip() const {
		return m_ip.to_string();
	}

	uint16_t port() const {
		return m_port;
	}

	bool is_zero() const {
		return m_ip.is_zero() && m_port == 0;
	}

	void set_ip(const void *src) {
		return m_ip.set(src);
	};

	void set_ip(const std::string &ipaddr) {
		return m_ip.set(ipaddr);
	};

	void set_port(uint16_t port) {
		m_port = port;
	};
};

using addr_ipv4 = address<IPv4>;
using addr_ipv6 = address<IPv6>;

enum class addr_type {
	ipv4, ipv6, unknow
};

inline addr_type guess_addr_type(const std::string &addr) {
	if (addr.size() < 7) return addr_type::unknow;
	if (addr.front() == '[') return addr_type::ipv6;
	if (addr.find('.') != addr.npos) return addr_type::ipv4;
	return addr_type::ipv6;
}
