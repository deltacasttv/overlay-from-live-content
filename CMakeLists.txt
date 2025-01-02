cmake_minimum_required(VERSION 3.16)

include(cmake/version.cmake)
git_version(GIT_VERSION)
set_version(${GIT_VERSION})

project(
    "videomaster-overlay-from-live-content"
    VERSION ${PROJECT_VERSION}
    DESCRIPTION "Application that demonstrates the possibility to overlay content generated from live stream with minimal latency, using the VideoMaster SDK"
    LANGUAGES CXX
)

add_subdirectory("src")
add_subdirectory("deps")