set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

file(GLOB_RECURSE SOURCES **.h **.cpp **.qrc)

add_executable(taf-cpp-client ${SOURCES})

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
    Qt5::Widgets)
