#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace linkscreen::protocol {

inline constexpr std::size_t kHelloAckFixedSize = 18;
inline constexpr std::size_t kMaximumHostNameSize = 64;

enum class HelloAckStatus : std::uint8_t {
    accepted = 0,
    unsupported_version = 1,
    incompatible_capabilities = 2,
};

struct HelloAckMessage {
    HelloAckStatus status{HelloAckStatus::accepted};
    std::uint16_t selected_version{1};
    std::uint32_t selected_capabilities{0};
    std::uint64_t session_id{0};
    std::string host_name;

    bool operator==(const HelloAckMessage&) const = default;
};

[[nodiscard]] std::vector<std::byte> encode_hello_ack(
    const HelloAckMessage& message);

[[nodiscard]] std::optional<HelloAckMessage> decode_hello_ack(
    std::span<const std::byte> payload);

} // namespace linkscreen::protocol

