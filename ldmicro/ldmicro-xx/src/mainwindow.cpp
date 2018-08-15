/*******************************************************************************
*  file    : mainwindow.cpp
*  created : 15.08.2018
*  author  : Slyshyk Oleksiy (alexslyshyk@gmail.com)
*******************************************************************************/
#include "mainwindow.hpp"

CMainWindow::CMainWindow() : CDockFrame()
{
    SetView(ladder_);
}

CMainWindow::~CMainWindow()
{

}

LRESULT CMainWindow::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return WndProcDefault(uMsg, wParam, lParam);
}
