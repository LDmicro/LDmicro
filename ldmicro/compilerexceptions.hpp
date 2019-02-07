
#pragma once

#include <exception>
#include <cstdio>


extern int CompileFailure;      ///// added by JG

/*
#define THROW_COMPILER_EXCEPTION(MSG) do{ \
    char message[1024];\
    sprintf(message, "%s [%s:%i]", MSG, __FILE__, __LINE__); \
    throw std::runtime_error(message);\
    }while(0)
*/
/////   variable list of args added
/////   throw std::runtime_error(message) \;
/////   replaced twice above by :                                   // by JG
/////   Error(message); \
/////   CompileFailure= 1; \
/////   return __VA_ARGS__; \


#define THROW_COMPILER_EXCEPTION(MSG, ...) do{ \
    char message[1024]; \
    sprintf(message, "%s\n[%s: %i]", MSG, __FILE__, __LINE__); \
    Error(message); \
    CompileFailure= 1; \
    return __VA_ARGS__; \
    }while(0)

/*
#define THROW_COMPILER_EXCEPTION_FMT(FMT,...) do{ \
    char format[512];\
    sprintf(format, FMT, __VA_ARGS__); \
    char message[1024];\
    sprintf(message, "%s [%s:%i]", format, __FILE__, __LINE__); \
    throw std::runtime_error(message);\
    }while(0)
*/

#define THROW_COMPILER_EXCEPTION_FMT(FMT,...) do{ \
    char format[512]; \
    sprintf(format, FMT, __VA_ARGS__); \
    char message[1024];\
    sprintf(message, "%s\n[%s: %i]", format, __FILE__, __LINE__); \
    Error(message); \
    CompileFailure= 1; \
    }while(0)


