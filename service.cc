#include <thread>
#include <string>
#include <memory>
#include <chrono>
#include <stdexcept>
#include <iostream>

#include "tun.h"
#include "sudp.h"
#include "utils.h"
#include "session_mgr.h"
#include "cipher.h"

using std::thread;
using std::string;
using std::unique_ptr;
using std::runtime_error;
using std::cerr;
using std::endl;

static void server_tun2net(const tun_t *tun, sudp4<aes_128_cbc_cmac> *u, session_mgr<IPv4, addr_ipv4> *smgr) {
	const size_t buff_size = 4096;
	unique_ptr<uint8_t[]> buff(new uint8_t[buff_size]);
	for (;;) {
		try {
			size_t size = tun_read(*tun, buff.get(), buff_size);
			IPv4 dst = parse_dst_ip<IPv4>(buff.get());
			addr_ipv4 client = smgr->get(dst);
			u->sendto(buff.get(), size, client);
		} catch (runtime_error e) {
			cerr << "[error] server_tun2net " << e.what() << endl;
		}
	}
}

static void server_net2tun(const tun_t *tun, sudp4<aes_128_cbc_cmac> *u, session_mgr<IPv4, addr_ipv4> *smgr) {
	const size_t buff_size = 4096;
	unique_ptr<uint8_t[]> buff(new uint8_t[buff_size]);
	addr_ipv4 client;
	for (;;) {
		try {
			size_t size = u->recvfrom(buff.get(), buff_size, client);
			IPv4 src = parse_src_ip<IPv4>(buff.get());
			smgr->put(src, client);
			tun_write(*tun, buff.get(), size);
		} catch (runtime_error e) {
			cerr << "[error] server_net2tun " << e.what() << endl;
		}
	}
}

template <typename Mgr>
static void update_session_mgr(Mgr &mgr) {
	using namespace std::chrono;
	for (;;) {
		mgr.update();
		std::this_thread::sleep_for(1s);
	}
}

void start_server(const string &listen_addr) {
	string name = "subtun";
	tun_t tun = tun_alloc(name);
	if (guess_addr_type(listen_addr) == addr_type::ipv4) {
		session_mgr<IPv4, addr_ipv4> smgr(600);
		addr_ipv4 ad(listen_addr);
		sudp4<aes_128_cbc_cmac> udp(ad);
		thread t2n(server_tun2net, &tun, &udp, &smgr),
			   n2t(server_net2tun, &tun, &udp, &smgr);

		update_session_mgr(smgr);
		t2n.join(), n2t.join();
	} else {
		throw runtime_error("unknow ip address format `" + listen_addr + "'");
	}
}

static void client_tun2net(const tun_t *tun, sudp4<aes_128_cbc_cmac> *u, const addr_ipv4 *server) {
	const size_t buff_size = 4096;
	unique_ptr<uint8_t[]> buff(new uint8_t[buff_size]);
	for (;;) {
		size_t size = tun_read(*tun, buff.get(), buff_size);
		u->sendto(buff.get(), size, *server);
	}
}

static void client_net2tun(const tun_t *tun, sudp4<aes_128_cbc_cmac> *u) {
	const size_t buff_size = 4096;
	unique_ptr<uint8_t[]> buff(new uint8_t[buff_size]);
	for (;;) {
		size_t size = u->recvfrom(buff.get(), buff_size);
		tun_write(*tun, buff.get(), size);
	}
}

void start_client(const string &server_addr) {
	string name = "subtun";
	tun_t tun = tun_alloc(name);
	if (guess_addr_type(server_addr) == addr_type::ipv4) {
		addr_ipv4 ad(server_addr);
		sudp4<aes_128_cbc_cmac> udp;
		thread t2n(client_tun2net, &tun, &udp, &ad),
			   n2t(client_net2tun, &tun, &udp);

		t2n.join(), n2t.join();
	} else {
		throw runtime_error("unknow ip address format `" + server_addr + "'");
	}
}
