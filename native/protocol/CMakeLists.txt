add_library(linkscreen_protocol STATIC
    src/frame.cpp
    src/frame_stream_decoder.cpp
    src/hello_ack_message.cpp
    src/hello_message.cpp
    src/message_header.cpp
)

target_include_directories(linkscreen_protocol
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(linkscreen_protocol PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(linkscreen_protocol PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(linkscreen_protocol PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BUILD_TESTING)
    add_executable(linkscreen_protocol_tests
        tests/message_header_tests.cpp
    )

    target_link_libraries(linkscreen_protocol_tests PRIVATE linkscreen_protocol)
    target_compile_features(linkscreen_protocol_tests PRIVATE cxx_std_20)

    if(MSVC)
        target_compile_options(linkscreen_protocol_tests PRIVATE /W4 /permissive- /utf-8)
    else()
        target_compile_options(linkscreen_protocol_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME linkscreen_protocol_tests COMMAND linkscreen_protocol_tests)

    add_executable(linkscreen_hello_message_tests
        tests/hello_message_tests.cpp
    )

    target_link_libraries(linkscreen_hello_message_tests PRIVATE linkscreen_protocol)
    target_compile_features(linkscreen_hello_message_tests PRIVATE cxx_std_20)

    if(MSVC)
        target_compile_options(linkscreen_hello_message_tests PRIVATE /W4 /permissive- /utf-8)
    else()
        target_compile_options(linkscreen_hello_message_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME linkscreen_hello_message_tests COMMAND linkscreen_hello_message_tests)

    add_executable(linkscreen_hello_ack_message_tests
        tests/hello_ack_message_tests.cpp
    )

    target_link_libraries(linkscreen_hello_ack_message_tests PRIVATE linkscreen_protocol)
    target_compile_features(linkscreen_hello_ack_message_tests PRIVATE cxx_std_20)

    if(MSVC)
        target_compile_options(linkscreen_hello_ack_message_tests PRIVATE /W4 /permissive- /utf-8)
    else()
        target_compile_options(linkscreen_hello_ack_message_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME linkscreen_hello_ack_message_tests
        COMMAND linkscreen_hello_ack_message_tests)

    add_executable(linkscreen_frame_tests
        tests/frame_tests.cpp
    )

    target_link_libraries(linkscreen_frame_tests PRIVATE linkscreen_protocol)
    target_compile_features(linkscreen_frame_tests PRIVATE cxx_std_20)

    if(MSVC)
        target_compile_options(linkscreen_frame_tests PRIVATE /W4 /permissive- /utf-8)
    else()
        target_compile_options(linkscreen_frame_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME linkscreen_frame_tests COMMAND linkscreen_frame_tests)

    add_executable(linkscreen_frame_stream_decoder_tests
        tests/frame_stream_decoder_tests.cpp
    )

    target_link_libraries(
        linkscreen_frame_stream_decoder_tests PRIVATE linkscreen_protocol)
    target_compile_features(
        linkscreen_frame_stream_decoder_tests PRIVATE cxx_std_20)

    if(MSVC)
        target_compile_options(
            linkscreen_frame_stream_decoder_tests PRIVATE /W4 /permissive- /utf-8)
    else()
        target_compile_options(
            linkscreen_frame_stream_decoder_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME linkscreen_frame_stream_decoder_tests
        COMMAND linkscreen_frame_stream_decoder_tests)
endif()
