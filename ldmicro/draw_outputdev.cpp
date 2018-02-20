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
// The two 'output devices' for the drawing code: either the export as text
// stuff to write to a file, or all the routines concerned with drawing to
// the screen.
// Jonathan Westhues, Dec 2004
//-----------------------------------------------------------------------------
#include "stdafx.h"

#include "ldmicro.h"
#include "intcode.h"

void (*DrawChars)(int, int, char *);

// After an undo all the memory addresses change but make an effort to put
// the cursor roughly where it should be.
int SelectedGxAfterNextPaint = -1;
int SelectedGyAfterNextPaint = -1;

// After pushing a rung up or down the position of that rung in the table
// changes, but the cursor should stay where it was.
BOOL ScrollSelectedIntoViewAfterNextPaint;

// Buffer that we write to when exporting (drawing) diagram to a text file.
// Dynamically allocated so that we're at least slightly efficient.
static char **ExportBuffer;

// The fonts that we will use to draw the ladder diagram: fixed width, one
// normal-weight, one bold.
HFONT       FixedWidthFont;
HFONT       FixedWidthFontBold;

// Different colour brushes for right and left buses in simulation, but same
// colour for both in edit mode; also for the backgrounds in simulation and
// edit modes.
static HBRUSH   BusRightBus;
static HBRUSH   BusLeftBrush;
static HBRUSH   BusBrush;
static HBRUSH   BgBrush;
static HBRUSH   SimBgBrush;

// Parameters that determine our offset if we are scrolled
int ScrollXOffset;
int ScrollYOffset;
int ScrollXOffsetMax;
int ScrollYOffsetMax;

// Is the cursor currently drawn? We XOR it so this gets toggled.
static BOOL CursorDrawn;

// Colours with which to do syntax highlighting, configurable
SyntaxHighlightingColours HighlightColours;

// The number of the current color scheme.
DWORD scheme = 0;

#define X_RIGHT_PADDING 30

//-----------------------------------------------------------------------------
// Blink the cursor on the schematic; called by a Windows timer. We XOR
// draw it so just draw the same rectangle every time to show/erase the
// cursor. Cursor may be in one of four places in the selected leaf (top,
// bottom, left, right) but we don't care; just go from the coordinates
// computed when we drew the schematic in the paint procedure.
//-----------------------------------------------------------------------------
void CALLBACK BlinkCursor(HWND hwnd, UINT msg, UINT_PTR id, DWORD time)
{
    if(GetFocus() != MainWindow && !CursorDrawn) return;
    if(Cursor.left == 0) return;

    PlcCursor c;
    memcpy(&c, &Cursor, sizeof(c));

    c.top -= ScrollYOffset*POS_HEIGHT*FONT_HEIGHT;
    c.left -= ScrollXOffset;

    if(c.top >= IoListTop) return;

    if(c.top + c.height >= IoListTop) {
        c.height = IoListTop - c.top - 3;
    }

    Hdc = GetDC(MainWindow);
    SelectObject(Hdc, GetStockObject(WHITE_BRUSH));
    PatBlt(Hdc, c.left, c.top, c.width, c.height, PATINVERT);
    CursorDrawn = !CursorDrawn;
    ReleaseDC(MainWindow, Hdc);

    if(strlen(CurrentSaveFile)) {
      tGetLastWriteTime(CurrentSaveFile, (PFILETIME)&LastWriteTime);
    }
}

