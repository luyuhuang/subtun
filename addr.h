#pragma once

#include <string>
#include <cstddef>
#include <stdexcept>
#include <algorithm>

#include <arpa/inet.h>

class addr {
public:
	enum addr_type {
		ipv4, ipv6, unknow
	};

	virtual addr_type type() const = 0;
	virtual void copy_ip(void *dst) const = 0;
	virtual std::string to_string() const = 0;
	virtual std::string ip() const = 0;
	virtual uint16_t port() const = 0;
	virtual bool is_zero() const = 0;
	virtual void set_ip(const void *src) = 0;
	virtual void set_ip(const std::string &ipaddr) = 0;
	virtual void set_port(uint16_t port) = 0;

	virtual ~addr() {}

	static addr_type guess_type(const std::string &addr) {
		if (addr.size() < 7) return unknow;
		if (addr.front() == '[') return ipv6;
		if (addr.find('.') != addr.npos) return ipv4;
		return ipv6;
	}
};

class addr_ipv4 : public addr {
	uint16_t m_port;
	union {
		uint8_t c[4];
		uint32_t i;
	} m_ip;

	void setip(const std::string &ipaddr) {
		if (int ret = inet_pton(AF_INET, ipaddr.c_str(), m_ip.c); ret == 0) {
			throw std::runtime_error("`" + ipaddr + "' is an invalid ipv4 address");
		} else if (ret < 0) {
			throw std::logic_error("inet_pton returns -1");
		}
	}

public:
	addr_ipv4() : m_ip{}, m_port(0) {};
	explicit addr_ipv4(const std::string &addr) : m_port(0) {
		size_t c = addr.find(':');
		if (c == addr.npos) {
			throw std::runtime_error("addr `" + addr + "' does not contain a port");
		}

		auto i = addr.begin() + c;
		setip(std::string(addr.begin(), i));

		++i;
		while (i != addr.end()) {
			m_port *= 10;
			m_port += *i++ - '0';
		}
	}
	addr_ipv4(const std::string &ipaddr, uint16_t port) : m_port(port) {
		setip(ipaddr);
	}

	virtual addr_type type() const {
		return ipv4;
	}

	virtual void copy_ip(void *dst) const {
		std::copy(std::cbegin(m_ip.c), std::cend(m_ip.c), reinterpret_cast<uint8_t *>(dst));
	}

	virtual std::string to_string() const {
		return ip() + ':' + std::to_string(m_port);
	}

	virtual std::string ip() const {
		std::string ans;
		ans.resize(12);
		inet_ntop(AF_INET, m_ip.c, ans.data(), ans.size());
		return ans;
	}

	virtual uint16_t port() const {
		return m_port;
	}

	virtual bool is_zero() const {
		return m_ip.i == 0 && m_port == 0;
	}

	virtual void set_ip(const void *src) {
		const uint8_t *p = reinterpret_cast<const uint8_t *>(src);
		std::copy(p, p + sizeof(m_ip), m_ip.c);
	};

	virtual void set_ip(const std::string &ipaddr) {
		setip(ipaddr);
	};

	virtual void set_port(uint16_t port) {
		m_port = port;
	};
};

class addr_ipv6 : public addr {
	uint16_t m_port;
	union {
		uint8_t c[16];
		uint64_t l[2];
	} m_ip;

	void setip(const std::string &ipaddr) {
		if (int ret = inet_pton(AF_INET6, ipaddr.c_str(), m_ip.c); ret == 0) {
			throw std::runtime_error("`" + ipaddr + "' is an invalid ipv4 address");
		} else if (ret < 0) {
			throw std::logic_error("inet_pton returns -1");
		}
	}

public:
	addr_ipv6() : m_ip{}, m_port(0) {};
	explicit addr_ipv6(const std::string &addr) : m_port(0) {
		auto i = addr.begin();
		if (*i != '[') throw std::runtime_error("addr `" + addr + "' is not the correct format");
		auto j = ++i;
		while (j != addr.end() && *j != ']') ++j;
		if (j == addr.end()) throw std::runtime_error("addr `" + addr + "' is not the correct format");
		setip(std::string(i, j));
		
		++j;
		if (j == addr.end() || *j != ':') throw std::runtime_error("addr `" + addr + "' is not the correct format");
		++j;
		if (j == addr.end()) throw std::runtime_error("addr `" + addr + "' is not the correct format");
		
		while (j != addr.end()) {
			m_port *= 10;
			m_port += *j++ - '0';
		}
	}
	addr_ipv6(const std::string &ipaddr, uint16_t port) : m_port(port) {
		setip(ipaddr);
	}

	virtual addr_type type() const {
		return ipv6;
	}

	virtual void copy_ip(void *dst) const {
		std::copy(std::cbegin(m_ip.c), std::cend(m_ip.c), reinterpret_cast<uint8_t *>(dst));
	}

	virtual std::string to_string() const {
		return ip() + ':' + std::to_string(m_port);
	}

	virtual std::string ip() const {
		std::string ans;
		ans.resize(16);
		inet_ntop(AF_INET6, m_ip.c, ans.data(), ans.size());
		return ans;
	}

	virtual uint16_t port() const {
		return m_port;
	}

	virtual bool is_zero() const {
		return m_ip.l[0] == 0 && m_ip.l[1] == 0 && m_port == 0;
	}

	virtual void set_ip(const void *src) {
		const uint8_t *p = reinterpret_cast<const uint8_t *>(src);
		std::copy(p, p + sizeof(m_ip), m_ip.c);
	};

	virtual void set_ip(const std::string &ipaddr) {
		setip(ipaddr);
	};

	virtual void set_port(uint16_t port) {
		m_port = port;
	};
};
