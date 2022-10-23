#pragma once

#include <map>
#include <vector>
#include <list>
#include <iterator>
#include <chrono>
#include <mutex>
#include <stdexcept>

template <typename VIP, typename Conn, size_t Wheel = 101, typename Camp = typename VIP::less>
class session_mgr {
	using time_type = std::chrono::steady_clock::rep;

	struct node {
		node() : prev(this), next(this) {}
		node *prev, *next;
	};
	struct entry : node {
		template <typename VIP_, typename Conn_>
		entry(VIP_ &&vip_, Conn_ &&conn_, time_type t_)
			: vip(std::forward<VIP_>(vip_)), conn(std::forward<Conn_>(conn_)), t(t_) {
		}
		VIP vip;
		Conn conn;
		time_type t;
	};

	std::map<VIP, node*, Camp> m_dict;
	node m_time_wheel[Wheel];
	time_type m_last = -1;
	const time_type m_expire;
	std::mutex m_lock;

	void insert_node(node *target, node *to_be_inserted) {
		to_be_inserted->next = target->next;
		to_be_inserted->prev = target;
		target->next->prev = to_be_inserted;
		target->next = to_be_inserted;
	}

	void remove_node(node *to_be_removed) {
		to_be_removed->next->prev = to_be_removed->prev;
		to_be_removed->prev->next = to_be_removed->next;
		to_be_removed->next = to_be_removed->prev = to_be_removed;
	}

	time_type get_cur_time() {
		using namespace std::chrono;
		return duration_cast<seconds>(steady_clock::now().time_since_epoch()).count();
	}

	size_t calc_slot(time_type t) {
		if ((t %= Wheel) < 0) t += Wheel;
		return t;
	}

public:
	session_mgr(time_type expire) : m_time_wheel{}, m_expire(expire) {}
	session_mgr(const session_mgr &) = delete;
	session_mgr &operator=(const session_mgr &) = delete;
	~session_mgr() {
		for (auto &e : m_time_wheel) {
			node *p = e.next, *t;
			while (p != &e) {
				t = p;
				p = p->next;
				delete t;
			}
		}
	}

	template <typename VIP_, typename Conn_>
	void put(VIP_ &&vip, Conn_ &&conn) {
		std::lock_guard<std::mutex> guard(m_lock);
		time_type expire = get_cur_time() + m_expire;
		size_t slot = calc_slot(expire);

		auto it = m_dict.find(vip);
		if (it == m_dict.end()) {
			entry *e = new entry{ vip, std::forward<Conn_>(conn), expire };
			insert_node(m_time_wheel + slot, e);
			m_dict.emplace(std::forward<VIP_>(vip), e);
		} else {
			entry *e = static_cast<entry *>(it->second);
			e->conn = std::forward<Conn_>(conn);
			e->t = expire;
			remove_node(e);
			insert_node(m_time_wheel + slot, e);
		}
	}

	bool has(const VIP &vip) {
		std::lock_guard<std::mutex> guard(m_lock);
		return m_dict.find(vip) != m_dict.end();
	}

	Conn get(const VIP &vip) {
		std::lock_guard<std::mutex> guard(m_lock);
		auto it = m_dict.find(vip);
		if (it == m_dict.end())
			throw std::runtime_error('`' + vip.to_string() + "' not found");

		time_type expire = get_cur_time() + m_expire;
		size_t slot = calc_slot(expire);

		entry *e = static_cast<entry*>(it->second);
		e->t = expire;
		remove_node(e);
		insert_node(m_time_wheel + slot, e);

		return e->conn;
	}

	void del(const VIP &vip) {
		std::lock_guard<std::mutex> guard(m_lock);
		auto it = m_dict.find(vip);
		if (it == m_dict.end())
			throw std::runtime_error('`' + vip.to_string() + "' not found");

		m_dict.erase(it);
		remove_node(it->second);
		delete it->second;
	}

	void update() {
		std::lock_guard<std::mutex> guard(m_lock);
		const time_type now = get_cur_time();
		if (m_last < 0) m_last = now;
		while (m_last <= now) {
			size_t slot = calc_slot(m_last);
			auto &e = m_time_wheel[slot];
			node *p = e.next;
			while (p != &e) {
				entry *t = static_cast<entry *>(p);
				p = p->next;
				if (t->t == m_last) {
					m_dict.erase(t->vip);
					remove_node(t);
				}
			}
			++m_last;
		}
	}
};
