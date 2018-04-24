cmake_minimum_required(VERSION 3.2)

project(ldlanglib LANGUAGES CXX)

if( NOT LDLANG )
    set(LDLANG "EN")
endif()

message(STATUS "Lang : ${LDLANG}")

find_package(Perl REQUIRED)

set(LDLANG_SRC_LIST
    ../lang.h
    ../lang.cpp

    "${CMAKE_CURRENT_SOURCE_DIR}/obj/helptext.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/obj/lang-tables.h"
    )

file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/obj)

execute_process(
    COMMAND ${PERL_EXECUTABLE} txt2c.pl
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../
    OUTPUT_FILE      "${CMAKE_CURRENT_SOURCE_DIR}/obj/helptext.cpp"
    )

execute_process(
    COMMAND ${PERL_EXECUTABLE} lang-tables.pl
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../
    OUTPUT_FILE      "${CMAKE_CURRENT_SOURCE_DIR}/obj/lang-tables.h"
    )

if(MSVC)
    string(APPEND CMAKE_CXX_FLAGS " /J")
    string(APPEND CMAKE_C_FLAGS " /J")
endif()

if(${LDLANG} STREQUAL "ALL")
    foreach(lang_id ${LDLANG_ALL})
        if(${lang_id} STREQUAL "EN")
            add_library               (${PROJECT_NAME} ${LDLANG_SRC_LIST})
            target_compile_definitions(${PROJECT_NAME} PRIVATE -DLDLANG_${lang_id})
            target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
        else()
            string(TOLOWER ${lang_id} ldlang_lower)
            add_library               (${PROJECT_NAME}-${ldlang_lower} ${LDLANG_SRC_LIST})
            target_compile_definitions(${PROJECT_NAME}-${ldlang_lower} PRIVATE -DLDLANG_${lang_id})
            target_include_directories(${PROJECT_NAME}-${ldlang_lower} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
        endif()
    endforeach()

else()

    if(${LDLANG} STREQUAL "EN")
        add_definitions(-DLDLANG_${LDLANG})
        add_library          (${PROJECT_NAME} ${LDLANG_SRC_LIST})
        target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    elseif( NOT (${LDLANG} STREQUAL "ALL"))
        string(TOLOWER ${LDLANG} ldlang_lower)
        add_library               (${PROJECT_NAME}-${ldlang_lower} ${LDLANG_SRC_LIST})
        target_compile_definitions(${PROJECT_NAME}-${ldlang_lower} PRIVATE -DLDLANG_${lang_id})
        target_include_directories(${PROJECT_NAME}-${ldlang_lower} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    endif()

endif()
