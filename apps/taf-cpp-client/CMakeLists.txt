set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

file(GLOB_RECURSE SOURCES **.h **.cpp **.qrc)

add_executable(taf-cpp-client WIN32 ${SOURCES})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
         PREFIX "Sources"
         FILES ${SOURCES})

target_include_directories(taf-cpp-client
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/libs
    )

target_link_libraries(taf-cpp-client
    gpgnet
    tafclient
    taflib
    Qt5::Core
    Qt5::Gui
    Qt5::Network
    Qt5::Widgets)

install(TARGETS taf-cpp-client)

# ---------------------------------------------------------------------------
# Locate Qt bin and plugins directories for debugger PATH injection below.
#
# Strategy (in order):
#  1. Conan 2.x CMakeDeps sets qt_BIN_DIRS_DEBUG / qt_BIN_DIRS_RELEASE — use
#     those directly (Qt5::Core is INTERFACE in this layout, so
#     get_target_property IMPORTED_LOCATION returns nothing).
#  2. Fall back to reading IMPORTED_LOCATION from the Conan intermediate
#     target CONAN_LIB::qt_Qt5_Core_DEBUG / _RELEASE.
#  3. Last resort: try IMPORTED_LOCATION on Qt5::Core itself (works with
#     non-Conan system installs).
# ---------------------------------------------------------------------------
if(WIN32 AND MSVC)
    set(_qt_bin_dir "")

    # Strategy 1: Conan data variables (preferred)
    if(DEFINED qt_BIN_DIRS_DEBUG AND qt_BIN_DIRS_DEBUG)
        list(GET qt_BIN_DIRS_DEBUG 0 _qt_bin_dir)
    elseif(DEFINED qt_BIN_DIRS_RELEASE AND qt_BIN_DIRS_RELEASE)
        list(GET qt_BIN_DIRS_RELEASE 0 _qt_bin_dir)
    endif()

    # Strategy 2: Conan intermediate SHARED targets
    if(NOT _qt_bin_dir)
        foreach(_conan_tgt
                CONAN_LIB::qt_Qt5_Core_DEBUG
                CONAN_LIB::qt_Qt5_Core_RELEASE
                CONAN_LIB::qt_Qt5_Core)
            if(TARGET ${_conan_tgt})
                get_target_property(_dll ${_conan_tgt} IMPORTED_LOCATION_DEBUG)
                if(NOT _dll)
                    get_target_property(_dll ${_conan_tgt} IMPORTED_LOCATION_RELEASE)
                endif()
                if(NOT _dll)
                    get_target_property(_dll ${_conan_tgt} IMPORTED_LOCATION)
                endif()
                if(_dll)
                    get_filename_component(_qt_bin_dir "${_dll}" DIRECTORY)
                    break()
                endif()
            endif()
        endforeach()
    endif()

    # Strategy 3: system Qt (non-Conan)
    if(NOT _qt_bin_dir)
        foreach(_prop IMPORTED_LOCATION_DEBUG IMPORTED_LOCATION_RELEASE IMPORTED_LOCATION)
            get_target_property(_dll Qt5::Core ${_prop})
            if(_dll)
                get_filename_component(_qt_bin_dir "${_dll}" DIRECTORY)
                break()
            endif()
        endforeach()
    endif()

    if(_qt_bin_dir)
        get_filename_component(_qt_plugins_dir "${_qt_bin_dir}/../plugins" ABSOLUTE)
        file(TO_NATIVE_PATH "${_qt_bin_dir}"     _qt_bin_dir_native)
        file(TO_NATIVE_PATH "${_qt_plugins_dir}" _qt_plugins_dir_native)
        message(STATUS "Qt bin dir for debugger PATH: ${_qt_bin_dir_native}")
    else()
        set(_qt_bin_dir_native "")
        set(_qt_plugins_dir_native "")
        message(WARNING "Qt5::Core DLL location unknown; Qt DLLs won't be added to VS debugger PATH")
    endif()
endif()

# ---------------------------------------------------------------------------
# Locate external TAF natives base directory for the VS debugger.
# TAF_NATIVES_DIR is the base directory with the following layout:
#   TAF_NATIVES_DIR/faf-uid.exe
#   TAF_NATIVES_DIR/faf-ice-adapter.jar
#   TAF_NATIVES_DIR/bin/talauncher.exe  (+ its DLLs)
#   TAF_NATIVES_DIR/bin/gpgnet4ta.exe   (+ its DLLs)
#   TAF_NATIVES_DIR/bin/maptool.exe     (+ its DLLs)
# Passed to taf-cpp-client at debug time via --native-dir.
# ---------------------------------------------------------------------------
set(TAF_NATIVES_DIR "" CACHE PATH
    "Base directory for external TAF natives. faf-uid.exe and faf-ice-adapter.jar \
live here directly; talauncher.exe, gpgnet4ta.exe and maptool.exe live in bin/.")

if(TAF_NATIVES_DIR)
    string(REGEX REPLACE "[/\\\\]+$" "" _taf_natives_dir_stripped "${TAF_NATIVES_DIR}")
    file(TO_NATIVE_PATH "${_taf_natives_dir_stripped}" _taf_natives_dir_native)
    set(_native_dir_debugger_arg "--native-dir \"${_taf_natives_dir_native}\"")
    message(STATUS "TAF_NATIVES_DIR for debugger --native-dir: ${_taf_natives_dir_native}")
else()
    set(_native_dir_debugger_arg "")
endif()

# ---------------------------------------------------------------------------
# Generate a Visual Studio per-user debugger settings file so the working
# directory is the exe output folder (required for Qt plugin discovery).
# The generated file lives in the build tree; CMake regenerates it on reconfigure.
# ---------------------------------------------------------------------------
if(MSVC)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/taf-cpp-client.vcxproj.user.in"
        "${CMAKE_CURRENT_BINARY_DIR}/taf-cpp-client.vcxproj.user"
        @ONLY)
endif()
