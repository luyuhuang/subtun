#include <stdexcept>
#include <string>

#include "wintun.h"
#include "../tun.h"
#include "err.h"

using std::string;
using std::logic_error;
using std::runtime_error;

static WINTUN_CREATE_ADAPTER_FUNC *WintunCreateAdapter;
static WINTUN_CLOSE_ADAPTER_FUNC *WintunCloseAdapter;
static WINTUN_OPEN_ADAPTER_FUNC *WintunOpenAdapter;
static WINTUN_GET_ADAPTER_LUID_FUNC *WintunGetAdapterLUID;
static WINTUN_GET_RUNNING_DRIVER_VERSION_FUNC *WintunGetRunningDriverVersion;
static WINTUN_DELETE_DRIVER_FUNC *WintunDeleteDriver;
static WINTUN_SET_LOGGER_FUNC *WintunSetLogger;
static WINTUN_START_SESSION_FUNC *WintunStartSession;
static WINTUN_END_SESSION_FUNC *WintunEndSession;
static WINTUN_GET_READ_WAIT_EVENT_FUNC *WintunGetReadWaitEvent;
static WINTUN_RECEIVE_PACKET_FUNC *WintunReceivePacket;
static WINTUN_RELEASE_RECEIVE_PACKET_FUNC *WintunReleaseReceivePacket;
static WINTUN_ALLOCATE_SEND_PACKET_FUNC *WintunAllocateSendPacket;
static WINTUN_SEND_PACKET_FUNC *WintunSendPacket;

void InitializeWintun() {
	HMODULE wintun = LoadLibraryExW(L"wintun.dll", nullptr, LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (!wintun) {
		throw logic_error("fail to open wintun.dll");
	}

#define X(name) ((*reinterpret_cast<FARPROC *>(&name) = GetProcAddress(wintun, #name)) == nullptr)
	if (X(WintunCreateAdapter) || X(WintunCloseAdapter) || X(WintunOpenAdapter) || X(WintunGetAdapterLUID) ||
		X(WintunGetRunningDriverVersion) || X(WintunDeleteDriver) || X(WintunSetLogger) || X(WintunStartSession) ||
		X(WintunEndSession) || X(WintunGetReadWaitEvent) || X(WintunReceivePacket) || X(WintunReleaseReceivePacket) ||
		X(WintunAllocateSendPacket) || X(WintunSendPacket))
#undef X
	{
		auto err = last_error_str();
		FreeLibrary(wintun);
		throw logic_error("fail to get function from wintun.dll: " + err);
	}
}

tun_t tun_alloc(std::string &name) {
	if (name.size() > MAX_ADAPTER_NAME)
		throw runtime_error("name is too long");
	std::wstring wname(name.begin(), name.end());

	// af2643d2-0026-416a-a305-d634f3b26232
	GUID guid = {
		0xaf2643d2,
		0x0026,
		0x416a,
		{0xa3, 0x05, 0xd6, 0x34, 0xf3, 0xb2, 0x62, 0x32},
	};
	auto adapter = WintunCreateAdapter(wname.c_str(), L"Wintun", &guid);
	if (!adapter)
		throw runtime_error("fail to create an adapter: " + last_error_str());
	auto session = WintunStartSession(adapter, 0x400000);
	if (!session)
		throw runtime_error("fail to start a session: " + last_error_str());
	return { adapter, session };
}

size_t tun_read(const tun_t &tun, void *buf, size_t len) {
	DWORD size;
	BYTE *packet = WintunReceivePacket(tun.session, &size);
	if (packet) {
		memcpy(buf, packet, size = min(size, len));
		WintunReleaseReceivePacket(tun.session, packet);
		return size;
	} else if (auto e = GetLastError(); e != ERROR_NO_MORE_ITEMS) {
		throw runtime_error("fail to read from the tun: ");
	} else {
		WaitForSingleObject(WintunGetReadWaitEvent(tun.session), INFINITE);
		return tun_read(tun, buf, len);
	}
}

size_t tun_write(const tun_t &tun, const void *buf, size_t len) {
	BYTE *packet = WintunAllocateSendPacket(tun.session, len);
	if (packet) {
		memcpy(packet, buf, len);
		WintunSendPacket(tun.session, packet);
		return len;
	} else if (auto e = GetLastError(); e != ERROR_BUFFER_OVERFLOW)
		throw runtime_error("fail to write to the tun: " + last_error_str(e));
}

void tun_free(tun_t &tun) {
	WintunEndSession(tun.session);
	WintunCloseAdapter(tun.adapter);
	tun.session = nullptr, tun.adapter = nullptr;
}
