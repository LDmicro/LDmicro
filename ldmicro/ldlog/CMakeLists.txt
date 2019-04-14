cmake_minimum_required(VERSION 3.11)

project(ldloglib LANGUAGES CXX)

set(ldlog_cxx_files
    src/logger.cpp
    src/logger.hpp
    src/time_utils.hpp
    src/winerror.cpp
    src/winerror.hpp
    src/ldlogexport.h
    src/sinks/consolesink.cpp
    src/sinks/consolesink.hpp
    src/sinks/windowsdebugstringsink.cpp
    src/sinks/windowsdebugstringsink.hpp
    src/sinks/windowsmessagesink.cpp
    src/sinks/windowsmessagesink.hpp
    )

set(fmt_cxx_files
    src/fmt/container.h
    src/fmt/format.cc
    src/fmt/format.h
    src/fmt/ostream.cc
    src/fmt/ostream.h
    src/fmt/posix.cc
    src/fmt/posix.h
    src/fmt/printf.cc
    src/fmt/printf.h
    src/fmt/string.h
    src/fmt/time.h
    )

set(ldlog_inc_files include/ldlog.hpp)

if(MSVC)
    string(APPEND CMAKE_CXX_FLAGS " /J")
    string(APPEND CMAKE_C_FLAGS " /J")

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

add_library (${PROJECT_NAME} ${ldlog_cxx_files} ${ldlog_inc_files} ${fmt_cxx_files})

target_include_directories(${PROJECT_NAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    )

add_library (ldlog SHARED ${ldlog_cxx_files} ${ldlog_inc_files} ${fmt_cxx_files})

target_include_directories(ldlog
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    )

target_compile_definitions(ldlog
    PRIVATE
        LDLOG_EXPORT
        FMT_EXPORT
    INTERFACE
        LDLOG_IMPORT
        FMT_SHARED
    )
