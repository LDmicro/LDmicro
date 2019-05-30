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
// Dialog to enter the text of a comment; make it long and skinny to
// encourage people to write it the way it will look on the diagram.
// Jonathan Westhues, Jun 2005
//-----------------------------------------------------------------------------
#include "stdafx.h"

#include "ldmicro.h"

static HWND CommentDialog;

static HWND CommentTextbox;

static void MakeControls(RECT r)
{
    CommentTextbox = CreateWindowEx(WS_EX_CLIENTEDGE,
                                    WC_EDIT,
                                    "",
                                    WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE | ES_MULTILINE
                                        | ES_WANTRETURN,
                                    7,
                                    10,
                                    r.right - 137,
                                    38,
                                    CommentDialog,
                                    nullptr,
                                    Instance,
                                    nullptr);
    FixedFont(CommentTextbox);

    OkButton = CreateWindowEx(0,
                              WC_BUTTON,
                              _("OK"),
                              WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE | BS_DEFPUSHBUTTON,
                              r.right - 120,
                              6,
                              70,
                              23,
                              CommentDialog,
                              nullptr,
                              Instance,
                              nullptr);
    NiceFont(OkButton);

    CancelButton = CreateWindowEx(0,
                                  WC_BUTTON,
                                  _("Cancel"),
                                  WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
                                  r.right - 120,
                                  36,
                                  70,
                                  23,
                                  CommentDialog,
                                  nullptr,
                                  Instance,
                                  nullptr);
    NiceFont(CancelButton);
}

void ShowCommentDialog(char *comment)
{
    RECT r;
    GetClientRect(MainWindow, &r);

    CommentDialog = CreateWindowClient(0,
                                       "LDmicroDialog",
                                       _("Comment"),
                                       WS_OVERLAPPED | WS_SYSMENU,
                                       r.left + 20,
                                       100,
                                       r.right - r.left - 40,
                                       65,
                                       nullptr,
                                       nullptr,
                                       Instance,
                                       nullptr);

    MakeControls(r);

    SendMessage(CommentTextbox, WM_SETTEXT, 0, (LPARAM)comment);

    EnableWindow(MainWindow, false);
    ShowWindow(CommentDialog, true);
    SetFocus(CommentTextbox);
    SendMessage(CommentTextbox, EM_SETSEL, 0, -1);

    MSG   msg;
    DialogDone = false;
    DialogCancel = false;
    while((GetMessage(&msg, nullptr, 0, 0) > 0) && !DialogDone) {
        if(msg.message == WM_KEYDOWN) {
            if(msg.wParam == VK_TAB && GetFocus() == CommentTextbox) {
                SetFocus(OkButton);
                continue;
            } else if(msg.wParam == VK_RETURN) {
                if(GetAsyncKeyState(VK_CONTROL) & 0x8000) {
                    DialogDone = true;
                    break;
                }
            } else if(msg.wParam == VK_ESCAPE) {
                DialogDone = true;
                DialogCancel = true;
                break;
            }
        }

        if(IsDialogMessage(CommentDialog, &msg))
            continue;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if(!DialogCancel) {
        SendMessage(CommentTextbox, WM_GETTEXT, (WPARAM)(MAX_COMMENT_LEN - 1), (LPARAM)comment);
    }

    EnableWindow(MainWindow, true);
    SetFocus(MainWindow);
    DestroyWindow(CommentDialog);
    return;
}
