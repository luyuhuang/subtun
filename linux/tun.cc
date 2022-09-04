#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/if.h>
#include <linux/if_tun.h>

#include <stdexcept>
#include <string>
#include <algorithm>

#include "../tun.h"

using std::string;
using std::runtime_error;

tun_t tun_alloc(string &name) {
	if (name.size() >= IFNAMSIZ) {
		throw runtime_error("name is too long");
	}

	int fd = open("/dev/net/tun", O_RDWR);
	if (fd < 0) {
		throw runtime_error(string("open /dev/net/tun fail. errno: ") + strerror(errno));
	}
	
	struct ifreq ifr {};
	std::copy(name.begin(), name.end(), ifr.ifr_name);
	ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

	if (int err = ioctl(fd, TUNSETIFF, &ifr); err < 0) {
		close(fd);
		throw runtime_error(string("create/attach the network interface fail. errno: ") + strerror(errno));
	}

	name.assign(ifr.ifr_name);
	return fd;
}

size_t tun_read(const tun_t& tun, void *buf, size_t len) {
	ssize_t size = read(tun, buf, len);
	if (size < 0)
		throw runtime_error(string("tun_read: read returns err. errno: ") + strerror(errno));
	return size;
}

size_t tun_write(const tun_t& tun, const void* buf, size_t len) {
	ssize_t size = write(tun, buf, len);
	if (size < 0)
		throw runtime_error(string("tun_read: write returns err. errno: ") + strerror(errno));
	return size;
}

