cmake_minimum_required(VERSION 3.16)
project(multimic-server CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/third_party
)

set(SERVER_SOURCES
    src/config.cpp
    src/logger.cpp
    src/protocol.cpp
    src/network.cpp
    src/device_manager.cpp
    src/audio_jitter_buffer.cpp
    src/audio_decoder.cpp
    src/audio_mixer.cpp
    src/audio_output.cpp
    src/transport.cpp
    src/audio_receiver.cpp
    src/web_server.cpp
    src/server.cpp
)

find_library(OPUS_LIB opus HINTS /usr/lib/x86_64-linux-gnu)

add_executable(multimic-server src/main.cpp ${SERVER_SOURCES})
target_link_libraries(multimic-server PRIVATE ${OPUS_LIB} dl pthread)

# Unit & Integration Tests
enable_testing()

add_executable(test_protocol tests/test_protocol.cpp src/protocol.cpp)
add_test(NAME test_protocol COMMAND test_protocol)

add_executable(test_jitter_buffer tests/test_jitter_buffer.cpp src/audio_jitter_buffer.cpp src/logger.cpp)
add_test(NAME test_jitter_buffer COMMAND test_jitter_buffer)

add_executable(test_mixer tests/test_mixer.cpp src/audio_mixer.cpp)
add_test(NAME test_mixer COMMAND test_mixer)

add_executable(test_device_manager tests/test_device_manager.cpp src/device_manager.cpp src/logger.cpp)
add_test(NAME test_device_manager COMMAND test_device_manager)

add_executable(test_integration tests/test_integration.cpp ${SERVER_SOURCES})
target_link_libraries(test_integration PRIVATE ${OPUS_LIB} dl pthread)
add_test(NAME test_integration COMMAND test_integration)

# CLI Tool
add_executable(multimic-cli ${CMAKE_CURRENT_SOURCE_DIR}/../tools/multimic-cli/main.cpp src/protocol.cpp)

