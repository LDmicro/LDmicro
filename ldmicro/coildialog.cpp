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
// Dialog for setting the properties of a relay coils: negated or not,
// plus the name, plus set-only or reset-only
// Jonathan Westhues, Oct 2004
//-----------------------------------------------------------------------------
#include "stdafx.h"

#include "ldmicro.h"

static HWND CoilDialog;

static HWND SourceInternalRelayRadio;
static HWND SourceMcuPinRadio;
static HWND SourceModbusRadio;
static HWND NegatedRadio;
static HWND NormalRadio;
static HWND SetOnlyRadio;
static HWND ResetOnlyRadio;
static HWND TtriggerRadio;
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
            wParam == '\b' || wParam == '@'))
        {
            return 0;
        }
    }

    return CallWindowProc((WNDPROC)PrevNameProc, hwnd, msg, wParam, lParam);
}

static void MakeControls(void)
{
    HWND grouper = CreateWindowExW(0, WC_BUTTONW, _("Type"),
        WS_CHILD | BS_GROUPBOX | WS_VISIBLE | WS_TABSTOP,
        7, 3, 120, 125, CoilDialog, NULL, Instance, NULL);
    NiceFont(grouper);

    NormalRadio = CreateWindowExW(0, WC_BUTTONW, _("( ) Normal"),
        WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_VISIBLE | WS_GROUP,
        16, 21, 100, 20, CoilDialog, NULL, Instance, NULL);
    NiceFont(NormalRadio);

    NegatedRadio = CreateWindowExW(0, WC_BUTTONW, _("(/) Negated"),
        WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_VISIBLE,
        16, 41, 100, 20, CoilDialog, NULL, Instance, NULL);
    NiceFont(NegatedRadio);

    SetOnlyRadio = CreateWindowExW(0, WC_BUTTONW, _("(S) Set-Only"),
        WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_VISIBLE,
        16, 61, 100, 20, CoilDialog, NULL, Instance, NULL);
    NiceFont(SetOnlyRadio);

    ResetOnlyRadio = CreateWindowExW(0, WC_BUTTONW, _("(R) Reset-Only"),
        WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_VISIBLE,
        16, 81, 105, 20, CoilDialog, NULL, Instance, NULL);
    NiceFont(ResetOnlyRadio);

    TtriggerRadio = CreateWindowExW(0, WC_BUTTONW, _("(T) T-trigger"),
        WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_VISIBLE,
        16, 101, 105, 20, CoilDialog, NULL, Instance, NULL);
    NiceFont(TtriggerRadio);

    HWND grouper2 = CreateWindowExW(0, WC_BUTTONW, _("Source"),
        WS_CHILD | BS_GROUPBOX | WS_VISIBLE,
        135, 3, 150, 85, CoilDialog, NULL, Instance, NULL);
    NiceFont(grouper2);

    SourceInternalRelayRadio = CreateWindowExW(0, WC_BUTTONW, _("Internal Relay"),
        WS_CHILD | BS_AUTORADIOBUTTON | WS_VISIBLE | WS_GROUP | WS_TABSTOP,
        144, 21, 125, 20, CoilDialog, NULL, Instance, NULL);
    NiceFont(SourceInternalRelayRadio);

    SourceMcuPinRadio = CreateWindowExW(0, WC_BUTTONW, _("Output Pin on MCU"),
        WS_CHILD | BS_AUTORADIOBUTTON | WS_VISIBLE | WS_TABSTOP,
        144, 41, 125, 20, CoilDialog, NULL, Instance, NULL);
    NiceFont(SourceMcuPinRadio);

    SourceModbusRadio = CreateWindowExW(0, WC_BUTTONW, _("Modbus"),
        WS_CHILD | BS_AUTORADIOBUTTON | WS_VISIBLE | WS_TABSTOP,
        144, 61, 125, 20, CoilDialog, NULL, Instance, NULL);
    NiceFont(SourceModbusRadio);

    HWND textLabel = CreateWindowExW(0, WC_STATICW, _("Name:"),
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT,
        135, 90, 50, 21, CoilDialog, NULL, Instance, NULL);
    NiceFont(textLabel);

    NameTextbox = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, L"",
        WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
        190, 90, 175, 21, CoilDialog, NULL, Instance, NULL);
    FixedFont(NameTextbox);

    OkButton = CreateWindowExW(0, WC_BUTTONW, _("OK"),
        WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE | BS_DEFPUSHBUTTON,
        295, 10, 70, 23, CoilDialog, NULL, Instance, NULL);
    NiceFont(OkButton);

    CancelButton = CreateWindowExW(0, WC_BUTTONW, _("Cancel"),
        WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
        295, 40, 70, 23, CoilDialog, NULL, Instance, NULL);
    NiceFont(CancelButton);

    PrevNameProc = SetWindowLongPtr(NameTextbox, GWLP_WNDPROC,
        (LONG_PTR)MyNameProc);
}