//-----------------------------------------------------------------------------
// Output a string to the screen at a particular location, in character-
// sized units.
//-----------------------------------------------------------------------------
static void DrawCharsToScreen(int cx, int cy, char *str)
{
    cy -= ScrollYOffset*POS_HEIGHT;
    if(cy < -2) return;
    if(cy*FONT_HEIGHT + Y_PADDING > IoListTop) return;

    COLORREF prev;
    BOOL firstTime = TRUE;
    BOOL inNumber = FALSE;
    BOOL inComment = FALSE;
    int inBrace = 0;
    for(; *str; str++, cx++) {
        int x = cx*FONT_WIDTH + X_PADDING;
        int y = cy*FONT_HEIGHT + Y_PADDING;

        BOOL hiOk = !(InSimulationMode || ThisHighlighted);

        if(strchr("{}[]", *str) && hiOk && !inComment)  {
            if(*str == '{' || *str == '[') inBrace++;
            if(inBrace == 1) {
                prev = GetTextColor(Hdc);
                SetTextColor(Hdc, HighlightColours.punct);
                TextOut(Hdc, x, y, str, 1);
                SetTextColor(Hdc, prev);
            } else {
                TextOut(Hdc, x, y, str, 1);
            }
            if(*str == ']' || *str == '}') inBrace--;
        } else if((
            (isdigit(*str) && (firstTime || isspace(str[-1])
                || str[-1] == ':' || str[-1] == '{' || str[-1] == '[')) ||
            (*str == '-' && isdigit(str[1]))) && hiOk && !inComment)
        {
            prev = GetTextColor(Hdc);
            SetTextColor(Hdc, HighlightColours.lit);
            TextOut(Hdc, x, y, str, 1);
            SetTextColor(Hdc, prev);
            inNumber = TRUE;
        } else if(*str == '\x01') {
            cx--;
            if(hiOk) {
                prev = GetTextColor(Hdc);
                SetTextColor(Hdc, HighlightColours.op);
            }
        } else if(*str == '\x02') {
            cx--;
            if(hiOk) {
                SetTextColor(Hdc, prev);
                inComment = FALSE;
            }
        } else if(*str == '\x03') {
            cx--;
            if(hiOk || InSimulationMode) {
                prev = GetTextColor(Hdc);
                SetTextColor(Hdc, HighlightColours.comment);
                inComment = TRUE;
            }
        } else if(inNumber) {
            if(isdigit(*str) || *str == '.') {
                prev = GetTextColor(Hdc);
                SetTextColor(Hdc, HighlightColours.lit);
                TextOut(Hdc, x, y, str, 1);
                SetTextColor(Hdc, prev);
            } else {
                TextOut(Hdc, x, y, str, 1);
                inNumber = FALSE;
            }
        } else {
            TextOut(Hdc, x, y, str, 1);
        }

        firstTime = FALSE;
    }
}

//-----------------------------------------------------------------------------
// Total number of columns that we can display in the given amount of
// window area. Need to leave some slop on the right for the scrollbar, of
// course.
//-----------------------------------------------------------------------------
int ScreenColsAvailable(void)
{
    RECT r;
    GetClientRect(MainWindow, &r);

    return (r.right - (X_PADDING + X_RIGHT_PADDING)) / (POS_WIDTH*FONT_WIDTH);
}

//-----------------------------------------------------------------------------
// Total number of columns that we can display in the given amount of
// window area. Need to leave some slop on the right for the scrollbar, of
// course, and extra slop at the bottom for the horiz scrollbar if it is
// shown.
//-----------------------------------------------------------------------------
int ScreenRowsAvailable(void)
{
    int adj;
    if(ScrollXOffsetMax == 0) {
        adj = 0;
    } else {
        adj = GetSystemMetrics(SM_CYHSCROLL); // 18;
    }
    return (IoListTop - Y_PADDING - adj + FONT_HEIGHT) / (POS_HEIGHT*FONT_HEIGHT);
}

