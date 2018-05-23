cmake_minimum_required(VERSION 3.11)

project(ldinterpret LANGUAGES CXX)

set (LDINTERPRET_SRC
     ../ldinterpret.cpp
    )

set (LDXINTERPRET_SRC
     ../ldxinterpret.cpp
    )

add_definitions(
    "-DISOLATION_AWARE_ENABLED"
    "-DWIN32_LEAN_AND_MEAN"
    "-D_CRT_SECURE_NO_WARNINGS"
    "-D_CRT_SECURE_NO_DEPRECATE"
    "-DNOMINMAX"
    )
INCLUDE_DIRECTORIES("../" "../../common/gsl-lite/include" "../../common/stringarray")

add_executable(ldinterpret  ${LDINTERPRET_SRC})
add_executable(ldxinterpret ${LDXINTERPRET_SRC})
