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
// Window to show our internal help. Roll my own using a rich edit control so
// that I don't have to distribute a separate Windows Help or HTML file; the
// manual is compiled in to the exe. Do pretty syntax highlighting style
// colours.
// Jonathan Westhues, Dec 2004
//-----------------------------------------------------------------------------
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <commctrl.h>
#include <richedit.h>

#include "ldmicro.h"

extern char *HelpText[];
extern char *HelpTextDe[];
extern char *HelpTextFr[];
extern char *HelpTextTr[];
extern char *HelpTextJa[];
extern char *HelpTextRu[];
extern char *HelpTextEs[];

extern char *AboutText[] = {
"",
"ABOUT LDMICRO",
"=============",
"",
"LDmicro is a ladder logic editor, simulator and compiler for 8-bit",
"microcontrollers. It can generate native code for Atmel AVR and Microchip",
"PIC16 CPUs from a ladder diagram.",
"",
"This program is free software: you can redistribute it and/or modify it",
"under the terms of the GNU General Public License as published by the",
"Free Software Foundation, either version 3 of the License, or (at your",
"option) any later version.",
"",
"This program is distributed in the hope that it will be useful, but",
"WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY",
"or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License",
"for more details.",
"",
"You should have received a copy of the GNU General Public License along",
"with this program. If not, see <http://www.gnu.org/licenses/>.",
"",
"The source code for LDmicro is available at",
"",
"    http://cq.cx/ladder.pl",
"",
"Copyright 2005-2016 Jonathan Westhues",
"    Email: user jwesthues, at host cq.cx",
"",
"Netzer extension by Sven Schlender (C) 2012",
"    http://www.mobacon.de/wiki/doku.php/en/netzer/index",
"",
"Controllino Maxi support 2016",
"    Frederic Rible <frible@teaser.fr>",
"",
"Unofficial support:",
"    Repository: https://github.com/LDmicro/LDmicro",
"    Email:      LDmicro.GitHub@gmail.com",
"",
"Release 4.3.8.1, built " __TIME__ " " __DATE__ ".", // AboutText[38]
"",
NULL
};

static char **Text[] = {
#if defined(LDLANG_EN) || \
    defined(LDLANG_IT) || \
    defined(LDLANG_PT)
    HelpText,
#elif defined(LDLANG_DE)
    HelpTextDe,
#elif defined(LDLANG_FR)
    HelpTextFr,
#elif defined(LDLANG_TR)
    HelpTextTr,
#elif defined(LDLANG_JA)
    HelpTextJa,
#elif defined(LDLANG_RU)
    HelpTextRu,
#elif defined(LDLANG_ES)
    HelpTextEs,
#else
    #error "Bad language"
#endif

    // Let's always keep the about text in English.
    AboutText
};

static HWND HelpDialog[2];
static HWND RichEdit[2];

static BOOL HelpWindowOpen[2];

static int TitleHeight;

#define RICH_EDIT_HEIGHT(h)  \
    ((((h) - 3 + (FONT_HEIGHT/2)) / FONT_HEIGHT) * FONT_HEIGHT)

static void SizeRichEdit(int a)
{
    RECT r;
    GetClientRect(HelpDialog[a], &r);

    SetWindowPos(RichEdit[a], HWND_TOP, 6, 3, r.right - 6,
        RICH_EDIT_HEIGHT(r.bottom), 0);
}

static BOOL Resizing(RECT *r, int wParam)
{
    BOOL touched = FALSE;
    if(r->right - r->left < 650) {
        int diff = 650 - (r->right - r->left);
        if(wParam == WMSZ_RIGHT || wParam == WMSZ_TOPRIGHT ||
            wParam == WMSZ_BOTTOMRIGHT)
        {
            r->right += diff;
        } else {
            r->left -= diff;
        }
        touched = TRUE;
    }

    if(!(wParam == WMSZ_LEFT || wParam == WMSZ_RIGHT)) {
        int h = r->bottom - r->top - TitleHeight - 5;
        if(RICH_EDIT_HEIGHT(h) != h) {
            int diff = h - RICH_EDIT_HEIGHT(h);
            if(wParam == WMSZ_TOP || wParam == WMSZ_TOPRIGHT ||
                wParam == WMSZ_TOPLEFT)
            {
                r->top += diff;
            } else {
                r->bottom -= diff;
            }
            touched = TRUE;
        }
    }

    return !touched;
}

