#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>
#include <memory>

#include "ldmicro.h"
#include "ldconfig.h"
#include "flash.h"
#include "ldlog.hpp"

buflist *stBuf = NULL;
buflist *startBuf = NULL;

HWND       hwndChildTexte = NULL;
HANDLE     hChild_Stdout_Rd = NULL;
HANDLE     hChild_Stdout_Wr = NULL;
OVERLAPPED over;

BOOL running = FALSE;



UINT codepage = 866; // 850

struct BuildRunData {
    std::string batchfile;
    std::string fpath1;
    std::string fname2;
    std::string target3;
    std::string compiler4;
    std::string progtool5;
};

typedef std::shared_ptr<BuildRunData> BuildRunDataPtr;

int CreateChildThread(BuildRunDataPtr runData);

// Capture function
void Capture(const char * title, char *batchFile, char *fpath1, char *fname2, const char *target3, char *compiler4, char *progtool5)
{
#if 1
    char batchArgs[MAX_PATH] = "";
//  sprintf(batchArgs, "\"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\"", batchfile, fpath1, fname2, target3, compiler4, progtool5);
//	sprintf(batchArgs, "\"%s\" \"%s\" \"%s\" \"%s\" \"%s\"", fpath1, fname2, target3, compiler4, progtool5);
	if(strlen(fpath1)||strlen(fname2)||strlen(target3)||strlen(compiler4)||strlen(progtool5)) {
		strcat(batchArgs, "\"");
		if(strlen(fpath1))
		   strcat(batchArgs, fpath1);
		else
		   strcat(batchArgs, " ");
		strcat(batchArgs, "\"");
	}
	if(strlen(fname2)||strlen(target3)||strlen(compiler4)||strlen(progtool5)) {
		strcat(batchArgs, " \"");
		if(strlen(fname2))
			strcat(batchArgs, fname2);
		else
		   strcat(batchArgs, " ");
		strcat(batchArgs, "\"");
	}
	if(strlen(target3)||strlen(compiler4)||strlen(progtool5)) {
		strcat(batchArgs, " \"");
		if(strlen(target3))
			strcat(batchArgs, target3);
		else
		   strcat(batchArgs, " ");
		strcat(batchArgs, "\"");
	}
	if(strlen(compiler4)||strlen(progtool5)) {
		strcat(batchArgs, " \"");
		if(strlen(compiler4))
			strcat(batchArgs, compiler4);
		else
		   strcat(batchArgs, " ");
		strcat(batchArgs, "\"");
	}
	if(strlen(progtool5)) {
		strcat(batchArgs, " \"");
		strcat(batchArgs, progtool5);
		strcat(batchArgs, "\"");
	}
	IsErr(Execute(batchFile, batchArgs, SW_SHOWNORMAL), batchFile);
#else
    WNDCLASSEX wc;
    RECT       rect;

    BuildRunDataPtr runData = std::shared_ptr<BuildRunData>(new BuildRunData);
    runData->batchfile = batchfile;
    runData->fpath1 = fpath1;
    runData->fname2 = fname2;
    runData->target3 = target3;
    runData->compiler4 = compiler4;
    runData->progtool5 = progtool5;


    if(!running) {
        running = TRUE;
        /*
        AllocConsole(); // Console window is displayed. :(
        codepage = GetConsoleCP(); // Get local codepage (850 for instance)
        FreeConsole();
        */
        //codepage = GetACP();
        codepage = GetOEMCP();

        // Window class for captures
        memset(&wc, 0, sizeof(wc));
        wc.cbSize = sizeof(wc);

        wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wc.lpfnWndProc = (WNDPROC)WndProcTexte;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = Instance;
        wc.hIcon = NULL;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)CreateSolidBrush(BACK_COLOR);
        wc.lpszMenuName = NULL;
        wc.lpszClassName = "WTEXTCLASS";

        RegisterClassEx(&wc);

        GetWindowRect(MainWindow, &rect);

        hwndChildTexte = CreateWindow("WTEXTCLASS",
                                      title,
                                      WS_POPUP | WS_OVERLAPPED | WS_VISIBLE | WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_VSCROLL | ES_LEFT | WS_MINIMIZEBOX | ES_MULTILINE,
                                      rect.left + 100,
                                      rect.top + 100,
                                      700,
                                      rect.bottom - 100,
                                      MainWindow,
                                      NULL,
                                      Instance,
                                      NULL);

        //SetWindowPos(hwndChildTexte, HWND_TOP, rect.left + 100, rect.top + 100, 700, rect.bottom - 100, SWP_SHOWWINDOW);

        //Sleep(200);

        CreateChildThread(runData);
    }