//-----------------------------------------------------------------------------
// Paint the ladder logic program to the screen. Also figure out where the
// cursor should go and fill in coordinates for BlinkCursor. Not allowed to
// draw deeper than IoListTop, as we would run in to the I/O listbox.
//-----------------------------------------------------------------------------
void PaintWindow(void)
{
    static HBITMAP BackBitmap;
    static HDC BackDc;
    static int BitmapWidth;

    KillTimer(MainWindow, TIMER_BLINK_CURSOR);
    if (CursorDrawn)
        BlinkCursor(NULL, 0, NULL, 0); //Hide Cursor
    CursorDrawn = FALSE;

    ok();

    RECT r;
    GetClientRect(MainWindow, &r);
    int bw = r.right;
    int bh = IoListTop;

    HDC paintDc;
    if(!BackDc) {
        HWND desktop = GetDesktopWindow();
        RECT dk;
        GetClientRect(desktop, &dk);

        BitmapWidth = max(2000 +2096, dk.right + 300 +500);
        BackBitmap = CreateCompatibleBitmap(Hdc, BitmapWidth, dk.bottom + 300 +500);
        BackDc = CreateCompatibleDC(Hdc);
        SelectObject(BackDc, BackBitmap);
    }
    paintDc = Hdc;
    Hdc = BackDc;

    RECT fi;
    fi.left = 0; fi.top = 0;
    fi.right = BitmapWidth; fi.bottom = bh;
    FillRect(Hdc, &fi, InSimulationMode ? SimBgBrush : BgBrush);

    // now figure out how we should draw the ladder logic
    ColsAvailable = ProgCountWidestRow();
    if(ColsAvailable < ScreenColsAvailable()) {
        ColsAvailable = ScreenColsAvailable();
    }
    memset(DisplayMatrix, 0, sizeof(DisplayMatrix));
    SelectionActive = FALSE;
    memset(&Cursor, 0, sizeof(Cursor));

    DrawChars = DrawCharsToScreen;

    char str[10] = "";
    int i,y,yp;
    int cx = 0;
    int cy = 0;
    int rowsAvailable = ScreenRowsAvailable();
    for(i = 0; i < Prog.numRungs; i++) {
        int thisHeight = POS_HEIGHT*CountHeightOfElement(ELEM_SERIES_SUBCKT,
            Prog.rungs[i]);

        // For speed, there is no need to draw everything all the time, but
        // we still must draw a bit above and below so that the DisplayMatrix
        // is filled in enough to make it possible to reselect using the
        // cursor keys.
        if(((cy + thisHeight) >= (ScrollYOffset - 8)*POS_HEIGHT) &&
            (cy < (ScrollYOffset + rowsAvailable + 8)*POS_HEIGHT))
        {
            SetBkColor(Hdc, InSimulationMode ? HighlightColours.simBg :
                HighlightColours.bg);
            SetTextColor(Hdc, InSimulationMode ? HighlightColours.simRungNum :
                HighlightColours.rungNum);
            SelectObject(Hdc, FixedWidthFont);
            int rung = i + 1;
            y = Y_PADDING + FONT_HEIGHT*cy;
            yp = y + FONT_HEIGHT*(POS_HEIGHT/2) -
                POS_HEIGHT*FONT_HEIGHT*ScrollYOffset;

            COLORREF prev = GetTextColor(Hdc);
            SetTextColor(Hdc, HighlightColours.def);

            sprintf(str,"%04d", i+1);
            TextOut(Hdc, 8, yp, str, 4);

            SetTextColor(Hdc, HighlightColours.rungNum);

            sprintf(str,"%4d",Prog.OpsInRung[i]);
            TextOut(Hdc, 8, yp + FONT_HEIGHT, str, 4);

            sprintf(str,"%4d",Prog.HexInRung[i]);
            TextOut(Hdc, 8, yp + FONT_HEIGHT * 2, str, 4);

            SetTextColor(Hdc, HighlightColours.selected);
            TextOut(Hdc, 8-FONT_WIDTH, yp , &Prog.rungSelected[i], 1);

            SetTextColor(Hdc, prev);

            cx = 0;
            DrawElement(Prog.rungs[i], ELEM_SERIES_SUBCKT, Prog.rungs[i], &cx, &cy,
                Prog.rungPowered[i]);
        }

        cy += thisHeight;
//      cy += POS_HEIGHT; // OR one empty Rung between Rungs
// // //cy += 1;          // OR one empty text line between Rungs
    }
    DrawEndRung(0, cy);

    y = Y_PADDING + FONT_HEIGHT*cy;
    yp = y + FONT_HEIGHT*(POS_HEIGHT/2) -
         POS_HEIGHT*FONT_HEIGHT*ScrollYOffset/* - FONT_HEIGHT/2*/;

    COLORREF prev = GetTextColor(Hdc);
    SetTextColor(Hdc, HighlightColours.def);

    sprintf(str,"%4d", Prog.numRungs);
    TextOut(Hdc, 8, yp, str, 4);

    SetTextColor(Hdc, HighlightColours.rungNum);

    sprintf(str,"%4d", IntCodeLen);
    TextOut(Hdc, 8, yp + FONT_HEIGHT, str, 4);

    sprintf(str,"%4d", ProgWriteP);
    TextOut(Hdc, 8, yp + FONT_HEIGHT * 2, str, 4);

    SetTextColor(Hdc, prev);

    if(SelectedGxAfterNextPaint >= 0) {
        int gx=SelectedGxAfterNextPaint, gy=SelectedGyAfterNextPaint;
        MoveCursorNear(&gx, &gy);
        InvalidateRect(MainWindow, NULL, FALSE);
        SelectedGxAfterNextPaint = -1;
        SelectedGyAfterNextPaint = -1;
    } else if(ScrollSelectedIntoViewAfterNextPaint && Selected) {
        SelectElement(-1, -1, Selected->selectedState);
        ScrollSelectedIntoViewAfterNextPaint = FALSE;
        InvalidateRect(MainWindow, NULL, FALSE);
    } else {
        if(!SelectionActive) {
            if(Prog.numRungs > 0) {
                if(MoveCursorTopLeft()) {
                    InvalidateRect(MainWindow, NULL, FALSE);
                }
            }
        }
    }

    // draw the 'buses' at either side of the screen
    r.left = X_PADDING - FONT_WIDTH / 2 - 2;
    r.top = 0;
    r.right = r.left + 3;
    r.bottom = IoListTop;
    FillRect(Hdc, &r, InSimulationMode ? BusLeftBrush : BusBrush);

    r.left += POS_WIDTH*FONT_WIDTH*ColsAvailable + 4;
    r.right += POS_WIDTH*FONT_WIDTH*ColsAvailable + 4;
    FillRect(Hdc, &r, InSimulationMode ? BusRightBus : BusBrush);

    CursorDrawn = FALSE;

    BitBlt(paintDc, 0, 0, bw, bh, BackDc, ScrollXOffset, 0, SRCCOPY);

    if(InSimulationMode) {
        KillTimer(MainWindow, TIMER_BLINK_CURSOR);
    } else {
        KillTimer(MainWindow, TIMER_BLINK_CURSOR);
        BlinkCursor(NULL, 0, NULL, 0); //Draw Cursor
        SetTimer(MainWindow, TIMER_BLINK_CURSOR, 800, BlinkCursor);
    }

    Hdc = paintDc;
    ok();
}

