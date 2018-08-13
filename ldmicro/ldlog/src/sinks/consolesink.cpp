/*******************************************************************************
*  file    : consolesink.cpp
*  created : 05.08.2018
*  author  : Slyshyk Oleksiy (alexslyshyk@gmail.com)
*******************************************************************************/
#include "consolesink.hpp"
#include "../fmt/format.h"

namespace ldlog
{
void ConsoleSink::write(const LogMessage& message)
{
    fmt::print("{}", format(message));
}

std::shared_ptr<Sink> newConsoleSink()
{
    std::shared_ptr<Sink> res;
    res.reset(new ConsoleSink);
    return res;
}

}//namespace ldlog
