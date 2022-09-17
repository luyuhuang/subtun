#include <stdexcept>

#include <winsock2.h>

#include "err.h"
#include "tun.h"

using std::runtime_error;

void init_platform() {
	InitializeWintun();

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		throw runtime_error("WSAStartup fails: " + last_error_str());
}