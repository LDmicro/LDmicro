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
static LRESULT CALLBACK MyNameProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if(msg == WM_CHAR) {
        if(!(isalpha(wParam) || isdigit(wParam) || wParam == '_' || wParam == '\b' || wParam == '@')) {
            return 0;
        }
    }

    return CallWindowProc((WNDPROC)PrevNameProc, hwnd, msg, wParam, lParam);
}

static void MakeControls()
{
    HWND grouper = CreateWindowEx(0, WC_BUTTON, _("Type"), WS_CHILD | BS_GROUPBOX | WS_VISIBLE | WS_TABSTOP, 7, 3, 120, 125, CoilDialog, nullptr, Instance, nullptr);
    NiceFont(grouper);

    NormalRadio = CreateWindowEx(0, WC_BUTTON, _("( ) Normal"), WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_VISIBLE | WS_GROUP, 16, 21, 100, 20, CoilDialog, nullptr, Instance, nullptr);
    NiceFont(NormalRadio);

    NegatedRadio = CreateWindowEx(0, WC_BUTTON, _("(/) Negated"), WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_VISIBLE, 16, 41, 100, 20, CoilDialog, nullptr, Instance, nullptr);
    NiceFont(NegatedRadio);

    SetOnlyRadio = CreateWindowEx(0, WC_BUTTON, _("(S) Set-Only"), WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_VISIBLE, 16, 61, 100, 20, CoilDialog, nullptr, Instance, nullptr);
    NiceFont(SetOnlyRadio);

    ResetOnlyRadio = CreateWindowEx(0, WC_BUTTON, _("(R) Reset-Only"), WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_VISIBLE, 16, 81, 105, 20, CoilDialog, nullptr, Instance, nullptr);
    NiceFont(ResetOnlyRadio);

    TtriggerRadio = CreateWindowEx(0, WC_BUTTON, _("(T) T-trigger"), WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_VISIBLE, 16, 101, 105, 20, CoilDialog, nullptr, Instance, nullptr);
    NiceFont(TtriggerRadio);

    HWND grouper2 = CreateWindowEx(0, WC_BUTTON, _("Source"), WS_CHILD | BS_GROUPBOX | WS_VISIBLE, 135, 3, 150, 85, CoilDialog, nullptr, Instance, nullptr);
    NiceFont(grouper2);

    SourceInternalRelayRadio = CreateWindowEx(0, WC_BUTTON, _("Internal Relay"), WS_CHILD | BS_AUTORADIOBUTTON | WS_VISIBLE | WS_GROUP | WS_TABSTOP, 144, 21, 125, 20, CoilDialog, nullptr, Instance, nullptr);
    NiceFont(SourceInternalRelayRadio);

    SourceMcuPinRadio = CreateWindowEx(0, WC_BUTTON, _("Output Pin on MCU"), WS_CHILD | BS_AUTORADIOBUTTON | WS_VISIBLE | WS_TABSTOP, 144, 41, 125, 20, CoilDialog, nullptr, Instance, nullptr);
    NiceFont(SourceMcuPinRadio);

    SourceModbusRadio = CreateWindowEx(0, WC_BUTTON, _("Modbus"), WS_CHILD | BS_AUTORADIOBUTTON | WS_VISIBLE | WS_TABSTOP, 144, 61, 125, 20, CoilDialog, nullptr, Instance, nullptr);
    NiceFont(SourceModbusRadio);

    HWND textLabel = CreateWindowEx(0, WC_STATIC, _("Name:"), WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT, 135, 90, 50, 21, CoilDialog, nullptr, Instance, nullptr);
    NiceFont(textLabel);

    NameTextbox = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE, 190, 90, 175, 21, CoilDialog, nullptr, Instance, nullptr);
    FixedFont(NameTextbox);

    OkButton = CreateWindowEx(0, WC_BUTTON, _("OK"), WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE | BS_DEFPUSHBUTTON, 295, 10, 70, 23, CoilDialog, nullptr, Instance, nullptr);
    NiceFont(OkButton);

    CancelButton = CreateWindowEx(0, WC_BUTTON, _("Cancel"), WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE, 295, 40, 70, 23, CoilDialog, nullptr, Instance, nullptr);
    NiceFont(CancelButton);

    PrevNameProc = SetWindowLongPtr(NameTextbox, GWLP_WNDPROC, (LONG_PTR)MyNameProc);
}

