#pragma once

#include <functional>
#include <vector>
#include <algorithm>
#include <stdexcept>

#include <sys/epoll.h>
#include <unistd.h>

#include "../socket.h"
#include "../utils.h"

template <typename Data>
class epoll {
	struct entry {
		template <typename ...Args>
		entry(socket_t s, Args && ...args) : fd(s), data(std::forward<Args>(args)...) {}

		socket_t fd;
		uint32_t events = 0;
		Data data;
	};

	using efd_t = movable_fd<int, decltype(close), close, -1>;
	efd_t m_efd;
	std::vector<struct epoll_event> m_events;
	std::vector<entry*> m_fd_data;

public:
	enum event_types {
		EV_READ = EPOLLIN,
		EV_WRITE = EPOLLOUT,
	};

	epoll();
	epoll(const epoll &) = delete;
	epoll &operator=(const epoll &) = delete;
	epoll(epoll &&) = default;
	epoll &operator=(epoll &&);
	~epoll();

	template <typename ...Args>
	void add(socket_t sock, Args && ...args);

	void set(socket_t sock, event_types event);
	void unset(socket_t sock, event_types event);
	void del(socket_t sock);
	void wait(int timeout, const std::function<void(socket_t, uint32_t, Data&)> &f);
};

template <typename Data>
epoll<Data>::epoll() : m_efd(epoll_create1(0)), m_events(64) {
	if (m_efd <= 0) {
		throw std::runtime_error("fail to create an epoll fd");
	}
}

template <typename Data>
epoll<Data> &epoll<Data>::operator=(epoll &&e) {
	if (&e == this) return *this;
	for (entry *&e : m_fd_data) {
		if (e) delete e;
	}
	m_efd = e.m_efd;
	m_events = std::move(e.m_events);
	m_fd_data = std::move(e.m_fd_data);
}

template <typename Data>
epoll<Data>::~epoll() {
	for (entry *&e : m_fd_data) {
		if (e) delete e;
	}
}

template <typename Data>
template <typename ...Args>
void epoll<Data>::add(socket_t sock, Args && ...args) {
	if (sock >= m_fd_data.size())
		m_fd_data.resize(std::max(m_fd_data.size(), static_cast<size_t>(sock + 1)));

	entry *e = m_fd_data[sock];
	if (e) return;
	m_fd_data[sock] = e = new entry(sock, std::forward<Args>(args)...);

	struct epoll_event ev;
	ev.data.ptr = e;
	ev.events = e->events = EPOLLERR;
	epoll_ctl(m_efd, EPOLL_CTL_ADD, sock, &ev);
}

template <typename Data>
void epoll<Data>::set(socket_t sock, event_types event) {
	entry *e = m_fd_data[sock];
	if (!e)
		throw std::logic_error("fd is not in epoll");
	if (e->events & event) return;

	struct epoll_event ev;
	ev.data.ptr = e;
	ev.events = e->events |= event;
	epoll_ctl(m_efd, EPOLL_CTL_MOD, sock, &ev);
}

template <typename Data>
void epoll<Data>::unset(socket_t sock, event_types event) {
	entry *e = m_fd_data[sock];
	if (!e)
		throw std::logic_error("fd is not in epoll");
	if (!(e->events & event)) return;

	struct epoll_event ev;
	ev.data.ptr = e;
	ev.events = e->events &= ~static_cast<uint32_t>(event);
	epoll_ctl(m_efd, EPOLL_CTL_MOD, sock, &ev);
}

template <typename Data>
void epoll<Data>::del(socket_t sock) {
	entry *e = m_fd_data[sock];
	if (!e)
		throw std::logic_error("fd is not in epoll");

	delete e;
	m_fd_data[sock] = nullptr;
	epoll_ctl(m_efd, EPOLL_CTL_DEL, sock, nullptr);
}

template <typename Data>
void epoll<Data>::wait(int timeout, const std::function<void(socket_t, uint32_t, Data&)> &f) {
	int n = epoll_wait(m_efd, m_events.data(), m_events.size(), timeout);
	if (n < 0) {
		throw std::runtime_error("epoll_wait fails");
	}
	for (int i = 0; i < n; ++i) {
		entry *e = static_cast<entry*>(m_events[i].data.ptr);
		f(e->fd, e->data, m_events[i].events);
	}
}
