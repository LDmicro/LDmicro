/*******************************************************************************
*  file    : windowsdebugstring.cpp
*  created : 05.08.2018
*  author  : Slyshyk Oleksiy (alexslyshyk@gmail.com)
*******************************************************************************/
#include "windowsdebugstringsink.hpp"
#include "../fmt/format.h"

#include <Windows.h>

namespace ldlog
{

void WindowsDebugStringSink::write(const LogMessage& message)
{
    std::string msg = format(message);
    OutputDebugStringA(msg.c_str());
}

std::shared_ptr<Sink> newWindowsDebugStringSink()
{
    std::shared_ptr<Sink> res;
    res.reset(new WindowsDebugStringSink);
    return res;
}

} // namespace ldlog
