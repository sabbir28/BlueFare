cmake_minimum_required(VERSION 3.22.1)
project("multimic_native")

add_library(
    multimic_native
    SHARED
    opus_jni.cpp
)

find_library(
    log-lib
    log
)

target_link_libraries(
    multimic_native
    ${log-lib}
)
