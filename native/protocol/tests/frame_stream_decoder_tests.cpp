#include "linkscreen/protocol/frame_stream_decoder.h"

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

std::vector<std::byte> make_frame(
    linkscreen::protocol::MessageType type,
    std::uint32_t sequence,
    std::initializer_list<std::byte> payload)
{
    return linkscreen::protocol::encode_frame(
        type,
        sequence,
        std::span<const std::byte>{payload.begin(), payload.size()});
}

void split_frame_is_reassembled() {
    using namespace linkscreen::protocol;
    const auto bytes = make_frame(
        MessageType::heartbeat, 7, {std::byte{0x12}, std::byte{0x34}});
    FrameStreamDecoder decoder;

    expect(decoder.append(std::span<const std::byte>{bytes}.first(5)),
           "decoder accepts the first TCP chunk");
    expect(decoder.next().status == StreamDecodeStatus::need_more_data,
           "partial header needs more data");

    expect(decoder.append(std::span<const std::byte>{bytes}.subspan(5, 12)),
           "decoder accepts a chunk crossing the header boundary");
    expect(decoder.next().status == StreamDecodeStatus::need_more_data,
           "partial payload needs more data");

    expect(decoder.append(std::span<const std::byte>{bytes}.subspan(17)),
           "decoder accepts the final TCP chunk");
    const auto result = decoder.next();
    expect(result.status == StreamDecodeStatus::frame_ready,
           "complete bytes produce one frame");
    expect(result.frame.has_value() && result.frame->header.sequence == 7,
           "reassembled frame preserves its sequence");
    expect(decoder.buffered_bytes() == 0,
           "consumed frame bytes are removed from the buffer");
}

void multiple_frames_in_one_chunk_are_separated() {
    using namespace linkscreen::protocol;
    auto first = make_frame(MessageType::heartbeat, 10, {});
    const auto second = make_frame(MessageType::heartbeat_ack, 11, {});
    first.insert(first.end(), second.begin(), second.end());

    FrameStreamDecoder decoder;
    expect(decoder.append(first), "decoder accepts two coalesced frames");
    const auto first_result = decoder.next();
    const auto second_result = decoder.next();
    expect(first_result.frame.has_value()
               && first_result.frame->header.sequence == 10,
           "decoder returns the first coalesced frame");
    expect(second_result.frame.has_value()
               && second_result.frame->header.sequence == 11,
           "decoder returns the second coalesced frame");
    expect(decoder.next().status == StreamDecodeStatus::need_more_data,
           "decoder waits after all complete frames are consumed");
}

void invalid_header_permanently_fails_decoder() {
    using namespace linkscreen::protocol;
    auto bytes = make_frame(MessageType::heartbeat, 1, {});
    bytes[0] = std::byte{0};

    FrameStreamDecoder decoder;
    expect(decoder.append(bytes), "invalid bytes can enter the receive buffer");
    expect(decoder.next().status == StreamDecodeStatus::protocol_error,
           "invalid header produces a protocol error");
    expect(decoder.failed(), "decoder remains failed after malformed input");
    expect(!decoder.append(bytes), "failed decoder rejects subsequent input");
}

} // namespace

int main() {
    split_frame_is_reassembled();
    multiple_frames_in_one_chunk_are_separated();
    invalid_header_permanently_fails_decoder();

    if (failures == 0) {
        std::cout << "All frame-stream decoder tests passed.\n";
        return 0;
    }

    std::cerr << failures << " frame-stream decoder test(s) failed.\n";
    return 1;
}

