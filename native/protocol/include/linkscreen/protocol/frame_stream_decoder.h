#pragma once

#include "linkscreen/protocol/frame.h"

#include <cstddef>
#include <optional>
#include <span>
#include <vector>

namespace linkscreen::protocol {

inline constexpr std::size_t kMaximumBufferedControlBytes =
    2 * (kHeaderSize + kMaximumPayloadSize);

enum class StreamDecodeStatus {
    need_more_data,
    frame_ready,
    protocol_error,
};

struct StreamDecodeResult {
    StreamDecodeStatus status{StreamDecodeStatus::need_more_data};
    std::optional<Frame> frame;
};

// Reassembles complete LinkScreen frames from arbitrary TCP receive chunks.
// After a protocol error the decoder remains failed and must be discarded.
class FrameStreamDecoder {
public:
    [[nodiscard]] bool append(std::span<const std::byte> bytes);
    [[nodiscard]] StreamDecodeResult next();
    [[nodiscard]] std::size_t buffered_bytes() const noexcept;
    [[nodiscard]] bool failed() const noexcept;

private:
    std::vector<std::byte> buffer_;
    bool failed_{false};
};

} // namespace linkscreen::protocol

