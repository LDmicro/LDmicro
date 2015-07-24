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
#include <windows.h>
#include <stdio.h>
#include <commctrl.h>

#include "ldmicro.h"

static HWND CommentDialog;

static HWND CommentTextbox;

static void MakeControls(void)
{
    CommentTextbox = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "",
        WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE |
        ES_MULTILINE | ES_WANTRETURN,
        7, 10, 600, 38, CommentDialog, NULL, Instance, NULL);
    FixedFont(CommentTextbox);

    OkButton = CreateWindowEx(0, WC_BUTTON, _("OK"),
        WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE | BS_DEFPUSHBUTTON,
        620,  6, 70, 23, CommentDialog, NULL, Instance, NULL); 
    NiceFont(OkButton);

    CancelButton = CreateWindowEx(0, WC_BUTTON, _("Cancel"),
        WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
        620, 36, 70, 23, CommentDialog, NULL, Instance, NULL); 
    NiceFont(CancelButton);
}

void ShowCommentDialog(char *comment)
{
    CommentDialog = CreateWindowClient(0, "LDmicroDialog",
        _("Comment"), WS_OVERLAPPED | WS_SYSMENU,
        100, 100, 700, 65, NULL, NULL, Instance, NULL);

    MakeControls();
   
    SendMessage(CommentTextbox, WM_SETTEXT, 0, (LPARAM)comment);

    EnableWindow(MainWindow, FALSE);
    ShowWindow(CommentDialog, TRUE);
    SetFocus(CommentTextbox);
    SendMessage(CommentTextbox, EM_SETSEL, 0, -1);

    MSG msg;
    DWORD ret;
    DialogDone = FALSE;
    DialogCancel = FALSE;
    while((ret = GetMessage(&msg, NULL, 0, 0)) && !DialogDone) {
        if(msg.message == WM_KEYDOWN) {
            if(msg.wParam == VK_TAB && GetFocus() == CommentTextbox) {
                SetFocus(OkButton);
                continue;
            } else if(msg.wParam == VK_ESCAPE) {
                DialogDone = TRUE;
                DialogCancel = TRUE;
                break;
            }
        }

        if(IsDialogMessage(CommentDialog, &msg)) continue;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if(!DialogCancel) {
        SendMessage(CommentTextbox, WM_GETTEXT, (WPARAM)(MAX_COMMENT_LEN-1),
            (LPARAM)comment);
    }

    EnableWindow(MainWindow, TRUE);
    DestroyWindow(CommentDialog);
    return;
}
