#pragma once

#include <cstdint>

namespace linkscreen::windows {

inline constexpr std::uint16_t kDevelopmentControlPort = 47831;

// Development server: accepts one loopback client, performs one Hello
// handshake, sends HelloAck, and exits. LAN exposure is intentionally disabled
// until pairing and authenticated encryption are implemented.
class HostServer {
public:
    [[nodiscard]] bool run_once();
};

} // namespace linkscreen::windows

