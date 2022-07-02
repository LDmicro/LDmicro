#pragma once

#include <exception>
#include <cstdio>

#define RM_SLASH(str) (strstr(str, "/") != nullptr ? strrchr(str, '/') + 1 : strstr(str, "\\") != nullptr ? strrchr(str, '\\') + 1 : str)

#define __LLFILE__ RM_SLASH(__FILE__)

#define THROW_COMPILER_EXCEPTION(MSG)                                \
    do {                                                             \
        static char ___message[1024 * 4];                            \
        sprintf(___message, "%s[%i:%s]", MSG, __LINE__, __LLFILE__); \
        throw std::runtime_error(___message);                        \
    } while(0)

#define THROW_COMPILER_EXCEPTION_FMT(FMT, ...)                           \
    do {                                                                 \
        static char __message[1024 * 4];                                 \
        static char __format[1024];                                      \
        sprintf(__format, (FMT), __VA_ARGS__);                           \
        sprintf(__message, "%s[%i:%s]", __format, __LINE__, __LLFILE__); \
        throw std::runtime_error(__message);                             \
    } while(0)
