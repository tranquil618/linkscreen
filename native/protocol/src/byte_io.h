#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace linkscreen::protocol::detail {

// These helpers intentionally use explicit shifts instead of platform socket
// functions so the wire format behaves identically on Windows and HarmonyOS.
inline void write_u16(
    std::span<std::byte> output,
    std::size_t offset,
    std::uint16_t value)
{
    output[offset] = static_cast<std::byte>(value >> 8);
    output[offset + 1] = static_cast<std::byte>(value);
}

inline void write_u32(
    std::span<std::byte> output,
    std::size_t offset,
    std::uint32_t value)
{
    output[offset] = static_cast<std::byte>(value >> 24);
    output[offset + 1] = static_cast<std::byte>(value >> 16);
    output[offset + 2] = static_cast<std::byte>(value >> 8);
    output[offset + 3] = static_cast<std::byte>(value);
}

inline void write_u64(
    std::span<std::byte> output,
    std::size_t offset,
    std::uint64_t value)
{
    write_u32(output, offset, static_cast<std::uint32_t>(value >> 32));
    write_u32(output, offset + 4, static_cast<std::uint32_t>(value));
}

[[nodiscard]] inline std::uint16_t read_u16(
    std::span<const std::byte> input,
    std::size_t offset)
{
    const auto high = std::to_integer<std::uint16_t>(input[offset]);
    const auto low = std::to_integer<std::uint16_t>(input[offset + 1]);
    return static_cast<std::uint16_t>((high << 8) | low);
}

[[nodiscard]] inline std::uint32_t read_u32(
    std::span<const std::byte> input,
    std::size_t offset)
{
    const auto first = std::to_integer<std::uint32_t>(input[offset]);
    const auto second = std::to_integer<std::uint32_t>(input[offset + 1]);
    const auto third = std::to_integer<std::uint32_t>(input[offset + 2]);
    const auto fourth = std::to_integer<std::uint32_t>(input[offset + 3]);
    return (first << 24) | (second << 16) | (third << 8) | fourth;
}

[[nodiscard]] inline std::uint64_t read_u64(
    std::span<const std::byte> input,
    std::size_t offset)
{
    const auto high = static_cast<std::uint64_t>(read_u32(input, offset));
    const auto low = static_cast<std::uint64_t>(read_u32(input, offset + 4));
    return (high << 32) | low;
}

} // namespace linkscreen::protocol::detail
