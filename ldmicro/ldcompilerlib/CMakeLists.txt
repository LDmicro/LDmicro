cmake_minimum_required(VERSION 3.11)
include(../cmake/PrecompiledHeader.cmake)

project(ldcompilerlib LANGUAGES CXX)

if( NOT LDLANG )
    set(LDLANG "EN")
endif()

message(STATUS "ldcompilerlib lang : ${LDLANG}")

set(LD_COMPILER_SRC_LIST
    ../accel.cpp
    ../ansic.cpp
    ../avr.cpp
    ../circuit.cpp
    ../compilercommon.cpp
    ../display.cpp
    ../intcode.cpp
    ../interpreted.cpp
    ../loadsave.cpp
    ../netzer.cpp
    ../pic16.cpp
    ../mcutable.cpp
    ../pascal.cpp
    ../pcports.cpp
    ../plcprogram.cpp
    ../translit.cpp
    ../xinterpreted.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../stdafx.cpp

    ../accel.h
    ../bits.h
    ../circuit.h
    ../display.h
    ../intcode.h
    ../netzer.h
    ../mcutable.h
    ../mcutable.hpp
    ../pcports.h
    ../plcprogram.h
    ../compilerexceptions.hpp
    ../compilercommon.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../stdafx.h
    )

if(MSVC)
    string(APPEND CMAKE_CXX_FLAGS " /J")
    string(APPEND CMAKE_C_FLAGS " /J")
elseif(CMAKE_COMPILER_IS_GNUCXX)
    string(APPEND CMAKE_CXX_FLAGS "-Wall -Wextra ")
    string(APPEND CMAKE_C_FLAGS "-Wall -Wextra ")
endif()

INCLUDE_DIRECTORIES("../" "../../common/gsl-lite/include" "../../common/stringarray" "../../common/utils")
add_definitions(
    "-DISOLATION_AWARE_ENABLED"
    "-DWIN32_LEAN_AND_MEAN"
    "-D_CRT_SECURE_NO_WARNINGS"
    "-D_CRT_SECURE_NO_DEPRECATE"
    "-DNOMINMAX"
    )


if(${LDLANG} STREQUAL "ALL")
    foreach(lang_id ${LDLANG_ALL})
        if(${lang_id} STREQUAL "EN")
            add_library               (${PROJECT_NAME} ${LD_COMPILER_SRC_LIST})
            target_compile_definitions(${PROJECT_NAME}
                PUBLIC LDLANG_${lang_id} $<$<COMPILE_FEATURES:cxx_constexpr>:LD_WITH_CONSTEXPR>)
        else()
            string(TOLOWER ${lang_id} ldlang_lower)
            add_library               (${PROJECT_NAME}-${ldlang_lower} ${LD_COMPILER_SRC_LIST})
            target_compile_definitions(${PROJECT_NAME}-${ldlang_lower}
                PUBLIC LDLANG_${lang_id} $<$<COMPILE_FEATURES:cxx_constexpr>:LD_WITH_CONSTEXPR>)
        endif()
    endforeach()

else()

    if(${LDLANG} STREQUAL "EN")
        add_library          (${PROJECT_NAME} ${LD_COMPILER_SRC_LIST})
        target_compile_definitions(${PROJECT_NAME}
            PUBLIC LDLANG_${LDLANG} $<$<COMPILE_FEATURES:cxx_constexpr>:LD_WITH_CONSTEXPR>)
        add_precompiled_header(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/../stdafx.h FORCEINCLUDE SOURCE_CXX ${CMAKE_CURRENT_SOURCE_DIR}/../stdafx.cpp)
    elseif( NOT (${LDLANG} STREQUAL "ALL"))
        string(TOLOWER ${LDLANG} ldlang_lower)
        set(_project_name ${PROJECT_NAME}-${ldlang_lower})
        add_library               (${PROJECT_NAME}-${ldlang_lower} ${LD_COMPILER_SRC_LIST})
        target_compile_definitions(${PROJECT_NAME}-${ldlang_lower}
            PUBLIC LDLANG_${LDLANG} $<$<COMPILE_FEATURES:cxx_constexpr>:LD_WITH_CONSTEXPR>)
        add_precompiled_header(${_project_name} ${CMAKE_CURRENT_SOURCE_DIR}/../stdafx.h FORCEINCLUDE SOURCE_CXX ${CMAKE_CURRENT_SOURCE_DIR}/../stdafx.cpp)
    endif()

endif()