void ShowCoilDialog(BOOL *negated, BOOL *setOnly, BOOL *resetOnly, BOOL *ttrigger, char *name)
{
    char nameSave[MAX_NAME_LEN];
    strcpy(nameSave, name);

    CoilDialog = CreateWindowClient(0, L"LDmicroDialog",
        _("Coil"), WS_OVERLAPPED | WS_SYSMENU,
        100, 100, 375, 135, NULL, NULL, Instance, NULL);
    RECT r;
    GetClientRect(CoilDialog, &r);

    MakeControls();

    switch (name[0]) {
    case 'R':
        SendMessage(SourceInternalRelayRadio, BM_SETCHECK, BST_CHECKED, 0);
        break;
    case 'Y':
        SendMessage(SourceMcuPinRadio, BM_SETCHECK, BST_CHECKED, 0);
        break;
    case 'M':
        SendMessage(SourceModbusRadio, BM_SETCHECK, BST_CHECKED, 0);
        break;
    default:
        oops();
        break;
    }

    SendMessage(NameTextbox, WM_SETTEXT, 0, (LPARAM)(name + 1));
    if(*negated) {
        SendMessage(NegatedRadio, BM_SETCHECK, BST_CHECKED, 0);
    } else if(*setOnly) {
        SendMessage(SetOnlyRadio, BM_SETCHECK, BST_CHECKED, 0);
    } else if(*resetOnly) {
        SendMessage(ResetOnlyRadio, BM_SETCHECK, BST_CHECKED, 0);
    } else if(*ttrigger) {
        SendMessage(TtriggerRadio, BM_SETCHECK, BST_CHECKED, 0);
    } else {
        SendMessage(NormalRadio, BM_SETCHECK, BST_CHECKED, 0);
    }

    EnableWindow(MainWindow, FALSE);
    ShowWindow(CoilDialog, TRUE);
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

        if(IsDialogMessage(CoilDialog, &msg)) continue;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if(!DialogCancel) {
        if(SendMessage(SourceInternalRelayRadio, BM_GETSTATE, 0, 0)
            & BST_CHECKED)
        {
            name[0] = 'R';
        } else if (SendMessage(SourceModbusRadio, BM_GETSTATE, 0, 0)
            & BST_CHECKED)
        {
            name[0] = 'M';
        } else {
            name[0] = 'Y';
        }

        SendMessage(NameTextbox, WM_GETTEXT, (WPARAM)(MAX_NAME_LEN-1), (LPARAM)(name+1));

        if(SendMessage(NormalRadio, BM_GETSTATE, 0, 0) & BST_CHECKED) {
            *negated = FALSE;
            *setOnly = FALSE;
            *resetOnly = FALSE;
            *ttrigger = FALSE;
        } else if(SendMessage(NegatedRadio, BM_GETSTATE, 0, 0) & BST_CHECKED) {
            *negated = TRUE;
            *setOnly = FALSE;
            *resetOnly = FALSE;
            *ttrigger = FALSE;
        } else if(SendMessage(SetOnlyRadio, BM_GETSTATE, 0, 0) & BST_CHECKED) {
            *negated = FALSE;
            *setOnly = TRUE;
            *resetOnly = FALSE;
            *ttrigger = FALSE;
        } else if(SendMessage(ResetOnlyRadio, BM_GETSTATE, 0, 0) & BST_CHECKED) {
            *negated = FALSE;
            *setOnly = FALSE;
            *resetOnly = TRUE;
            *ttrigger = FALSE;
        } else if(SendMessage(TtriggerRadio, BM_GETSTATE, 0, 0) & BST_CHECKED) {
            *negated = FALSE;
            *setOnly = FALSE;
            *resetOnly = FALSE;
            *ttrigger = TRUE;
        }

        if(strcmp(name, nameSave)) {
          int n = CountWhich(ELEM_CONTACTS, ELEM_COIL, nameSave);
          if(n >= 1) {
            BOOL rename = FALSE;
            wchar_t str[1000];
            swprintf_s(str, _("Rename the ALL other %d coils/contacts named '%ls' to '%ls' ?"), n, u16(nameSave), u16(name));
            rename = IDYES == MessageBoxW(MainWindow, str, L"LDmicro", MB_YESNO | MB_ICONQUESTION);
            if(rename)
                RenameSet1(ELEM_COIL, nameSave, name, FALSE); // rename and reset
          }
        }
    }

    EnableWindow(MainWindow, TRUE);
    SetFocus(MainWindow);
    DestroyWindow(CoilDialog);
    return;
}
