add_executable(maptool
    maptool.cpp)

target_include_directories(taflib 
    PUBLIC
    ${CMAKE_SOURCE_DIR}/libs
)

target_link_libraries(maptool
    ta
    rwe
    nswf
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets)

install(TARGETS maptool)