//-----------------------------------------------------------------------------
// Set up the syntax highlighting colours, according to the currently selected
// scheme.
//-----------------------------------------------------------------------------
SyntaxHighlightingColours Schemes[NUM_SUPPORTED_SCHEMES] = {
    {   "Original black color scheme\tJonathan",
        RGB(0, 0, 0),           // bg
        RGB(255, 255, 225),     // def
        RGB(255, 110, 90),      // selected
        RGB(255, 150, 90),      // op
        RGB(255, 255, 100),     // punct
        RGB(255, 160, 160),     // lit
        RGB(120, 255, 130),     // name
        RGB(130, 130, 130),     // rungNum
        RGB(130, 130, 245),     // comment
        RGB(255, 255, 255),     // bus

        RGB(0, 0, 0),           // simBg
        RGB(130, 130, 130),     // simRungNum
        RGB(100, 130, 130),     // simOff
        RGB(255, 150, 150),     // simOn
        RGB(255, 150, 150),     // simBusLeft
        RGB(150, 150, 255),     // simBusRight
    },
    {   "Modified black color scheme\tIhor",
        RGB( 16,  16,  16),     // (0, 0, 0)       // bg
        RGB(255, 255, 225),     // (255, 255, 225) // def
        RGB(255, 128, 128),     // (255, 110,  90) // selected
        RGB(255, 153,  85),     // (255, 150,  90) // op
        RGB(255, 221,  85),     // (255, 255, 100) // punct
        RGB(255, 170, 170),     // (255, 160, 160) // lit
        RGB( 96, 255,  96),     // (120, 255, 130) // name
        RGB(160, 160, 160),     // (130, 130, 130) // rungNum
        RGB(128, 128, 255),     // (130, 130, 245) // comment
        RGB(255, 255, 255),     // (255, 255, 255) // bus

        RGB( 32,  32,  32),     // (0, 0, 0)       // simBg
        RGB(128, 128, 128),     // (130, 130, 130) // simRungNum
        RGB( 96, 128, 128),     // (100, 130, 130) // simOff
        RGB(255, 128, 128),     // (255, 150, 150) // simOn
        RGB(255, 128, 128),     // (255, 150, 150) // simBusLeft
        RGB(128, 128, 255),     // (150, 150, 255) // simBusRight
    },
    {   "White color scheme\tIhor",
        RGB(255, 255, 255),     // background
        RGB(  0,   0,   0),     // default foreground
        RGB(192,   0,  48),     // selected element
        RGB(153,  48,   0),     // 'op code' (like OSR, OSF, ADD, ...)
        RGB(  0,   0,   0),     // punctuation, like square or curly braces
        RGB(160,  20,  20),     // a literal number
        RGB(  0, 128,   0),     // the name of an item
        RGB(128, 128, 128),     // rung numbers
        RGB(102, 102, 102),     // user-written comment text
        RGB(128, 128, 128),     // the 'bus' at the right and left of screen

        RGB(255, 255, 255),     // background, simulation mode
        RGB(128, 128, 128),     // rung number, simulation mode
        RGB( 48, 140,  48),     // de-energized element, simulation mode
        RGB(255,   0, 192),     // energzied element, simulation mode
        RGB(255, 153, 153),     // the 'bus,' can be different colours for
        RGB(153, 153, 255),     // right and left of the screen
    },
    {   "White color scheme\tMark",
        RGB(0xe8, 0xeb, 0xec),  // bg
        RGB(0x00, 0x00, 0x00),  // def
        RGB(0x33, 0x99, 0xff),  // selected
        RGB(0x99, 0xb4, 0xd1),  // op
        RGB(0x00, 0x00, 0x00),  // punct
        RGB(0xb9, 0xd1, 0xea),  // lit
        RGB(0x99, 0xb4, 0xd1),  // name
        RGB(0x6d, 0x6d, 0x6d),  // rungNum
        RGB(0x6d, 0x6d, 0x6d),  // comment
        RGB(0xbf, 0xcd, 0xdb),  // bus

        RGB(0xff, 0xff, 0xff),  // simBg
        RGB(0x6d, 0x6d, 0x6d),  // simRungNum
        RGB(0xbf, 0xcd, 0xdb),  // simOff
        RGB(0x99, 0xb4, 0xd1),  // simOn
        RGB(0x99, 0xb4, 0xd1),  // simBusLeft
        RGB(0xbf, 0xcd, 0xdb),  // simBusRight
    },
    {   "System Colors GetSysColor() in color scheme\tWindows",
        GetSysColor(COLOR_WINDOW),         // background
        GetSysColor(COLOR_WINDOWTEXT),     // default foreground
        GetSysColor(COLOR_HIGHLIGHT),      // selected element
        GetSysColor(COLOR_ACTIVECAPTION),  // 'op code' (like OSR, OSF, ADD, ...)
        GetSysColor(COLOR_INFOTEXT),       // punctuation, like square or curly braces
        GetSysColor(COLOR_GRADIENTACTIVECAPTION),// a literal number
        GetSysColor(COLOR_ACTIVECAPTION),  // the name of an item
        GetSysColor(COLOR_GRAYTEXT),       // rung numbers
        GetSysColor(COLOR_GRAYTEXT),       // user-written comment text
        GetSysColor(COLOR_INACTIVECAPTION),// the 'bus' at the right and left of screen

        GetSysColor(COLOR_WINDOW),         // background, simulation mode
        GetSysColor(COLOR_GRAYTEXT),       // rung number, simulation mode
        GetSysColor(COLOR_INACTIVECAPTION),// de-energized element, simulation mode
        GetSysColor(COLOR_ACTIVECAPTION),  // energzied element, simulation mode COLOR_WINDOWFRAME
        GetSysColor(COLOR_ACTIVECAPTION),  // the 'bus,' can be different colours for
        GetSysColor(COLOR_INACTIVECAPTION),// right and left of the screen
    },
    {   // User uses Black as default color scheme
        "User redefined color scheme\tYou",
        RGB(0, 0, 0),           // bg
        RGB(255, 255, 225),     // def
        RGB(255, 110, 90),      // selected
        RGB(255, 150, 90),      // op
        RGB(255, 220,  50),     // punct // 255, 255, 100
        RGB(255, 160, 160),     // lit
        RGB(120, 255, 130),     // name
        RGB(130, 130, 130),     // rungNum
        RGB(130, 130, 245),     // comment
        RGB(255, 255, 255),     // bus

        RGB(0, 0, 0),           // simBg
        RGB(130, 130, 130),     // simRungNum
        RGB(100, 130, 130),     // simOff
        RGB(255, 150, 150),     // simOn
        RGB(255, 150, 150),     // simBusLeft
        RGB(130, 130, 245),     // simBusRight // 150, 150, 255
    }
};

