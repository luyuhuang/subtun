#include <thread>
#include <string>
#include <memory>
#include <chrono>
#include <stdexcept>
#include <iostream>

#include "tun.h"
#include "udp.h"
#include "tcp.h"
#include "utils.h"
#include "session_mgr.h"
#include "cipher.h"

using std::thread;
using std::string;
using std::unique_ptr;
using std::runtime_error;
using std::cerr;
using std::endl;

typedef sudp4<chacha20_poly1305_indep> udp_type;

static void client_tun2net(const tun_t *tun, udp_type *u, const addr_ipv4 *server) {
	const size_t buff_size = 4096;
	unique_ptr<uint8_t[]> buff(new uint8_t[buff_size]);
	for (;;) {
		try {
			size_t size = tun_read(*tun, buff.get(), buff_size);
			u->sendto(buff.get(), size, *server);
		} catch (runtime_error e) {
			cerr << "[error] client_tun2net " << e.what() << endl;
		}
	}
}

static void client_net2tun(const tun_t *tun, udp_type *u) {
	const size_t buff_size = 4096;
	unique_ptr<uint8_t[]> buff(new uint8_t[buff_size]);
	for (;;) {
		try {
			size_t size = u->recvfrom(buff.get(), buff_size);
			tun_write(*tun, buff.get(), size);
		} catch (runtime_error e) {
			cerr << "[error] client_net2tun " << e.what() << endl;
		}
	}
}

void start_client(const string &server_addr) {
	string name = "subtun";
	tun_t tun = tun_alloc(name);
	if (guess_addr_type(server_addr) == addr_type::ipv4) {
		addr_ipv4 ad(server_addr);
		udp_type udp;
		udp.connect(ad);
		thread t2n(client_tun2net, &tun, &udp, &ad),
			   n2t(client_net2tun, &tun, &udp);

		t2n.join(), n2t.join();
	} else {
		throw runtime_error("unknow ip address format `" + server_addr + "'");
	}
}
