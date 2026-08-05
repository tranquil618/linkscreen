#include "linkscreen/protocol/frame.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace linkscreen::protocol {

std::vector<std::byte> encode_frame(
    MessageType type,
    std::uint32_t sequence,
    std::span<const std::byte> payload)
{
    if (payload.size() > kMaximumPayloadSize
        || payload.size() > std::numeric_limits<std::uint32_t>::max()) {
        throw std::invalid_argument("frame payload exceeds protocol limit");
    }

    const MessageHeader header{
        .magic = kMagic,
        .version = kProtocolVersion,
        .type = type,
        .payload_size = static_cast<std::uint32_t>(payload.size()),
        .sequence = sequence,
    };
    const auto encoded_header = encode_header(header);

    std::vector<std::byte> frame(kHeaderSize + payload.size());
    std::copy(encoded_header.begin(), encoded_header.end(), frame.begin());
    std::copy(payload.begin(), payload.end(), frame.begin() + kHeaderSize);
    return frame;
}

std::optional<Frame> decode_frame(std::span<const std::byte> bytes) {
    if (bytes.size() < kHeaderSize) {
        return std::nullopt;
    }

    const auto header = decode_header(bytes.first(kHeaderSize));
    if (!header.has_value()
        || bytes.size() != kHeaderSize + header->payload_size) {
        return std::nullopt;
    }

    const auto payload = bytes.subspan(kHeaderSize);
    return Frame{
        .header = *header,
        .payload = std::vector<std::byte>{payload.begin(), payload.end()},
    };
}

} // namespace linkscreen::protocol

