cmake_minimum_required(VERSION 3.0.0)
project(xfyun_asr VERSION 0.0.0)

# pkg-config 是必须的!
find_package(PkgConfig REQUIRED)
# 导出 Clang compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
# 要引入的自定义 CMake 脚本
set(CMAKE_MODULE_PATH ${CMAKE_HOME_DIRECTORY}/script/cmake)

# uniMRCP plugin dependencies:
IF(DEFINED ENV{APR_INCLUDE_DIR})
	set (APR_INCLUDE_DIR $ENV{APR_INCLUDE_DIR})
	set (APRUTIL_INCLUDE_DIR $ENV{APR_INCLUDE_DIR})
ELSE()
    message(STATUS "Environment variable `APR_INCLUDE_DIR` not defined, try find pacakge APR ...")
	find_package(APR REQUIRED)
ENDIF()

# uniMRCP
IF(NOT DEFINED ENV{UNIMRCP_SRC_DIR})
    message(FATAL_ERROR "Environment variable `UNIMRCP_SRC_DIR` not defined")
ENDIF()
set (MRCP_INCLUDE_DIRS
    $ENV{UNIMRCP_SRC_DIR}/libs/mrcp-engine/include
    $ENV{UNIMRCP_SRC_DIR}/libs/mrcp-engine/include
    $ENV{UNIMRCP_SRC_DIR}/libs/mrcp/include
    $ENV{UNIMRCP_SRC_DIR}/libs/mrcp/message/include
    $ENV{UNIMRCP_SRC_DIR}/libs/mrcp/control/include
    $ENV{UNIMRCP_SRC_DIR}/libs/mrcp/resources/include
    $ENV{UNIMRCP_SRC_DIR}/libs/mpf/include
    $ENV{UNIMRCP_SRC_DIR}/libs/apr-toolkit/include
)

# 讯飞 MSC SDK
IF(NOT DEFINED ENV{XFYUN_MSC_SDK_DIR})
    message(FATAL_ERROR "Environment variable `XFYUN_MSC_SDK_DIR` not defined")
ENDIF()
set (XFYUN_MSC_SDK_INCLUDE_DIRS $ENV{XFYUN_MSC_SDK_DIR}/include)
if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "x86_64")
    message(STATUS "add link-dir: $ENV{XFYUN_MSC_SDK_DIR}/libs/x64")
    link_directories($ENV{XFYUN_MSC_SDK_DIR}/libs/x64)
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "x86")
    message(STATUS "add link-dir: $ENV{XFYUN_MSC_SDK_DIR}/libs/x86")
    link_directories($ENV{XFYUN_MSC_SDK_DIR}/libs/x86)
else()
    message(FATAL_ERROR "Un-supported prosessor: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
endif()

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

# uniMRCP plugin dependencies: apr, apr-utils
set (PLUGIN_INCLUDE_DIRS
    ${APR_INCLUDE_DIR}
    ${APRUTIL_INCLUDE_DIR}
)
include_directories(${PLUGIN_INCLUDE_DIRS})

# uniMRCP
include_directories(${MRCP_INCLUDE_DIRS})

# 讯飞 MSC
include_directories(${XFYUN_MSC_SDK_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} msc)
