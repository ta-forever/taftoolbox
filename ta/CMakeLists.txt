add_library(ta STATIC
    tdf.h
    tdf.cpp
    palette.h
    palette.cpp)

target_include_directories(ta 
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
