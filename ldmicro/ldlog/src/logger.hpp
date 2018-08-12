/*******************************************************************************
*  file    : logger.hpp
*  created : 01.07.2018
*  author  : Slyshyk Oleksiy (alexslyshyk@gmail.com)
*******************************************************************************/
#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <vector>
#include <string>
#include <memory>
#include <map>
#include <ctime>

#include "ldlogexport.h"

namespace ldlog
{
class Logger;
class Sink;
class ConsoleSink;
LDLOG_API std::shared_ptr<Logger> getLogger(const std::string& name);

enum level
{
    Trace    = 6,
    Debug    = 5,
    Info     = 4,
    Warning  = 3,
    Error    = 2,
    Critical = 1,
    Disable  = 0
};

struct LogMessage
{
    int         line;
    const char* file;
    std::string message;
    std::string logger_name;
    level       lvl;
    std::tm     msg_time;
};

class Sink
{
    Sink(const Sink&) {}
    Sink& operator=(const Sink&) { return *this; }

public:
    Sink();
    virtual ~Sink();
    virtual void write(const LogMessage& message) = 0;
public:
    LDLOG_API void set_message_format(const std::string& msg);
    LDLOG_API void set_message_format(level lvl, const std::string& msg);
protected:
    LDLOG_API std::string time_format(const std::tm& t) const;
    LDLOG_API std::string format(const LogMessage& message) const;
private:
    std::string time_format_;
    std::map<level, std::string> message_lvl_format_;
};

class Logger
{
private:
    static std::vector<std::shared_ptr<Logger>> log_vector_;

public:
    static std::shared_ptr<Logger> getLogger(const std::string& name);

public:
    Logger(const std::string& name);
    const std::string name() const { return name_; }
    LDLOG_API void    add_sink(std::shared_ptr<Sink> s);
    LDLOG_API void    set_level(level nl);

public:
    LDLOG_API void write(level lvl, const char* file, int line, const std::string& msg);
    inline void trace   (const char* file, int line, const std::string& msg){write(Trace, file, line, msg);}
    inline void debug   (const char* file, int line, const std::string& msg){write(Debug, file, line, msg);}
    inline void info    (const char* file, int line, const std::string& msg){write(Info, file, line, msg);}
    inline void warning (const char* file, int line, const std::string& msg){write(Warning, file, line, msg);}
    inline void error   (const char* file, int line, const std::string& msg){write(Error, file, line, msg);}
    inline void critical(const char* file, int line, const std::string& msg){write(Critical, file, line, msg);}
    inline void disable (const char* , int , const std::string& ){}

private:
    void sink(const LogMessage&& message);

private:
    std::string                        name_;
    level                              level_;
    std::vector<std::shared_ptr<Sink>> sinks_;
};

} //namespace ldlog

#endif // LOGGER_HPP
