#include "ring_buffer.h"

#include <cstring>

using std::min;

ring_buffer::ring_buffer(size_t n) :
	m_base(new uint8_t[n + 1]), m_last(m_base + n + 1),
	m_front(m_base), m_back(m_base) {
}

ring_buffer::ring_buffer(const ring_buffer &b) :
	m_base(new uint8_t[b.m_last - b.m_base]), m_last(m_base + (b.m_last - b.m_base)),
	m_front(m_base + (b.m_front - b.m_base)), m_back(m_base + (b.m_back - b.m_base)) {
	memcpy(m_base, b.m_base, b.m_last - b.m_base);
}

ring_buffer::ring_buffer(ring_buffer &&b) :
	m_base(b.m_base), m_last(b.m_last), m_front(b.m_front), m_back(b.m_back) {
	b.m_base = b.m_last = b.m_front = b.m_back = nullptr;
}

ring_buffer &ring_buffer::operator=(const ring_buffer &b) {
	if (&b == this) return *this;
	free();
	m_base = new uint8_t[b.m_last - b.m_base];
	m_last = m_base + (b.m_last - b.m_base);
	m_front = m_base + (b.m_front - b.m_base);
	m_back = m_base + (b.m_back - b.m_base);
	memcpy(m_base, b.m_base, b.m_last - b.m_base);
	return *this;
}

ring_buffer &ring_buffer::operator=(ring_buffer &&b) {
	if (&b == this) return *this;
	free();
	m_base = b.m_base, m_last = b.m_last, m_front = b.m_front, m_back = b.m_back;
	b.m_base = b.m_last = b.m_front = b.m_back = nullptr;
	return *this;
}

size_t ring_buffer::size() const {
	return end() - begin();
}

size_t ring_buffer::capacity() const {
	return m_last - m_base - 1;
}

bool ring_buffer::empty() const {
	return m_front == m_back;
}

bool ring_buffer::full() const {
	uint8_t *next = m_back + 1;
	if (next == m_last) next = m_base;
	return next == m_front;
}

void ring_buffer::push_back(uint8_t v) {
	assert(!full());
	*m_back = v;
	if (++m_back == m_last)
		m_back = m_base;
}

void ring_buffer::pop_back() {
	assert(!empty());
	m_back = m_back == m_base ? m_last - 1 : m_back - 1;
}

void ring_buffer::push_front(uint8_t v) {
	assert(!full());
	m_front = m_front == m_base ? m_last - 1 : m_front - 1;
	*m_front = v;
}

void ring_buffer::pop_front() {
	assert(!empty());
	if (++m_front == m_last)
		m_front = m_base;
}

uint8_t &ring_buffer::front() {
	assert(!empty());
	return *m_front;
}
const uint8_t &ring_buffer::front() const {
	assert(!empty());
	return *m_front;
}

uint8_t &ring_buffer::back() {
	assert(!empty());
	uint8_t *prev = m_back == m_base ? m_last - 1 : m_back - 1;
	return *prev;
}
const uint8_t &ring_buffer::back() const {
	assert(!empty());
	uint8_t *prev = m_back == m_base ? m_last - 1 : m_back - 1;
	return *prev;
}

void ring_buffer::free() {
	if (!m_base) return;
	delete[] m_base;
}

void ring_buffer::append(const uint8_t *data, size_t len) {
	assert(capacity() - size() >= len);
	size_t m = m_last - m_back, n = min(len, m);
	memcpy(m_back, data, n);
	if ((m_back += n) == m_last)
		m_back = m_base;
	if (len -= n) {
		memcpy(m_back, data + n, len);
		m_back += len;
	}
}

void ring_buffer::poll(uint8_t *data, size_t len) {
	assert(size() >= len);
	size_t m = m_last - m_front, n = min(len, m);
	memcpy(data, m_front, n);
	if ((m_front += n) == m_last)
		m_front = m_base;
	if (len -= n) {
		memcpy(data + n, m_front, len);
		m_front += len;
	}
}

size_t ring_buffer::poll(const std::function<size_t(const void *buf, size_t len)> &f) {
	if (m_front == m_back) return 0;
	if (m_front < m_back) {
		size_t n = f(m_front, m_back - m_front);
		m_front += n;
		return n;
	} else { // m_front > m_back
		size_t n = f(m_front, m_last - m_front);
		if ((m_front += n) == m_last) {
			m_front = m_base;
			size_t m = f(m_front, m_back - m_front);
			m_front += m;
			n += m;
		}
		return n;
	}
}