#include "linkscreen/windows/host_server.h"
#include "linkscreen/windows/windows_socket.h"

#include "linkscreen/protocol/frame.h"
#include "linkscreen/protocol/frame_stream_decoder.h"
#include "linkscreen/protocol/hello_ack_message.h"
#include "linkscreen/protocol/hello_message.h"

#include <ws2tcpip.h>

#include <array>
#include <cstddef>
#include <exception>
#include <iostream>
#include <limits>
#include <span>

namespace {

[[nodiscard]] bool run_client() {
    using namespace linkscreen;

    windows::WinsockRuntime winsock;
    windows::SocketHandle socket_handle{
        socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)};
    if (!socket_handle.valid()) {
        std::cerr << "client socket failed: " << WSAGetLastError() << '\n';
        return false;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(windows::kDevelopmentControlPort);
    inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);
    if (connect(
            socket_handle.get(),
            reinterpret_cast<const sockaddr*>(&address),
            sizeof(address)) == SOCKET_ERROR) {
        std::cerr << "connect failed: " << WSAGetLastError() << '\n';
        return false;
    }

    const protocol::HelloMessage hello{
        .device_kind = protocol::DeviceKind::harmony_tablet,
        .capabilities = protocol::capability::touch
                      | protocol::capability::keyboard
                      | protocol::capability::pen
                      | protocol::capability::h264_decode,
        .device_name = "HarmonyOS Development Client",
    };
    const auto payload = protocol::encode_hello(hello);
    const auto frame = protocol::encode_frame(
        protocol::MessageType::hello, 1, payload);
    if (frame.size() > static_cast<std::size_t>(
            std::numeric_limits<int>::max())) {
        return false;
    }

    // Intentionally split the send to verify that the host handles TCP
    // fragmentation instead of assuming one recv call equals one frame.
    constexpr std::size_t first_chunk_size = 5;
    if (!windows::send_all(
            socket_handle.get(), frame.data(),
            static_cast<int>(first_chunk_size))
        || !windows::send_all(
            socket_handle.get(), frame.data() + first_chunk_size,
            static_cast<int>(frame.size() - first_chunk_size))) {
        std::cerr << "failed to send Hello\n";
        return false;
    }

    protocol::FrameStreamDecoder decoder;
    std::array<std::byte, 1024> receive_buffer{};
    while (true) {
        const auto received = recv(
            socket_handle.get(),
            reinterpret_cast<char*>(receive_buffer.data()),
            static_cast<int>(receive_buffer.size()),
            0);
        if (received <= 0) {
            std::cerr << "host closed before HelloAck arrived\n";
            return false;
        }
        if (!decoder.append(std::span<const std::byte>{
                receive_buffer.data(), static_cast<std::size_t>(received)})) {
            return false;
        }

        const auto result = decoder.next();
        if (result.status == protocol::StreamDecodeStatus::need_more_data) {
            continue;
        }
        if (result.status != protocol::StreamDecodeStatus::frame_ready
            || !result.frame.has_value()
            || result.frame->header.type != protocol::MessageType::hello_ack) {
            std::cerr << "invalid HelloAck frame\n";
            return false;
        }

        const auto ack = protocol::decode_hello_ack(result.frame->payload);
        if (!ack.has_value()
            || ack->status != protocol::HelloAckStatus::accepted) {
            std::cerr << "handshake was rejected\n";
            return false;
        }

        std::cout << "Connected to: " << ack->host_name << '\n'
                  << "Session ID: " << ack->session_id << '\n'
                  << "Selected capabilities: 0x" << std::hex
                  << ack->selected_capabilities << std::dec << '\n';
        return true;
    }
}

} // namespace

int main() {
    try {
        return run_client() ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << "fatal client error: " << error.what() << '\n';
        return 1;
    }
}