#endif
}

// Create a Thread (to free Main window Proc )
int CreateChildThread(BuildRunDataPtr runData)
{
    HANDLE ChildThread = NULL;
    DWORD  dwThreadId;

    struct ThData {
        HANDLE child;
        BuildRunDataPtr arg;
        ThData()
        {
            child = NULL;
            arg.reset();
        }
    };

    static std::vector<std::shared_ptr<ThData> > thData;

    thData.erase(std::remove_if(std::begin(thData),
                                std::end(thData),
                                [](std::shared_ptr<ThData> &x) -> bool {
        DWORD exitCode;
        auto  res = GetExitCodeThread(x->child, &exitCode);
        if((res > 0) && (exitCode != STILL_ACTIVE)) {
            return true;
        }
        return false;
        }), std::end(thData));


    ChildThread = CreateThread(NULL,           // default security attributes
                               0,              // use default stack size
                               ThreadFunction, // thread function
                               (LPVOID) runData.get(),  // argument passed to thread function
                               0,              // use default creation flags
                               &dwThreadId);   // returns the thread identifier

    if(ChildThread != NULL) {
        auto data = std::make_shared<ThData>();
        data->arg = runData;
        data->child = ChildThread;
        thData.push_back(data);
    }
    return 0;
}

// Thread function (with specific prototype)
DWORD WINAPI ThreadFunction(LPVOID lpParam)
{
    char  command[CMDSIZE];
	char comspec[MAX_PATH*2];

	GetComspec(comspec, sizeof(comspec));

    const BuildRunData *rd = reinterpret_cast<const BuildRunData *>(lpParam);
    sprintf(command, "%s /c \"\"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\"\"", comspec, rd->batchfile.c_str(), 
        rd->fpath1.c_str(), rd->fname2.c_str(), rd->target3.c_str(), rd->compiler4.c_str(), rd->progtool5.c_str());

    CreateChildPiped(command);
    return 0;
}

// Create Child process and Pipe
int CreateChildPiped(char *cmdline)
{
    SECURITY_ATTRIBUTES saAttr;
    //BOOL                conv = FALSE;
    DWORD               dwWritten;

    // Set the bInheritHandle flag so that pipe handles are inherited
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    // Create an anonymous pipe to make master and child communicate
    if(!CreatePipe(&hChild_Stdout_Rd, &hChild_Stdout_Wr, &saAttr, 0))
        Error(_("Pipe creation error"));

    // Ensure the read handle to the pipe for STDOUT can't be inherited
    if(!SetHandleInformation(hChild_Stdout_Rd, HANDLE_FLAG_INHERIT, 0))
        Error(_("Pipe error"));

    int mute = 0;
    if(cmdline[0] == '@')
        mute = 1; // dont display command if @

    // Send the command line to the pipe
    if(mute == 0) {
        WriteFile(hChild_Stdout_Wr, cmdline, strlen(cmdline), &dwWritten, NULL);
        WriteFile(hChild_Stdout_Wr, "\r\n\r\n", 4, &dwWritten, NULL);
    }

    // Create child process
    CreateChildProcess(&cmdline[mute]);

    // Write handle must absolutely be closed before calling ReadFromPipe()
    // so that the pipe be fully closed when child process ends
    // so that ReadFile() shouldn't block infinitely !!!
    CloseHandle(hChild_Stdout_Wr);

    // Read from pipe that is the standard output for child process
    ReadFromPipe(true);

    // Close last access to the pipe to set it free
    CloseHandle(hChild_Stdout_Rd);
    hChild_Stdout_Rd = NULL;

    return 0;
}

