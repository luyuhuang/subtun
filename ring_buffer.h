#pragma once

#include <algorithm>
#include <cassert>
#include <functional>

class ring_buffer;

template <typename T>
class ring_buffer_iterator;

template <typename T1, typename T2>
ptrdiff_t operator-(const ring_buffer_iterator<T1> &a, const ring_buffer_iterator<T2> &b);

template <typename T>
class ring_buffer_iterator {
	const ring_buffer *m_buff;
	T *m_curr;

	friend class ring_buffer;

	ring_buffer_iterator(const ring_buffer *c, T *i) : m_buff(c), m_curr(i) {}

public:
	ring_buffer_iterator(const ring_buffer_iterator &) = default;
	ring_buffer_iterator(ring_buffer_iterator &&) = default;

	ring_buffer_iterator &operator=(const ring_buffer_iterator &) = default;
	ring_buffer_iterator &operator=(ring_buffer_iterator &&) = default;

	T &operator*() const {
		return *m_curr;
	}
	T *operator->() const {
		return m_curr;
	}

	ring_buffer_iterator &operator++();
	ring_buffer_iterator operator++(int);

	ring_buffer_iterator &operator--();
	ring_buffer_iterator operator--(int);

	ring_buffer_iterator &operator+=(ptrdiff_t n);
	ring_buffer_iterator &operator-=(ptrdiff_t n);

	ring_buffer_iterator operator+(ptrdiff_t n);
	ring_buffer_iterator operator-(ptrdiff_t n);

	T *base() const;
	ptrdiff_t index() const;
	
private:
	void move(ptrdiff_t n);
};

class ring_buffer {
	uint8_t *m_base, *m_last;
	uint8_t *m_front, *m_back;

	friend ring_buffer_iterator<uint8_t>;
	friend ring_buffer_iterator<const uint8_t>;

public:
	using value_type = uint8_t;
	using size_type = size_t;
	using iterator = ring_buffer_iterator<uint8_t>;
	using const_iterator = ring_buffer_iterator<const uint8_t>;

	explicit ring_buffer(size_t n);
	ring_buffer(const ring_buffer &b);
	ring_buffer(ring_buffer &&b);
	ring_buffer &operator=(const ring_buffer &b);
	ring_buffer &operator=(ring_buffer &&b);
	~ring_buffer() {
		free();
	}

	iterator begin() {
		return iterator(this, m_front);
	}
	iterator end() {
		return iterator(this, m_back);
	}

	const_iterator begin() const {
		return const_iterator(this, m_front);
	}
	const_iterator end() const {
		return const_iterator(this, m_back);
	}

	const_iterator cbegin() const {
		return const_iterator(this, m_front);
	}
	const_iterator cend() const {
		return const_iterator(this, m_back);
	}

	size_t size() const;
	size_t capacity() const;
	bool empty() const;
	bool full() const;

	void push_back(uint8_t v);
	void pop_back();
	void push_front(uint8_t v);
	void pop_front();

	uint8_t &front();
	const uint8_t &front() const;
	uint8_t &back();
	const uint8_t &back() const;

	void append(const uint8_t *data, size_t len);
	void poll(uint8_t *data, size_t len);
	size_t poll(const std::function<size_t(const void *buf, size_t len)> &f);

private:
	void free();
};

template <typename T>
ring_buffer_iterator<T> &ring_buffer_iterator<T>::operator++() {
	assert(m_curr != m_buff->m_back);
	if (++m_curr == m_buff->m_last)
		m_curr = m_buff->m_base;
	return *this;
}
template <typename T>
ring_buffer_iterator<T> ring_buffer_iterator<T>::operator++(int) {
	auto ans = *this;
	++(*this);
	return ans;
}

template <typename T>
ring_buffer_iterator<T> &ring_buffer_iterator<T>::operator--() {
	assert(m_curr != m_buff->m_front);
	m_curr = m_curr == m_buff->m_base ? m_buff->m_last - 1 : m_curr - 1;
	return *this;
}
template <typename T>
ring_buffer_iterator<T> ring_buffer_iterator<T>::operator--(int) {
	auto ans = *this;
	--(*this);
	return ans;
}

template <typename T>
ring_buffer_iterator<T> &ring_buffer_iterator<T>::operator+=(ptrdiff_t n) {
	return move(n), *this;
}
template <typename T>
ring_buffer_iterator<T> &ring_buffer_iterator<T>::operator-=(ptrdiff_t n) {
	return move(-n), *this;
}

template <typename T>
ring_buffer_iterator<T> ring_buffer_iterator<T>::operator+(ptrdiff_t n) {
	ring_buffer_iterator<T> ans = *this;
	return ans += n;
}
template <typename T>
ring_buffer_iterator<T> ring_buffer_iterator<T>::operator-(ptrdiff_t n) {
	ring_buffer_iterator<T> ans = *this;
	return ans -= n;
}

template <typename T>
T *ring_buffer_iterator<T>::base() const {
	return m_curr;
}

template <typename T>
ptrdiff_t ring_buffer_iterator<T>::index() const {
	if (m_curr >= m_buff->m_front) {
		return m_curr - m_buff->m_front;
	} else {
		return (m_buff->m_last - m_buff->m_front) - (m_curr - m_buff->m_base);
	}
}

template <typename T>
void ring_buffer_iterator<T>::move(ptrdiff_t n) {
	if (n >= 0) {
		assert(m_buff->end() - *this >= n);
		int m = std::min(m_buff->m_last - m_curr - 1, n);
		if (n -= m) {
			m_curr = m_buff->m_base + n - 1;
		} else {
			m_curr += m;
		}
	} else {
		assert(m_buff->begin() - *this <= n);
		int m = std::max(m_buff->m_base - m_curr, n);
		if (n -= m) {
			m_curr = m_buff->m_base + n;
		} else {
			m_curr += m;
		}
	}
}

template <typename T1, typename T2>
ptrdiff_t operator-(const ring_buffer_iterator<T1> &a, const ring_buffer_iterator<T2> &b) {
	return a.index() - b.index();
}

template <typename T1, typename T2>
bool operator==(const ring_buffer_iterator<T1> &a, const ring_buffer_iterator<T2> &b) {
	return a.base() == b.base();
}

template <typename T1, typename T2>
bool operator!=(const ring_buffer_iterator<T1> &a, const ring_buffer_iterator<T2> &b) {
	return !(a == b);
}

template <typename T1, typename T2>
bool operator<(const ring_buffer_iterator<T1> &a, const ring_buffer_iterator<T2> &b) {
	return a.index() < b.index();
}

template <typename T1, typename T2>
bool operator>(const ring_buffer_iterator<T1> &a, const ring_buffer_iterator<T2> &b) {
	return a.index() > b.index();
}

template <typename T1, typename T2>
bool operator<=(const ring_buffer_iterator<T1> &a, const ring_buffer_iterator<T2> &b) {
	return !(a > b);
}

template <typename T1, typename T2>
bool operator>=(const ring_buffer_iterator<T1> &a, const ring_buffer_iterator<T2> &b) {
	return !(a < b);
}