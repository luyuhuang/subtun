#pragma once

#include <stdexcept>
#include <cstring>
#include <string>

#include "addr.h"

template <typename IP_T>
inline auto parse_src_ip(uint8_t *data) -> IP_T = delete;

template <>
inline auto parse_src_ip<IPv4>(uint8_t *data) -> IPv4 {
	uint8_t version;
	memcpy(&version, data, sizeof(uint8_t));
	version >>= 4;
	if (version != 4u) throw std::runtime_error("it does not seem to be an ipv4 packet");
	IPv4 ans;
	ans.set(data + 12);
	return ans;
}
template <>
inline auto parse_src_ip<IPv6>(uint8_t *data) -> IPv6 {
	uint8_t version;
	memcpy(&version, data, sizeof(uint8_t));
	version >>= 4;
	if (version != 6u) throw std::runtime_error("it does not seem to be an ipv6 packet");
	IPv6 ans;
	ans.set(data + 8);
	return ans;
}

template <typename IP_T>
inline auto parse_dst_ip(uint8_t *data)->IP_T = delete;

template <>
inline auto parse_dst_ip<IPv4>(uint8_t *data) -> IPv4 {
	uint8_t version;
	memcpy(&version, data, sizeof(uint8_t));
	version >>= 4;
	if (version != 4u) throw std::runtime_error("it does not seem to be an ipv4 packet");
	IPv4 ans;
	ans.set(data + 16);
	return ans;
}
template <>
inline auto parse_dst_ip<IPv6>(uint8_t *data) -> IPv6 {
	uint8_t version;
	memcpy(&version, data, sizeof(uint8_t));
	version >>= 4;
	if (version != 6u) throw std::runtime_error("it does not seem to be an ipv6 packet");
	IPv6 ans;
	ans.set(data + 24);
	return ans;
}

template <typename T, typename CT, CT *close, T invalid = 0>
class movable_fd {
	T m_fd;
public:
	movable_fd() : m_fd(invalid) {}
	movable_fd(const T &fd) : m_fd(fd) {}
	movable_fd(const movable_fd &) = delete;
	movable_fd &operator=(const movable_fd &) = delete;

	movable_fd(movable_fd &&o) : m_fd(o.m_fd) {
		o.m_fd = invalid;
	}
	movable_fd &operator=(movable_fd &&o) {
		if (&o == this) return *this;
		close(m_fd);
		m_fd = o.m_fd;
		o.m_fd = invalid;
		return *this;
	}
	~movable_fd() {
		close(m_fd);
	}

	operator T() const {
		return m_fd;
	}
};
