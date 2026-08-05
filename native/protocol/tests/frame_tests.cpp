#include "linkscreen/protocol/frame.h"
#include "linkscreen/protocol/hello_message.h"

#include <cstddef>
#include <iostream>
#include <span>
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

std::vector<std::byte> hello_payload() {
    using namespace linkscreen::protocol;
    return encode_hello(HelloMessage{
        .device_kind = DeviceKind::harmony_tablet,
        .capabilities = capability::touch | capability::h264_decode,
        .device_name = "MatePad",
    });
}

void complete_frame_round_trip() {
    using namespace linkscreen::protocol;
    const auto payload = hello_payload();
    const auto bytes = encode_frame(MessageType::hello, 42, payload);
    const auto frame = decode_frame(bytes);

    expect(bytes.size() == kHeaderSize + payload.size(),
           "encoded frame contains one header and one payload");
    expect(frame.has_value(), "decode_frame accepts a complete frame");
    expect(frame.has_value() && frame->header.type == MessageType::hello,
           "frame preserves the message type");
    expect(frame.has_value() && frame->header.sequence == 42,
           "frame preserves the sequence number");
    expect(frame.has_value() && frame->payload == payload,
           "frame preserves the payload bytes");
}

void incomplete_or_ambiguous_frames_are_rejected() {
    using namespace linkscreen::protocol;
    auto bytes = encode_frame(MessageType::hello, 1, hello_payload());

    expect(!decode_frame(std::span<const std::byte>{bytes}.first(15)).has_value(),
           "decode_frame rejects a truncated header");
    expect(!decode_frame(std::span<const std::byte>{bytes}.first(bytes.size() - 1))
                .has_value(),
           "decode_frame rejects a truncated payload");

    bytes.push_back(std::byte{0});
    expect(!decode_frame(bytes).has_value(),
           "decode_frame rejects trailing bytes");
}

} // namespace

int main() {
    complete_frame_round_trip();
    incomplete_or_ambiguous_frames_are_rejected();

    if (failures == 0) {
        std::cout << "All frame tests passed.\n";
        return 0;
    }

    std::cerr << failures << " frame test(s) failed.\n";
    return 1;
}

