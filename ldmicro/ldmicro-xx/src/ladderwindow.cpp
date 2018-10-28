/*******************************************************************************
*  file    : ladderwindow.cpp
*  created : 15.08.2018
*  author  : Slyshyk Oleksiy (alexslyshyk@gmail.com)
*******************************************************************************/
#include "ladderwindow.hpp"
#include "ldmicro.h"

extern HWND MainWindow;
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

static const COLORREF rgbDefaultBkColor       = RGB(0, 0, 0);

CLadderWindow::CLadderWindow() : CScrollWnd()
{
    SetWndBkColor(rgbDefaultBkColor);
}

CLadderWindow::~CLadderWindow()
{

}

void CLadderWindow::OnInitialUpdate()
{
    ShowHScrollBar(0);
}

void CLadderWindow::PreCreate(CREATESTRUCT& cs)
{
    CClientDC dc(*this);
    // set font metric to pixels
    dc.SetMapMode(MM_TEXT);
    font_.SetDefault(dc);

      // Set the extended style to include a 3-D look with border and
      // sunken edge
    cs.dwExStyle = WS_EX_CLIENTEDGE;
    CScrollWnd::PreCreate(cs);
}

void CLadderWindow::PreRegisterClass(WNDCLASS& wc)
{
    // Set the Window Class name
    wc.lpszClassName = _T("LDmicro View");
}

void CLadderWindow::Paint(CDC& dcMem)
{
    int  cx = 0;
    int  cy = 0;
}

LRESULT CLadderWindow::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return CScrollWnd::WndProc(uMsg, wParam, lParam);
}

std::vector<std::string> CLadderWindow::LadderToText()
{
    std::vector<std::string> result;
    int  maxWidth = ProgCountWidestRow();
    int totalHeight = ProgCountRows();
    totalHeight += 1; // EndRung
    totalHeight *= POS_HEIGHT;
    totalHeight += Prog.numRungs; // for one empty line between rungs
    totalHeight += 2;             // after EndRung for # of int and # of AVR/PIC
    //totalHeight is Ok!
    int l = maxWidth * POS_WIDTH + 9;
    for(int i = 0; i < totalHeight; i++)
        {
            std::string s(l, ' ');
            result.emplace_back(s);
        }

    char str[10] = "";
    int  cx;
    int  cy = 1;

    for(int i = 0; i < Prog.numRungs; ++i)
        {
            cx = 6;
        }

    return result;
}

void CLadderWindow::DrawEndRung(int cx, int cy)
{
//    CenterWithWires(cx, cy, "[END]", false, false);
//    cx += POS_WIDTH;
//    for(int i = 1; i < ColsAvailable; i++)
//        DrawWire(&cx, &cy, '-');
}

