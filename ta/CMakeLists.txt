add_library(ta
    hpi.h
    hpi.cpp
    tdf.h
    tdf.cpp)

target_include_directories(ta 
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
