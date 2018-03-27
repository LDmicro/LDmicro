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
#include "stdafx.h"

#include "ldmicro.h"

static HWND ContactsDialog;

static HWND NegatedCheckbox;
static HWND Set1Checkbox;
static HWND SourceInternalRelayRadio;
static HWND SourceInputPinRadio;
static HWND SourceOutputPinRadio;
static HWND SourceModbusContactRadio;
static HWND SourceModbusCoilRadio;
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
    HWND grouper = CreateWindowExW(0, WC_BUTTONW, _("Source"),
        WS_CHILD | BS_GROUPBOX | WS_VISIBLE,
        7, 3, 140, 125, ContactsDialog, NULL, Instance, NULL);
    NiceFont(grouper);

    SourceInternalRelayRadio = CreateWindowExW(0, WC_BUTTONW, _("Internal Relay"),
        WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_VISIBLE,
        16, 21, 125, 20, ContactsDialog, NULL, Instance, NULL);
    NiceFont(SourceInternalRelayRadio);

    SourceInputPinRadio = CreateWindowExW(0, WC_BUTTONW, _("Input Pin on MCU"),
        WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_VISIBLE,
        16, 41, 125, 20, ContactsDialog, NULL, Instance, NULL);
    NiceFont(SourceInputPinRadio);

    SourceOutputPinRadio = CreateWindowExW(0, WC_BUTTONW, _("Output Pin on MCU"),
        WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_VISIBLE,
        16, 61, 125, 20, ContactsDialog, NULL, Instance, NULL);
    NiceFont(SourceOutputPinRadio);

    SourceModbusContactRadio = CreateWindowExW(0, WC_BUTTONW, _("Modbus Contact"),
        WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_VISIBLE,
        16, 81, 125, 20, ContactsDialog, NULL, Instance, NULL);
    NiceFont(SourceModbusContactRadio);

    SourceModbusCoilRadio = CreateWindowExW(0, WC_BUTTONW, _("Modbus Coil"),
        WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_VISIBLE,
        16, 101, 125, 20, ContactsDialog, NULL, Instance, NULL);
    NiceFont(SourceModbusCoilRadio);

    HWND textLabel = CreateWindowExW(0, WC_STATICW, _("Name:"),
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT,
        145, 16, 50, 21, ContactsDialog, NULL, Instance, NULL);
    NiceFont(textLabel);

    NameTextbox = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, L"",
        WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
        200, 16, 115, 21, ContactsDialog, NULL, Instance, NULL);
    FixedFont(NameTextbox);

    NegatedCheckbox = CreateWindowExW(0, WC_BUTTONW, _("|/| Negated"),
        WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP | WS_VISIBLE,
        155, 44, 160, 20, ContactsDialog, NULL, Instance, NULL);
    NiceFont(NegatedCheckbox);

    Set1Checkbox = CreateWindowExW(0, WC_BUTTONW, _("Set HI input level before simulation"),
        WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP | WS_VISIBLE,
        155, 72, 260, 20, ContactsDialog, NULL, Instance, NULL);
    NiceFont(Set1Checkbox);

    OkButton = CreateWindowExW(0, WC_BUTTONW, _("OK"),
        WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE | BS_DEFPUSHBUTTON,
        325, 10, 70, 23, ContactsDialog, NULL, Instance, NULL);
    NiceFont(OkButton);

    CancelButton = CreateWindowExW(0, WC_BUTTONW, _("Cancel"),
        WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
        325, 40, 70, 23, ContactsDialog, NULL, Instance, NULL);
    NiceFont(CancelButton);

    PrevNameProc = SetWindowLongPtr(NameTextbox, GWLP_WNDPROC, (LONG_PTR)MyNameProc);
}

