set(CMAKE_AUTOMOC ON)

# STATIC: PMF signal connects (e.g. TafLobbyClient::session) fail across a
# MinGW DLL boundary because the class has no export macros
add_library(tafclient STATIC
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
