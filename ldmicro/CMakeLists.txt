cmake_minimum_required(VERSION 3.2)

if ( ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR} )
    message( FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a BUILD directory) and run CMake from there. You may need to remove CMakeCache.txt." )
endif()


project(ldmicro
    LANGUAGES C CXX
    )

# set all posible languages
SET(LDLANG_ALL EN DE ES FR IT PT TR RU JA )
#set language. Must be one of LDLANG_ALL or "ALL"
if( NOT LDLANG)
    set(LDLANG "EN")
endif()
set(LDLANG_OK FALSE)
foreach(lang_id ${LDLANG_ALL})
    if(${lang_id} STREQUAL ${LDLANG})
        set(LDLANG_OK TRUE)
    endif()
endforeach()
if(${LDLANG} STREQUAL "ALL")
    set(LDLANG_OK TRUE)
endif()

if( NOT LDLANG_OK)
    set(LDLANG "EN")
else()

endif()
message(STATUS "Lang : ${LDLANG}")

find_package(Perl REQUIRED)

set(LDMICRO_SRC_LIST
    ansic.cpp
    avr.cpp
    circuit.cpp
    coildialog.cpp
    colorDialog.cpp
    commentdialog.cpp
    compilecommon.cpp
    confdialog.cpp
    contactsdialog.cpp
    display.cpp
    draw.cpp
    draw_outputdev.cpp
    helpdialog.cpp
    intcode.cpp
    interpreted.cpp
    iolist.cpp
    lang.cpp
    ldmicro.cpp
    loadsave.cpp
    lutdialog.cpp
    maincontrols.cpp
    miscutil.cpp
    netzer.cpp
    pic16.cpp
    resetdialog.cpp
    schematic.cpp
    simpledialog.cpp
    simulate.cpp
    undoredo.cpp
    xinterpreted.cpp
    stdafx.cpp

    ../common/win32/freeze.cpp
    ../common/win32/freeze.h

    "${CMAKE_CURRENT_SOURCE_DIR}/obj/helptext.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/obj/lang-tables.h"
    )

file( GLOB LDMICRO_HEADERS ./*.h)

set(HELPER_SCRIPRS
    ${CMAKE_SOURCE_DIR}/flashMcu.bat
    ${CMAKE_SOURCE_DIR}/fuseMcu.bat
    ${CMAKE_SOURCE_DIR}/readMcu.bat
    ${CMAKE_SOURCE_DIR}/notepad.bat
    ${CMAKE_SOURCE_DIR}/postCompile.bat
    ${CMAKE_SOURCE_DIR}/clear.bat
    )

set(LDMICRO_SRC_LIST ${LDMICRO_SRC_LIST} ${LDMICRO_HEADERS})

if(WIN32)
    set(LDMICRO_SRC_LIST ${LDMICRO_SRC_LIST}
         ldmicro.rc
        )
endif(WIN32)

set (LDINTERPRET_SRC
     ldinterpret.c
    )

set (LDXINTERPRET_SRC
     ldxinterpret.c
    )


INCLUDE_DIRECTORIES("../common/win32" "ldmicro" "./")
add_definitions(
    "-DISOLATION_AWARE_ENABLED"
    "-DWIN32_LEAN_AND_MEAN"
    "-D_CRT_SECURE_NO_WARNINGS"
    "-D_CRT_SECURE_NO_DEPRECATE"
    )

set(LDMICRO_LIBS "")

if(WIN32)
    SET(GUI_TYPE WIN32)
    list(APPEND LDMICRO_LIBS Comctl32)
endif(WIN32)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/obj)

execute_process(
    COMMAND ${PERL_EXECUTABLE} txt2c.pl
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_FILE      "${CMAKE_CURRENT_SOURCE_DIR}/obj/helptext.cpp"
    )

execute_process(
    COMMAND ${PERL_EXECUTABLE} lang-tables.pl
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_FILE      "${CMAKE_CURRENT_SOURCE_DIR}/obj/lang-tables.h"
    )

# For custom manifest
if(MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "/MANIFEST:NO")
endif()


if(${LDLANG} STREQUAL "ALL")
    foreach(lang_id ${LDLANG_ALL})
        if(${lang_id} STREQUAL "EN")
            add_executable            (${PROJECT_NAME} ${GUI_TYPE} ${LDMICRO_SRC_LIST})
            target_compile_definitions(${PROJECT_NAME} PRIVATE -DLDLANG_${lang_id})
            target_link_libraries     (${PROJECT_NAME} ${LDMICRO_LIBS})
        else()
            string(TOLOWER ${lang_id} ldlang_lower)
            add_executable            (${PROJECT_NAME}-${ldlang_lower} ${GUI_TYPE} ${LDMICRO_SRC_LIST})
            target_compile_definitions(${PROJECT_NAME}-${ldlang_lower} PRIVATE -DLDLANG_${lang_id})
            target_link_libraries     (${PROJECT_NAME}-${ldlang_lower} ${LDMICRO_LIBS})
        endif()
    endforeach()

else()

    if(${LDLANG} STREQUAL "EN")
        add_definitions(-DLDLANG_${LDLANG})
        add_executable       (${PROJECT_NAME} ${GUI_TYPE} ${LDMICRO_SRC_LIST})
        target_link_libraries(${PROJECT_NAME} ${LDMICRO_LIBS})
    elseif( NOT (${LDLANG} STREQUAL "ALL"))
        add_definitions(-DLDLANG_${LDLANG})
        string(TOLOWER ${LDLANG} ldlang_lower)
        add_executable       (${PROJECT_NAME}-${ldlang_lower} ${GUI_TYPE} ${LDMICRO_SRC_LIST})
        target_link_libraries(${PROJECT_NAME}-${ldlang_lower} ${LDMICRO_LIBS})
    endif()

endif()

add_executable(ldinterpret  ${LDINTERPRET_SRC})
add_executable(ldxinterpret ${LDXINTERPRET_SRC})

add_custom_command(TARGET ldinterpret POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy  ${HELPER_SCRIPRS} ${CMAKE_CURRENT_BINARY_DIR}
    )
