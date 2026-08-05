#pragma once

#include <winsock2.h>

namespace linkscreen::windows {

class WinsockRuntime {
public:
    WinsockRuntime();
    ~WinsockRuntime();

    WinsockRuntime(const WinsockRuntime&) = delete;
    WinsockRuntime& operator=(const WinsockRuntime&) = delete;

private:
    bool initialized_{false};
};

class SocketHandle {
public:
    SocketHandle() = default;
    explicit SocketHandle(SOCKET socket) noexcept;
    ~SocketHandle();

    SocketHandle(const SocketHandle&) = delete;
    SocketHandle& operator=(const SocketHandle&) = delete;

    SocketHandle(SocketHandle&& other) noexcept;
    SocketHandle& operator=(SocketHandle&& other) noexcept;

    [[nodiscard]] SOCKET get() const noexcept;
    [[nodiscard]] bool valid() const noexcept;
    void reset(SOCKET socket = INVALID_SOCKET) noexcept;

private:
    SOCKET socket_{INVALID_SOCKET};
};

[[nodiscard]] bool send_all(SOCKET socket, const void* data, int size);

} // namespace linkscreen::windows

