#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace linkscreen::protocol {

inline constexpr std::size_t kHelloFixedSize = 8;
inline constexpr std::size_t kMaximumDeviceNameSize = 64;

enum class DeviceKind : std::uint8_t {
    harmony_tablet = 1,
};

namespace capability {

inline constexpr std::uint32_t touch = 1U << 0;
inline constexpr std::uint32_t keyboard = 1U << 1;
inline constexpr std::uint32_t pen = 1U << 2;
inline constexpr std::uint32_t h264_decode = 1U << 3;
inline constexpr std::uint32_t h265_decode = 1U << 4;

inline constexpr std::uint32_t known_mask =
    touch | keyboard | pen | h264_decode | h265_decode;

} // namespace capability

struct HelloMessage {
    DeviceKind device_kind{DeviceKind::harmony_tablet};
    std::uint32_t capabilities{0};
    std::string device_name;

    bool operator==(const HelloMessage&) const = default;
};

// Encodes the variable-length Hello payload. Throws std::invalid_argument
// when a field cannot be represented by protocol v1.
[[nodiscard]] std::vector<std::byte> encode_hello(const HelloMessage& message);

// Decodes and validates one complete Hello payload. The input does not include
// the 16-byte MessageHeader.
[[nodiscard]] std::optional<HelloMessage> decode_hello(
    std::span<const std::byte> payload);

} // namespace linkscreen::protocol

