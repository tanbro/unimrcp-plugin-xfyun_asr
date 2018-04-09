cmake_minimum_required(VERSION 3.0.0)
project(xfyun_asr VERSION 0.0.0)

include(CTest)
enable_testing()

add_library(xfyun_asr SHARED
    src/xfyun_asr.c
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
