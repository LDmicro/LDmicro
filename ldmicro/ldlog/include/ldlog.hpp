/*******************************************************************************
*  file    : ldlog.hpp
*  created : 01.07.2018
*  author  : Slyshyk Oleksiy (alexslyshyk@gmail.com)
*******************************************************************************/
#ifndef LDLOG_HPP
#define LDLOG_HPP

#include "../src/logger.hpp"
#include "../src/winerror.hpp"
#include "../src/sinks/consolesink.hpp"
#include "../src/sinks/windowsdebugstringsink.hpp"
#include "../src/sinks/windowsmessagesink.hpp"
#include "../src/fmt/format.h"

#define __LDLOG_LOG_FILE (strstr(__FILE__, "/") != NULL ? \
    strrchr(__FILE__, '/') + 1 : strrchr(__FILE__, '\\') + 1)

#define LOG(LVL,L,F,...) L->write(static_cast<ldlog::level>(LVL),__LDLOG_LOG_FILE, __LINE__, fmt::format(F, __VA_ARGS__))

#define LOG_DEBUG(L,F, ...) L->debug  (__LDLOG_LOG_FILE, __LINE__, fmt::format(F, __VA_ARGS__))
#define LOG_INFO(L,F, ...)  L->info   (__LDLOG_LOG_FILE, __LINE__, fmt::format(F, __VA_ARGS__))
#define LOG_WARN(L,F, ...)  L->warning(__LDLOG_LOG_FILE, __LINE__, fmt::format(F, __VA_ARGS__))
#define LOG_ERROR(L,F, ...) L->error  (__LDLOG_LOG_FILE, __LINE__, fmt::format(F, __VA_ARGS__))
#define LOG_DISABLE(...)   do{}while(0)

#endif // LDLOG_HPP
