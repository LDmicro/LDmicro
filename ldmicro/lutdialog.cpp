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
// Dialog for entering the elements of a look-up table. I allow two formats:
// as a simple list of integer values, or like a string. The lookup table
// can either be a straight LUT, or one with piecewise linear interpolation
// in between the points.
// Jonathan Westhues, Dec 2005
//-----------------------------------------------------------------------------
#include "stdafx.h"

#include "ldmicro.h"

static HWND LutDialog;

static HWND AsStringCheckbox;
static HWND CountTextbox;
static HWND NameTextbox;
static HWND DestTextbox;
static HWND IndexTextbox;
static HWND Labels[4];

static HWND StringTextbox;

static bool WasAsString;
static int  WasCount;

static HWND     ValuesTextbox[MAX_LOOK_UP_TABLE_LEN * 2];
static LONG_PTR PrevValuesProc[MAX_LOOK_UP_TABLE_LEN * 2];
static HWND     ValuesLabel[MAX_LOOK_UP_TABLE_LEN * 2];

static int32_t ValuesCache[MAX_LOOK_UP_TABLE_LEN * 2];

static LONG_PTR PrevNameProc;
static LONG_PTR PrevDestProc;
static LONG_PTR PrevIndexProc;
static LONG_PTR PrevCountProc;

//-----------------------------------------------------------------------------
// Don't allow any characters other than 0-9 and minus in the values.
//-----------------------------------------------------------------------------
static LRESULT CALLBACK MyNumberProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if(msg == WM_CHAR) {
        if(!(ishobdigit(wParam) || wParam == '\b' || wParam == '-')) {
            return 0;
        }
    }

    WNDPROC w = nullptr;
    for(int i = 0; i < MAX_LOOK_UP_TABLE_LEN; i++) {
        if(hwnd == ValuesTextbox[i]) {
            w = (WNDPROC)PrevValuesProc[i];
            break;
        }
    }
    if(w == nullptr)
        oops();

    return CallWindowProc(w, hwnd, msg, wParam, lParam);
}

//-----------------------------------------------------------------------------
// Don't allow any characters other than 0-9 in the count.
//-----------------------------------------------------------------------------
static LRESULT CALLBACK MyDigitsProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if(msg == WM_CHAR) {
        if(!(isdigit(wParam) || wParam == '\b')) {
            return 0;
        }
    }

    return CallWindowProc((WNDPROC)PrevCountProc, hwnd, msg, wParam, lParam);
}

//-----------------------------------------------------------------------------
// Don't allow any characters other than A-Za-z0-9_ in the name.
//-----------------------------------------------------------------------------
static LRESULT CALLBACK MyNameProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if(msg == WM_CHAR) {
        if(!(isalpha(wParam) || isdigit(wParam) || wParam == '_' || wParam == '\b')) {
            return 0;
        }
    }

    WNDPROC w;
    if(hwnd == DestTextbox) {
        w = (WNDPROC)PrevDestProc;
    } else if(hwnd == NameTextbox) {
        w = (WNDPROC)PrevNameProc;
    } else if(hwnd == IndexTextbox) {
        w = (WNDPROC)PrevIndexProc;
    } else
        oops();
    return CallWindowProc(w, hwnd, msg, wParam, lParam);
}

