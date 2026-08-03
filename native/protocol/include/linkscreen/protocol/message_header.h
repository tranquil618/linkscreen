#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace linkscreen::protocol {

inline constexpr std::uint32_t kMagic = 0x4C534E4BU; // ASCII: LSNK
inline constexpr std::uint16_t kProtocolVersion = 1;
inline constexpr std::size_t kHeaderSize = 16;
inline constexpr std::uint32_t kMaximumPayloadSize = 1024U * 1024U;

enum class MessageType : std::uint16_t {
    hello = 1,
    hello_ack = 2,
    heartbeat = 3,
    heartbeat_ack = 4,
};

struct MessageHeader {
    std::uint32_t magic{kMagic};
    std::uint16_t version{kProtocolVersion};
    MessageType type{MessageType::hello};
    std::uint32_t payload_size{0};
    std::uint32_t sequence{0};

    bool operator==(const MessageHeader&) const = default;
};

// Serializes every integer in network byte order (big-endian).
[[nodiscard]] std::array<std::byte, kHeaderSize> encode_header(
    const MessageHeader& header);

// Returns std::nullopt when the byte count, magic, version, message type, or
// payload size is invalid.
[[nodiscard]] std::optional<MessageHeader> decode_header(
    std::span<const std::byte> bytes);

} // namespace linkscreen::protocol

