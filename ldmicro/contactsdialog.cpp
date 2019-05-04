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
    HWND grouper = CreateWindowEx(0,
                                  WC_BUTTON,
                                  _("Source"),
                                  WS_CHILD | BS_GROUPBOX | WS_VISIBLE,
                                  7,
                                  3,
                                  140,
                                  125,
                                  ContactsDialog,
                                  nullptr,
                                  Instance,
                                  nullptr);
    NiceFont(grouper);

    SourceInternalRelayRadio = CreateWindowEx(0,
                                              WC_BUTTON,
                                              _("Internal Relay"),
                                              WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_VISIBLE,
                                              16,
                                              21,
                                              125,
                                              20,
                                              ContactsDialog,
                                              nullptr,
                                              Instance,
                                              nullptr);
    NiceFont(SourceInternalRelayRadio);

    SourceInputPinRadio = CreateWindowEx(0,
                                         WC_BUTTON,
                                         _("Input Pin on MCU"),
                                         WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_VISIBLE,
                                         16,
                                         41,
                                         125,
                                         20,
                                         ContactsDialog,
                                         nullptr,
                                         Instance,
                                         nullptr);
    NiceFont(SourceInputPinRadio);

    SourceOutputPinRadio = CreateWindowEx(0,
                                          WC_BUTTON,
                                          _("Output Pin on MCU"),
                                          WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_VISIBLE,
                                          16,
                                          61,
                                          125,
                                          20,
                                          ContactsDialog,
                                          nullptr,
                                          Instance,
                                          nullptr);
    NiceFont(SourceOutputPinRadio);

    SourceModbusContactRadio = CreateWindowEx(0,
                                              WC_BUTTON,
                                              _("Modbus Contact"),
                                              WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_VISIBLE,
                                              16,
                                              81,
                                              125,
                                              20,
                                              ContactsDialog,
                                              nullptr,
                                              Instance,
                                              nullptr);
    NiceFont(SourceModbusContactRadio);

    SourceModbusCoilRadio = CreateWindowEx(0,
                                           WC_BUTTON,
                                           _("Modbus Coil"),
                                           WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_VISIBLE,
                                           16,
                                           101,
                                           125,
                                           20,
                                           ContactsDialog,
                                           nullptr,
                                           Instance,
                                           nullptr);
    NiceFont(SourceModbusCoilRadio);

    HWND textLabel = CreateWindowEx(0,
                                    WC_STATIC,
                                    _("Name:"),
                                    WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT,
                                    145,
                                    16,
                                    50,
                                    21,
                                    ContactsDialog,
                                    nullptr,
                                    Instance,
                                    nullptr);
    NiceFont(textLabel);

    NameTextbox = CreateWindowEx(WS_EX_CLIENTEDGE,
                                 WC_EDIT,
                                 "",
                                 WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
                                 200,
                                 16,
                                 115,
                                 21,
                                 ContactsDialog,
                                 nullptr,
                                 Instance,
                                 nullptr);
    FixedFont(NameTextbox);

    NegatedCheckbox = CreateWindowEx(0,
                                     WC_BUTTON,
                                     _("|/| Negated"),
                                     WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP | WS_VISIBLE,
                                     155,
                                     44,
                                     160,
                                     20,
                                     ContactsDialog,
                                     nullptr,
                                     Instance,
                                     nullptr);
    NiceFont(NegatedCheckbox);

    Set1Checkbox = CreateWindowEx(0,
                                  WC_BUTTON,
                                  _("Set HI input level before simulation"),
                                  WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP | WS_VISIBLE,
                                  155,
                                  72,
                                  260,
                                  20,
                                  ContactsDialog,
                                  nullptr,
                                  Instance,
                                  nullptr);
    NiceFont(Set1Checkbox);

    OkButton = CreateWindowEx(0,
                              WC_BUTTON,
                              _("OK"),
                              WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE | BS_DEFPUSHBUTTON,
                              325,
                              10,
                              70,
                              23,
                              ContactsDialog,
                              nullptr,
                              Instance,
                              nullptr);
    NiceFont(OkButton);

    CancelButton = CreateWindowEx(0,
                                  WC_BUTTON,
                                  _("Cancel"),
                                  WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
                                  325,
                                  40,
                                  70,
                                  23,
                                  ContactsDialog,
                                  nullptr,
                                  Instance,
                                  nullptr);
    NiceFont(CancelButton);

    PrevNameProc = SetWindowLongPtr(NameTextbox, GWLP_WNDPROC, (LONG_PTR)MyNameProc);
}

