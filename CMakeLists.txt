cmake_minimum_required(VERSION 3.20)

project(LinkScreen VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

add_subdirectory(native/protocol)

if(WIN32)
    add_subdirectory(native/windows-engine)
endif()
