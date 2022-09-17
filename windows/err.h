#pragma once

#include <string>
#include <winbase.h>

inline std::string last_error_str(DWORD err) {
	std::string ans(128, '\0');
	size_t n = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), ans.data(), ans.size(), nullptr);
	ans.resize(n);
	return ans;
}

inline std::string last_error_str() {
	DWORD err = GetLastError();
	return last_error_str(err);
}