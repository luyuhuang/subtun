﻿# CMakeList.txt : CMake project for subtun, include source and define
# project specific logic here.
#
cmake_minimum_required(VERSION 3.8)

project("subtun")

set(src "main.cc"
		"addr.h" "socket.h" "tun.h"
		"utils.h" "utils.cc"
		"service.h" "service.cc"
		"sudp.h" "sudp.cc")

if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

if(LINUX)
	list(APPEND src "linux/socket.cc" "linux/tun.cc")
elseif(WIN32)
	list(APPEND src "windows/socket.cc" "windows/tun.cc" "windows/wintun.h")
elseif(APPLE)
	
endif()

add_executable(subtun ${src})

set_property(TARGET subtun PROPERTY CXX_STANDARD 17)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(subtun PRIVATE Threads::Threads)

find_package(OpenSSL REQUIRED)
target_link_libraries(subtun PRIVATE OpenSSL::SSL OpenSSL::Crypto)