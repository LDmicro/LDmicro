/*******************************************************************************
*  file    : cldmicroapp.hpp
*  created : 15.08.2018
*  author  : Slyshyk Oleksiy (alexslyshyk@gmail.com)
*******************************************************************************/
#ifndef CLDMICROAPP_HPP
#define CLDMICROAPP_HPP

#include "precomp/ldmicroxxprecomphdr.hpp"
#include "mainwindow.hpp"
#include "ladderwindow.hpp"

class CLdmicroApp : public CWinApp
{
public:
    CLdmicroApp();
    virtual ~CLdmicroApp();
    virtual BOOL InitInstance();

    //CMainWindow& mainWindow() {return main_window_;}
private:
    //CMainWindow main_window_;
    CLadderWindow main_window_;
};

// returns a reference to the CTabbedMDIApp object
inline CLdmicroApp& GetLdmicroApp() { return static_cast<CLdmicroApp&>(GetApp()); }

#endif // CLDMICROAPP_HPP
