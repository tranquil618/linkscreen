add_library(linkscreen_windows_network STATIC
    src/host_server.cpp
    src/windows_socket.cpp
)

target_include_directories(linkscreen_windows_network
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(linkscreen_windows_network
    PUBLIC
        linkscreen_protocol
    PRIVATE
        bcrypt
        ws2_32
)

target_compile_features(linkscreen_windows_network PUBLIC cxx_std_20)
target_compile_definitions(linkscreen_windows_network PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(linkscreen_windows_network PRIVATE /W4 /permissive- /utf-8)

add_executable(linkscreen_host_cli
    src/host_main.cpp
)
target_link_libraries(linkscreen_host_cli PRIVATE linkscreen_windows_network)
target_compile_features(linkscreen_host_cli PRIVATE cxx_std_20)
target_compile_options(linkscreen_host_cli PRIVATE /W4 /permissive- /utf-8)

add_executable(linkscreen_handshake_client
    src/handshake_client_main.cpp
    src/windows_socket.cpp
)
target_link_libraries(linkscreen_handshake_client
    PRIVATE
        linkscreen_protocol
        ws2_32
)
target_include_directories(linkscreen_handshake_client
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_features(linkscreen_handshake_client PRIVATE cxx_std_20)
target_compile_definitions(linkscreen_handshake_client PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(linkscreen_handshake_client PRIVATE /W4 /permissive- /utf-8)

