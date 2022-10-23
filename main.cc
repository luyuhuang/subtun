#include <iostream>
#include <stdexcept>

#include "init.h"
#include "server.h"
#include "client.h"
#include "tun.h"
#include "cipher.h"

using std::cout;
using std::cerr;
using std::endl;

void init() {
	init_platform();
}

int main(int argc, char **argv) {
	init();
	if (argc < 3) {
		cerr << "usage: " << argv[0] << " client server_addr" << endl;
		cerr << "       " << argv[0] << " server listen_addr" << endl;
		return 1;
	}
	try {
		if (argv[1][0] == 'c') {
			start_client(argv[2]);
		} else if (argv[1][0] == 's') {
			start_server(argv[2]);
		}
	} catch (std::runtime_error e) {
		cerr << "[error] " << e.what() << endl;
		return 1;
	}
	return 0;
}
