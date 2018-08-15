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

/*
HWND WINAPI CreateWindowEx(
  _In_     DWORD     dwExStyle,
  _In_opt_ LPCTSTR   lpClassName,
  _In_opt_ LPCTSTR   lpWindowName,
  _In_     DWORD     dwStyle,
  _In_     int       x,
  _In_     int       y,
  _In_     int       nWidth,
  _In_     int       nHeight,
  _In_opt_ HWND      hWndParent,
  _In_opt_ HMENU     hMenu,
  _In_opt_ HINSTANCE hInstance,
  _In_opt_ LPVOID    lpParam
);

LPVOID    lpCreateParams;
  HINSTANCE hInstance;
  HMENU     hMenu;
  HWND      hwndParent;
  int       cy;
  int       cx;
  int       y;
  int       x;
  LONG      style;
  LPCTSTR   lpszName;
  LPCTSTR   lpszClass;
  DWORD     dwExStyle;


 */

void CLadderWindow::PreCreate(CREATESTRUCT& cs)
{
    HMENU top = MakeMainWindowMenus();

    cs.hInstance = Instance;
    cs.hMenu = top;
    cs.hwndParent = NULL;
    cs.lpszClass = "LDmicro";
    cs.lpszName = "";
}

