/*******************************************************************************
*  file    : mainwindow.hpp
*  created : 15.08.2018
*  author  : Slyshyk Oleksiy (alexslyshyk@gmail.com)
*******************************************************************************/
#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "precomp/ldmicroxxprecomphdr.hpp"
#include "ladderwindow.hpp"

class CMainWindow : public CDockFrame
{
public:
    CMainWindow();
    virtual ~CMainWindow();
protected:
    virtual void PreRegisterClass(WNDCLASS& wc);
    virtual LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    CLadderWindow ladder_;
};

#endif // MAINWINDOW_HPP
