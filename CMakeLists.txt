cmake_minimum_required(VERSION 3.16)
project(taftoolbox)
set (CMAKE_CXX_STANDARD 17)

find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)

add_subdirectory(ta)
add_subdirectory(rwe)
add_subdirectory(nswf)

# -------------- COMPARE ASSETS TOOL --

add_executable(compare_assets
    compare_assets.cpp)

target_link_libraries(compare_assets
    rwe
    nswf
    Qt5::Core)

install(TARGETS compare_assets)

# -------------- MAP TOOL -------------

add_executable(maptool
    maptool.cpp)

target_link_libraries(maptool
    ta
    rwe
    nswf
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets)

install(TARGETS maptool)
