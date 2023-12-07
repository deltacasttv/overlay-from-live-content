cmake_minimum_required(VERSION 3.19)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

include(../cmake/env.cmake)
if(WIN32 OR (UNIX AND NOT APPLE))
    check_environment_variable(VIDEOMASTERHD_LIBRARIES)
    check_environment_variable(VIDEOMASTERHD_INCLUDE_DIRS)
endif()

include(FetchContent)
FetchContent_Declare(
    cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG        v2.3.2
)
FetchContent_Declare(
    video-viewer
    GIT_REPOSITORY https://github.com/deltacasttv/video-viewer.git
    GIT_TAG        7197b483d08df009de5351d3b1437fa82c1834c3
)

FetchContent_MakeAvailable(cli11)
FetchContent_MakeAvailable(video-viewer)

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

    api_helper/api.cpp
    api_helper/handle_manager.cpp
    api_helper/enum_to_string.cpp
)

if(WIN32)
    target_include_directories(${PROJECT_NAME} PUBLIC ${VIDEOMASTERHD_INCLUDE_DIRS})
    target_link_directories(${PROJECT_NAME} PUBLIC ${VIDEOMASTERHD_LIBRARIES}/x64/)
    target_link_libraries(${PROJECT_NAME} VideoMasterHD CLI11::CLI11 video-viewer)
elseif(UNIX AND NOT APPLE)
    target_include_directories(${PROJECT_NAME} PUBLIC ${VIDEOMASTERHD_INCLUDE_DIRS})
    target_link_directories(${PROJECT_NAME} PUBLIC ${VIDEOMASTERHD_LIBRARIES}/)
    target_link_libraries(${PROJECT_NAME} PUBLIC videomasterhd pthread rt CLI11::CLI11 video-viewer)
elseif(UNIX AND APPLE)
    find_library(VIDEOMASTERHD_LIBRARIES VideoMasterHD /Library/Frameworks/VideoMasterHD.framework/)
    target_link_libraries(${PROJECT_NAME} PUBLIC ${VIDEOMASTERHD_LIBRARIES} CLI11::CLI11 video-viewer)
endif()