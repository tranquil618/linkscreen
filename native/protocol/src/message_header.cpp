#include "linkscreen/protocol/message_header.h"

#include "byte_io.h"

#include <stdexcept>

namespace {

[[nodiscard]] bool is_known_message_type(
    linkscreen::protocol::MessageType type)
{
    using linkscreen::protocol::MessageType;

    switch (type) {
    case MessageType::hello:
    case MessageType::hello_ack:
    case MessageType::heartbeat:
    case MessageType::heartbeat_ack:
        return true;
    }

    return false;
}

} // namespace

namespace linkscreen::protocol {

std::array<std::byte, kHeaderSize> encode_header(const MessageHeader& header) {
    if (header.magic != kMagic
        || header.version != kProtocolVersion
        || !is_known_message_type(header.type)
        || header.payload_size > kMaximumPayloadSize) {
        throw std::invalid_argument("invalid message header");
    }

    std::array<std::byte, kHeaderSize> bytes{};
    detail::write_u32(bytes, 0, header.magic);
    detail::write_u16(bytes, 4, header.version);
    detail::write_u16(bytes, 6, static_cast<std::uint16_t>(header.type));
    detail::write_u32(bytes, 8, header.payload_size);
    detail::write_u32(bytes, 12, header.sequence);
    return bytes;
}

std::optional<MessageHeader> decode_header(std::span<const std::byte> bytes) {
    if (bytes.size() != kHeaderSize) {
        return std::nullopt;
    }

    MessageHeader header;
    header.magic = detail::read_u32(bytes, 0);
    header.version = detail::read_u16(bytes, 4);
    header.type = static_cast<MessageType>(detail::read_u16(bytes, 6));
    header.payload_size = detail::read_u32(bytes, 8);
    header.sequence = detail::read_u32(bytes, 12);

    if (header.magic != kMagic
        || header.version != kProtocolVersion
        || !is_known_message_type(header.type)
        || header.payload_size > kMaximumPayloadSize) {
        return std::nullopt;
    }

    return header;
}

} // namespace linkscreen::protocol
