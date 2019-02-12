/*******************************************************************************
*  file    : windowsmessagesink.hpp
*  created : 05.08.2018
*  author  : Slyshyk Oleksiy (alexslyshyk@gmail.com)
*******************************************************************************/
#ifndef WINDOWSMESSAGESINK_HPP
#define WINDOWSMESSAGESINK_HPP

#include "../logger.hpp"

#include <Windows.h>

namespace ldlog
{
LDLOG_API std::shared_ptr<Sink>   newWindowsMessageSink(HWND hwnd, UINT msg, WPARAM wparam = 0);

class WindowsMessageSink: public Sink
{
public:
    WindowsMessageSink(HWND hwnd, UINT msg, WPARAM wparam = 0): Sink(), hwnd_(hwnd), msg_(msg), wparam_(wparam) {};
    virtual void write(const LogMessage& message);
private:
    HWND hwnd_;
    UINT msg_;
    WPARAM wparam_;
};

} // namespace ldlog

#endif // WINDOWSMESSAGESINK_HPP