// Create Child process and redirec STDOUT + STDERR to Pipe
void CreateChildProcess(char *cmdline)
{
    PROCESS_INFORMATION piProcInfo;
    STARTUPINFO         siStartInfo;
    BOOL                bSuccess = FALSE;

    // Set up members of the PROCESS_INFORMATION structure
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

    // Set up members of the STARTUPINFO structure
    // This structure specifies the STDIN and STDOUT handles for redirection
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    siStartInfo.hStdError = hChild_Stdout_Wr;
    siStartInfo.hStdOutput = hChild_Stdout_Wr;
    siStartInfo.hStdInput = INVALID_HANDLE_VALUE;
    siStartInfo.wShowWindow = SW_HIDE; // Hide window at startup

    // Create child process
    bSuccess = CreateProcess(NULL,
                             cmdline,      // command line
                             NULL,         // process security attributes
                             NULL,         // primary thread security attributes
                             TRUE,         // handles are inherited
                             0,            // creation flags
                             NULL,         // use parent's environment
                             NULL,         // use parent's current directory
                             &siStartInfo, // STARTUPINFO pointer
                             &piProcInfo); // receives PROCESS_INFORMATION

    // If an error occurs
    if(!bSuccess)
        Error(_("Exec error from batch file"));
    else {
        // Close handles to the child process and its primary thread
        // Some applications might keep these handles to monitor the status
        // of the child process, for example.
        CloseHandle(piProcInfo.hProcess);
        CloseHandle(piProcInfo.hThread);
    }
}

// Read STDOUT / STDERR from Pipe
// Infine loop breaks only when Child process ends because Read() returns 0
// Else ReadFile() is blocking
void ReadFromPipe(BOOL convert)
{
    DWORD    dwRead;
    BOOL     bSuccess = FALSE;
    //HANDLE   hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CHAR     chBuf[BUFSIZE + 1];
    buflist *bufptr;

    while(1) {
        bSuccess = ReadFile(hChild_Stdout_Rd, chBuf, BUFSIZE, &dwRead, NULL);
        if((!bSuccess) || (dwRead == 0))
            break;
        else {
            bufptr = stBuf;
            stBuf = (buflist *)malloc(sizeof(buflist));
            stBuf->next = NULL;
            //stBuf->prev = bufptr;

            chBuf[dwRead] = 0;
            stBuf->buffer = (CHAR *)malloc(dwRead + 1);
            strcpy(stBuf->buffer, chBuf);
            if(convert)
                CodePage(stBuf->buffer); // if command = cmd.exe

            if(startBuf == NULL)
                startBuf = stBuf;
            if(bufptr != NULL)
                bufptr->next = stBuf;

            PostMessage(hwndChildTexte, WM_TIMER, 1, 0); // to refresh Capture window
        }
    }

    bufptr = stBuf; // add an empty line
    stBuf = (buflist *)malloc(sizeof(buflist));
    stBuf->next = NULL;
    //stBuf->prev = bufptr;
    stBuf->buffer = (CHAR *)malloc(4);
    strcpy(stBuf->buffer, "\r\n");
    if(bufptr != NULL)
        bufptr->next = stBuf;

    PostMessage(hwndChildTexte, WM_TIMER, 1, 0); // to refresh Capture window
}

// convert Code page 850 to Windows local Code page
void CodePage(LPSTR lpString)
{
    int     n = 0;
    wchar_t wString[BUFSIZE + 1];
    n = MultiByteToWideChar(/*codepage*/ 1 ? CP_OEMCP : GetOEMCP(), 0, lpString, -1, wString, sizeof(wString) / sizeof(wchar_t));
    WideCharToMultiByte(1?CP_ACP:GetACP(), 0, wString, n, lpString, strlen(lpString), NULL, NULL); // ok
    //WideCharToMultiByte(CP_THREAD_ACP, 0, wString, n, lpString, strlen(lpString), NULL, NULL); // ok
}