void ShowContactsDialog(BOOL *negated, BOOL *set1, char *name)
{
    BOOL set1Save = *set1;
    char nameSave[MAX_NAME_LEN];
    strcpy(nameSave, name);

    ContactsDialog = CreateWindowClient(0, L"LDmicroDialog",
        _("Contacts"), WS_OVERLAPPED | WS_SYSMENU,
        100, 100, 404, 135, NULL, NULL, Instance, NULL);

    MakeControls();

    switch (name[0]) {
    case 'R':
        SendMessageW(SourceInternalRelayRadio, BM_SETCHECK, BST_CHECKED, 0);
        break;
    case 'Y':
        SendMessageW(SourceOutputPinRadio, BM_SETCHECK, BST_CHECKED, 0);
        break;
    case 'X':
        SendMessageW(SourceInputPinRadio, BM_SETCHECK, BST_CHECKED, 0);
        break;
    case 'I':
        SendMessageW(SourceModbusContactRadio, BM_SETCHECK, BST_CHECKED, 0);
        break;
    case 'M':
        SendMessageW(SourceModbusCoilRadio, BM_SETCHECK, BST_CHECKED, 0);
        break;
    default:
        oops();
        break;
    }

    if(*negated) {
        SendMessageW(NegatedCheckbox, BM_SETCHECK, BST_CHECKED, 0);
    }

    if(*set1) {
        SendMessageW(Set1Checkbox, BM_SETCHECK, BST_CHECKED, 0);
    }
    EnableWindow(Set1Checkbox, SendMessage(SourceInputPinRadio, BM_GETSTATE, 0, 0) & BST_CHECKED);

    auto name_w = to_utf16(name + 1);
    SendMessageW(NameTextbox, WM_SETTEXT, 0, (LPARAM)(name_w.c_str()));

    EnableWindow(MainWindow, FALSE);
    ShowWindow(ContactsDialog, TRUE);
    SetFocus(NameTextbox);
    SendMessageW(NameTextbox, EM_SETSEL, 0, -1);

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

        EnableWindow(Set1Checkbox, SendMessageW(SourceInputPinRadio, BM_GETSTATE, 0, 0) & BST_CHECKED);

        if(IsDialogMessage(ContactsDialog, &msg)) continue;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if(!DialogCancel) {
        if(SendMessageW(NegatedCheckbox, BM_GETSTATE, 0, 0) & BST_CHECKED) {
            *negated = TRUE;
        } else {
            *negated = FALSE;
        }
        if(SendMessageW(SourceInternalRelayRadio, BM_GETSTATE, 0, 0)
            & BST_CHECKED)
        {
            name[0] = 'R';
        } else if(SendMessageW(SourceInputPinRadio, BM_GETSTATE, 0, 0)
            & BST_CHECKED)
        {
            name[0] = 'X';
        } else if (SendMessageW(SourceModbusContactRadio, BM_GETSTATE, 0, 0)
            & BST_CHECKED)
        {
            name[0] = 'I';
        } else if (SendMessageW(SourceModbusCoilRadio, BM_GETSTATE, 0, 0)
            & BST_CHECKED)
        {
            name[0] = 'M';
        } else {
            name[0] = 'Y';
        }
        wchar_t name_w[MAX_NAME_LEN];
        name_w[0] = name[0];
        SendMessageW(NameTextbox, WM_GETTEXT, (WPARAM)(MAX_NAME_LEN-1), (LPARAM)(name_w + 1));
        std::string name_s = to_utf8(name_w);
        strncpy(name, name_s.c_str(), MAX_NAME_LEN);

        if(SendMessage(Set1Checkbox, BM_GETSTATE, 0, 0) & BST_CHECKED) {
            *set1 = TRUE;
        } else {
            *set1 = FALSE;
        }

        if((*set1 != set1Save) || strcmp(name, nameSave)) {
          int n = CountWhich(ELEM_CONTACTS, ELEM_COIL, nameSave);
          if(n >= 1) {
            BOOL rename = FALSE;
            if(strcmp(name, nameSave)) {
                wchar_t str[1000];
                swprintf_s(str, _("Rename the ALL other %d contacts/coils named '%ls' to '%ls' ?"), n, u16(nameSave), u16(name));
                rename = IDYES == MessageBoxW(MainWindow, str, L"LDmicro", MB_YESNO | MB_ICONQUESTION);
            }
            if(rename)
                if(name[0] == 'X')
                    RenameSet1(ELEM_CONTACTS, nameSave, name, *set1); // rename and set as set1
                else
                    RenameSet1(ELEM_CONTACTS, nameSave, name, FALSE); // rename and reset
            else
                if(name[0] == 'X')
                    RenameSet1(ELEM_CONTACTS, name, NULL, *set1); // set as set1
                else
                    RenameSet1(ELEM_CONTACTS, name, NULL, FALSE); // reset
          }
        }

    }

    EnableWindow(MainWindow, TRUE);
    SetFocus(MainWindow);
    DestroyWindow(ContactsDialog);
    return;
}
