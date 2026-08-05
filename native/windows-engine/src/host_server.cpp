#include "linkscreen/windows/host_server.h"

#include "linkscreen/windows/windows_socket.h"

#include "linkscreen/protocol/frame.h"
#include "linkscreen/protocol/frame_stream_decoder.h"
#include "linkscreen/protocol/hello_ack_message.h"
#include "linkscreen/protocol/hello_message.h"

#include <bcrypt.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <string>

namespace {

using linkscreen::protocol::Frame;
using linkscreen::protocol::HelloAckMessage;
using linkscreen::protocol::HelloAckStatus;

[[nodiscard]] std::string windows_host_name() {
    std::array<wchar_t, 256> wide_name{};
    DWORD size = static_cast<DWORD>(wide_name.size());
    if (!GetComputerNameW(wide_name.data(), &size) || size == 0) {
        return "LinkScreen Host";
    }

    const auto utf8_size = WideCharToMultiByte(
        CP_UTF8, 0, wide_name.data(), static_cast<int>(size),
        nullptr, 0, nullptr, nullptr);
    if (utf8_size <= 0) {
        return "LinkScreen Host";
    }

    std::string name(static_cast<std::size_t>(utf8_size), '\0');
    WideCharToMultiByte(
        CP_UTF8, 0, wide_name.data(), static_cast<int>(size),
        name.data(), utf8_size, nullptr, nullptr);
    return name;
}

[[nodiscard]] std::optional<std::uint64_t> create_session_id() {
    std::uint64_t session_id = 0;
    const auto status = BCryptGenRandom(
        nullptr,
        reinterpret_cast<PUCHAR>(&session_id),
        sizeof(session_id),
        BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if (status < 0) {
        return std::nullopt;
    }
    if (session_id == 0) {
        session_id = 1;
    }
    return session_id;
}

[[nodiscard]] HelloAckMessage make_rejection(HelloAckStatus status) {
    return {
        .status = status,
        .selected_version = 0,
        .selected_capabilities = 0,
        .session_id = 0,
        .host_name = windows_host_name(),
    };
}

[[nodiscard]] HelloAckMessage make_ack(const Frame& frame) {
    using namespace linkscreen::protocol;

    const auto hello = decode_hello(frame.payload);
    if (!hello.has_value()) {
        return make_rejection(HelloAckStatus::incompatible_capabilities);
    }

    constexpr auto host_capabilities = capability::touch
        | capability::keyboard
        | capability::pen
        | capability::h264_decode;
    const auto selected = hello->capabilities & host_capabilities;
    if ((selected & capability::h264_decode) == 0) {
        return make_rejection(HelloAckStatus::incompatible_capabilities);
    }

    const auto session_id = create_session_id();
    if (!session_id.has_value()) {
        return make_rejection(HelloAckStatus::incompatible_capabilities);
    }

    std::cout << "Client: " << hello->device_name << "\n";
    return {
        .status = HelloAckStatus::accepted,
        .selected_version = kProtocolVersion,
        .selected_capabilities = selected,
        .session_id = *session_id,
        .host_name = windows_host_name(),
    };
}

} // namespace

namespace linkscreen::windows {

bool HostServer::run_once() {
    WinsockRuntime winsock;
    SocketHandle listener{socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)};
    if (!listener.valid()) {
        std::cerr << "socket failed: " << WSAGetLastError() << '\n';
        return false;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(kDevelopmentControlPort);
    inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);

    if (bind(
            listener.get(),
            reinterpret_cast<const sockaddr*>(&address),
            sizeof(address)) == SOCKET_ERROR) {
        std::cerr << "bind failed: " << WSAGetLastError() << '\n';
        return false;
    }
    if (listen(listener.get(), 1) == SOCKET_ERROR) {
        std::cerr << "listen failed: " << WSAGetLastError() << '\n';
        return false;
    }

    std::cout << "LinkScreen development host listening on 127.0.0.1:"
              << kDevelopmentControlPort << "\n";
    SocketHandle client{accept(listener.get(), nullptr, nullptr)};
    if (!client.valid()) {
        std::cerr << "accept failed: " << WSAGetLastError() << '\n';
        return false;
    }

    protocol::FrameStreamDecoder decoder;
    std::array<std::byte, 4096> receive_buffer{};
    while (true) {
        const auto received = recv(
            client.get(),
            reinterpret_cast<char*>(receive_buffer.data()),
            static_cast<int>(receive_buffer.size()),
            0);
        if (received <= 0) {
            std::cerr << "connection closed before handshake completed\n";
            return false;
        }
        if (!decoder.append(std::span<const std::byte>{
                receive_buffer.data(), static_cast<std::size_t>(received)})) {
            std::cerr << "control receive buffer limit exceeded\n";
            return false;
        }

        while (true) {
            auto result = decoder.next();
            if (result.status == protocol::StreamDecodeStatus::need_more_data) {
                break;
            }
            if (result.status == protocol::StreamDecodeStatus::protocol_error
                || !result.frame.has_value()) {
                std::cerr << "invalid control-channel frame\n";
                return false;
            }
            if (result.frame->header.type != protocol::MessageType::hello) {
                std::cerr << "expected Hello as the first frame\n";
                return false;
            }

            const auto ack = make_ack(*result.frame);
            const auto ack_payload = protocol::encode_hello_ack(ack);
            const auto ack_frame = protocol::encode_frame(
                protocol::MessageType::hello_ack,
                result.frame->header.sequence,
                ack_payload);
            if (ack_frame.size() > static_cast<std::size_t>(
                    std::numeric_limits<int>::max())
                || !send_all(
                    client.get(), ack_frame.data(),
                    static_cast<int>(ack_frame.size()))) {
                std::cerr << "failed to send HelloAck\n";
                return false;
            }

            std::cout << "Handshake status: "
                      << (ack.status == HelloAckStatus::accepted
                              ? "accepted" : "rejected")
                      << '\n';
            return ack.status == HelloAckStatus::accepted;
        }
    }
}

} // namespace linkscreen::windows
