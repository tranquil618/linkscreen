#pragma once

#include "linkscreen/protocol/message_header.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace linkscreen::protocol {

struct Frame {
    MessageHeader header;
    std::vector<std::byte> payload;

    bool operator==(const Frame&) const = default;
};

// Creates one complete control-channel frame: fixed header followed by payload.
[[nodiscard]] std::vector<std::byte> encode_frame(
    MessageType type,
    std::uint32_t sequence,
    std::span<const std::byte> payload);

// Decodes exactly one complete frame. Truncated input and trailing bytes are
// rejected so callers cannot accidentally merge two protocol messages.
[[nodiscard]] std::optional<Frame> decode_frame(
    std::span<const std::byte> bytes);

} // namespace linkscreen::protocol

