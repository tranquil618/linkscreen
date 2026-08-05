#include "linkscreen/protocol/frame_stream_decoder.h"

#include <algorithm>

namespace linkscreen::protocol {

bool FrameStreamDecoder::append(std::span<const std::byte> bytes) {
    if (failed_ || bytes.size() > kMaximumBufferedControlBytes - buffer_.size()) {
        failed_ = true;
        return false;
    }

    buffer_.insert(buffer_.end(), bytes.begin(), bytes.end());
    return true;
}

StreamDecodeResult FrameStreamDecoder::next() {
    if (failed_) {
        return {.status = StreamDecodeStatus::protocol_error};
    }
    if (buffer_.size() < kHeaderSize) {
        return {.status = StreamDecodeStatus::need_more_data};
    }

    const auto header = decode_header(
        std::span<const std::byte>{buffer_}.first(kHeaderSize));
    if (!header.has_value()) {
        failed_ = true;
        return {.status = StreamDecodeStatus::protocol_error};
    }

    const auto frame_size = kHeaderSize + header->payload_size;
    if (buffer_.size() < frame_size) {
        return {.status = StreamDecodeStatus::need_more_data};
    }

    auto frame = decode_frame(
        std::span<const std::byte>{buffer_}.first(frame_size));
    if (!frame.has_value()) {
        failed_ = true;
        return {.status = StreamDecodeStatus::protocol_error};
    }

    buffer_.erase(
        buffer_.begin(),
        buffer_.begin() + static_cast<std::ptrdiff_t>(frame_size));
    return {
        .status = StreamDecodeStatus::frame_ready,
        .frame = std::move(frame),
    };
}

std::size_t FrameStreamDecoder::buffered_bytes() const noexcept {
    return buffer_.size();
}

bool FrameStreamDecoder::failed() const noexcept {
    return failed_;
}

} // namespace linkscreen::protocol

