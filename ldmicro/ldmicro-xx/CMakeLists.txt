cmake_minimum_required(VERSION 3.11)
include(../cmake/PrecompiledHeader.cmake)

project(ldmicro-xx LANGUAGES CXX)

set(ldmicro-xx-src
    src/main.cpp
    src/ldmicroapp.cpp
    src/ldmicroapp.hpp
    src/mainwindow.hpp
    src/mainwindow.cpp
    src/ladderwindow.cpp
    src/ladderwindow.hpp

    ../coildialog.cpp
    ../colorDialog.cpp
    ../commentdialog.cpp
    ../confdialog.cpp
    ../contactsdialog.cpp
    ../draw.cpp
    ../draw_outputdev.cpp
    ../helpdialog.cpp
    ../iolist.cpp
    ../ldmicro.cpp
    ../lutdialog.cpp
    ../maincontrols.cpp
    ../miscutil.cpp
    ../resetdialog.cpp
    ../schematic.cpp
    ../simpledialog.cpp
    ../simulate.cpp
    ../undoredo.cpp
    ../ldversion.h

    ../../common/win32/freeze.cpp
    ../../common/win32/freeze.h

    ${CMAKE_CURRENT_SOURCE_DIR}/src/precomp/ldmicroxxprecomphdr.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/precomp/ldmicroxxprecomphdr.hpp

    res/ldmicro.rc
    res/resource.h
    )

set(ldmicro-xx-win32xx
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_appcore.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_appcore0.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_archive.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_commondlg.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_controls.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_cstring.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_ddx.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_dialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_dockframe.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_docking.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_exception.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_file.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_filefind.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_folderdialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_frame.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_gdi.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_imagelist.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_listview.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_mdi.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_menu.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_menubar.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_metafile.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_printdialogs.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_propertysheet.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_rebar.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_rect.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_regkey.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_ribbon.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_richedit.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_scrollview.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_shared_ptr.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_socket.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_statusbar.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_stdcontrols.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_tab.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_taskdialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_textconv.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_themes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_time.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_toolbar.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_treeview.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_wceframe.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_wcestddef.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_webbrowser.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_wincore.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include/wxx_wincore0.h
    )

add_executable(${PROJECT_NAME} WIN32 ${ldmicro-xx-src} ${ldmicro-xx-win32xx})

add_precompiled_header(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/src/precomp/ldmicroxxprecomphdr.hpp FORCEINCLUDE
    SOURCE_CXX ${CMAKE_CURRENT_SOURCE_DIR}/src/precomp/ldmicroxxprecomphdr.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE Comctl32 ldlanglib ldcompilerlib ldlog)

target_include_directories(${PROJECT_NAME}
        PRIVATE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/precomp>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../ldmicro>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32xx/include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../common/gsl-lite/include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../common/stringarray>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32>
    )

target_compile_definitions(${PROJECT_NAME}
        PUBLIC
            LDMICRO_GUI_XX
            LDLANG_EN
            ISOLATION_AWARE_ENABLED
            _CRT_SECURE_NO_WARNINGS
            _CRT_SECURE_NO_DEPRECATE
            NOMINMAX
            COLOR_GRADIENTACTIVECAPTION=27
    )

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE "/J")
    set_property(TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY LINK_FLAGS " /MANIFEST:NO")
endif()
