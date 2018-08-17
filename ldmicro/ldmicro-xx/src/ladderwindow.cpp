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

}

void CLadderWindow::PreCreate(CREATESTRUCT& cs)
{
    cs.style = WS_CHILD /*| WS_HSCROLL*/ | WS_VSCROLL;
}

LRESULT CLadderWindow::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return WndProcDefault(uMsg, wParam, lParam);
}

