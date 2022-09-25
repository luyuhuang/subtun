# CMakeList.txt : CMake project for subtun, include source and define
# project specific logic here.
#
cmake_minimum_required(VERSION 3.8)

project("subtun")

set(src "main.cc"
		"addr.h" "socket.h" "tun.h"
		"utils.h" "utils.cc"
		"service.h" "service.cc"
		"udp.h" "udp.cc" "poller.h"  "session_mgr.h"
		"cipher.h" "cipher.cc" "init.h" "tcp.h" "ring_buffer.h" "ring_buffer.cc")

if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

if(LINUX)
	list(APPEND src "linux/socket.cc" "linux/tun.cc" "linux/poller.h" "linux/poller.cc" "linux/init.cc")
elseif(WIN32)
	list(APPEND src "windows/socket.cc" "windows/tun.cc" "windows/tun.h" "windows/wintun.h" "windows/err.h" "windows/init.cc")
elseif(APPLE)
	
endif()

add_executable(subtun ${src})

set_property(TARGET subtun PROPERTY CXX_STANDARD 17)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(subtun PRIVATE Threads::Threads)

find_package(OpenSSL 1.1.1 REQUIRED)
target_link_libraries(subtun PRIVATE OpenSSL::SSL OpenSSL::Crypto)

if(WIN32)
	target_link_libraries(subtun PRIVATE wsock32 ws2_32)
endif()