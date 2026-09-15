cmake_minimum_required(VERSION 3.16)
project(Memoris)

set(CMAKE_CXX_STANDARD 17)

include_directories(.)

add_executable(Memoris
        main.cpp
        Server.cpp
        Server.h
        Store.cpp
        Store.h)