//-----------------------------------------------------------------------------
// Make the controls that are guaranteed not to move around as the count/
// as string settings change. This is different for the piecewise linear,
// because in that case we should not provide a checkbox to change whether
// the table is edited as a string or table.
//-----------------------------------------------------------------------------
static void MakeFixedControls(bool forPwl)
{
    Labels[0] = CreateWindowEx(0, WC_STATIC, _("Destination:"), WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT, 0, 10, 78 + 20, 21, LutDialog, nullptr, Instance, nullptr);
    NiceFont(Labels[0]);

    DestTextbox = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE, 85 + 20, 10, 120, 21, LutDialog, nullptr, Instance, nullptr);
    FixedFont(DestTextbox);

    Labels[1] = CreateWindowEx(0, WC_STATIC, _("Name:"), WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT, 0, 40, 78 + 20, 21, LutDialog, nullptr, Instance, nullptr);
    NiceFont(Labels[1]);

    NameTextbox = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE, 85 + 20, 40, 120, 21, LutDialog, nullptr, Instance, nullptr);
    FixedFont(NameTextbox);

    Labels[2] = CreateWindowEx(0, WC_STATIC, _("Index:"), WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT, 10, 70, 68 + 20, 21, LutDialog, nullptr, Instance, nullptr);
    NiceFont(Labels[2]);

    IndexTextbox = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE, 85 + 20, 40 + 30, 120, 21, LutDialog, nullptr, Instance, nullptr);
    FixedFont(IndexTextbox);

    const char *txt1 = forPwl ? _("Points:") : _("Table size:");
    char        txt[1024];
    sprintf(txt, "%s\n (max %d)", txt1, forPwl ? MAX_LOOK_UP_TABLE_LEN / 2 : MAX_LOOK_UP_TABLE_LEN);
    Labels[3] = CreateWindowEx(0,
                               WC_STATIC,
                               txt,
                               WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT,
                               0,
                               70 + 22, /////
                               78 + 20,
                               42, ///// Modified by JG
                               LutDialog,
                               nullptr,
                               Instance,
                               nullptr);
    NiceFont(Labels[3]);

    CountTextbox = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE, 85 + 20, 70 + 30, 120, 21, LutDialog, nullptr, Instance, nullptr);
    NiceFont(CountTextbox);

    if(!forPwl) {
        AsStringCheckbox = CreateWindowEx(0,
                                          WC_BUTTON,
                                          _("Edit table of ASCII values like a string"),
                                          WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE | BS_AUTOCHECKBOX,
                                          10,
                                          100 + 30, ///// Modified by JG
                                          300,
                                          21,
                                          LutDialog,
                                          nullptr,
                                          Instance,
                                          nullptr);
        NiceFont(AsStringCheckbox);
    }

    OkButton = CreateWindowEx(0, WC_BUTTON, _("OK"), WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE | BS_DEFPUSHBUTTON, 231 + 20, 10, 70, 23, LutDialog, nullptr, Instance, nullptr);
    NiceFont(OkButton);

    CancelButton = CreateWindowEx(0, WC_BUTTON, _("Cancel"), WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE, 231 + 20, 40, 70, 23, LutDialog, nullptr, Instance, nullptr);
    NiceFont(CancelButton);

    PrevNameProc = SetWindowLongPtr(NameTextbox, GWLP_WNDPROC, (LONG_PTR)MyNameProc);
    PrevDestProc = SetWindowLongPtr(DestTextbox, GWLP_WNDPROC, (LONG_PTR)MyNameProc);
    PrevIndexProc = SetWindowLongPtr(IndexTextbox, GWLP_WNDPROC, (LONG_PTR)MyNameProc);
    PrevCountProc = SetWindowLongPtr(CountTextbox, GWLP_WNDPROC, (LONG_PTR)MyDigitsProc);
}

//-----------------------------------------------------------------------------
// Destroy all of the controls so that we can start anew. This is necessary
// because if the size of the LUT changes, or if the user switches from
// table entry to string entry, we must completely reconfigure the dialog.
//-----------------------------------------------------------------------------
static void DestroyLutControls()
{
    if(WasAsString) {
        // Nothing to do; we constantly update the cache from the user-
        // specified string, because we might as well do that when we
        // calculate the length.
    } else {
        for(int i = 0; i < WasCount; i++) {
            char buf[20];
            SendMessage(ValuesTextbox[i], WM_GETTEXT, (WPARAM)sizeof(buf), (LPARAM)buf);
            ValuesCache[i] = hobatoi(buf);
        }
    }

    DestroyWindow(StringTextbox);

    for(int i = 0; i < MAX_LOOK_UP_TABLE_LEN; i++) {
        DestroyWindow(ValuesTextbox[i]);
        DestroyWindow(ValuesLabel[i]);
    }
}

