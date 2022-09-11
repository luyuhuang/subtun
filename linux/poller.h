#pragma once

#include <vector>
#include <functional>

#include <sys/epoll.h>
#include "../socket.h"

class poller_epoll;
class socket_epoll {
	socket_t m_fd;
	std::vector<uint8_t> write_buf;

	friend class poller_epoll;

	explicit socket_epoll(socket_t fd) : m_fd(fd) {}
	~socket_epoll() {}
public:
	socket_epoll(const socket_epoll &) = delete;
	socket_epoll &operator=(const socket_epoll &) = delete;

	size_t recv(void *buf, size_t len) const {
		// read until eagain
		return 0;
	}
	size_t send(const void *buf, size_t len) {
		// write, if get an eagain, save to write_buf
		return 0;
	}
};

class poller_epoll {
	int m_efd;
	std::vector<struct epoll_event> m_events;
public:
	poller_epoll() : m_efd(epoll_create1(0)), m_events(512) {}
	poller_epoll(const poller_epoll &) = delete;
	poller_epoll &operator=(const poller_epoll &) = delete;

	socket_epoll *add(socket_t &sock) const {
		socket_epoll *esock = new socket_epoll(sock);
		struct epoll_event event {};
		event.events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLET;
		event.data.ptr = esock;
		epoll_ctl(m_efd, EPOLL_CTL_ADD, sock, &event);
		return esock;
	}

	socket_t del(socket_epoll *esock) const {
		epoll_ctl(m_efd, EPOLL_CTL_DEL, esock->m_fd, nullptr);
		socket_t ans = esock->m_fd;
		delete esock;
		return ans;
	}

	void poll(const std::function<void(socket_epoll*)> &cb) {
		int n = epoll_wait(m_efd, m_events.data(), m_events.size(), 1);
		m_events.resize(n);
		for (auto &event : m_events) {
			socket_epoll *esock = reinterpret_cast<socket_epoll *>(event.data.ptr);
			if ((event.events & EPOLLOUT) && !esock->write_buf.empty()) {
				// TODO: write
			}
			if (event.events & EPOLLIN) {
				cb(esock);
			}
		}
	}
};