static void MakeControls(int a)
{
    HMODULE re = LoadLibrary("RichEd20.dll");
    if(!re) oops();

    RichEdit[a] = CreateWindowEx(0, RICHEDIT_CLASS,
        "", WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | ES_READONLY |
        ES_MULTILINE | WS_VSCROLL,
        0, 0, 100, 100, HelpDialog[a], NULL, Instance, NULL);

    SendMessage(RichEdit[a], WM_SETFONT, (WPARAM)FixedWidthFont, TRUE);
    SendMessage(RichEdit[a], EM_SETBKGNDCOLOR, (WPARAM)0, HighlightColours.bg); // RGB(0, 0, 0)

    SizeRichEdit(a);

    int i;
    BOOL nextSubHead = FALSE;
    for(i = 0; Text[a][i]; i++) {
        char *s = Text[a][i];

        CHARFORMAT cf;
        cf.cbSize = sizeof(cf);
        cf.dwMask = CFM_BOLD | CFM_COLOR;
        cf.dwEffects = 0;
        if((s[0] == '=') ||
           (Text[a][i+1] && Text[a][i+1][0] == '='))
        {
            cf.crTextColor = HighlightColours.simBusRight; // RGB(255, 255, 110);
        } else if(s[3] == '|' && s[4] == '|') {
            cf.crTextColor = HighlightColours.lit; // RGB(255, 110, 255);
        } else if(s[0] == '>' || nextSubHead) {
            // Need to make a copy because the strings we are passed aren't
            // mutable.
            char copy[1024];
            if(strlen(s) >= sizeof(copy)) oops();
            strcpy(copy, s);

            int j;
            for(j = 1; copy[j]; j++) {
                if(copy[j] == ' ' && copy[j-1] == ' ')
                    break;
            }
            BOOL justHeading = (copy[j] == '\0');
            copy[j] = '\0';
            cf.crTextColor = HighlightColours.selected; // RGB(110, 255, 110);
            SendMessage(RichEdit[a], EM_SETCHARFORMAT, SCF_SELECTION,
                (LPARAM)&cf);
            SendMessage(RichEdit[a], EM_REPLACESEL, (WPARAM)FALSE,
                (LPARAM)copy);
            SendMessage(RichEdit[a], EM_SETSEL, (WPARAM)-1, (LPARAM)-1);

            // Special case if there's nothing except title on the line
            if(!justHeading) {
                copy[j] = ' ';
            }
            s += j;
            cf.crTextColor = HighlightColours.name; // RGB(255, 110, 255);
            nextSubHead = !nextSubHead;
        } else {
            cf.crTextColor = HighlightColours.def; // RGB(255, 255, 255);
        }

        SendMessage(RichEdit[a], EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
        SendMessage(RichEdit[a], EM_REPLACESEL, (WPARAM)FALSE, (LPARAM)s);
        SendMessage(RichEdit[a], EM_SETSEL, (WPARAM)-1, (LPARAM)-1);

        if(Text[a][i+1]) {
            SendMessage(RichEdit[a], EM_REPLACESEL, FALSE, (LPARAM)"\r\n");
            SendMessage(RichEdit[a], EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
        }
    }

    SendMessage(RichEdit[a], EM_SETSEL, (WPARAM)0, (LPARAM)0);
}

//-----------------------------------------------------------------------------
// Window proc for the help dialog.
//-----------------------------------------------------------------------------
static LRESULT CALLBACK HelpProc(HWND hwnd, UINT msg, WPARAM wParam,
    LPARAM lParam)
{
    int a = (hwnd == HelpDialog[0] ? 0 : 1);
    switch (msg) {
        case WM_SIZING: {
            RECT *r = (RECT *)lParam;
            return Resizing(r, wParam);
            break;
        }
        case WM_SIZE:
            SizeRichEdit(a);
            break;

        case WM_ACTIVATE:
            SetFocus(RichEdit[a]);
            break;

        case WM_KEYDOWN:
            SetFocus(RichEdit[a]);
            break;

        case WM_DESTROY:
        case WM_CLOSE:
            HelpWindowOpen[a] = FALSE;
            // fall through
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 1;
}

//-----------------------------------------------------------------------------
// Create the class for the help window.
//-----------------------------------------------------------------------------
static void MakeClass(void)
{
    WNDCLASSEX wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(wc);

    wc.style            = CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW | CS_OWNDC |
                          CS_DBLCLKS;
    wc.lpfnWndProc      = (WNDPROC)HelpProc;
    wc.hInstance        = Instance;
    wc.hbrBackground    = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName    = "LDmicroHelp";
    wc.lpszMenuName     = NULL;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon            = (HICON)LoadImage(Instance, MAKEINTRESOURCE(4000),
                            IMAGE_ICON, 32, 32, 0);
    wc.hIconSm          = (HICON)LoadImage(Instance, MAKEINTRESOURCE(4000),
                            IMAGE_ICON, 16, 16, 0);

    RegisterClassEx(&wc);
}

void ShowHelpDialog(BOOL about)
{
    int a = about ? 1 : 0;
    if(HelpWindowOpen[a]) {
        SetForegroundWindow(HelpDialog[a]);
        return;
    }

    MakeClass();

    char *s = about ? "About LDmicro" : "LDmicro Help";
    HelpDialog[a] = CreateWindowEx(0, "LDmicroHelp", s,
        WS_OVERLAPPED | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX |
        WS_MAXIMIZEBOX |
        WS_SIZEBOX,
        100, 100, 650 +50, (about ? 120 : 300)+ 300+10*FONT_HEIGHT, NULL, NULL, Instance, NULL);
    MakeControls(a);

    ShowWindow(HelpDialog[a], TRUE);
    SetFocus(RichEdit[a]);

    HelpWindowOpen[a] = TRUE;

    RECT r;
    GetClientRect(HelpDialog[a], &r);
    TitleHeight = 300 - r.bottom;

    GetWindowRect(HelpDialog[a], &r);
    Resizing(&r, WMSZ_TOP);
    SetWindowPos(HelpDialog[a], HWND_TOP, r.left, r.top, r.right - r.left,
        r.bottom - r.top, 0);
}
