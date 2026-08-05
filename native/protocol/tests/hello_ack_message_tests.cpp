#include "linkscreen/protocol/hello_ack_message.h"
#include "linkscreen/protocol/hello_message.h"
#include "linkscreen/protocol/message_header.h"

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

int failures = 0;

void expect(bool condition, std::string_view description) {
    if (!condition) {
        std::cerr << "FAILED: " << description << '\n';
        ++failures;
    }
}

linkscreen::protocol::HelloAckMessage accepted_message() {
    using namespace linkscreen::protocol;
    return HelloAckMessage{
        .status = HelloAckStatus::accepted,
        .selected_version = kProtocolVersion,
        .selected_capabilities = capability::touch | capability::h264_decode,
        .session_id = 0x0102030405060708ULL,
        .host_name = "Studio-PC",
    };
}

std::vector<std::byte> accepted_wire_payload() {
    return {
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x01},
        std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x09},
        std::byte{0x01}, std::byte{0x02}, std::byte{0x03}, std::byte{0x04},
        std::byte{0x05}, std::byte{0x06}, std::byte{0x07}, std::byte{0x08},
        std::byte{0x00}, std::byte{0x09},
        std::byte{'S'}, std::byte{'t'}, std::byte{'u'}, std::byte{'d'},
        std::byte{'i'}, std::byte{'o'}, std::byte{'-'}, std::byte{'P'},
        std::byte{'C'},
    };
}

void accepted_ack_has_stable_wire_format() {
    const auto actual = linkscreen::protocol::encode_hello_ack(accepted_message());
    const auto expected = accepted_wire_payload();
    expect(actual == expected, "HelloAck uses the documented wire format");
}

void accepted_ack_round_trip_preserves_every_field() {
    const auto expected = accepted_message();
    const auto bytes = linkscreen::protocol::encode_hello_ack(expected);
    const auto actual = linkscreen::protocol::decode_hello_ack(bytes);
    expect(actual.has_value(), "decode_hello_ack accepts a valid response");
    expect(actual.has_value() && *actual == expected,
           "HelloAck encode/decode preserves every field");
}

void rejection_ack_round_trip_is_supported() {
    const linkscreen::protocol::HelloAckMessage expected{
        .status = linkscreen::protocol::HelloAckStatus::unsupported_version,
        .selected_version = 0,
        .selected_capabilities = 0,
        .session_id = 0,
        .host_name = "Studio-PC",
    };
    const auto bytes = linkscreen::protocol::encode_hello_ack(expected);
    const auto actual = linkscreen::protocol::decode_hello_ack(bytes);
    expect(actual.has_value() && *actual == expected,
           "HelloAck supports a well-formed rejection response");
}

void malformed_ack_payloads_are_rejected() {
    auto bytes = accepted_wire_payload();
    expect(!linkscreen::protocol::decode_hello_ack(
                std::span<const std::byte>{bytes}.first(17)).has_value(),
           "HelloAck rejects a truncated fixed section");

    bytes = accepted_wire_payload();
    bytes[0] = std::byte{0x7F};
    expect(!linkscreen::protocol::decode_hello_ack(bytes).has_value(),
           "HelloAck rejects an unknown result status");

    bytes = accepted_wire_payload();
    bytes[1] = std::byte{0x01};
    expect(!linkscreen::protocol::decode_hello_ack(bytes).has_value(),
           "HelloAck rejects a nonzero reserved byte");

    bytes = accepted_wire_payload();
    bytes[17] = std::byte{0x0A};
    expect(!linkscreen::protocol::decode_hello_ack(bytes).has_value(),
           "HelloAck rejects a mismatched host-name length");

    bytes = accepted_wire_payload();
    std::fill(bytes.begin() + 8, bytes.begin() + 16, std::byte{0});
    expect(!linkscreen::protocol::decode_hello_ack(bytes).has_value(),
           "an accepted HelloAck requires a nonzero session id");
}

void inconsistent_ack_cannot_be_encoded() {
    auto invalid = accepted_message();
    invalid.status = linkscreen::protocol::HelloAckStatus::unsupported_version;

    bool rejected = false;
    try {
        (void)linkscreen::protocol::encode_hello_ack(invalid);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    expect(rejected, "a rejection HelloAck cannot contain accepted fields");
}

} // namespace

int main() {
    accepted_ack_has_stable_wire_format();
    accepted_ack_round_trip_preserves_every_field();
    rejection_ack_round_trip_is_supported();
    malformed_ack_payloads_are_rejected();
    inconsistent_ack_cannot_be_encoded();

    if (failures == 0) {
        std::cout << "All HelloAck-message tests passed.\n";
        return 0;
    }

    std::cerr << failures << " HelloAck-message test(s) failed.\n";
    return 1;
}
