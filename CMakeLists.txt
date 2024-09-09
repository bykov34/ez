cmake_minimum_required(VERSION 3.5)

project(ez LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

option (EZ_WITH_TEST "" ON)

include_directories(
    include
    tls/include
)

set(SOURCES
    source/ip.cpp
    source/buffer.cpp
    source/socket.cpp
    source/http.cpp
    source/tls.cpp
    source/hex.cpp
    source/base64.cpp
    source/url.cpp
    source/smp.cpp
    source/sha1.cpp
    source/sha2.cpp
    source/poly1305.cpp
    source/chacha20.cpp
    source/chacha20_poly1305.cpp
)

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR 
    CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    list(APPEND SOURCES source/kqueue.cpp)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND SOURCES source/epoll.cpp)
endif()

add_library(${PROJECT_NAME} ${SOURCES})

if (EZ_WITH_TEST)
    add_executable(test test.cpp)
    target_link_libraries(test ${PROJECT_NAME} ez tls)
endif()
