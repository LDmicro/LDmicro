cmake_minimum_required(VERSION 3.2)

project(ldmicro
    VERSION 4.3.7.0
    LANGUAGES C CXX
    )

find_package(Perl REQUIRED)

set(LDMICRO_SRC_LIST
    ldmicro/ansic.cpp
    ldmicro/avr.cpp
    ldmicro/circuit.cpp
    ldmicro/coildialog.cpp
    ldmicro/colorDialog.cpp
    ldmicro/commentdialog.cpp
    ldmicro/compilecommon.cpp
    ldmicro/confdialog.cpp
    ldmicro/contactsdialog.cpp
    ldmicro/display.cpp
    ldmicro/draw.cpp
    ldmicro/draw_outputdev.cpp
    ldmicro/helpdialog.cpp
    ldmicro/intcode.cpp
    ldmicro/interpreted.cpp
    ldmicro/iolist.cpp
    ldmicro/lang.cpp
    ldmicro/ldmicro.cpp
    ldmicro/loadsave.cpp
    ldmicro/lutdialog.cpp
    ldmicro/maincontrols.cpp
    ldmicro/miscutil.cpp
    ldmicro/netzer.cpp
    ldmicro/pic16.cpp
    ldmicro/resetdialog.cpp
    ldmicro/schematic.cpp
    ldmicro/simpledialog.cpp
    ldmicro/simulate.cpp
    ldmicro/undoredo.cpp
    ldmicro/xinterpreted.cpp

    common/win32/freeze.cpp

    "${CMAKE_CURRENT_BINARY_DIR}/helptext.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ldmicro/obj/lang-tables.h"
    )

if(WIN32)
    set(LDMICRO_SRC_LIST ${LDMICRO_SRC_LIST}
        ldmicro/ldmicro.rc
        )
endif(WIN32)

set (LDINTERPRET_SRC
    ldmicro/ldinterpret.c
    )

set (LDXINTERPRET_SRC
    ldmicro/ldxinterpret.c
    )


INCLUDE_DIRECTORIES("common/win32" "ldmicro")
add_definitions(
    "-DLDLANG_EN"
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

file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ldmicro/obj)

execute_process(
    COMMAND ${PERL_EXECUTABLE} txt2c.pl
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ldmicro
    OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/helptext.cpp"
    )

execute_process(
    COMMAND ${PERL_EXECUTABLE} lang-tables.pl
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ldmicro
    OUTPUT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/ldmicro/obj/lang-tables.h"
    )

# For custom manifest
set(CMAKE_EXE_LINKER_FLAGS "/MANIFEST:NO")

add_executable(${PROJECT_NAME} ${GUI_TYPE} ${LDMICRO_SRC_LIST})
target_link_libraries(${PROJECT_NAME} ${LDMICRO_LIBS})

add_executable(ldinterpret ${LDINTERPRET_SRC})
add_executable(ldxinterpret ${LDXINTERPRET_SRC})
