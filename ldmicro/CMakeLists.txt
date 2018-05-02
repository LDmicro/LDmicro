cmake_minimum_required(VERSION 3.2)

if ( ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR} )
    message( FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a BUILD directory) and run CMake from there. You may need to remove CMakeCache.txt." )
endif()


project(ldmicro
    LANGUAGES CXX
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

add_subdirectory(ldlanglib)
add_subdirectory(ldcompilerlib)
add_subdirectory(ldinterpret)

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
    xinterpreted.cpp
    stdafx.cpp
    ldversion.h

    ../common/win32/freeze.cpp
    ../common/win32/freeze.h
    )

set(LDMICRO_HEADERS
    ldmicro.h
    ldversion.h
    ldconfig.h
    stdafx.h
    targetver.h
    )

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

INCLUDE_DIRECTORIES("../common/win32" "ldmicro" "./")
add_definitions(
    "-DISOLATION_AWARE_ENABLED"
    "-DWIN32_LEAN_AND_MEAN"
    "-D_CRT_SECURE_NO_WARNINGS"
    "-D_CRT_SECURE_NO_DEPRECATE"
    "-DNOMINMAX"
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
endif()


if(${LDLANG} STREQUAL "ALL")
    foreach(lang_id ${LDLANG_ALL})
        if(${lang_id} STREQUAL "EN")
            add_executable            (${PROJECT_NAME} ${GUI_TYPE} ${LDMICRO_SRC_LIST})
            target_compile_definitions(${PROJECT_NAME} PRIVATE -DLDLANG_${lang_id})
            target_link_libraries     (${PROJECT_NAME} ${LDMICRO_LIBS} ldlanglib ldcompilerlib)
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                               COMMAND ${CMAKE_COMMAND} -E copy  ${HELPER_SCRIPRS} ${CMAKE_CURRENT_BINARY_DIR}
                               )
        else()
            string(TOLOWER ${lang_id} ldlang_lower)
            add_executable            (${PROJECT_NAME}-${ldlang_lower} ${GUI_TYPE} ${LDMICRO_SRC_LIST})
            target_compile_definitions(${PROJECT_NAME}-${ldlang_lower} PRIVATE -DLDLANG_${lang_id})
            target_link_libraries     (${PROJECT_NAME}-${ldlang_lower} ${LDMICRO_LIBS} ldlanglib-${ldlang_lower} ldcompilerlib-${ldlang_lower})
        endif()
    endforeach()

else()

    if(${LDLANG} STREQUAL "EN")
        add_definitions(-DLDLANG_${LDLANG})
        add_executable       (${PROJECT_NAME} ${GUI_TYPE} ${LDMICRO_SRC_LIST})
        target_link_libraries(${PROJECT_NAME} ${LDMICRO_LIBS} ldlanglib ldcompilerlib)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                           COMMAND ${CMAKE_COMMAND} -E copy  ${HELPER_SCRIPRS} ${CMAKE_CURRENT_BINARY_DIR}
                           )
    elseif( NOT (${LDLANG} STREQUAL "ALL"))
        add_definitions(-DLDLANG_${LDLANG})
        string(TOLOWER ${LDLANG} ldlang_lower)
        add_executable       (${PROJECT_NAME}-${ldlang_lower} ${GUI_TYPE} ${LDMICRO_SRC_LIST})
        target_link_libraries(${PROJECT_NAME}-${ldlang_lower} ${LDMICRO_LIBS} ldlanglib-${ldlang_lower} ldcompilerlib-${ldlang_lower})
        add_custom_command(TARGET ${PROJECT_NAME}-${ldlang_lower} POST_BUILD
                           COMMAND ${CMAKE_COMMAND} -E copy  ${HELPER_SCRIPRS} ${CMAKE_CURRENT_BINARY_DIR}
                           )
    endif()

endif()

add_custom_command(TARGET ldinterpret POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy  ${HELPER_SCRIPRS} ${CMAKE_CURRENT_BINARY_DIR}
