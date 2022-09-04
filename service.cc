#include <thread>
#include <string>
#include <memory>
#include <chrono>
#include <stdexcept>

#include "tun.h"
#include "sudp.h"

using std::thread;
using std::string;
using std::unique_ptr;
using std::runtime_error;

static void server_tun2net(const tun_t *tun, const sudp4 *u, const addr_ipv4 *client) {
	const size_t buff_size = 4096;
	unique_ptr<uint8_t[]> buff(new uint8_t[buff_size]);
	for (;;) {
		if (client->is_zero()) {
			using namespace std::chrono;
			std::this_thread::sleep_for(10ms);
			continue;
		}
		size_t size = tun_read(*tun, buff.get(), buff_size);
		u->sendto(buff.get(), size, *client);
	}
}

static void server_net2tun(const tun_t *tun, const sudp4 *u, addr_ipv4 *client) {
	const size_t buff_size = 4096;
	unique_ptr<uint8_t[]> buff(new uint8_t[buff_size]);
	for (;;) {
		size_t size;
		if (client->is_zero())
			size = u->recvfrom(buff.get(), buff_size, *client);
		else
			size = u->recvfrom(buff.get(), buff_size);
		tun_write(*tun, buff.get(), size);
	}
}

void start_server(const string &listen_addr) {
	string name = "subtun";
	tun_t tun = tun_alloc(name);
	if (addr::guess_type(listen_addr) == addr::ipv4) {
		addr_ipv4 ad(listen_addr), client_addr;
		sudp4 udp(ad);
		thread t2n(server_tun2net, &tun, &udp, &client_addr), n2t(server_net2tun, &tun, &udp, &client_addr);
		t2n.join(), n2t.join();
	} else {
		throw runtime_error("unknow ip address format `" + listen_addr + "'");
	}
}

static void client_tun2net(const tun_t *tun, const sudp4 *u, const addr_ipv4 *server) {
	const size_t buff_size = 4096;
	unique_ptr<uint8_t[]> buff(new uint8_t[buff_size]);
	for (;;) {
		size_t size = tun_read(*tun, buff.get(), buff_size);
		u->sendto(buff.get(), size, *server);
	}
}

static void client_net2tun(const tun_t *tun, const sudp4 *u) {
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
	if (addr::guess_type(server_addr) == addr::ipv4) {
		addr_ipv4 ad(server_addr);
		const sudp4 udp;
		thread t2n(client_tun2net, &tun, &udp, &ad), n2t(client_net2tun, &tun, &udp);
		t2n.join(), n2t.join();
	} else {
		throw runtime_error("unknow ip address format `" + server_addr + "'");
	}
}