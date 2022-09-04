#pragma once

#include <stddef.h>
#include <string>

using tun_t = int;

tun_t tun_alloc(std::string &name);
size_t tun_read(const tun_t &tun, void *buf, size_t len);
size_t tun_write(const tun_t &tun, const void *buf, size_t len);
