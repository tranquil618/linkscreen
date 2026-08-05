#include "linkscreen/protocol/hello_ack_message.h"

#include "byte_io.h"
#include "linkscreen/protocol/hello_message.h"
#include "linkscreen/protocol/message_header.h"

#include <cstring>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace {

using linkscreen::protocol::HelloAckMessage;
using linkscreen::protocol::HelloAckStatus;

[[nodiscard]] bool is_known_status(HelloAckStatus status) {
    switch (status) {
    case HelloAckStatus::accepted:
    case HelloAckStatus::unsupported_version:
    case HelloAckStatus::incompatible_capabilities:
        return true;
    }
    return false;
}

[[nodiscard]] bool has_only_known_capabilities(std::uint32_t capabilities) {
    return (capabilities & ~linkscreen::protocol::capability::known_mask) == 0;
}

[[nodiscard]] bool is_valid_host_name(std::string_view name) {
    return !name.empty()
        && name.size() <= linkscreen::protocol::kMaximumHostNameSize
        && name.find('\0') == std::string_view::npos;
}

[[nodiscard]] bool has_valid_result_fields(const HelloAckMessage& message) {
    using linkscreen::protocol::kProtocolVersion;

    if (message.status == HelloAckStatus::accepted) {
        return message.selected_version == kProtocolVersion
            && message.session_id != 0;
    }

    return message.selected_version == 0
        && message.selected_capabilities == 0
        && message.session_id == 0;
}

} // namespace

namespace linkscreen::protocol {

std::vector<std::byte> encode_hello_ack(const HelloAckMessage& message) {
    if (!is_known_status(message.status)) {
        throw std::invalid_argument("unknown HelloAck status");
    }
    if (!has_only_known_capabilities(message.selected_capabilities)) {
        throw std::invalid_argument("HelloAck contains unknown capability bits");
    }
    if (!is_valid_host_name(message.host_name)) {
        throw std::invalid_argument("invalid HelloAck host name");
    }
    if (!has_valid_result_fields(message)) {
        throw std::invalid_argument("inconsistent HelloAck result fields");
    }

    std::vector<std::byte> payload(
        kHelloAckFixedSize + message.host_name.size());
    payload[0] = static_cast<std::byte>(message.status);
    payload[1] = std::byte{0};
    detail::write_u16(payload, 2, message.selected_version);
    detail::write_u32(payload, 4, message.selected_capabilities);
    detail::write_u64(payload, 8, message.session_id);
    detail::write_u16(
        payload,
        16,
        static_cast<std::uint16_t>(message.host_name.size()));

    std::memcpy(
        payload.data() + kHelloAckFixedSize,
        message.host_name.data(),
        message.host_name.size());
    return payload;
}

std::optional<HelloAckMessage> decode_hello_ack(
    std::span<const std::byte> payload)
{
    if (payload.size() < kHelloAckFixedSize) {
        return std::nullopt;
    }

    HelloAckMessage message;
    message.status = static_cast<HelloAckStatus>(
        std::to_integer<std::uint8_t>(payload[0]));
    const auto reserved = std::to_integer<std::uint8_t>(payload[1]);
    message.selected_version = detail::read_u16(payload, 2);
    message.selected_capabilities = detail::read_u32(payload, 4);
    message.session_id = detail::read_u64(payload, 8);
    const auto host_name_size = detail::read_u16(payload, 16);

    if (!is_known_status(message.status)
        || reserved != 0
        || !has_only_known_capabilities(message.selected_capabilities)
        || host_name_size == 0
        || host_name_size > kMaximumHostNameSize
        || payload.size() != kHelloAckFixedSize + host_name_size) {
        return std::nullopt;
    }

    const auto* name_data = reinterpret_cast<const char*>(
        payload.data() + kHelloAckFixedSize);
    message.host_name.assign(name_data, host_name_size);

    if (!is_valid_host_name(message.host_name)
        || !has_valid_result_fields(message)) {
        return std::nullopt;
    }

    return message;
}

} // namespace linkscreen::protocol

