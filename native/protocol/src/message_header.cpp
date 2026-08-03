#include "linkscreen/protocol/message_header.h"
#include <stdexcept>

namespace {
// 将 16 位无符号整数按网络字节序（大端序）写入 output。
// 调用方负责保证从 offset 开始至少还有 2 个可写字节。
void write_u16(
    std::span<std::byte> output,
    std::size_t offset,
    std::uint16_t value)
{
    // 高位字节先写入，低位字节后写入。例如 0x1234 编码为 12 34。
    output[offset] = static_cast<std::byte>(value >> 8);
    output[offset + 1] = static_cast<std::byte>(value);
}

// 将 32 位无符号整数按网络字节序（大端序）写入 output。
// 调用方负责保证从 offset 开始至少还有 4 个可写字节。
void write_u32(
    std::span<std::byte> output,
    std::size_t offset,
    std::uint32_t value)
{
    // 每次右移取出一个字节，并按照从最高位到最低位的顺序写入。
    output[offset] = static_cast<std::byte>(value >> 24);
    output[offset + 1] = static_cast<std::byte>(value >> 16);
    output[offset + 2] = static_cast<std::byte>(value >> 8);
    output[offset + 3] = static_cast<std::byte>(value);
}

// 从 input 的指定偏移读取 2 个大端序字节，并还原为 16 位无符号整数。
// 调用方必须先验证输入长度，确保读取不会越界。
[[nodiscard]] std::uint16_t read_u16(
    std::span<const std::byte> input,
    std::size_t offset)
{
    const auto high = std::to_integer<std::uint16_t>(input[offset]);
    const auto low = std::to_integer<std::uint16_t>(input[offset + 1]);

    // 将高位字节移到 bit 8～15，再与低位字节合并。
    return static_cast<std::uint16_t>((high << 8) | low);
}

// 从 input 的指定偏移读取 4 个大端序字节，并还原为 32 位无符号整数。
// 中间值使用 uint32_t，避免按有符号 int 进行高位左移。
[[nodiscard]] std::uint32_t read_u32(
    std::span<const std::byte> input,
    std::size_t offset)
{
    const auto first = std::to_integer<std::uint32_t>(input[offset]);
    const auto second = std::to_integer<std::uint32_t>(input[offset + 1]);
    const auto third = std::to_integer<std::uint32_t>(input[offset + 2]);
    const auto fourth = std::to_integer<std::uint32_t>(input[offset + 3]);

    // 将四个字节放回各自的位区间，再通过按位或组合成完整整数。
    return (first << 24) | (second << 16) | (third << 8) | fourth;
}

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
    if(header.payload_size>kMaximumPayloadSize) {
        throw std::invalid_argument("payload_size exceeds maximum allowed size");
    }
    
    std::array<std::byte,kHeaderSize>bytes{};
    write_u32(bytes,0,header.magic);
    write_u16(bytes,4,header.version);
    write_u16(bytes,6,static_cast<std::uint16_t>(header.type));
    write_u32(bytes,8,header.payload_size);
    write_u32(bytes,12,header.sequence);
    return bytes;
}

std::optional<MessageHeader> decode_header(std::span<const std::byte> bytes) {
    if(bytes.size() != kHeaderSize) {
        return std::nullopt;
    }

    MessageHeader header;
    header.magic       =read_u32(bytes,0);
    header.version     =read_u16(bytes,4);
    header.type        =static_cast<MessageType>(read_u16(bytes,6));
    header.payload_size=read_u32(bytes,8);
    header.sequence    =read_u32(bytes,12);
    
    if(header.magic!=kMagic){
        return std::nullopt;
    }
    if(header.version!=kProtocolVersion){
        return std::nullopt;
    }
    if(header.payload_size>kMaximumPayloadSize){
        return std::nullopt;
    }
    if(!is_known_message_type(header.type)){
        return std::nullopt;
    }
    return header;
}

} // namespace linkscreen::protocol
