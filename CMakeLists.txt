project(taftoolbox)
cmake_minimum_required(VERSION 3.16)

find_package(Qt5 COMPONENTS Core Gui REQUIRED)

#if(MSVC)
#    add_compile_options(
#        $<$<CONFIG:>:/MT> #---------|
#        $<$<CONFIG:Debug>:/MTd> #---|-- Statically link the runtime libraries
#        $<$<CONFIG:Release>:/MT> #--|
#    )
#endif()

add_subdirectory(ta)
add_subdirectory(rwe)
add_subdirectory(nswf)

# -------------- MAP TOOL -------------

add_executable(maptool
    maptool.cpp)

target_link_libraries(maptool
    ta
    rwe
    nswf
    Qt5::Core
    Qt5::Gui)

install(TARGETS maptool)
