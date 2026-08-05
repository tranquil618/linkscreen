#include "linkscreen/protocol/hello_message.h"

#include "byte_io.h"

#include <cstring>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace {

[[nodiscard]] bool is_known_device_kind(
    linkscreen::protocol::DeviceKind kind)
{
    using linkscreen::protocol::DeviceKind;
    return kind == DeviceKind::harmony_tablet;
}

[[nodiscard]] bool has_only_known_capabilities(std::uint32_t capabilities) {
    return (capabilities & ~linkscreen::protocol::capability::known_mask) == 0;
}

[[nodiscard]] bool is_valid_device_name(std::string_view name) {
    return !name.empty()
        && name.size() <= linkscreen::protocol::kMaximumDeviceNameSize
        && name.find('\0') == std::string_view::npos;
}

} // namespace

namespace linkscreen::protocol {

std::vector<std::byte> encode_hello(const HelloMessage& message) {
    if (!is_known_device_kind(message.device_kind)) {
        throw std::invalid_argument("unknown Hello device kind");
    }
    if (!has_only_known_capabilities(message.capabilities)) {
        throw std::invalid_argument("Hello contains unknown capability bits");
    }
    if (!is_valid_device_name(message.device_name)) {
        throw std::invalid_argument("invalid Hello device name");
    }

    std::vector<std::byte> payload(
        kHelloFixedSize + message.device_name.size());

    payload[0] = static_cast<std::byte>(message.device_kind);
    payload[1] = std::byte{0};
    detail::write_u32(payload, 2, message.capabilities);
    detail::write_u16(
        payload,
        6,
        static_cast<std::uint16_t>(message.device_name.size()));

    std::memcpy(
        payload.data() + kHelloFixedSize,
        message.device_name.data(),
        message.device_name.size());

    return payload;
}

std::optional<HelloMessage> decode_hello(std::span<const std::byte> payload) {
    if (payload.size() < kHelloFixedSize) {
        return std::nullopt;
    }

    const auto device_kind = static_cast<DeviceKind>(
        std::to_integer<std::uint8_t>(payload[0]));
    const auto reserved = std::to_integer<std::uint8_t>(payload[1]);
    const auto capabilities = detail::read_u32(payload, 2);
    const auto device_name_size = detail::read_u16(payload, 6);

    if (!is_known_device_kind(device_kind)
        || reserved != 0
        || !has_only_known_capabilities(capabilities)
        || device_name_size == 0
        || device_name_size > kMaximumDeviceNameSize
        || payload.size() != kHelloFixedSize + device_name_size) {
        return std::nullopt;
    }

    const auto* name_data = reinterpret_cast<const char*>(
        payload.data() + kHelloFixedSize);
    std::string device_name{name_data, device_name_size};
    if (!is_valid_device_name(device_name)) {
        return std::nullopt;
    }

    return HelloMessage{
        .device_kind = device_kind,
        .capabilities = capabilities,
        .device_name = std::move(device_name),
    };
}

} // namespace linkscreen::protocol