void ShowCoilDialog(bool *negated, bool *setOnly, bool *resetOnly, bool *ttrigger, char *name)
{
    char nameSave[MAX_NAME_LEN];
    strcpy(nameSave, name);

    CoilDialog = CreateWindowClient(0, "LDmicroDialog", _("Coil"), WS_OVERLAPPED | WS_SYSMENU, 100, 100, 375, 135, nullptr, nullptr, Instance, nullptr);
    RECT r;
    GetClientRect(CoilDialog, &r);

    MakeControls();

    switch(name[0]) {
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

    EnableWindow(MainWindow, false);
    ShowWindow(CoilDialog, true);
    SetFocus(NameTextbox);
    SendMessage(NameTextbox, EM_SETSEL, 0, -1);

    MSG msg;
    DialogDone = false;
    DialogCancel = false;
    while((GetMessage(&msg, nullptr, 0, 0) > 0) && !DialogDone) {
        if(msg.message == WM_KEYDOWN) {
            if(msg.wParam == VK_RETURN) {
                DialogDone = true;
                break;
            } else if(msg.wParam == VK_ESCAPE) {
                DialogDone = true;
                DialogCancel = true;
                break;
            }
        }

        if(IsDialogMessage(CoilDialog, &msg))
            continue;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if(!DialogCancel) {
        if(SendMessage(SourceInternalRelayRadio, BM_GETSTATE, 0, 0) & BST_CHECKED) {
            name[0] = 'R';
        } else if(SendMessage(SourceModbusRadio, BM_GETSTATE, 0, 0) & BST_CHECKED) {
            name[0] = 'M';
        } else {
            name[0] = 'Y';
        }

        SendMessage(NameTextbox, WM_GETTEXT, (WPARAM)(MAX_NAME_LEN - 1), (LPARAM)(name + 1));

        if(SendMessage(NormalRadio, BM_GETSTATE, 0, 0) & BST_CHECKED) {
            *negated = false;
            *setOnly = false;
            *resetOnly = false;
            *ttrigger = false;
        } else if(SendMessage(NegatedRadio, BM_GETSTATE, 0, 0) & BST_CHECKED) {
            *negated = true;
            *setOnly = false;
            *resetOnly = false;
            *ttrigger = false;
        } else if(SendMessage(SetOnlyRadio, BM_GETSTATE, 0, 0) & BST_CHECKED) {
            *negated = false;
            *setOnly = true;
            *resetOnly = false;
            *ttrigger = false;
        } else if(SendMessage(ResetOnlyRadio, BM_GETSTATE, 0, 0) & BST_CHECKED) {
            *negated = false;
            *setOnly = false;
            *resetOnly = true;
            *ttrigger = false;
        } else if(SendMessage(TtriggerRadio, BM_GETSTATE, 0, 0) & BST_CHECKED) {
            *negated = false;
            *setOnly = false;
            *resetOnly = false;
            *ttrigger = true;
        }

        if(strcmp(name, nameSave)) {
            int n = CountWhich(ELEM_CONTACTS, ELEM_COIL, nameSave);
            if(n >= 1) {
                bool rename = false;
                char str[1000];
                sprintf(str, _("Rename the ALL other %d coils/contacts named '%s' to '%s' ?"), n, nameSave, name);
                rename = IDYES == MessageBox(MainWindow, str, "LDmicro", MB_YESNO | MB_ICONQUESTION);
                if(rename)
                    RenameSet1(ELEM_COIL, nameSave, name, false); // rename and reset
            }
        }
    }

    EnableWindow(MainWindow, true);
    SetFocus(MainWindow);
    DestroyWindow(CoilDialog);
    return;
}
