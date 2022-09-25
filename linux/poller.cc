#include "poller.h"
//
//#include <stdexcept>
//
//#include <unistd.h>
//#include <errno.h>
//#include <fcntl.h>
//
//#include <deque>
//
//using std::runtime_error;

//size_t socket_epoll::recv(void *buf, size_t len) const {
//	ssize_t n = ::recv(m_fd, buf, len, 0);
//	if (n < 0) {
//		if (errno == EAGAIN || errno == EWOULDBLOCK)
//			return 0;
//		throw runtime_error("fail to receive from socket");
//	} else if (n == 0) {
//		throw runtime_error("disconnected");
//	}
//	return n;
//}
//
//size_t socket_epoll::send(const void *buf, size_t len) {
//	ssize_t n = ::send(m_fd, buf, len, 0);
//	if (n < 0) {
//		if (errno == EAGAIN || errno == EWOULDBLOCK) {
//			// TODO write to the buffer
//		}
//		throw runtime_error("fail to send to socket");
//	}
//	return 0;
//}
//
//void looper_epoll::update(int timeout, const std::function<void(socket_epoll *)> &cb) {
//	int n = epoll_wait(m_efd, m_events.data(), m_events.size(), timeout);
//	m_events.resize(n);
//	for (auto &event : m_events) {
//		socket_epoll *esock = static_cast<socket_epoll *>(event.data.ptr);
//		if ((event.events & EPOLLOUT) && !esock->write_buf.empty()) {
//			// TODO: write
//		}
//		if (event.events & EPOLLIN) {
//			cb(esock);
//		}
//	}
//}

//poller_epoll::poller_epoll(poller_epoll &&l) : m_efds(std::move(l.m_efds)) {
//	l.m_efds.clear();
//}
//
//poller_epoll &poller_epoll::operator=(poller_epoll &&l) {
//	if (&l == this) return *this;
//	for (int efd : m_efds) close(efd);
//	m_efds = std::move(l.m_efds);
//	l.m_efds.clear();
//	return *this;
//}
//
//poller_epoll::~poller_epoll() {
//	for (int efd : m_efds) close(efd);
//}
//
//looper_epoll poller_epoll::create_looper() {
//	int efd = epoll_create1(0);
//	m_efds.push_back(efd);
//	return looper_epoll(efd);
//}
//
//socket_epoll *poller_epoll::add(socket_t &sock) const {
//	int flags = fcntl(sock, F_SETFL, 0);
//	if (flags < 0 || fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0)
//		throw runtime_error("fail to set socket nonblock");
//
//	int efd = m_efds[rand() % m_efds.size()];
//
//	socket_epoll *esock = new socket_epoll(sock, efd);
//	struct epoll_event event {};
//	event.events = EPOLLIN | EPOLLOUT | EPOLLERR;
//	event.data.ptr = esock;
//	epoll_ctl(efd, EPOLL_CTL_ADD, sock, &event);
//	return esock;
//}
//
//socket_t poller_epoll::del(socket_epoll *esock) const {
//	epoll_ctl(esock->m_efd, EPOLL_CTL_DEL, esock->m_fd, nullptr);
//	socket_t fd = esock->m_fd;
//	delete esock;
//	return fd;
//}
