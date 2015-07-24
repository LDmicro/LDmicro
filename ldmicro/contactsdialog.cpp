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
// Dialog for setting the properties of a set of contacts: negated or not,
// plus the name
// Jonathan Westhues, Oct 2004
//-----------------------------------------------------------------------------
#include <windows.h>
#include <stdio.h>
#include <commctrl.h>

#include "ldmicro.h"

static HWND ContactsDialog;

static HWND NegatedCheckbox;
static HWND SourceInternalRelayRadio;
static HWND SourceInputPinRadio;
static HWND SourceOutputPinRadio;
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
    HWND grouper = CreateWindowEx(0, WC_BUTTON, _("Source"),
        WS_CHILD | BS_GROUPBOX | WS_VISIBLE,
        7, 3, 120, 85, ContactsDialog, NULL, Instance, NULL);
    NiceFont(grouper);

    SourceInternalRelayRadio = CreateWindowEx(0, WC_BUTTON, _("Internal Relay"),
        WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_VISIBLE,
        16, 21, 100, 20, ContactsDialog, NULL, Instance, NULL);
    NiceFont(SourceInternalRelayRadio);

    SourceInputPinRadio = CreateWindowEx(0, WC_BUTTON, _("Input pin"),
        WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_VISIBLE,
        16, 41, 100, 20, ContactsDialog, NULL, Instance, NULL);
    NiceFont(SourceInputPinRadio);

    SourceOutputPinRadio = CreateWindowEx(0, WC_BUTTON, _("Output pin"),
        WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_VISIBLE,
        16, 61, 100, 20, ContactsDialog, NULL, Instance, NULL);
    NiceFont(SourceOutputPinRadio);

    HWND textLabel = CreateWindowEx(0, WC_STATIC, _("Name:"),
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT,
        135, 16, 50, 21, ContactsDialog, NULL, Instance, NULL);
    NiceFont(textLabel);

    NameTextbox = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "",
        WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
        190, 16, 115, 21, ContactsDialog, NULL, Instance, NULL);
    FixedFont(NameTextbox);

    NegatedCheckbox = CreateWindowEx(0, WC_BUTTON, _("|/| Negated"),
        WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP | WS_VISIBLE,
        146, 44, 160, 20, ContactsDialog, NULL, Instance, NULL);
    NiceFont(NegatedCheckbox);

    OkButton = CreateWindowEx(0, WC_BUTTON, _("OK"),
        WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE | BS_DEFPUSHBUTTON,
        321, 10, 70, 23, ContactsDialog, NULL, Instance, NULL); 
    NiceFont(OkButton);

    CancelButton = CreateWindowEx(0, WC_BUTTON, _("Cancel"),
        WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
        321, 40, 70, 23, ContactsDialog, NULL, Instance, NULL); 
    NiceFont(CancelButton);

    PrevNameProc = SetWindowLongPtr(NameTextbox, GWLP_WNDPROC, 
        (LONG_PTR)MyNameProc);
}

void ShowContactsDialog(BOOL *negated, char *name)
{
    ContactsDialog = CreateWindowClient(0, "LDmicroDialog",
        _("Contacts"), WS_OVERLAPPED | WS_SYSMENU,
        100, 100, 404, 95, NULL, NULL, Instance, NULL);

    MakeControls();
   
    if(name[0] == 'R') {
        SendMessage(SourceInternalRelayRadio, BM_SETCHECK, BST_CHECKED, 0);
    } else if(name[0] == 'Y') {
        SendMessage(SourceOutputPinRadio, BM_SETCHECK, BST_CHECKED, 0);
    } else {
        SendMessage(SourceInputPinRadio, BM_SETCHECK, BST_CHECKED, 0);
    }
    if(*negated) {
        SendMessage(NegatedCheckbox, BM_SETCHECK, BST_CHECKED, 0);
    }
    SendMessage(NameTextbox, WM_SETTEXT, 0, (LPARAM)(name + 1));

    EnableWindow(MainWindow, FALSE);
    ShowWindow(ContactsDialog, TRUE);
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

        if(IsDialogMessage(ContactsDialog, &msg)) continue;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if(!DialogCancel) {
        if(SendMessage(NegatedCheckbox, BM_GETSTATE, 0, 0) & BST_CHECKED) {
            *negated = TRUE;
        } else {
            *negated = FALSE;
        }
        if(SendMessage(SourceInternalRelayRadio, BM_GETSTATE, 0, 0)
            & BST_CHECKED)
        {
            name[0] = 'R';
        } else if(SendMessage(SourceInputPinRadio, BM_GETSTATE, 0, 0)
            & BST_CHECKED)
        {
            name[0] = 'X';
        } else {
            name[0] = 'Y';
        }
        SendMessage(NameTextbox, WM_GETTEXT, (WPARAM)16, (LPARAM)(name+1));
    }

    EnableWindow(MainWindow, TRUE);
    DestroyWindow(ContactsDialog);
    return;
}
