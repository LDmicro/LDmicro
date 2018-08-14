/*******************************************************************************
*  file    : logger.cpp
*  created : 01.07.2018
*  author  : Slyshyk Oleksiy (alexslyshyk@gmail.com)
*******************************************************************************/
#include "logger.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include "fmt/format.h"
#include "time_utils.hpp"

namespace ldlog
{
std::vector<std::shared_ptr<Logger>> Logger::log_vector_;

std::shared_ptr<Logger> Logger::getLogger(const std::string& name)
{
    auto l = std::find_if(log_vector_.begin(), log_vector_.end(), [&name](const std::shared_ptr<Logger>& logger) { return logger->name() == name; });
    if (l != log_vector_.end())
        return *l;

    auto nl = std::shared_ptr<Logger>(new Logger(name));
    log_vector_.push_back(nl);
    return nl;
}

namespace details
{
    const char* level_name(ldlog::level l)
    {
        return (
            l == Info ? "I"
                      : l == Debug ? "D"
                                   : l == Warning ? "W"
                                                  : l == Error ? "E"
                                                               : l == Trace ? "T"
                                                                            : l == Critical ? "C"
                                                               : "X");
    }
} //namespace details

Logger::Logger(const std::string& name)
    : name_(name)
    , level_(Info)
{
}

void Logger::add_sink(std::shared_ptr<Sink> s)
{
    sinks_.push_back(s);
}

void Logger::set_level(level nl)
{
    level_ = nl;
}

void Logger::write(level lvl, const char* file, int line, const std::string& msg)
{
    if(lvl == Disable)
        return;
    if(level_ < lvl)
        return;

    LogMessage lm;
    lm.lvl         = lvl;
    lm.file        = file;
    lm.line        = line;
    lm.message     = msg;
    lm.logger_name = name_;
    lm.msg_time    = details::gmtime();

    sink(std::move(lm));
}

void Logger::sink(const LogMessage&& message)
{
    for (decltype(sinks_.size()) i = 0; i < sinks_.size(); ++i)
        sinks_[i]->write(message);
}

std::shared_ptr<Logger> getLogger(const std::string& name)
{
    return Logger::getLogger(name);
}

Sink::Sink():
    time_format_("%H:%M:%S")
{
    set_message_format("[%t]{%l} %m(%f:%r)%n");
}

Sink::~Sink()
{
}

void Sink::set_message_format(const std::string& fmt)
{
    message_lvl_format_[Trace]    = fmt;
    message_lvl_format_[Debug]    = fmt;
    message_lvl_format_[Info]     = fmt;
    message_lvl_format_[Warning]  = fmt;
    message_lvl_format_[Error]    = fmt;
    message_lvl_format_[Critical] = fmt;
}

void Sink::set_message_format(level lvl, const std::string& fmt)
{
    message_lvl_format_[lvl] = fmt;
}

std::string Sink::time_format(const tm& t) const
{
    char buf[128];
    std::strftime(buf, sizeof(buf), time_format_.c_str(), &t);
    return std::string(buf);
}

std::string Sink::format(const LogMessage& message) const
{
    std::string res;
    res.reserve(512);
    bool         is_prev_persent = false;
    bool         done            = false;
    unsigned int cur_pos         = 0;

    auto& message_format_ = message_lvl_format_.at(message.lvl);

    while (!done)
        {
            if (is_prev_persent)
                {
                    is_prev_persent = false;
                    if (message_format_[cur_pos] == '%')
                        {
                            res.push_back('%');
                        }
                    else if (strncmp(&message_format_[cur_pos], "t", 1) == 0)
                        {
                            res += time_format(message.msg_time);
                        }
                    else if (strncmp(&message_format_[cur_pos], "m", 1) == 0)
                        {
                            res += message.message;
                        }
                    else if (strncmp(&message_format_[cur_pos], "l", 1) == 0)
                        {
                            res += details::level_name(message.lvl);
                        }
                    else if (strncmp(&message_format_[cur_pos], "f", 1) == 0)
                        {
                            res += message.file;
                        }
                    else if (strncmp(&message_format_[cur_pos], "r", 1) == 0)
                        {
                            char buf[32];
                            sprintf_s(buf, "%i", message.line);
                            res += buf;
                        }
                    else if (strncmp(&message_format_[cur_pos], "n", 1) == 0)
                        {
                            res += "\r\n";
                        }
                }
            else
                {
                    if (message_format_[cur_pos] == '%')
                        {
                            is_prev_persent = true;
                        }
                    else
                        {
                            res.push_back(message_format_[cur_pos]);
                        }
                }
            cur_pos++;
            if (cur_pos == message_format_.size())
                done = true;
        }

    return res;
}

} //namespace ldlog
