project(taftoolbox)
cmake_minimum_required(VERSION 3.16)
set (CMAKE_CXX_STANDARD 17)

find_package(Qt5 COMPONENTS Core Gui Network Widgets REQUIRED)

# -------------- libcommuni submodule --------
set(ENABLE_IRC ON CACHE BOOL "Enable IRC integration using libcommuni https://github.com/communi/libcommuni")
set(LIBCOMMUNI_DIR "${CMAKE_BINARY_DIR}/libcommuni" CACHE PATH "Path to libcommuni binaries https://github.com/communi/libcommuni")

if (ENABLE_IRC)
    file(MAKE_DIRECTORY ${LIBCOMMUNI_DIR})

    get_target_property (QT_QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)
    add_custom_target(libcommuni ALL
      COMMAND ${QT_QMAKE_EXECUTABLE} ${CMAKE_SOURCE_DIR}/libcommuni WORKING_DIRECTORY ${LIBCOMMUNI_DIR}
      COMMAND make WORKING_DIRECTORY ${LIBCOMMUNI_DIR}
      )

    set(LIBCOMMUNI_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/libcommuni/include/IrcCore;${CMAKE_SOURCE_DIR}/libcommuni/include/IrcModel;${CMAKE_SOURCE_DIR}/libcommuni/include/IrcUtil")
    set(LIBCOMMUNI_LIB_DIR "${LIBCOMMUNI_DIR}/lib")
    set(LIBCOMMUNI_LIBS_STATIC "IrcCore;IrcModel;IrcUtil")

    add_definitions(-D_ENABLE_IRC)
endif()

add_subdirectory(libs/gpgnet)
add_subdirectory(libs/nswf)
add_subdirectory(libs/rwe)
add_subdirectory(libs/ta)
add_subdirectory(libs/tafclient)
add_subdirectory(libs/taflib)
add_subdirectory(apps/maptool)
add_subdirectory(apps/taf-cpp-client)
