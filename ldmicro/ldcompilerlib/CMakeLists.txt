cmake_minimum_required(VERSION 3.11)
include(../cmake/PrecompiledHeader.cmake)

project(ldcompilerlib LANGUAGES CXX)

include(CheckCXXSourceCompiles)
check_cxx_source_compiles("constexpr int x = 0; int main(){return 0;}" LD_WITH_CONSTEXPR)
check_cxx_source_compiles("void func() noexcept {} int main(){return 0;}" LD_WITH_NOEXEPT)

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
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/utils/filetracker.hpp
    )

if(MSVC)
    string(APPEND CMAKE_CXX_FLAGS " /J")
    string(APPEND CMAKE_C_FLAGS " /J")
    string(APPEND CMAKE_CXX_FLAGS " /F 16000000")
    string(APPEND CMAKE_C_FLAGS " /F 16000000")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:16000000")

    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    endif()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4127")

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
                PUBLIC LDLANG_${lang_id}
                $<$<BOOL:${LD_WITH_CONSTEXPR}>:LD_WITH_CONSTEXPR>
                $<$<BOOL:${LD_WITH_NOEXEPT}>:LD_WITH_NOEXEPT>
                )
        else()
            string(TOLOWER ${lang_id} ldlang_lower)
            add_library               (${PROJECT_NAME}-${ldlang_lower} ${LD_COMPILER_SRC_LIST})
            target_compile_definitions(${PROJECT_NAME}-${ldlang_lower}
                PUBLIC LDLANG_${lang_id}
                $<$<BOOL:${LD_WITH_CONSTEXPR}>:LD_WITH_CONSTEXPR>
                $<$<BOOL:${LD_WITH_NOEXEPT}>:LD_WITH_NOEXEPT>
                )
        endif()
    endforeach()

else()

    if(${LDLANG} STREQUAL "EN")
        add_library          (${PROJECT_NAME} ${LD_COMPILER_SRC_LIST})
        target_compile_definitions(${PROJECT_NAME}
            PUBLIC LDLANG_${LDLANG}
            $<$<BOOL:${LD_WITH_CONSTEXPR}>:LD_WITH_CONSTEXPR>
            $<$<BOOL:${LD_WITH_NOEXEPT}>:LD_WITH_NOEXEPT>
            )
        add_precompiled_header(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/../stdafx.h FORCEINCLUDE SOURCE_CXX ${CMAKE_CURRENT_SOURCE_DIR}/../stdafx.cpp)
    elseif( NOT (${LDLANG} STREQUAL "ALL"))
        string(TOLOWER ${LDLANG} ldlang_lower)
        set(_project_name ${PROJECT_NAME}-${ldlang_lower})
        add_library               (${PROJECT_NAME}-${ldlang_lower} ${LD_COMPILER_SRC_LIST})
        target_compile_definitions(${PROJECT_NAME}-${ldlang_lower}
            PUBLIC LDLANG_${LDLANG}
            $<$<BOOL:${LD_WITH_CONSTEXPR}>:LD_WITH_CONSTEXPR>
            $<$<BOOL:${LD_WITH_NOEXEPT}>:LD_WITH_NOEXEPT>
            )
        add_precompiled_header(${_project_name} ${CMAKE_CURRENT_SOURCE_DIR}/../stdafx.h FORCEINCLUDE SOURCE_CXX ${CMAKE_CURRENT_SOURCE_DIR}/../stdafx.cpp)
    endif()

endif()
