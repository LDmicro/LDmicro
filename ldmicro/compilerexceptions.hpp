
#pragma once

#include <exception>
#include <cstdio>

#define TROW_COMPILER_EXCEPTION(MSG) do{ \
    char message[1024];\
    sprintf(message, "%s [%s:%i]", MSG, __FILE__, __LINE__); \
    throw std::runtime_error(message);\
    }while(0)

#define TROW_COMPILER_EXCEPTION_FMT(FMT,...) do{ \
    char format[512];\
    sprintf(format, FMT, __VA_ARGS__); \
    char message[1024];\
    sprintf(message, "%s [%s:%i]", format, __FILE__, __LINE__); \
    throw std::runtime_error(message);\
    }while(0)
