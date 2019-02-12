/*******************************************************************************
*  file    : windowsmessagesink.cpp
*  created : 05.08.2018
*  author  : Slyshyk Oleksiy (alexslyshyk@gmail.com)
*******************************************************************************/
#include "windowsmessagesink.hpp"

namespace ldlog
{

void WindowsMessageSink::write(const LogMessage& message)
{
    static std::string msg;
    msg = format(message);
    SendMessageA(hwnd_, msg_, wparam_, (LPARAM)msg.c_str());
}

std::shared_ptr<Sink> newWindowsMessageSink(HWND hwnd, UINT msg, WPARAM wparam)
{
    std::shared_ptr<Sink> res;
    res.reset(new WindowsMessageSink(hwnd, msg, wparam));
    return res;
}

} // namespace ldlog
