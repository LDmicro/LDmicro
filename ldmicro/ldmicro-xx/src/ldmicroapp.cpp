/*******************************************************************************
*  file    : cldmicroapp.cpp
*  created : 15.08.2018
*  author  : Slyshyk Oleksiy (alexslyshyk@gmail.com)
*******************************************************************************/
#include "ldmicroapp.hpp"

CLdmicroApp::CLdmicroApp() : CWinApp()
{

}

CLdmicroApp::~CLdmicroApp()
{

}

BOOL CLdmicroApp::InitInstance()
{
    main_window_.Create();
    return TRUE;
}
