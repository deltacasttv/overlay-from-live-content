cmake_minimum_required(VERSION 3.19)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

include(FetchContent)
FetchContent_Declare(
    cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG        v2.3.2
    SOURCE_DIR     "${CMAKE_SOURCE_DIR}/deps/cli11"
)
FetchContent_Declare(
    video-viewer
    GIT_REPOSITORY  https://github.com/deltacasttv/video-viewer.git
    GIT_TAG         1.1.1
    SOURCE_DIR      "${CMAKE_SOURCE_DIR}/deps/video-viewer"
)

FetchContent_Declare(
    VideoMasterAPIHelper
    GIT_REPOSITORY  https://github.com/deltacasttv/videomaster-api-helper.git
    GIT_TAG         1.0.0
    SOURCE_DIR      "${CMAKE_SOURCE_DIR}/deps/api_helper"
)

FetchContent_MakeAvailable(cli11)
FetchContent_MakeAvailable(video-viewer)
FetchContent_MakeAvailable(VideoMasterAPIHelper)

find_package(VideoMaster REQUIRED)

configure_file(${CMAKE_SOURCE_DIR}/src/version.h.in ${CMAKE_SOURCE_DIR}/src/version.h)

add_executable(${PROJECT_NAME}
    main.cpp

    device.cpp
    stream.cpp
    shared_resources.cpp
    signal_information.cpp
    rx_renderer.cpp
    rx_stream.cpp
    tx_stream.cpp
    processing.cpp
    allocation.cpp
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} VideoMaster::VideoMaster CLI11::CLI11 video-viewer VideoMasterAPIHelper)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PUBLIC VideoMaster::VideoMaster pthread rt CLI11::CLI11 video-viewer VideoMasterAPIHelper)
endif()