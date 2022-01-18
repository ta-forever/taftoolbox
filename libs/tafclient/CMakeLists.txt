set(CMAKE_AUTOMOC ON)

add_library(tafclient
    TafLobbyClient.h
    TafLobbyClient.cpp
    TafLobbyJsonProtocol.h
    TafLobbyJsonProtocol.cpp
    TafHwIdGenerator.h
    TafHwIdGenerator.cpp
    )

target_include_directories(tafclient
    PUBLIC
    ${CMAKE_SOURCE_DIR}/libs
    )

target_link_libraries(tafclient
    Qt5::Core
    Qt5::Network
    )
