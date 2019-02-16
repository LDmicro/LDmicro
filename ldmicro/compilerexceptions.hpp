#pragma once

#include <exception>
#include <cstdio>

#ifndef __LDLOG_LOG_FILE

#define __LDLOG_LOG_FILE (strstr(__FILE__, "/") != NULL ? \
    strrchr(__FILE__, '/') + 1 : strstr(__FILE__, "\\") != NULL ? strrchr(__FILE__, '\\') + 1 : __FILE__)

#endif

//#define USE_JG

extern int CompileFailure;      ///// added by JG

#ifdef USE_JG
#else
    #define THROW_COMPILER_EXCEPTION(MSG, ...) do{ \
        char message[1024*3];\
        sprintf(message, "%s\n[%i:%s]", MSG, __LINE__, __LDLOG_LOG_FILE); \
        throw std::runtime_error(message);\
        }while(0)
#endif

#ifdef USE_JG
/////   variable list of args added
/////   throw std::runtime_error(message) \;
/////   replaced twice above by :                                   // by JG
/////   Error(message); \
/////   CompileFailure= 1; \
/////   return __VA_ARGS__; \

    #define THROW_COMPILER_EXCEPTION(MSG, ...) do{ \
        char message[1024*3]; \
        sprintf(message, "%s\n[%i:%s]", MSG, __LINE__, __LDLOG_LOG_FILE); \
        Error(message); \
        CompileFailure= 1; \
        return __VA_ARGS__; \
        }while(0)

#else
    #define THROW_COMPILER_EXCEPTION_FMT(FMT,...) do{ \
        char format[1024];\
        sprintf(format, FMT, __VA_ARGS__); \
        char message[1024*3];\
        sprintf(message, "%s\n[%i:%s]", format, __LINE__, __LDLOG_LOG_FILE); \
        throw std::runtime_error(message);\
        }while(0)
#endif

#ifdef USE_JG
    #define THROW_COMPILER_EXCEPTION_FMT(FMT,...) do{ \
        char format[1024]; \
        sprintf(format, FMT, __VA_ARGS__); \
        char message[1024*3];\
        sprintf(message, "%s\n[%i:%s]", format, __LINE__, __LDLOG_LOG_FILE); \
        Error(message); \
        CompileFailure= 1; \
        }while(0)
#endif
