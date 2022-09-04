#include <iostream>
#include <stdexcept>

#include "service.h"

using std::cout;
using std::cerr;
using std::endl;

int main(int argc, char **argv) {
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