void ShowContactsDialog(bool *negated, bool *set1, char *name)
{
    bool set1Save = *set1;
    char nameSave[MAX_NAME_LEN];
    strcpy(nameSave, name);

    ContactsDialog = CreateWindowClient(0,
                                        "LDmicroDialog",
                                        _("Contacts"),
                                        WS_OVERLAPPED | WS_SYSMENU,
                                        100,
                                        100,
                                        404,
                                        135,
                                        nullptr,
                                        nullptr,
                                        Instance,
                                        nullptr);

    MakeControls();

    switch(name[0]) {
        case 'R':
            SendMessage(SourceInternalRelayRadio, BM_SETCHECK, BST_CHECKED, 0);
            break;
        case 'Y':
            SendMessage(SourceOutputPinRadio, BM_SETCHECK, BST_CHECKED, 0);
            break;
        case 'X':
            SendMessage(SourceInputPinRadio, BM_SETCHECK, BST_CHECKED, 0);
            break;
        case 'I':
            SendMessage(SourceModbusContactRadio, BM_SETCHECK, BST_CHECKED, 0);
            break;
        case 'M':
            SendMessage(SourceModbusCoilRadio, BM_SETCHECK, BST_CHECKED, 0);
            break;
        default:
            oops();
            break;
    }

    if(*negated) {
        SendMessage(NegatedCheckbox, BM_SETCHECK, BST_CHECKED, 0);
    }

    if(*set1) {
        SendMessage(Set1Checkbox, BM_SETCHECK, BST_CHECKED, 0);
    }
    EnableWindow(Set1Checkbox, SendMessage(SourceInputPinRadio, BM_GETSTATE, 0, 0) & BST_CHECKED);

    SendMessage(NameTextbox, WM_SETTEXT, 0, (LPARAM)(name + 1));

    EnableWindow(MainWindow, false);
    ShowWindow(ContactsDialog, true);
    SetFocus(NameTextbox);
    SendMessage(NameTextbox, EM_SETSEL, 0, -1);

    MSG   msg;
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

        EnableWindow(Set1Checkbox, SendMessage(SourceInputPinRadio, BM_GETSTATE, 0, 0) & BST_CHECKED);

        if(IsDialogMessage(ContactsDialog, &msg))
            continue;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if(!DialogCancel) {
        if(SendMessage(NegatedCheckbox, BM_GETSTATE, 0, 0) & BST_CHECKED) {
            *negated = true;
        } else {
            *negated = false;
        }
        if(SendMessage(SourceInternalRelayRadio, BM_GETSTATE, 0, 0) & BST_CHECKED) {
            name[0] = 'R';
        } else if(SendMessage(SourceInputPinRadio, BM_GETSTATE, 0, 0) & BST_CHECKED) {
            name[0] = 'X';
        } else if(SendMessage(SourceModbusContactRadio, BM_GETSTATE, 0, 0) & BST_CHECKED) {
            name[0] = 'I';
        } else if(SendMessage(SourceModbusCoilRadio, BM_GETSTATE, 0, 0) & BST_CHECKED) {
            name[0] = 'M';
        } else {
            name[0] = 'Y';
        }
        SendMessage(NameTextbox, WM_GETTEXT, (WPARAM)(MAX_NAME_LEN - 1), (LPARAM)(name + 1));

        if(SendMessage(Set1Checkbox, BM_GETSTATE, 0, 0) & BST_CHECKED) {
            *set1 = true;
        } else {
            *set1 = false;
        }

        if((*set1 != set1Save) || strcmp(name, nameSave)) {
            int n = CountWhich(ELEM_CONTACTS, ELEM_COIL, nameSave);
            if(n >= 1) {
                bool rename = false;
                if(strcmp(name, nameSave)) {
                    char str[1000];
                    sprintf(str, _("Rename the ALL other %d contacts/coils named '%s' to '%s' ?"), n, nameSave, name);
                    rename = IDYES == MessageBox(MainWindow, str, "LDmicro", MB_YESNO | MB_ICONQUESTION);
                }
                if(rename)
                    if(name[0] == 'X')
                        RenameSet1(ELEM_CONTACTS, nameSave, name, *set1); // rename and set as set1
                    else
                        RenameSet1(ELEM_CONTACTS, nameSave, name, false); // rename and reset
                else if(name[0] == 'X')
                    RenameSet1(ELEM_CONTACTS, name, nullptr, *set1); // set as set1
                else
                    RenameSet1(ELEM_CONTACTS, name, nullptr, false); // reset
            }
        }
    }

    EnableWindow(MainWindow, true);
    SetFocus(MainWindow);
    DestroyWindow(ContactsDialog);
    return;
}
