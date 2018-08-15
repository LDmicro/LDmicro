/*******************************************************************************
*  file    : ladderwindow.cpp
*  created : 15.08.2018
*  author  : Slyshyk Oleksiy (alexslyshyk@gmail.com)
*******************************************************************************/
#include "ladderwindow.hpp"
#include "ldmicro.h"

extern HWND MainWindow;
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

CLadderWindow::CLadderWindow() : CWnd()
{

}

CLadderWindow::~CLadderWindow()
{

}

void CLadderWindow::OnInitialUpdate()
{
    MainWindow = this->GetHwnd();
    InitForDrawing();

    MakeMainWindowControls();
    MainWindowResized();

    NewProgram();
    strcpy(CurrentSaveFile, "");

    ::SetTimer(MainWindow, TIMER_BLINK_CURSOR, 800, BlinkCursor);
}

LRESULT CLadderWindow::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    auto res = MainWndProc(GetHwnd(), uMsg, wParam, lParam);
    if( res == 0)
        res = WndProcDefault(uMsg, wParam, lParam);

    return res;
}