static void SetSyntaxHighlightingColours(void)
{
    if(arraylen(Schemes) != NUM_SUPPORTED_SCHEMES)
        oops();
    if(scheme < 0) {
        scheme = 1;
        //oops();
    }
    if(scheme >= NUM_SUPPORTED_SCHEMES) {
        scheme = 1;
        //oops();
    }
    memcpy(&HighlightColours, &Schemes[scheme], sizeof(HighlightColours));
}

//-----------------------------------------------------------------------------
void InitBrushesForDrawing(void)
{
    LOGBRUSH lb;
    lb.lbStyle = BS_SOLID;
    lb.lbColor = HighlightColours.simBusRight;
    BusRightBus = CreateBrushIndirect(&lb);

    lb.lbColor = HighlightColours.simBusLeft;
    BusLeftBrush = CreateBrushIndirect(&lb);

    lb.lbColor = HighlightColours.bus;
    BusBrush = CreateBrushIndirect(&lb);

    lb.lbColor = HighlightColours.bg;
    BgBrush = CreateBrushIndirect(&lb);

    lb.lbColor = HighlightColours.simBg;
    SimBgBrush = CreateBrushIndirect(&lb);
}

//-----------------------------------------------------------------------------
// Set up the stuff we'll need to draw our schematic diagram. Fonts, brushes,
// pens, etc.
//-----------------------------------------------------------------------------
void InitForDrawing(void)
{
    SetSyntaxHighlightingColours();

    FixedWidthFont = CreateFont(
        FONT_HEIGHT, FONT_WIDTH,
        0, 0,
        FW_REGULAR,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET, // ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        FF_DONTCARE,
        "Lucida Console");

    FixedWidthFontBold = CreateFont(
        FONT_HEIGHT, FONT_WIDTH,
        0, 0,
        FW_REGULAR, // the bold text renders funny under Vista
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET, // ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        FF_DONTCARE,
        "Lucida Console");

    InitBrushesForDrawing();
}

