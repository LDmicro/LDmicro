cmake_minimum_required(VERSION 3.2)

project(ldinterpret LANGUAGES CXX)

set (LDINTERPRET_SRC
     ../ldinterpret.cpp
    )

set (LDXINTERPRET_SRC
     ../ldxinterpret.cpp
    )

add_executable(ldinterpret  ${LDINTERPRET_SRC})
add_executable(ldxinterpret ${LDXINTERPRET_SRC})
