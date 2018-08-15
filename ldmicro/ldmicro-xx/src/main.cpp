
#include "ldmicroapp.hpp"

extern HINSTANCE Instance;

INT WINAPI WinMain(HINSTANCE hInstance , HINSTANCE hPreviousInstace , LPSTR lpCmdLn , int nCmdShow)
{
    Instance = hInstance;

    CLdmicroApp m_Application;

    return  m_Application.Run();

    (void)hPreviousInstace;
    (void)lpCmdLn;
    (void)nCmdShow;
}
