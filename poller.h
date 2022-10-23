#pragma once

#include <vector>

#if defined(__linux__)
	#include "linux/epoll.h"
#endif

#include "utils.h"
#include "socket.h"

template <typename Impl, typename Conn, bool (*on_writable)(Conn&)>
class poller : private Impl {
public:
	template <typename ...Args>
	void add(socket_t sock, Args && ...args) {
		Impl::add(sock, std::forward<Args>(args)...);
		Impl::set(sock, Impl::EV_READ);
	}

	void del(socket_t sock) {
		Impl::del(sock);
	}

	void wait_write(socket_t sock) {
		Impl::set(sock, Impl::EV_WRITE);
	}

	void wait(int timeout, const std::function<void(socket_t, Conn&)> &f) {
		Impl::wait(timeout, [this, &f](socket_t sock, uint32_t events, Conn &conn) {
			if (events & Impl::EV_WRITE) {
				if (!on_writable(conn)) {
					Impl::unset(sock, Impl::EV_WRITE);
				}
			}
			if (events & Impl::EV_READ) {
				f(sock, conn);
			}
		});
	}
};

#if defined(__linux__)
	template <typename Conn, bool (*on_writable)(Conn&)>
	using event_poller = poller<epoll<Conn>, Conn, on_writable>;
#endif