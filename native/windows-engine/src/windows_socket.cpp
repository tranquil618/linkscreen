#include "linkscreen/windows/windows_socket.h"

#include <stdexcept>
#include <string>
#include <utility>

namespace linkscreen::windows {

WinsockRuntime::WinsockRuntime() {
    WSADATA data{};
    const auto result = WSAStartup(MAKEWORD(2, 2), &data);
    if (result != 0) {
        throw std::runtime_error("WSAStartup failed: " + std::to_string(result));
    }
    initialized_ = true;
}

WinsockRuntime::~WinsockRuntime() {
    if (initialized_) {
        WSACleanup();
    }
}

SocketHandle::SocketHandle(SOCKET socket) noexcept : socket_{socket} {}

SocketHandle::~SocketHandle() {
    reset();
}

SocketHandle::SocketHandle(SocketHandle&& other) noexcept
    : socket_{std::exchange(other.socket_, INVALID_SOCKET)} {}

SocketHandle& SocketHandle::operator=(SocketHandle&& other) noexcept {
    if (this != &other) {
        reset(std::exchange(other.socket_, INVALID_SOCKET));
    }
    return *this;
}

SOCKET SocketHandle::get() const noexcept {
    return socket_;
}

bool SocketHandle::valid() const noexcept {
    return socket_ != INVALID_SOCKET;
}

void SocketHandle::reset(SOCKET socket) noexcept {
    if (valid()) {
        closesocket(socket_);
    }
    socket_ = socket;
}

bool send_all(SOCKET socket, const void* data, int size) {
    const auto* bytes = static_cast<const char*>(data);
    int sent = 0;
    while (sent < size) {
        const auto result = send(socket, bytes + sent, size - sent, 0);
        if (result == SOCKET_ERROR || result == 0) {
            return false;
        }
        sent += result;
    }
    return true;
}

} // namespace linkscreen::windows
