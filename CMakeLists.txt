cmake_minimum_required(VERSION 3.5)

project(ez LANGUAGES CXX)

include_directories(
    include
)

set(SOURCES
    source/ip.cpp
    source/buffer.cpp
    source/socket.cpp
    source/http.cpp
    source/hex.cpp
    source/base64.cpp
    source/url.cpp
)

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR 
    CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    list(APPEND SOURCES source/kqueue.cpp)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND SOURCES source/epoll.cpp)
endif()

add_library(${PROJECT_NAME} ${SOURCES})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)