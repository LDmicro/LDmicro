//-----------------------------------------------------------------------------
// Copyright 2007 Jonathan Westhues
//
// This file is part of LDmicro.
// 
// LDmicro is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// LDmicro is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with LDmicro.  If not, see <http://www.gnu.org/licenses/>.
//------
//
// Dialog for setting the properties of a set of a RES reset element: name,
// which can be that of either a timer or a counter.
// Jonathan Westhues, Nov 2004
//-----------------------------------------------------------------------------
#include <windows.h>
#include <stdio.h>
#include <commctrl.h>

#include "ldmicro.h"

static HWND ResetDialog;

static HWND TypeTimerRadio;
static HWND TypeCounterRadio;
static HWND NameTextbox;

static LONG_PTR PrevNameProc;

//-----------------------------------------------------------------------------
// Don't allow any characters other than A-Za-z0-9_ in the name.
//-----------------------------------------------------------------------------
static LRESULT CALLBACK MyNameProc(HWND hwnd, UINT msg, WPARAM wParam,
    LPARAM lParam)
{
    if(msg == WM_CHAR) {
        if(!(isalpha(wParam) || isdigit(wParam) || wParam == '_' ||
            wParam == '\b'))
        {
            return 0;
        }
    }

    return CallWindowProc((WNDPROC)PrevNameProc, hwnd, msg, wParam, lParam);
}

static void MakeControls(void)
{
    HWND grouper = CreateWindowEx(0, WC_BUTTON, _("Type"),
        WS_CHILD | BS_GROUPBOX | WS_VISIBLE,
        7, 3, 120, 65, ResetDialog, NULL, Instance, NULL);
    NiceFont(grouper);

    TypeTimerRadio = CreateWindowEx(0, WC_BUTTON, _("Timer"),
        WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_VISIBLE,
        16, 21, 100, 20, ResetDialog, NULL, Instance, NULL);
    NiceFont(TypeTimerRadio);

    TypeCounterRadio = CreateWindowEx(0, WC_BUTTON, _("Counter"),
        WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_VISIBLE,
        16, 41, 100, 20, ResetDialog, NULL, Instance, NULL);
    NiceFont(TypeCounterRadio);

    HWND textLabel = CreateWindowEx(0, WC_STATIC, _("Name:"),
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT,
        135, 16, 50, 21, ResetDialog, NULL, Instance, NULL);
    NiceFont(textLabel);

    NameTextbox = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "",
        WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
        190, 16, 115, 21, ResetDialog, NULL, Instance, NULL);
    FixedFont(NameTextbox);

    OkButton = CreateWindowEx(0, WC_BUTTON, _("OK"),
        WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE | BS_DEFPUSHBUTTON,
        321, 10, 70, 23, ResetDialog, NULL, Instance, NULL); 
    NiceFont(OkButton);

    CancelButton = CreateWindowEx(0, WC_BUTTON, _("Cancel"),
        WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
        321, 40, 70, 23, ResetDialog, NULL, Instance, NULL); 
    NiceFont(CancelButton);

    PrevNameProc = SetWindowLongPtr(NameTextbox, GWLP_WNDPROC, 
        (LONG_PTR)MyNameProc);
}

void ShowResetDialog(char *name)
{
    ResetDialog = CreateWindowClient(0, "LDmicroDialog",
        _("Reset"), WS_OVERLAPPED | WS_SYSMENU,
        100, 100, 404, 75, NULL, NULL, Instance, NULL);

    MakeControls();
   
    if(name[0] == 'T') {
        SendMessage(TypeTimerRadio, BM_SETCHECK, BST_CHECKED, 0);
    } else {
        SendMessage(TypeCounterRadio, BM_SETCHECK, BST_CHECKED, 0);
    }
    SendMessage(NameTextbox, WM_SETTEXT, 0, (LPARAM)(name + 1));

    EnableWindow(MainWindow, FALSE);
    ShowWindow(ResetDialog, TRUE);
    SetFocus(NameTextbox);
    SendMessage(NameTextbox, EM_SETSEL, 0, -1);

    MSG msg;
    DWORD ret;
    DialogDone = FALSE;
    DialogCancel = FALSE;
    while((ret = GetMessage(&msg, NULL, 0, 0)) && !DialogDone) {
        if(msg.message == WM_KEYDOWN) {
            if(msg.wParam == VK_RETURN) {
                DialogDone = TRUE;
                break;
            } else if(msg.wParam == VK_ESCAPE) {
                DialogDone = TRUE;
                DialogCancel = TRUE;
                break;
            }
        }

        if(IsDialogMessage(ResetDialog, &msg)) continue;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if(!DialogCancel) {
        if(SendMessage(TypeTimerRadio, BM_GETSTATE, 0, 0) & BST_CHECKED) {
            name[0] = 'T';
        } else {
            name[0] = 'C';
        }
        SendMessage(NameTextbox, WM_GETTEXT, (WPARAM)16, (LPARAM)(name+1));
    }

    EnableWindow(MainWindow, TRUE);
    DestroyWindow(ResetDialog);
}
