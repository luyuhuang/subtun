#pragma once

#include <stddef.h>
#include <string>

#if defined(_WIN32)
	#include "windows/tun.h"
#else
	#define tun_openlib()
	using tun_t = int;
#endif

tun_t tun_alloc(std::string &name);
size_t tun_read(const tun_t &tun, void *buf, size_t len);
size_t tun_write(const tun_t &tun, const void *buf, size_t len);
void tun_free(tun_t &tun);