//-----------------------------------------------------------------------------
// DrawChars function, for drawing to the export buffer instead of to the
// screen.
//-----------------------------------------------------------------------------
static void DrawCharsToExportBuffer(int cx, int cy, char *str)
{
    while(*str) {
//      if(*str >= 10) {
        if(WORD(*str) >= 10) { // WORD typecast allow national charset in comments
            ExportBuffer[cy][cx] = *str;
            cx++;
        }
        str++;
    }
}

//-----------------------------------------------------------------------------
BOOL tGetLastWriteTime(char *FileName, FILETIME *ftWrite)
{
    FILETIME ftCreate, ftAccess;

    HANDLE hFile = CreateFile(FileName,
                   GENERIC_READ,
                   FILE_SHARE_READ,
                   NULL,
                   OPEN_EXISTING,
                   FILE_ATTRIBUTE_NORMAL,
                   NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        Error("Could not open file %s (error %d)\n", FileName, GetLastError());
        return FALSE;
    }
    BOOL b = GetFileTime(hFile, &ftCreate, &ftAccess, ftWrite);
    CloseHandle(hFile);
    return b;
}

//-----------------------------------------------------------------------------
// Возвращаемое значение - в случае успеха TRUE, иначе FALSE
// hFile - дескриптор файла
// lpszString - указатель на буфер для строки

BOOL GetLastWriteTime(HANDLE hFile, char *lpszString)
{
    FILETIME ftCreate, ftAccess, ftWrite;
    SYSTEMTIME stUTC, stLocal;

    // Получаем времена файла.
    if (!GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite))
        return FALSE;

    // Преобразуем время последнего изменения в локальное время.
    FileTimeToSystemTime(&ftWrite, &stUTC);
    SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

    // Составляем строку с датой и временем.
    sprintf(lpszString, "%02d/%02d/%d %02d:%02d:%02d",
        stLocal.wDay, stLocal.wMonth, stLocal.wYear,
        stLocal.wHour, stLocal.wMinute, stLocal.wSecond); // wMilliseconds

    return TRUE;
}

//-----------------------------------------------------------------------------
BOOL sGetLastWriteTime(char *FileName, char *sFileTime)
{
    sFileTime[0]=0;

    HANDLE hFile = CreateFile(FileName,   // открываемый файл
                   GENERIC_READ,          // открываем для чтения
                   FILE_SHARE_READ,       // для совместного чтения
                   NULL,                  // защита по умолчанию
                   OPEN_EXISTING,         // только существующий файл
                   FILE_ATTRIBUTE_NORMAL, // обычный файл
                   NULL);                 // атрибутов шаблона нет

    if (hFile == INVALID_HANDLE_VALUE) {
        Error("Could not open file %s (error %d)\n", FileName, GetLastError());
        return FALSE;
    }
    BOOL b = GetLastWriteTime(hFile, sFileTime);
    CloseHandle(hFile);
    return b;
}

