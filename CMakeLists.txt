cmake_minimum_required(VERSION 3.0.0)
project(xfyun_asr VERSION 0.0.0)

# pkg-config 是必须的!
find_package(PkgConfig REQUIRED)
# 导出 Clang compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# figure out the version string
INCLUDE(${CMAKE_HOME_DIRECTORY}/script/cmake/version_string.cmake)

# 要引入的自定义 CMake 脚本
set(CMAKE_MODULE_PATH ${CMAKE_HOME_DIRECTORY}/script/cmake)

# uniMRCP
IF(NOT DEFINED ENV{UNIMRCP_SRC_DIR})
    set(UNIMRCP_SRC_DIR "${CMAKE_HOME_DIRECTORY}/../deps/unimrcp-1.5.0")
    message("Environment variable `UNIMRCP_SRC_DIR` not defined. Consider it is ${UNIMRCP_SRC_DIR}")
ELSE()
    set(UNIMRCP_SRC_DIR $ENV{UNIMRCP_SRC_DIR})
ENDIF()
set (MRCP_INCLUDE_DIRS
    ${UNIMRCP_SRC_DIR}/libs/mrcp-engine/include
    ${UNIMRCP_SRC_DIR}/libs/mrcp-engine/include
    ${UNIMRCP_SRC_DIR}/libs/mrcp/include
    ${UNIMRCP_SRC_DIR}/libs/mrcp/message/include
    ${UNIMRCP_SRC_DIR}/libs/mrcp/control/include
    ${UNIMRCP_SRC_DIR}/libs/mrcp/resources/include
    ${UNIMRCP_SRC_DIR}/libs/mpf/include
    ${UNIMRCP_SRC_DIR}/libs/apr-toolkit/include
)
include_directories(${MRCP_INCLUDE_DIRS})

# uniMRCP plugin dependencies:
IF(DEFINED ENV{APR_INCLUDE_DIR})
	set(APR_INCLUDE_DIR $ENV{APR_INCLUDE_DIR})
	set(APRUTIL_INCLUDE_DIR $ENV{APR_INCLUDE_DIR})
ELSE()
    message("Environment variable `APR_INCLUDE_DIR` not defined, try find pacakge APR ...")
	find_package(APR REQUIRED)
ENDIF()
# include: apr, apr-utils
set (PLUGIN_INCLUDE_DIRS
    ${APR_INCLUDE_DIR}
    ${APRUTIL_INCLUDE_DIR}
)
include_directories(${PLUGIN_INCLUDE_DIRS})

# 讯飞 MSC SDK
IF(NOT DEFINED ENV{XFYUN_MSC_SDK_DIR})
    set(XFYUN_MSC_SDK_DIR "${CMAKE_HOME_DIRECTORY}/../deps/xfyun-msc-Linux_iat1166_5acb316c")
    message("Environment variable `XFYUN_MSC_SDK_DIR` not defined. Consider it is ${XFYUN_MSC_SDK_DIR}")
ELSE()
    set(XFYUN_MSC_SDK_DIR $ENV{XFYUN_MSC_SDK_DIR})
ENDIF()

set (XFYUN_MSC_SDK_INCLUDE_DIRS ${XFYUN_MSC_SDK_DIR}/include)
include_directories(${XFYUN_MSC_SDK_INCLUDE_DIRS})

if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "x86_64")
    message(STATUS "add link-dir: ${XFYUN_MSC_SDK_DIR}/libs/x64")
    link_directories(${XFYUN_MSC_SDK_DIR}/libs/x64)
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "x86")
    message(STATUS "add link-dir: ${XFYUN_MSC_SDK_DIR}/libs/x86")
    link_directories(${XFYUN_MSC_SDK_DIR}/libs/x86)
else()
    message(FATAL_ERROR "Un-supported prosessor: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
endif()
target_link_libraries(${PROJECT_NAME} msc)

# CTEST
include(CTest)
enable_testing()

# 该插件的共享库项目
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

# Source files
set (SRC_ROOT_DIR ${CMAKE_HOME_DIRECTORY}/src)
add_library(${PROJECT_NAME} SHARED
    ${SRC_ROOT_DIR}/xfyun_asr.c
)

