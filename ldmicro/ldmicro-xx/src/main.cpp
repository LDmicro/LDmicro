
#include "ldmicroapp.hpp"
#include "ldmicro.h"

extern HINSTANCE Instance;
extern char ExePath[MAX_PATH];

INT WINAPI WinMain(HINSTANCE hInstance , HINSTANCE hPreviousInstace , LPSTR lpCmdLn , int nCmdShow)
{
    GetModuleFileName(hInstance, ExePath, MAX_PATH);
    ExtractFilePath(ExePath);
    Instance = hInstance;

    setlocale(LC_ALL, "");

    fillPcPinInfos();

    MakeDialogBoxClass();

    CLdmicroApp m_Application;

    return  m_Application.Run();

    (void)hPreviousInstace;
    (void)lpCmdLn;
    (void)nCmdShow;
}
