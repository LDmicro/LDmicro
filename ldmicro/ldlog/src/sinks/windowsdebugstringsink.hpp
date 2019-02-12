/*******************************************************************************
*  file    : windowsdebugstring.hpp
*  created : 05.08.2018
*  author  : Slyshyk Oleksiy (alexslyshyk@gmail.com)
*******************************************************************************/
#ifndef WINDOWSDEBUGSTRING_HPP
#define WINDOWSDEBUGSTRING_HPP

#include "../logger.hpp"

namespace ldlog
{
LDLOG_API std::shared_ptr<Sink>   newWindowsDebugStringSink();

class WindowsDebugStringSink: public Sink
{
public:
    WindowsDebugStringSink() : Sink() {};
    virtual void write(const LogMessage& message);
};

}


#endif // WINDOWSDEBUGSTRING_HPP
