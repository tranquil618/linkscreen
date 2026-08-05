#include "linkscreen/protocol/hello_message.h"

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

int failures = 0;

void expect(bool condition, std::string_view description) {
    if (!condition) {
        std::cerr << "FAILED: " << description << '\n';
        ++failures;
    }
}

linkscreen::protocol::HelloMessage example_message() {
    using namespace linkscreen::protocol;

    return HelloMessage{
        .device_kind = DeviceKind::harmony_tablet,
        .capabilities = capability::touch
                      | capability::pen
                      | capability::h264_decode,
        .device_name = "MatePad",
    };
}

std::vector<std::byte> example_wire_payload() {
    return {
        std::byte{0x01},
        std::byte{0x00},
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x0D},
        std::byte{0x00}, std::byte{0x07},
        std::byte{'M'}, std::byte{'a'}, std::byte{'t'}, std::byte{'e'},
        std::byte{'P'}, std::byte{'a'}, std::byte{'d'},
    };
}

void hello_has_stable_wire_format() {
    const auto actual = linkscreen::protocol::encode_hello(example_message());
    const auto expected = example_wire_payload();

    expect(actual.size() == expected.size(),
           "encode_hello produces the expected payload size");
    expect(actual.size() == expected.size()
               && std::equal(actual.begin(), actual.end(), expected.begin()),
           "encode_hello uses the documented wire format");
}

void hello_round_trip_preserves_every_field() {
    const auto expected = example_message();
    const auto bytes = linkscreen::protocol::encode_hello(expected);
    const auto actual = linkscreen::protocol::decode_hello(bytes);

    expect(actual.has_value(), "decode_hello accepts a valid payload");
    expect(actual.has_value() && *actual == expected,
           "Hello encode/decode preserves every field");
}

void malformed_hello_payloads_are_rejected() {
    auto bytes = example_wire_payload();

    expect(!linkscreen::protocol::decode_hello(
                std::span<const std::byte>{bytes}.first(7)).has_value(),
           "decode_hello rejects a payload shorter than its fixed fields");

    bytes = example_wire_payload();
    bytes[0] = std::byte{0x7F};
    expect(!linkscreen::protocol::decode_hello(bytes).has_value(),
           "decode_hello rejects an unknown device kind");

    bytes = example_wire_payload();
    bytes[1] = std::byte{0x01};
    expect(!linkscreen::protocol::decode_hello(bytes).has_value(),
           "decode_hello rejects a nonzero reserved byte");

    bytes = example_wire_payload();
    bytes[7] = std::byte{0x08};
    expect(!linkscreen::protocol::decode_hello(bytes).has_value(),
           "decode_hello rejects a mismatched device-name length");
}

void invalid_hello_messages_cannot_be_encoded() {
    auto invalid = example_message();
    invalid.device_name.clear();

    bool empty_name_rejected = false;
    try {
        (void)linkscreen::protocol::encode_hello(invalid);
    } catch (const std::invalid_argument&) {
        empty_name_rejected = true;
    }
    expect(empty_name_rejected, "encode_hello rejects an empty device name");

    invalid = example_message();
    invalid.device_name = std::string(
        linkscreen::protocol::kMaximumDeviceNameSize + 1, 'x');

    bool long_name_rejected = false;
    try {
        (void)linkscreen::protocol::encode_hello(invalid);
    } catch (const std::invalid_argument&) {
        long_name_rejected = true;
    }
    expect(long_name_rejected, "encode_hello rejects an oversized device name");
}

} // namespace

int main() {
    hello_has_stable_wire_format();
    hello_round_trip_preserves_every_field();
    malformed_hello_payloads_are_rejected();
    invalid_hello_messages_cannot_be_encoded();

    if (failures == 0) {
        std::cout << "All Hello-message tests passed.\n";
        return 0;
    }

    std::cerr << failures << " Hello-message test(s) failed.\n";
    return 1;
}