//-----------------------------------------------------------------------------
// Make the controls that hold the LUT. The exact configuration of the dialog
// will depend on (a) whether the user chose table-type or string-type entry,
// and for table-type entry, on (b) the number of entries, and on (c)
// whether we are editing a PWL table (list of points) or a straight LUT.
//-----------------------------------------------------------------------------
static void MakeLutControls(bool asString, int count, bool forPwl)
{
    // Remember these, so that we know from where to cache stuff if we have
    // to destroy these textboxes and make something new later.
    WasAsString = asString;
    WasCount = count;

    if(forPwl && asString)
        oops();

    if(asString) {
        char str[3 * MAX_LOOK_UP_TABLE_LEN * 2 + 1];
        int  i, j;
        j = 0;
        for(i = 0; i < count; i++) {
            int c = ValuesCache[i];
            if(c >= 32 && c <= 127 && c != '\\') {
                str[j++] = (char)c;
            } else if(c == '\\') {
                str[j++] = '\\';
                str[j++] = '\\';
            } else if(c == '\r') {
                str[j++] = '\\';
                str[j++] = 'r';
            } else if(c == '\b') {
                str[j++] = '\\';
                str[j++] = 'b';
            } else if(c == '\f') {
                str[j++] = '\\';
                str[j++] = 'f';
            } else if(c == '\n') {
                str[j++] = '\\';
                str[j++] = 'n';
            } else {
                str[j++] = 'X';
            }
        }
        str[j++] = '\0';
        StringTextbox = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, str, WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE, 10, 130 + 30, 294, 21, LutDialog, nullptr, Instance, nullptr);
        FixedFont(StringTextbox);
        SendMessage(CountTextbox, EM_SETREADONLY, (WPARAM) true, 0);
        MoveWindow(LutDialog, 100, 30, 320 + 20, 185 + 30, true);
    } else {
        int base;
        if(forPwl) {
            base = 100 + 40; ///// Modified by JG
        } else {
            base = 140 + 25; /////
        }
        for(int i = 0; i < count; i++) {
            int x, y;

            if(i < 16) {
                x = 10;
                y = base + 30 * i;
            } else {
                x = 160;
                y = base + 30 * (i - 16);
            }

            x = 10 + (i / 16) * 150;
            y = base + 30 * (i % 16);

            char buf[20];
            sprintf(buf, "%d", ValuesCache[i]);
            ValuesTextbox[i] = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, buf, WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE, x + 30, y, 80, 21, LutDialog, nullptr, Instance, nullptr);
            NiceFont(ValuesTextbox[i]);

            if(forPwl) {
                sprintf(buf, "%c%d:", (i & 1) ? 'y' : 'x', i / 2);
            } else {
                sprintf(buf, "%2d:", i);
            }
            ValuesLabel[i] = CreateWindowEx(0, WC_STATIC, buf, WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE, x, y + 3, 100, 21, LutDialog, nullptr, Instance, nullptr);
            FixedFont(ValuesLabel[i]);

            PrevValuesProc[i] = SetWindowLongPtr(ValuesTextbox[i], GWLP_WNDPROC, (LONG_PTR)MyNumberProc);
        }
        if(count > MAX_LOOK_UP_TABLE_LEN)
            count = MAX_LOOK_UP_TABLE_LEN;
        SendMessage(CountTextbox, EM_SETREADONLY, (WPARAM) false, 0);

        MoveWindow(LutDialog, 100, 30, 320 + 20 + std::min(count / 16, 2) * 150, base + 60 + std::min(count, 16) * 30,
                   true); ///// Modified by JG
    }
}

//-----------------------------------------------------------------------------
// Decode a string into a look-up table; store the values in ValuesCache[],
// and update the count checkbox (which in string mode is read-only) to
// reflect the new length. Returns false if the new string is too long, else
// true.
//-----------------------------------------------------------------------------
bool StringToValuesCache(char *str, int *c)
{
    int count = 0;
    while(*str) {
        if(*str == '\\') {
            str++;
            // clang-format off
            switch(*str) {
                case 'r': ValuesCache[count++] = '\r'; break;
                case 'n': ValuesCache[count++] = '\n'; break;
                case 'f': ValuesCache[count++] = '\f'; break;
                case 'b': ValuesCache[count++] = '\b'; break;
                default:  ValuesCache[count++] = *str; break;
            }
            // clang-format on
        } else {
            ValuesCache[count++] = *str;
        }
        if(*str) {
            str++;
        }
        if(count >= (MAX_LOOK_UP_TABLE_LEN)) {
            return false;
        }
    }

    char buf[10];
    sprintf(buf, "%d", count);
    SendMessage(CountTextbox, WM_SETTEXT, 0, (LPARAM)(buf));
    *c = count;
    return true;
}

