#include "linkscreen/protocol/message_header.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string_view>

namespace {

int failures = 0;

void expect(bool condition, std::string_view description) {
    if (!condition) {
        std::cerr << "FAILED: " << description << '\n';
        ++failures;
    }
}

void encoded_header_has_stable_wire_format() {
    const linkscreen::protocol::MessageHeader header{
        .magic = linkscreen::protocol::kMagic,
        .version = linkscreen::protocol::kProtocolVersion,
        .type = linkscreen::protocol::MessageType::heartbeat,
        .payload_size = 0x00010203U,
        .sequence = 0x10203040U,
    };

    const auto actual = linkscreen::protocol::encode_header(header);
    const std::array<std::byte, linkscreen::protocol::kHeaderSize> expected{
        std::byte{0x4C}, std::byte{0x53}, std::byte{0x4E}, std::byte{0x4B},
        std::byte{0x00}, std::byte{0x01},
        std::byte{0x00}, std::byte{0x03},
        std::byte{0x00}, std::byte{0x01}, std::byte{0x02}, std::byte{0x03},
        std::byte{0x10}, std::byte{0x20}, std::byte{0x30}, std::byte{0x40},
    };

    expect(actual == expected, "encode_header uses the documented wire format");
}

void encoded_header_can_be_decoded() {
    const linkscreen::protocol::MessageHeader expected{
        .magic = linkscreen::protocol::kMagic,
        .version = linkscreen::protocol::kProtocolVersion,
        .type = linkscreen::protocol::MessageType::hello_ack,
        .payload_size = 128,
        .sequence = 42,
    };

    const auto bytes = linkscreen::protocol::encode_header(expected);
    const auto actual = linkscreen::protocol::decode_header(bytes);

    expect(actual.has_value(), "decode_header accepts a valid header");
    expect(actual.has_value() && *actual == expected,
           "decode_header preserves every field");
}

void malformed_headers_are_rejected() {
    const linkscreen::protocol::MessageHeader valid{};
    auto bytes = linkscreen::protocol::encode_header(valid);

    expect(!linkscreen::protocol::decode_header(
                std::span<const std::byte>{bytes}.first(bytes.size() - 1))
                .has_value(),
           "decode_header rejects an incorrect byte count");

    bytes[0] = std::byte{0x00};
    expect(!linkscreen::protocol::decode_header(bytes).has_value(),
           "decode_header rejects an invalid magic value");

    bytes = linkscreen::protocol::encode_header(valid);
    bytes[5] = std::byte{0x02};
    expect(!linkscreen::protocol::decode_header(bytes).has_value(),
           "decode_header rejects an unsupported protocol version");

    bytes = linkscreen::protocol::encode_header(valid);
    bytes[6] = std::byte{0x7F};
    bytes[7] = std::byte{0xFF};
    expect(!linkscreen::protocol::decode_header(bytes).has_value(),
           "decode_header rejects an unknown message type");

    bytes = linkscreen::protocol::encode_header(valid);
    bytes[8] = std::byte{0x00};
    bytes[9] = std::byte{0x10};
    bytes[10] = std::byte{0x00};
    bytes[11] = std::byte{0x01};
    expect(!linkscreen::protocol::decode_header(bytes).has_value(),
           "decode_header rejects a payload larger than 1 MiB");
}

void oversized_payload_cannot_be_encoded() {
    const linkscreen::protocol::MessageHeader invalid{
        .payload_size = linkscreen::protocol::kMaximumPayloadSize + 1,
    };

    bool rejected = false;
    try {
        (void)linkscreen::protocol::encode_header(invalid);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }

    expect(rejected, "encode_header rejects a payload larger than 1 MiB");
}

} // namespace

int main() {
    encoded_header_has_stable_wire_format();
    encoded_header_can_be_decoded();
    malformed_headers_are_rejected();
    oversized_payload_cannot_be_encoded();

    if (failures == 0) {
        std::cout << "All protocol tests passed.\n";
        return 0;
    }

    std::cerr << failures << " protocol test(s) failed.\n";
    return 1;
}
