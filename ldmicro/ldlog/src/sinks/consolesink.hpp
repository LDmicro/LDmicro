/*******************************************************************************
*  file    : consolesink.hpp
*  created : 05.08.2018
*  author  : Slyshyk Oleksiy (alexslyshyk@gmail.com)
*******************************************************************************/
#ifndef CONSOLESINK_HPP
#define CONSOLESINK_HPP

#include "../logger.hpp"

namespace ldlog
{
LDLOG_API std::shared_ptr<Sink>   newConsoleSink();

class ConsoleSink : public Sink
{
public:
    ConsoleSink() : Sink() {}
    virtual void write(const LogMessage& message);
};

}

#endif // CONSOLESINK_HPP
