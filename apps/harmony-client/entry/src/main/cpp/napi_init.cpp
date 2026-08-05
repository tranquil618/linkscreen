#include "linkscreen/protocol/frame.h"
#include "linkscreen/protocol/hello_ack_message.h"
#include "linkscreen/protocol/hello_message.h"

#include <napi/native_api.h>
#include <node_api.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <limits>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace {

void throw_type_error(napi_env env, const char* message) {
    napi_throw_type_error(env, nullptr, message);
}

[[nodiscard]] bool get_utf8_string(
    napi_env env,
    napi_value value,
    std::string& result)
{
    std::size_t length = 0;
    if (napi_get_value_string_utf8(env, value, nullptr, 0, &length) != napi_ok) {
        return false;
    }

    std::vector<char> buffer(length + 1);
    std::size_t written = 0;
    if (napi_get_value_string_utf8(
            env, value, buffer.data(), buffer.size(), &written) != napi_ok) {
        return false;
    }
    result.assign(buffer.data(), written);
    return true;
}

[[nodiscard]] napi_value encode_hello_frame(
    napi_env env,
    napi_callback_info info)
{
    std::size_t argc = 3;
    napi_value argv[3]{};
    if (napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr) != napi_ok
        || argc != 3) {
        throw_type_error(env, "encodeHelloFrame expects 3 arguments");
        return nullptr;
    }

    std::string device_name;
    std::uint32_t capabilities = 0;
    std::uint32_t sequence = 0;
    if (!get_utf8_string(env, argv[0], device_name)
        || napi_get_value_uint32(env, argv[1], &capabilities) != napi_ok
        || napi_get_value_uint32(env, argv[2], &sequence) != napi_ok) {
        throw_type_error(env, "invalid encodeHelloFrame argument types");
        return nullptr;
    }

    try {
        const linkscreen::protocol::HelloMessage hello{
            .device_kind = linkscreen::protocol::DeviceKind::harmony_tablet,
            .capabilities = capabilities,
            .device_name = std::move(device_name),
        };
        const auto payload = linkscreen::protocol::encode_hello(hello);
        const auto frame = linkscreen::protocol::encode_frame(
            linkscreen::protocol::MessageType::hello,
            sequence,
            payload);

        void* output = nullptr;
        napi_value array_buffer = nullptr;
        if (napi_create_arraybuffer(
                env, frame.size(), &output, &array_buffer) != napi_ok) {
            throw_type_error(env, "failed to allocate Hello frame");
            return nullptr;
        }
        std::memcpy(output, frame.data(), frame.size());
        return array_buffer;
    } catch (const std::exception& error) {
        throw_type_error(env, error.what());
        return nullptr;
    }
}

[[nodiscard]] bool set_uint32_property(
    napi_env env,
    napi_value object,
    const char* name,
    std::uint32_t value)
{
    napi_value number = nullptr;
    return napi_create_uint32(env, value, &number) == napi_ok
        && napi_set_named_property(env, object, name, number) == napi_ok;
}

[[nodiscard]] napi_value decode_hello_ack_frame(
    napi_env env,
    napi_callback_info info)
{
    std::size_t argc = 1;
    napi_value argv[1]{};
    if (napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr) != napi_ok
        || argc != 1) {
        throw_type_error(env, "decodeHelloAckFrame expects one ArrayBuffer");
        return nullptr;
    }

    bool is_array_buffer = false;
    if (napi_is_arraybuffer(env, argv[0], &is_array_buffer) != napi_ok
        || !is_array_buffer) {
        throw_type_error(env, "HelloAck frame must be an ArrayBuffer");
        return nullptr;
    }

    void* data = nullptr;
    std::size_t size = 0;
    if (napi_get_arraybuffer_info(env, argv[0], &data, &size) != napi_ok) {
        throw_type_error(env, "failed to read HelloAck ArrayBuffer");
        return nullptr;
    }

    const auto frame = linkscreen::protocol::decode_frame(
        std::span<const std::byte>{static_cast<const std::byte*>(data), size});
    if (!frame.has_value()
        || frame->header.type != linkscreen::protocol::MessageType::hello_ack) {
        throw_type_error(env, "invalid HelloAck frame");
        return nullptr;
    }
    const auto ack = linkscreen::protocol::decode_hello_ack(frame->payload);
    if (!ack.has_value()) {
        throw_type_error(env, "invalid HelloAck payload");
        return nullptr;
    }

    napi_value result = nullptr;
    napi_value session_id = nullptr;
    napi_value host_name = nullptr;
    if (napi_create_object(env, &result) != napi_ok
        || !set_uint32_property(
            env, result, "status", static_cast<std::uint32_t>(ack->status))
        || !set_uint32_property(
            env, result, "selectedVersion", ack->selected_version)
        || !set_uint32_property(
            env, result, "selectedCapabilities", ack->selected_capabilities)
        || napi_create_bigint_uint64(env, ack->session_id, &session_id) != napi_ok
        || napi_set_named_property(env, result, "sessionId", session_id) != napi_ok
        || napi_create_string_utf8(
            env, ack->host_name.data(), ack->host_name.size(), &host_name) != napi_ok
        || napi_set_named_property(env, result, "hostName", host_name) != napi_ok) {
        throw_type_error(env, "failed to create HelloAck result");
        return nullptr;
    }
    return result;
}

[[nodiscard]] napi_value frame_size_from_prefix(
    napi_env env,
    napi_callback_info info)
{
    std::size_t argc = 1;
    napi_value argv[1]{};
    if (napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr) != napi_ok
        || argc != 1) {
        throw_type_error(env, "frameSizeFromPrefix expects one ArrayBuffer");
        return nullptr;
    }

    bool is_array_buffer = false;
    void* data = nullptr;
    std::size_t size = 0;
    if (napi_is_arraybuffer(env, argv[0], &is_array_buffer) != napi_ok
        || !is_array_buffer
        || napi_get_arraybuffer_info(env, argv[0], &data, &size) != napi_ok) {
        throw_type_error(env, "frame prefix must be an ArrayBuffer");
        return nullptr;
    }

    std::uint32_t total_size = 0;
    if (size >= linkscreen::protocol::kHeaderSize) {
        const auto header = linkscreen::protocol::decode_header(
            std::span<const std::byte>{
                static_cast<const std::byte*>(data),
                linkscreen::protocol::kHeaderSize});
        if (!header.has_value()) {
            throw_type_error(env, "invalid frame header");
            return nullptr;
        }
        total_size = static_cast<std::uint32_t>(
            linkscreen::protocol::kHeaderSize + header->payload_size);
    }

    napi_value result = nullptr;
    if (napi_create_uint32(env, total_size, &result) != napi_ok) {
        return nullptr;
    }
    return result;
}

[[nodiscard]] napi_value initialize(napi_env env, napi_value exports) {
    const napi_property_descriptor properties[] = {
        {
            "encodeHelloFrame", nullptr, encode_hello_frame,
            nullptr, nullptr, nullptr, napi_default, nullptr,
        },
        {
            "decodeHelloAckFrame", nullptr, decode_hello_ack_frame,
            nullptr, nullptr, nullptr, napi_default, nullptr,
        },
        {
            "frameSizeFromPrefix", nullptr, frame_size_from_prefix,
            nullptr, nullptr, nullptr, napi_default, nullptr,
        },
    };
    if (napi_define_properties(
            env,
            exports,
            sizeof(properties) / sizeof(properties[0]),
            properties) != napi_ok) {
        return nullptr;
    }
    return exports;
}

} // namespace

NAPI_MODULE(entry, initialize)
