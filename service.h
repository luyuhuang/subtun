#pragma once

#include <string>

void start_server(const std::string &listen_addr);
void start_client(const std::string &server_addr);