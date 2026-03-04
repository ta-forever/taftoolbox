cmake_minimum_required(VERSION 3.16)
project(taftoolbox)
set (CMAKE_CXX_STANDARD 17)

find_package(Qt5 COMPONENTS Core Gui Network Widgets REQUIRED)

# -------------- libcommuni submodule --------
set(ENABLE_IRC ON CACHE BOOL "Enable IRC integration using libcommuni https://github.com/communi/libcommuni")
set(LIBCOMMUNI_DIR "${CMAKE_BINARY_DIR}/libcommuni" CACHE PATH "Path to libcommuni binaries https://github.com/communi/libcommuni")

# -------------- COMPARE ASSETS TOOL --
add_executable(compare_assets
    compare_assets.cpp)

target_link_libraries(compare_assets
    rwe
    nswf
    Qt5::Core)

install(TARGETS compare_assets)

# -------------- MAP TOOL -------------
add_subdirectory(libs/gpgnet)
add_subdirectory(libs/nswf)
add_subdirectory(libs/rwe)
add_subdirectory(libs/ta)
add_subdirectory(libs/tafclient)
add_subdirectory(libs/taflib)
add_subdirectory(apps/maptool)
add_subdirectory(apps/taf-cpp-client)
