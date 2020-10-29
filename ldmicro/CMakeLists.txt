cmake_minimum_required(VERSION 3.11)
include(./cmake/PrecompiledHeader.cmake)

if ( ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR} )
    message( FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a BUILD directory) and run CMake from there. You may need to remove CMakeCache.txt." )
endif()


project(ldmicro LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 14)

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

option(LDLOGLIB_SHARED "Build ldlog as shared lib." OFF)
option(BUILD_LDMICROXX "enable/disable ldmicro-xx (default is disabled)." OFF)
if (BUILD_LDMICROXX)
    add_subdirectory(ldmicro-xx)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(ldlanglib)
add_subdirectory(ldcompilerlib)
add_subdirectory(ldinterpret)
add_subdirectory(ldlog)

find_package(Perl REQUIRED)

set(LDMICRO_SRC_LIST
    coildialog.cpp
    colorDialog.cpp
    commentdialog.cpp
    confdialog.cpp
    contactsdialog.cpp
    draw.cpp
    draw_outputdev.cpp
    helpdialog.cpp
    iolist.cpp
    ldmicro.cpp
    lutdialog.cpp
    maincontrols.cpp
    miscutil.cpp
    resetdialog.cpp
    schematic.cpp
    simpledialog.cpp
    simulate.cpp
    undoredo.cpp
    undoredo.hpp
    flash.cpp
    flash.h
    ${CMAKE_CURRENT_SOURCE_DIR}/stdafx.cpp
    ldversion.h

    ../common/win32/freeze.cpp
    ../common/win32/freeze.h
    )

set(LDMICRO_HEADERS
    ldmicro.h
    ldversion.h
    ldconfig.h
    ${CMAKE_CURRENT_SOURCE_DIR}/stdafx.h
    targetver.h
    )

set(HELPER_SCRIPRS
    ${CMAKE_SOURCE_DIR}/flashMcu.bat
    ${CMAKE_SOURCE_DIR}/fuseMcu.bat
    ${CMAKE_SOURCE_DIR}/readMcu.bat
    ${CMAKE_SOURCE_DIR}/notepad.bat
    ${CMAKE_SOURCE_DIR}/postCompile.bat
    ${CMAKE_SOURCE_DIR}/clear.bat
    ${CMAKE_SOURCE_DIR}/buildarm.bat
    ${CMAKE_SOURCE_DIR}/buildavr.bat
    ${CMAKE_SOURCE_DIR}/buildPic16.bat
    ${CMAKE_SOURCE_DIR}/buildPic18.bat
    )

set(LDMICRO_SRC_LIST ${LDMICRO_SRC_LIST} ${LDMICRO_HEADERS})

if(WIN32)
    set(LDMICRO_SRC_LIST ${LDMICRO_SRC_LIST} ldmicro.rc)
endif(WIN32)

INCLUDE_DIRECTORIES("../common/win32" "ldmicro" "./" "../common/gsl-lite/include" "../common/stringarray" "../common/utils")
add_definitions(
    "-DISOLATION_AWARE_ENABLED"
    "-DWIN32_LEAN_AND_MEAN"
    "-D_CRT_SECURE_NO_WARNINGS"
    "-D_CRT_SECURE_NO_DEPRECATE"
    "-DNOMINMAX"
    -DFREEZE_SUBKEY="LDMicro"
    )

set(LDMICRO_LIBS "")

if(WIN32)
    SET(GUI_TYPE WIN32)
    list(APPEND LDMICRO_LIBS Comctl32)
endif(WIN32)

# For custom manifest
if(MSVC)
    string(APPEND CMAKE_CXX_FLAGS " /J")
    string(APPEND CMAKE_C_FLAGS " /J")
    set(CMAKE_EXE_LINKER_FLAGS "/MANIFEST:NO")

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

set(_ldloglib ldlog)
if(NOT LDLOGLIB_SHARED)
    set(_ldloglib ldloglib)
endif()

if(${LDLANG} STREQUAL "ALL")
    foreach(lang_id ${LDLANG_ALL})
        if(${lang_id} STREQUAL "EN")
            add_executable            (${PROJECT_NAME} ${GUI_TYPE} ${LDMICRO_SRC_LIST})
            target_compile_definitions(${PROJECT_NAME} PRIVATE -DLDLANG_${lang_id})
            target_link_libraries     (${PROJECT_NAME} ${LDMICRO_LIBS} ldlanglib ldcompilerlib ${_ldloglib})
            target_compile_definitions(${PROJECT_NAME} PRIVATE
                -DLDLANG_${lang_id}
                )

            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                               COMMAND ${CMAKE_COMMAND} -D pattern="${PROJECT_NAME}.exe"
                               -D _S="${HELPER_SCRIPRS}"
                               -D _D=${CMAKE_SOURCE_DIR}/LIBRARIES_FOR
                               -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/after_build.cmake
                               )

        else()
            string(TOLOWER ${lang_id} ldlang_lower)
            add_executable            (${PROJECT_NAME}-${ldlang_lower} ${GUI_TYPE} ${LDMICRO_SRC_LIST})
            target_compile_definitions(${PROJECT_NAME}-${ldlang_lower} PRIVATE
                -DLDLANG_${lang_id}
                )
            target_link_libraries     (${PROJECT_NAME}-${ldlang_lower} ${LDMICRO_LIBS}
                ldlanglib-${ldlang_lower} ldcompilerlib-${ldlang_lower} ${_ldloglib})
        endif()
    endforeach()

else()
    string(TOLOWER ${LDLANG} ldlang_lower)
    set(_project_name ${PROJECT_NAME})
    set(_ldlanglib ldlanglib)
    set(_ldcompilerlib ldcompilerlib)

    if( NOT (${LDLANG} STREQUAL "EN"))
        set(_project_name ${PROJECT_NAME}-${ldlang_lower})
        set(_ldlanglib ldlanglib-${ldlang_lower})
        set(_ldcompilerlib ldcompilerlib-${ldlang_lower})
    endif()

    add_executable       (${_project_name} ${GUI_TYPE} ${LDMICRO_SRC_LIST})
    target_link_libraries(${_project_name} ${LDMICRO_LIBS} ${_ldlanglib} ${_ldcompilerlib} ${_ldloglib})
    target_compile_definitions(${_project_name} PRIVATE
        -DLDLANG_${LDLANG}
        )

    add_custom_command(TARGET ${_project_name} POST_BUILD
                      COMMAND ${CMAKE_COMMAND} -D pattern="${_project_name}.exe"
                      -D _S="${HELPER_SCRIPRS}"
                      -D _D=${CMAKE_SOURCE_DIR}/LIBRARIES_FOR
                      -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/after_build.cmake
                      )

    add_precompiled_header(${_project_name} ${CMAKE_CURRENT_SOURCE_DIR}/stdafx.h FORCEINCLUDE SOURCE_CXX ${CMAKE_CURRENT_SOURCE_DIR}/stdafx.cpp)

endif()