//-----------------------------------------------------------------------------
// Show the look-up table dialog. This one is nasty, mostly because there are
// two ways to enter a look-up table: as a table, or as a string. Presumably
// I should convert between those two representations on the fly, as the user
// edit things, so I do.
//-----------------------------------------------------------------------------
void ShowLookUpTableDialog(ElemLeaf *l)
{
    ElemLookUpTable *t = &(l->d.lookUpTable);

    // First copy over all the stuff from the leaf structure; in particular,
    // we need our own local copy of the table entries, because it would be
    // bad to update those in the leaf before the user clicks okay (as he
    // might cancel).
    int  count = t->count;
    bool asString = t->editAsString;
    memset(ValuesCache, 0, sizeof(ValuesCache));
    for(int i = 0; i < count; i++) {
        ValuesCache[i] = t->vals[i];
    }

    // Now create the dialog's fixed controls, plus the changing (depending
    // on show style/entry count) controls for the initial configuration.
    LutDialog = CreateWindowClient(0,
                                   "LDmicroDialog",
                                   _("Look-Up Table"),
                                   WS_OVERLAPPED | WS_SYSMENU,
                                   100,
                                   100,
                                   320,
                                   405, //// Modified by JG
                                   nullptr,
                                   nullptr,
                                   Instance,
                                   nullptr);
    MakeFixedControls(false);
    MakeLutControls(asString, count, false);

    // Set up the controls to reflect the initial configuration.
    SendMessage(DestTextbox, WM_SETTEXT, 0, (LPARAM)(t->dest));
    SendMessage(NameTextbox, WM_SETTEXT, 0, (LPARAM)(t->name));
    SendMessage(IndexTextbox, WM_SETTEXT, 0, (LPARAM)(t->index));
    {
        char buf[30];
        sprintf(buf, "%d", t->count);
        SendMessage(CountTextbox, WM_SETTEXT, 0, (LPARAM)buf);
    }
    if(asString) {
        SendMessage(AsStringCheckbox, BM_SETCHECK, BST_CHECKED, 0);
    }

    // And show the window
    EnableWindow(MainWindow, false);
    ShowWindow(LutDialog, true);
    SetFocus(NameTextbox);
    SendMessage(NameTextbox, EM_SETSEL, 0, -1);

    char PrevTableAsString[1024] = "";

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

        if(!IsDialogMessage(LutDialog, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Are we in table mode? In that case watch the (user-editable) count
        // field, and use that to determine how many textboxes to show.
        char buf[20];
        SendMessage(CountTextbox, WM_GETTEXT, (WPARAM)sizeof(buf), (LPARAM)buf);
        if(atoi(buf) != count && !asString) {
            int newcount = atoi(buf);
            if(newcount < 0 || newcount > (MAX_LOOK_UP_TABLE_LEN)) {
                newcount = count;
                SendMessage(CountTextbox, WM_SETTEXT, 0, (LPARAM) "");
            } else
                count = atoi(buf);
            DestroyLutControls();
            MakeLutControls(asString, count, false);
        }

        // Are we in string mode? In that case watch the string textbox,
        // and use that to update the (read-only) count field.
        if(asString) {
            char scratch[1024];
            SendMessage(StringTextbox, WM_GETTEXT, (WPARAM)(sizeof(scratch) - 1), (LPARAM)scratch);
            if(strcmp(scratch, PrevTableAsString) != 0) {
                if(StringToValuesCache(scratch, &count)) {
                    strcpy(PrevTableAsString, scratch);
                } else {
                    // Too long; put back the old one
                    SendMessage(StringTextbox, WM_SETTEXT, 0, (LPARAM)PrevTableAsString);
                }
            }
        }

        // Did we just change modes?
        bool x = SendMessage(AsStringCheckbox, BM_GETCHECK, 0, 0) == BST_CHECKED;
        if((x && !asString) || (!x && asString)) {
            asString = x;
            DestroyLutControls();
            MakeLutControls(asString, count, false);
        }
    }

    if(!DialogCancel) {
        SendMessage(NameTextbox, WM_GETTEXT, (WPARAM)16 /*(MAX_NAME_LEN-1)*/, (LPARAM)(t->name));
        SendMessage(DestTextbox, WM_GETTEXT, (WPARAM)16 /*(MAX_NAME_LEN-1)*/, (LPARAM)(t->dest));
        SendMessage(IndexTextbox, WM_GETTEXT, (WPARAM)16 /*(MAX_NAME_LEN-1)*/, (LPARAM)(t->index));
        DestroyLutControls();
        // The call to DestroyLutControls updated ValuesCache, so just read
        // them out of there (whichever mode we were in before).
        for(int i = 0; i < count; i++) {
            t->vals[i] = ValuesCache[i];
        }
        t->count = count;
        t->editAsString = asString;
    }

    EnableWindow(MainWindow, true);
    SetFocus(MainWindow);
    DestroyWindow(LutDialog);
}

//-----------------------------------------------------------------------------
// Show the piecewise linear table dialog. This one can only be edited in
// only a single format, which makes things easier than before.
//-----------------------------------------------------------------------------
void ShowPiecewiseLinearDialog(ElemLeaf *l)
{
    ElemPiecewiseLinear *t = &(l->d.piecewiseLinear);

    // First copy over all the stuff from the leaf structure; in particular,
    // we need our own local copy of the table entries, because it would be
    // bad to update those in the leaf before the user clicks okay (as he
    // might cancel).
    int count = t->count;
    memset(ValuesCache, 0, sizeof(ValuesCache));
    for(int i = 0; i < count * 2; i++) {
        ValuesCache[i] = t->vals[i];
    }

    // Now create the dialog's fixed controls, plus the changing (depending
    // on show style/entry count) controls for the initial configuration.
    LutDialog = CreateWindowClient(0, "LDmicroDialog", _("Piecewise Linear Table"), WS_OVERLAPPED | WS_SYSMENU, 100, 100, 320, 375, nullptr, nullptr, Instance, nullptr);
    MakeFixedControls(true);
    MakeLutControls(false, count * 2, true);

    // Set up the controls to reflect the initial configuration.
    SendMessage(NameTextbox, WM_SETTEXT, 0, (LPARAM)(t->name));
    SendMessage(DestTextbox, WM_SETTEXT, 0, (LPARAM)(t->dest));
    SendMessage(IndexTextbox, WM_SETTEXT, 0, (LPARAM)(t->index));
    {
        char buf[30];
        sprintf(buf, "%d", t->count);
        SendMessage(CountTextbox, WM_SETTEXT, 0, (LPARAM)buf);
    }

    // And show the window
    EnableWindow(MainWindow, false);
    ShowWindow(LutDialog, true);
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

        if(!IsDialogMessage(LutDialog, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Watch the (user-editable) count field, and use that to
        // determine how many textboxes to show.
        char buf[20];
        SendMessage(CountTextbox, WM_GETTEXT, (WPARAM)sizeof(buf), (LPARAM)buf);
        if(atoi(buf) != count) {
            /*
            count = atoi(buf);
            if(count < 0 || count > 10) {
                count = 0;
                SendMessage(CountTextbox, WM_SETTEXT, 0, (LPARAM)"");
            }
            */
            int newcount = atoi(buf);
            if(newcount < 0 || newcount > (MAX_LOOK_UP_TABLE_LEN / 2)) {
                newcount = count;
                SendMessage(CountTextbox, WM_SETTEXT, 0, (LPARAM) "");
            } else
                count = atoi(buf);
            DestroyLutControls();
            MakeLutControls(false, count * 2, true);
        }
    }

    if(!DialogCancel) {
        SendMessage(NameTextbox, WM_GETTEXT, (WPARAM)16 /*(MAX_NAME_LEN-1)*/, (LPARAM)(t->name));
        SendMessage(DestTextbox, WM_GETTEXT, (WPARAM)16 /*(MAX_NAME_LEN-1)*/, (LPARAM)(t->dest));
        SendMessage(IndexTextbox, WM_GETTEXT, (WPARAM)16 /*(MAX_NAME_LEN-1)*/, (LPARAM)(t->index));
        DestroyLutControls();
        // The call to DestroyLutControls updated ValuesCache, so just read
        // them out of there.
        for(int i = 0; i < count * 2; i++) {
            t->vals[i] = ValuesCache[i];
        }
        t->count = count;
    }

    EnableWindow(MainWindow, true);
    SetFocus(MainWindow);
    DestroyWindow(LutDialog);
}