//-----------------------------------------------------------------------------
// Export a text drawing of the ladder logic program to a file.
//-----------------------------------------------------------------------------
void ExportDrawingAsText(char *file)
{
    char sFileTime[512];
    int maxWidth = ProgCountWidestRow();
    ColsAvailable = maxWidth;

    int totalHeight = ProgCountRows();
    totalHeight += 1; // EndRung
    totalHeight *= POS_HEIGHT;
    totalHeight += Prog.numRungs; // for one empty line between rungs
    totalHeight += 2; // after EndRung for # of int and # of AVR/PIC
    //totalHeight is Ok!

    ExportBuffer = (char **)CheckMalloc(totalHeight * sizeof(char *));

    int l = maxWidth*POS_WIDTH + 9;
    int i;
    for(i = 0; i < totalHeight; i++) {
        ExportBuffer[i] = (char *)CheckMalloc(l);
        memset(ExportBuffer[i], ' ', l-1);
        ExportBuffer[i][4] = '|';
        ExportBuffer[i][5] = '|';
        ExportBuffer[i][l-3] = '|';
        ExportBuffer[i][l-2] = '|';
        ExportBuffer[i][l-1] = '\0';
    }

    DrawChars = DrawCharsToExportBuffer;

    char str[10] = "";
    int cx;
    int cy = 1;
    for(i = 0; i < Prog.numRungs; i++) {
        cx = 6;
        DrawElement(Prog.rungs[i], ELEM_SERIES_SUBCKT, Prog.rungs[i], &cx, &cy,
            Prog.rungPowered[i]);
        /*
        if((i + 1) < 10) {
            ExportBuffer[cy+1][1] = '0' + (i + 1);
        } else {
            ExportBuffer[cy+1][1] = '0' + ((i + 1) % 10);
            ExportBuffer[cy+1][0] = '0' + ((i + 1) / 10);
        }
        */
        sprintf(str,"%04d", i+1);
        strncpy(ExportBuffer[cy+1], str, 4);

        if(Prog.OpsInRung[i]) {
            sprintf(str,"%4d",Prog.OpsInRung[i]);
            strncpy(ExportBuffer[cy+2], str, 4);
        }

        if(Prog.HexInRung[i]) {
            sprintf(str,"%4d",Prog.HexInRung[i]);
            strncpy(ExportBuffer[cy+3], str, 4);
        }

        cy += POS_HEIGHT*CountHeightOfElement(ELEM_SERIES_SUBCKT,
            Prog.rungs[i]);
        cy += 1; //+1 for one empty line
    }
    DrawEndRung(6, cy);

    sprintf(str,"%4d", Prog.numRungs);
    strncpy(ExportBuffer[cy+1], str, 4);

    if(IntCodeLen) {
        sprintf(str,"%4d", IntCodeLen);
        strncpy(ExportBuffer[cy+2], str, 4);
    }

    if(ProgWriteP) {
        sprintf(str,"%4d", ProgWriteP);
        strncpy(ExportBuffer[cy+3], str, 4);
    }

    FILE *f = fopen(file, "w");
    if(!f) {
        Error(_("Couldn't open '%s'\n"), f);
        return;
    }

    sGetLastWriteTime(CurrentSaveFile, sFileTime);

    fprintf(f, "LDmicro export text.\n");
    fprintf(f, "Source file: %s from %s\n", CurrentSaveFile, sFileTime);

    if(Prog.mcu && (Prog.mcu->core == PC_LPT_COM))
        fprintf(f, "for '%s', %.3f ms cycle time\n",
            Prog.mcu->mcuName, Prog.cycleTime/1e3);
    else
    if(Prog.mcu) {
        fprintf(f, "for '%s', %.9g MHz crystal, %.3f ms cycle time\n",
            Prog.mcu->mcuName, Prog.mcuClock/1e6, Prog.cycleTime/1e3);
    } else {
        fprintf(f, "no MCU assigned, %.9g MHz crystal, %.3f ms cycle time\n",
            Prog.mcuClock/1e6, Prog.cycleTime/1e3);
    }

    fprintf(f, "\nLADDER DIAGRAM:\n");

    for(i = 0; i < totalHeight; i++) {
        ExportBuffer[i][4] = '|';
        fprintf(f, "%s\n", ExportBuffer[i]);
        CheckFree(ExportBuffer[i]);
    }
    CheckFree(ExportBuffer);
    ExportBuffer = NULL;

    fprintf(f, _("\nI/O ASSIGNMENT:\n"));

    fprintf(f, _("  Name                       | Type               | Pin | Port | Pin name\n"));
    fprintf(f,   " ----------------------------+--------------------+-----+------+-----------\n");
    for(i = 0; i < Prog.io.count; i++) {
        char b[1024];
        memset(b, '\0', sizeof(b));

        PlcProgramSingleIo *io = &Prog.io.assignment[i];
        char *type = IoTypeToString(io->type);
        char pin[MAX_NAME_LEN] = "";
        char portName[MAX_NAME_LEN] = "";
        char pinName[MAX_NAME_LEN] = "";

        if(Prog.mcu && (Prog.mcu->core == PC_LPT_COM) && (io->pin != NO_PIN_ASSIGNED))
            sprintf(pin, "%s", PinToName(io->pin));
        else
            PinNumberForIo(pin, io, portName, pinName);

        sprintf(b, "                             |                    | %3s | %4s | %s\n",
            pin, portName, pinName);

        memcpy(b+2, io->name, strlen(io->name));
        memcpy(b+31, type, strlen(type));
        fprintf(f, "%s", b);
    }

    fprintf(f, "\nVAR LIST:\n");
    SaveVarListToFile(f);

    fclose(f);

    // we may have trashed the grid tables a bit; a repaint will fix that
    InvalidateRect(MainWindow, NULL, FALSE);
}

