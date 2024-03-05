if(UNIX AND NOT APPLE)
    find_path(VideoMaster_SDK_DIR NAMES Include/VideoMasterHD_Core.h PATHS /usr/local/deltacast REQUIRED)
    find_path(VideoMaster_INCLUDE_DIR NAMES VideoMasterHD_Core.h PATHS ${VideoMaster_SDK_DIR}/Include REQUIRED)
    mark_as_advanced(VideoMaster_INCLUDE_DIR)
    find_library(VideoMasterHD_core
        NAMES libvideomasterhd.so
        PATHS /usr/lib
        REQUIRED
    )
    if(VideoMasterHD_core)
        add_library(VideoMaster::VideoMaster SHARED IMPORTED)
        set_target_properties(VideoMaster::VideoMaster PROPERTIES
                    IMPORTED_LOCATION "${VideoMasterHD_core}"
                    INTERFACE_INCLUDE_DIRECTORIES "${VideoMaster_INCLUDE_DIR}"
                    IMPORTED_IMPLIB "${VideoMasterHD_core}"
                )
    endif()
elseif(WIN32)
    find_path(VideoMaster_SDK_DIR NAMES Include/VideoMasterHD_Core.h PATHS ${CMAKE_CURRENT_LIST_DIR}/../deps/VideoMaster REQUIRED)
    find_path(VideoMaster_INCLUDE_DIR NAMES VideoMasterHD_Core.h PATHS ${VideoMaster_SDK_DIR}/Include REQUIRED)
    mark_as_advanced(VideoMaster_INCLUDE_DIR)
    find_library(VideoMasterHD_core NAMES VideoMasterHD.lib PATHS ${VideoMaster_SDK_DIR}/Library/x64 REQUIRED)
    if(VideoMasterHD_core)
        add_library(VideoMaster::VideoMaster SHARED IMPORTED)
        set_target_properties(VideoMaster::VideoMaster PROPERTIES
                    IMPORTED_LOCATION "${VideoMasterHD_core}"
                    INTERFACE_INCLUDE_DIRECTORIES "${VideoMaster_INCLUDE_DIR}"
                    IMPORTED_IMPLIB "${VideoMasterHD_core}"
                )
    endif()
endif()

# Generate VideoMaster_FOUND
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(VideoMaster
    FOUND_VAR VideoMaster_FOUND
    REQUIRED_VARS VideoMasterHD_core
)