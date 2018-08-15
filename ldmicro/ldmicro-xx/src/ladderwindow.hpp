/*******************************************************************************
*  file    : ladderwindow.hpp
*  created : 15.08.2018
*  author  : Slyshyk Oleksiy (alexslyshyk@gmail.com)
*******************************************************************************/
#ifndef LADDERWINDOW_HPP
#define LADDERWINDOW_HPP

#include "precomp/ldmicroxxprecomphdr.hpp"

class CLadderWindow : public CWnd
{
public:
    CLadderWindow();
    virtual ~CLadderWindow();
protected:
    virtual void OnInitialUpdate();
    virtual LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual void PreCreate(CREATESTRUCT& cs);
};

#endif // LADDERWINDOW_HPP