//-----------------------------------------------------------------------------
// Determine the settings of the vertical and (if needed) horizontal
// scrollbars used to scroll our view of the program.
//-----------------------------------------------------------------------------
int totalHeightScrollbars = 0;
void SetUpScrollbars(BOOL *horizShown, SCROLLINFO *horiz, SCROLLINFO *vert)
{
    totalHeightScrollbars = ProgCountRows();
// // //           + (Prog.numRungs + 0) / POS_HEIGHT // one empty text line between Rungs
//                 + (Prog.numRungs + POS_HEIGHT/2) / POS_HEIGHT // one empty text line between Rungs
//                 + 1; // for the end rung
    /*
    int totalHeight = 0;
    int i;
    for(i = 0; i < Prog.numRungs; i++) {
        totalHeight += CountHeightOfElement(ELEM_SERIES_SUBCKT, Prog.rungs[i]);
        totalHeight++; //  for the empty rung between rungs
    }
    totalHeight += 1; // for the end rung
    */

    int totalWidth = ProgCountWidestRow();

    if(totalWidth <= ScreenColsAvailable()) {
        *horizShown = FALSE;
        ScrollXOffset = 0;
        ScrollXOffsetMax = 0;
    } else {
        *horizShown = TRUE;
        memset(horiz, 0, sizeof(*horiz));
        horiz->cbSize = sizeof(*horiz);
        horiz->fMask = SIF_DISABLENOSCROLL | SIF_ALL;
        horiz->nMin = 0;
        horiz->nMax = X_PADDING + totalWidth*POS_WIDTH*FONT_WIDTH;
        RECT r;
        GetClientRect(MainWindow, &r);
        horiz->nPage = r.right - X_PADDING;
        horiz->nPos = ScrollXOffset;

        ScrollXOffsetMax = horiz->nMax - horiz->nPage + 1;
        if(ScrollXOffset > ScrollXOffsetMax) ScrollXOffset = ScrollXOffsetMax;
        if(ScrollXOffset < 0) ScrollXOffset = 0;
    }

    vert->cbSize = sizeof(*vert);
    vert->fMask = SIF_DISABLENOSCROLL | SIF_ALL;
    vert->nMin = 0;
    vert->nMax = totalHeightScrollbars;// - 1;
    vert->nPos = ScrollYOffset;
    vert->nPage = ScreenRowsAvailable();

    ScrollYOffsetMax = vert->nMax - vert->nPage + 1;

    if(ScrollYOffset > ScrollYOffsetMax) ScrollYOffset = ScrollYOffsetMax;
    if(ScrollYOffset < 0) ScrollYOffset = 0;

    vert->nPos = ScrollYOffset; //???
}
