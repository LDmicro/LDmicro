/*******************************************************************************
*  file    : ladderwindow.hpp
*  created : 15.08.2018
*  author  : Slyshyk Oleksiy (alexslyshyk@gmail.com)
*******************************************************************************/
#ifndef LADDERWINDOW_HPP
#define LADDERWINDOW_HPP

#include "ScrollWnd.hpp"
#include "FontEx.hpp"

class CLadderWindow : public CScrollWnd
{
public:
    CLadderWindow();
    virtual ~CLadderWindow();
protected:
    virtual void OnInitialUpdate();
    virtual void PreCreate(CREATESTRUCT& cs);
    virtual void PreRegisterClass(WNDCLASS& wc);
    virtual void Paint(CDC& dcMem);
    virtual LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
public:
    std::vector<std::string> LadderToText();
    std::vector<std::string> ElementToText();
private:
    void DrawEndRung(int cx, int cy);
private:
    CFontEx     font_;           // the view display font
};

#endif // LADDERWINDOW_HPP