// Capture window Proc
LRESULT CALLBACK WndProcTexte(HWND hwndTexte, UINT message, WPARAM wParam, LPARAM lParam)
{
    static LOGFONT police;
    static HFONT   hPol = NULL;
    static int     cxChar, cxCaps, cyChar, cxClient, cyClient;
    static int     lignes = 0;
    static int     total = -1, oldtotal = -1;
    static int     VscrollPos = 0, scroll = 0, maxaff = 0;

    HDC         hdc;
    PAINTSTRUCT ps;
    TEXTMETRIC  tm;

    buflist * bufptr;
    int       index, pos;
    short int wheel;
    BOOL      cont = FALSE;
    char      line[100];

    switch(message) {

        case WM_CREATE:
            hdc = GetDC(hwndTexte);

            police.lfPitchAndFamily = FF_MODERN;
            strcpy(police.lfFaceName, "COURIER NEW");

            police.lfWeight = 300;
            police.lfItalic = 0;
            police.lfUnderline = 0;
            police.lfHeight = 16;
            police.lfWidth = 0;
            police.lfEscapement = 0;
            police.lfOrientation = 0;
            police.lfStrikeOut = 0;
            police.lfCharSet = GetOEMCP(); //DEFAULT_CHARSET; //GetACP(); //GetOEMCP();           //// ANSI_CHARSET; //// OEM_CHARSET;
            police.lfOutPrecision = OUT_TT_PRECIS;
            police.lfClipPrecision = CLIP_TT_ALWAYS;
            police.lfQuality = DEFAULT_QUALITY;
            hPol = CreateFontIndirect(&police);

            SelectObject(hdc, hPol);

            GetTextMetrics(hdc, &tm);
            cxChar = tm.tmAveCharWidth;
            cxCaps = (tm.tmPitchAndFamily & 1 ? 3 : 2) * cxChar / 2;
            cyChar = tm.tmHeight + tm.tmExternalLeading;
            ReleaseDC(hwndTexte, hdc);

            lignes = 0;
            total = -1;
            oldtotal = -1;
            scroll = 0;
            maxaff = 0;

            //MoveWindow(hwndTexte, 40, 80, 800, 400, TRUE);

            return 0;

        case WM_SIZE:
            cyClient = HIWORD(lParam);
            cxClient = LOWORD(lParam);
            return 0;

        case WM_SETFOCUS:
            SetFocus(hwndTexte);

            return 0;

        case WM_LBUTTONDOWN:

            return 0;

        case WM_VSCROLL:
            switch(LOWORD(wParam)) {
                case SB_LINEUP:
                    VscrollPos--;
                    break;

                case SB_LINEDOWN:
                    VscrollPos++;
                    break;

                case SB_PAGEUP:
                    VscrollPos -= cyClient / cyChar;
                    break;

                case SB_PAGEDOWN:
                    VscrollPos += cyClient / cyChar;
                    break;

                case SB_TOP:
                    VscrollPos = 0;
                    break;

                case SB_BOTTOM:
                    VscrollPos = total;
                    break;

                case SB_THUMBPOSITION:
                    VscrollPos = HIWORD(wParam);
                    break;

                default:
                    return 0;
            }

            VscrollPos = std::max(0, std::min(VscrollPos, total)); // securite  0 <= VscrollPos <= total

            if(VscrollPos != GetScrollPos(hwndTexte, SB_VERT)) {
                SetScrollPos(hwndTexte, SB_VERT, VscrollPos, TRUE);
                InvalidateRect(hwndTexte, NULL, TRUE);
            }
            return 0;

        case WM_KEYDOWN:
            switch(LOWORD(wParam)) {
                case VK_HOME:
                    SendMessage(hwndTexte, WM_VSCROLL, SB_TOP, 0L);
                    break;

                case VK_END:
                    SendMessage(hwndTexte, WM_VSCROLL, SB_BOTTOM, 0L);
                    break;

                case VK_PRIOR:
                    SendMessage(hwndTexte, WM_VSCROLL, SB_PAGEUP, 0L);
                    break;

                case VK_NEXT:
                    SendMessage(hwndTexte, WM_VSCROLL, SB_PAGEDOWN, 0L);
                    break;

                case VK_UP:
                    SendMessage(hwndTexte, WM_VSCROLL, SB_LINEUP, 0L);
                    break;

                case VK_DOWN:
                    SendMessage(hwndTexte, WM_VSCROLL, SB_LINEDOWN, 0L);
                    break;

                default:
                    if(hChild_Stdout_Rd == NULL)
                        PostMessage(hwndTexte, WM_CLOSE, 0, 0);
            }
            return 0;

        case WM_MOUSEWHEEL:
            wheel = (short int)HIWORD(wParam);
            if(wheel > 0)
                SendMessage(hwndTexte, WM_VSCROLL, SB_LINEUP, 0L);
            if(wheel < 0)
                SendMessage(hwndTexte, WM_VSCROLL, SB_LINEDOWN, 0L);
            return 0;

        case WM_RBUTTONDBLCLK:
            return 0;

        case WM_TIMER:
            if(wParam == 1) // timer 1 pour rafraichissement periodique de la fenetre
                InvalidateRect(hwndTexte, NULL, TRUE);

            return 0;

        case WM_PAINT:
            hdc = BeginPaint(hwndTexte, &ps);

            SetBkColor(hdc, BACK_COLOR);
            SelectObject(hdc, hPol);

            lignes = 0;
            cont = FALSE;
            pos = 0;

            if(stBuf == NULL) {
                EndPaint(hwndTexte, &ps);
                return 0;
            }

            // comptage des lignes a afficher :

            bufptr = startBuf;
            while(bufptr != NULL) {
                index = 0;
                if(!cont)
                    pos = 0;
                cont = FALSE;

                while(bufptr->buffer[index] != 0) {
                    while((pos < 80) && (bufptr->buffer[index] != '\n') && (bufptr->buffer[index] != 0)) {
                        index++;
                        pos++;
                    }

                    if(bufptr->buffer[index] == '\n') // ligne entiere terminee par '\n' ou '\r\n'
                    {
                        index++;
                        lignes++;
                        pos = 0;
                    } else if(pos == 80) // ligne pleine mais inachevee
                    {
                        lignes++;
                        pos = 0;
                    } else {
                        cont = TRUE; // ligne inachevee => attendre la suite
                    }
                }

                bufptr = (buflist *)bufptr->next;
            }

            total = lignes;
            maxaff = cyClient / cyChar - 1; // nb de lignes affichable
            if(maxaff < 0)
                maxaff = 0;

            if(total != oldtotal) {
                VscrollPos = total;
                SetScrollRange(hwndTexte, SB_VERT, 0, total, FALSE); // scroll range= 0 -> total
                SetScrollPos(hwndTexte, SB_VERT, total, TRUE);       // scroll box tout en bas
            }
            oldtotal = total;

            scroll = total - VscrollPos;
            if(scroll + maxaff > total)
                scroll = std::max(total - maxaff, 0);

            // affichage des lignes :

            lignes = 0;
            cont = FALSE;
            pos = 0;

            bufptr = startBuf;
            while(bufptr != NULL) {
                index = 0;
                if(!cont)
                    pos = 0;
                cont = FALSE;

                if((bufptr->buffer[0] == '\r') || (bufptr->buffer[0] == '\n')) // si saut de ligne
                {
                    index++;
                }

                while(bufptr->buffer[index] != 0) {
                    while((pos < 80) && (bufptr->buffer[index] != '\n') && (bufptr->buffer[index] != 0)) {
                        line[pos] = bufptr->buffer[index];
                        index++;
                        pos++;
                    }

                    if(bufptr->buffer[index] == '\n') // ligne entiere
                    {
                        line[pos] = 0;                                         // saute '\n'
                        if((index > 0) && (bufptr->buffer[index - 1] == '\r')) // si '\r\n'
                        {
                            if(pos > 0) {
                                pos--;
                                line[pos] = 0;
                            }
                        }
                        index++;

                        lignes++;

                        if(maxaff > total)
                            TextOut(hdc, 10, lignes * cyChar, line, pos);
                        else if((total - lignes >= scroll) && (total - lignes < maxaff + scroll))
                            TextOut(hdc, 10, (maxaff - total + lignes + scroll) * cyChar, line, pos);

                        pos = 0;
                    } else if(pos == 80) // ligne pleine mais inachevee
                    {
                        lignes++;

                        if(maxaff > total)
                            TextOut(hdc, 10, lignes * cyChar, line, pos);
                        else if((total - lignes >= scroll) && (total - lignes < maxaff + scroll))
                            TextOut(hdc, 10, (maxaff - total + lignes + scroll) * cyChar, line, pos);

                        pos = 0;
                    } else {
                        cont = TRUE; // ligne inachevee => attendre la suite
                    }
                }

                bufptr = (buflist *)bufptr->next;
            }

            EndPaint(hwndTexte, &ps);
            return 0;

        case WM_CLOSE:
            DestroyWindow(hwndTexte);
            return 0;

        case WM_DESTROY:
            if(hPol != NULL)
                DeleteObject(hPol);

            stBuf = startBuf;
            while(stBuf != NULL) {
                free(stBuf->buffer);
                bufptr = stBuf;
                stBuf = (buflist *)stBuf->next;
                free(bufptr);
            }
            stBuf = NULL;
            startBuf = NULL;

            running = FALSE;

            return 0;
    }
    return (DefWindowProc(hwndTexte, message, wParam, lParam));
}
/*
// Utilities
int min(int x, int y)
{
    if(x < y)
        return x;
    else
        return y;
}

int max(int x, int y)
{
    if(x > y)
        return x;
    else
        return y;
}
*/
