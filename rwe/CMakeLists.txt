find_package(ZLIB)

add_library(rwe STATIC
    io_utils.h
    io_utils.cpp
    rwe_string.h
    rwe_string.cpp
    hpi/hpi_headers.h
    hpi/hpi_util.h
    hpi/hpi_util.cpp
    hpi/HpiArchive.h
    hpi/HpiArchive.cpp
    tnt/TntArchive.h
    tnt/TntArchive.cpp)

target_include_directories(rwe 
    PUBLIC
    ${ZLIB_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(rwe
    ZLIB::ZLIB)
