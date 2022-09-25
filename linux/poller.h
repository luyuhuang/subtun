#pragma once

#include <vector>
#include <functional>
#include <stdexcept>

#include <sys/epoll.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include "../socket.h"


template <typename Protocol>
class poller_epoll;

template <typename Protocol>
class looper_epoll;

template <typename Protocol>
class event_epoll {
	Protocol m_protocol;
	int m_efd;

	friend class poller_epoll<Protocol>;
	friend class looper_epoll<Protocol>;

	event_epoll(int efd, const Protocol &p) : m_efd(efd), m_protocol(p) {}
	event_epoll(int efd, Protocol &&p) : m_efd(efd), m_protocol(std::move(p)) {}
	event_epoll(const event_epoll &) = delete;
	event_epoll &operator=(const event_epoll &) = delete;
	~event_epoll() {}
};

template <typename Protocol>
class looper_epoll {
	int m_efd;
	std::vector<struct epoll_event> m_events;

	friend class poller_epoll<Protocol>;

	looper_epoll(int efd) : m_efd(efd), m_events(512) {}
public:
	looper_epoll(const looper_epoll &) = delete;
	looper_epoll &operator=(const looper_epoll &) = delete;
	looper_epoll(looper_epoll &&l) = default;
	looper_epoll &operator=(looper_epoll &&l) = default;

	void update(int timeout, const std::function<void(event_epoll<Protocol> *)> &cb) {
		int n = epoll_wait(m_efd, m_events.data(), m_events.size(), timeout);
		m_events.resize(n);
		for (auto &event : m_events) {
			event_epoll<Protocol> *esock = static_cast<event_epoll<Protocol> *>(event.data.ptr);
			if ((event.events & EPOLLOUT)) {
				esock->m_protocol.flush();
			}
			if (event.events & EPOLLIN) {
				cb(esock);
			}
		}
	}
};

template <typename Protocol>
class poller_epoll {
	std::vector<int> m_efds;
public:
	poller_epoll() {}
	poller_epoll(const poller_epoll &) = delete;
	poller_epoll &operator=(const poller_epoll &) = delete;

	poller_epoll(poller_epoll &&l) : m_efds(std::move(l.m_efds)) {
		l.m_efds.clear();
	}
	poller_epoll &operator=(poller_epoll &&l) {
		if (&l == this) return *this;
		for (int efd : m_efds) close(efd);
		m_efds = std::move(l.m_efds);
		l.m_efds.clear();
		return *this;
	}
	~poller_epoll() {
		for (int efd : m_efds) close(efd);
	}

	looper_epoll<Protocol> create_looper() {
		int efd = epoll_create1(0);
		m_efds.push_back(efd);
		return looper_epoll<Protocol>(efd);
	}

	template <typename T>
	event_epoll<Protocol> *add(T &&p) {
		socket_t sock = p.get_socket();
		int flags = fcntl(sock, F_SETFL, 0);
		if (flags < 0 || fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0)
			throw std::runtime_error("fail to set socket nonblock");

		int efd = m_efds[rand() % m_efds.size()];
		auto *ev = new event_epoll<Protocol>(efd, std::forward<T>(p));
		struct epoll_event event {};
		ev.events = EPOLLIN | EPOLLOUT | EPOLLERR;
		ev.data.ptr = ev;
		epoll_ctl(efd, EPOLL_CTL_ADD, sock, &ev);
		return ev;
	}

	socket_t del(event_epoll<Protocol> *ev) const {
		socket_t sock = ev->m_protocol.get_socket();
		epoll_ctl(ev->m_efd, EPOLL_CTL_DEL, sock, nullptr);
		delete ev;
		return sock;
	}
};
