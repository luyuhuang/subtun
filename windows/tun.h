#pragma once

#include "wintun.h"

struct tun_t {
	WINTUN_ADAPTER_HANDLE adapter;
	WINTUN_SESSION_HANDLE session;
};

void InitializeWintun();
