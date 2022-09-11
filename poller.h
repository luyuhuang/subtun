#pragma once

#if defined(__linux__)
	#include "linux/poller.h"
	using poller = poller_epoll;
#endif
