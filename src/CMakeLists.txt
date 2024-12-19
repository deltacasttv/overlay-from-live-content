cmake_minimum_required(VERSION 3.19)
include(FetchContent)

configure_file(
    ${CMAKE_SOURCE_DIR}/src/version.h.in
    ${CMAKE_SOURCE_DIR}/src/version.h
    @ONLY
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME}
    ${CMAKE_SOURCE_DIR}/src/main.cpp

    ${CMAKE_SOURCE_DIR}/src/helper.cpp
    ${CMAKE_SOURCE_DIR}/src/shared_resources.cpp
    ${CMAKE_SOURCE_DIR}/src/windowed_renderer.cpp
    ${CMAKE_SOURCE_DIR}/src/processing.cpp
    ${CMAKE_SOURCE_DIR}/src/allocation.cpp
)

find_package(VideoMasterHD 6.26 REQUIRED)
FetchContent_MakeAvailable(VideoMasterCppApi)

target_link_libraries(${PROJECT_NAME} VideoMasterCppApi CLI11::CLI11 video-viewer)
if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PUBLIC rt)
endif()