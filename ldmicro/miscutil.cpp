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
// Miscellaneous utility functions that don't fit anywhere else. IHEX writing,
// verified memory allocator, other junk.
//-----------------------------------------------------------------------------
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "ldmicro.h"

// We should display messages to the user differently if we are running
// interactively vs. in batch (command-line) mode.
BOOL RunningInBatchMode = FALSE;

// Allocate memory on a local heap
HANDLE MainHeap;

// Running checksum as we build up IHEX records.
static int IhexChecksum;

// Try to common a bit of stuff between the dialog boxes, since only one
// can be open at any time.
HWND OkButton;
HWND CancelButton;
BOOL DialogDone;
BOOL DialogCancel;

HFONT MyNiceFont;
HFONT MyFixedFont;

//-----------------------------------------------------------------------------
// printf-like debug function, to the Windows debug log.
//-----------------------------------------------------------------------------
void dbp(char *str, ...)
{
    va_list f;
    char buf[1024];
    va_start(f, str);
    vsprintf(buf, str, f);
    OutputDebugString(buf);
    OutputDebugString("\n");
}

//-----------------------------------------------------------------------------
// Wrapper for AttachConsole that does nothing running under <WinXP, so that
// we still run (except for the console stuff) in earlier versions.
//-----------------------------------------------------------------------------
#define ATTACH_PARENT_PROCESS ((DWORD)-1) // defined in WinCon.h, but only if
                                          // _WIN32_WINNT >= 0x500
BOOL AttachConsoleDynamic(DWORD base)
{
    typedef BOOL WINAPI fptr_acd(DWORD base);
    fptr_acd *fp;

    HMODULE hm = LoadLibrary("kernel32.dll");
    if(!hm) return FALSE;

    fp = (fptr_acd *)GetProcAddress(hm, "AttachConsole");
    if(!fp) return FALSE;

    return fp(base);
}

//-----------------------------------------------------------------------------
// For error messages to the user; printf-like, to a message box.
//-----------------------------------------------------------------------------
void Error(char *str, ...)
{
    va_list f;
    char buf[1024];
    va_start(f, str);
    vsprintf(buf, str, f);
    if(RunningInBatchMode) {
        AttachConsoleDynamic(ATTACH_PARENT_PROCESS);
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD written;

        // Indicate that it's an error, plus the output filename
        char str[MAX_PATH+100];
        sprintf(str, "compile error ('%s'): ", CurrentCompileFile);
        WriteFile(h, str, strlen(str), &written, NULL);
        // The error message itself
        WriteFile(h, buf, strlen(buf), &written, NULL);
        // And an extra newline to be safe.
        strcpy(str, "\n");
        WriteFile(h, str, strlen(str), &written, NULL);
    } else {
        HWND h = GetForegroundWindow();
        MessageBox(h, buf, _("LDmicro Error"), MB_OK | MB_ICONERROR);
    }
}

//-----------------------------------------------------------------------------
// A standard format for showing a message that indicates that a compile
// was successful.
//-----------------------------------------------------------------------------
void CompileSuccessfulMessage(char *str)
{
    if(RunningInBatchMode) {
        char str[MAX_PATH+100];
        sprintf(str, "compiled okay, wrote '%s'\n", CurrentCompileFile);

        AttachConsoleDynamic(ATTACH_PARENT_PROCESS);
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD written;
        WriteFile(h, str, strlen(str), &written, NULL);
    } else {
        MessageBox(MainWindow, str, _("Compile Successful"),
            MB_OK | MB_ICONINFORMATION);
    }
}

//-----------------------------------------------------------------------------
// Check the consistency of the heap on which all the PLC program stuff is
// stored.
//-----------------------------------------------------------------------------
void CheckHeap(char *file, int line)
{
    static unsigned int SkippedCalls;
    static SDWORD LastCallTime;
    SDWORD now = GetTickCount();

    // It slows us down too much to do the check every time we are called;
    // but let's still do the check periodically; let's do it every 70
    // calls or every 20 ms, whichever is sooner.
    if(SkippedCalls < 70 && (now - LastCallTime) < 20) {
        SkippedCalls++;
        return;
    }

    SkippedCalls = 0;
    LastCallTime = now;

    if(!HeapValidate(MainHeap, 0, NULL)) {
        dbp("file %s line %d", file, line);
        Error("Noticed memory corruption at file '%s' line %d.", file, line);
        oops();
    }
}

//-----------------------------------------------------------------------------
// Like malloc/free, but memsets memory allocated to all zeros. Also TODO some
// checking and something sensible if it fails.
//-----------------------------------------------------------------------------
void *CheckMalloc(size_t n)
{
    ok();
    void *p = HeapAlloc(MainHeap, HEAP_ZERO_MEMORY, n);
    return p;
}
void CheckFree(void *p)
{
    ok();
    HeapFree(MainHeap, 0, p);
}


//-----------------------------------------------------------------------------
// Clear the checksum and write the : that starts an IHEX record.
//-----------------------------------------------------------------------------
void StartIhex(FILE *f)
{
    fprintf(f, ":");
    IhexChecksum = 0;
}

//-----------------------------------------------------------------------------
// Write an octet in hex format to the given stream, and update the checksum
// for the IHEX file.
//-----------------------------------------------------------------------------
void WriteIhex(FILE *f, BYTE b)
{
    fprintf(f, "%02X", b);
    IhexChecksum += b;
}

//-----------------------------------------------------------------------------
// Write the finished checksum to the IHEX file from the running sum
// calculated by WriteIhex.
//-----------------------------------------------------------------------------
void FinishIhex(FILE *f)
{
    IhexChecksum = ~IhexChecksum + 1;
    IhexChecksum = IhexChecksum & 0xff;
    fprintf(f, "%02X\n", IhexChecksum);
}

//-----------------------------------------------------------------------------
// Create a window with a given client area.
//-----------------------------------------------------------------------------
HWND CreateWindowClient(DWORD exStyle, char *className, char *windowName,
    DWORD style, int x, int y, int width, int height, HWND parent,
    HMENU menu, HINSTANCE instance, void *param)
{
    HWND h = CreateWindowEx(exStyle, className, windowName, style, x, y,
        width, height, parent, menu, instance, param);

    RECT r;
    GetClientRect(h, &r);
    width = width - (r.right - width);
    height = height - (r.bottom - height);
    
    SetWindowPos(h, HWND_TOP, x, y, width, height, 0);

    return h;
}

//-----------------------------------------------------------------------------
// Window proc for the dialog boxes. This Ok/Cancel stuff is common to a lot
// of places, and there are no other callbacks from the children.
//-----------------------------------------------------------------------------
static LRESULT CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam,
    LPARAM lParam)
{
    switch (msg) {
        case WM_NOTIFY:
            break;

        case WM_COMMAND: {
            HWND h = (HWND)lParam;
            if(h == OkButton && wParam == BN_CLICKED) {
                DialogDone = TRUE;
            } else if(h == CancelButton && wParam == BN_CLICKED) {
                DialogDone = TRUE;
                DialogCancel = TRUE;
            }
            break;
        }

        case WM_CLOSE:
        case WM_DESTROY:
            DialogDone = TRUE;
            DialogCancel = TRUE;
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 1;
}

//-----------------------------------------------------------------------------
// Set the font of a control to a pretty proportional font (typ. Tahoma).
//-----------------------------------------------------------------------------
void NiceFont(HWND h)
{
    SendMessage(h, WM_SETFONT, (WPARAM)MyNiceFont, TRUE);
}

//-----------------------------------------------------------------------------
// Set the font of a control to a pretty fixed-width font (typ. Lucida
// Console).
//-----------------------------------------------------------------------------
void FixedFont(HWND h)
{
    SendMessage(h, WM_SETFONT, (WPARAM)MyFixedFont, TRUE);
}

//-----------------------------------------------------------------------------
// Create our dialog box class, used for most of the popup dialogs.
//-----------------------------------------------------------------------------
void MakeDialogBoxClass(void)
{
    WNDCLASSEX wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(wc);

    wc.style            = CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW | CS_OWNDC |
                          CS_DBLCLKS;
    wc.lpfnWndProc      = (WNDPROC)DialogProc;
    wc.hInstance        = Instance;
    wc.hbrBackground    = (HBRUSH)COLOR_BTNSHADOW;
    wc.lpszClassName    = "LDmicroDialog";
    wc.lpszMenuName     = NULL;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon            = (HICON)LoadImage(Instance, MAKEINTRESOURCE(4000),
                            IMAGE_ICON, 32, 32, 0);
    wc.hIconSm          = (HICON)LoadImage(Instance, MAKEINTRESOURCE(4000),
                            IMAGE_ICON, 16, 16, 0);

    RegisterClassEx(&wc);

    MyNiceFont = CreateFont(16, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        FF_DONTCARE, "Tahoma");
    if(!MyNiceFont)
        MyNiceFont = (HFONT)GetStockObject(SYSTEM_FONT);

    MyFixedFont = CreateFont(14, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        FF_DONTCARE, "Lucida Console");
    if(!MyFixedFont)
        MyFixedFont = (HFONT)GetStockObject(SYSTEM_FONT);
}

//-----------------------------------------------------------------------------
// Map an I/O type to a string describing it. Used both in the on-screen
// list and when we write a text file to describe it.
//-----------------------------------------------------------------------------
char *IoTypeToString(int ioType)
{
    switch(ioType) {
        case IO_TYPE_DIG_INPUT:         return _("digital in"); 
        case IO_TYPE_DIG_OUTPUT:        return _("digital out"); 
        case IO_TYPE_INTERNAL_RELAY:    return _("int. relay"); 
        case IO_TYPE_UART_TX:           return _("UART tx"); 
        case IO_TYPE_UART_RX:           return _("UART rx"); 
        case IO_TYPE_PWM_OUTPUT:        return _("PWM out"); 
        case IO_TYPE_TON:               return _("turn-on delay"); 
        case IO_TYPE_TOF:               return _("turn-off delay"); 
        case IO_TYPE_RTO:               return _("retentive timer"); 
        case IO_TYPE_COUNTER:           return _("counter"); 
        case IO_TYPE_GENERAL:           return _("general var"); 
        case IO_TYPE_READ_ADC:          return _("adc input"); 
        default:                        return _("<corrupt!>");
    }
}

//-----------------------------------------------------------------------------
// Get a pin number for a given I/O; for digital ins and outs and analog ins,
// this is easy, but for PWM and UART this is forced (by what peripherals
// are available) so we look at the characteristics of the MCU that is in
// use.
//-----------------------------------------------------------------------------
void PinNumberForIo(char *dest, PlcProgramSingleIo *io)
{
    if(!dest) return;

    if(!io) {
        strcpy(dest, "");
        return;
    }
        
    int type = io->type;
    if(type == IO_TYPE_DIG_INPUT || type == IO_TYPE_DIG_OUTPUT
        || type == IO_TYPE_READ_ADC)
    {
        int pin = io->pin;
        if(pin == NO_PIN_ASSIGNED) {
            strcpy(dest, _("(not assigned)"));
        } else {
            sprintf(dest, "%d", pin);
        }
    } else if(type == IO_TYPE_UART_TX && Prog.mcu) {
        if(Prog.mcu->uartNeeds.txPin == 0) {
            strcpy(dest, _("<no UART!>"));
        } else {
            sprintf(dest, "%d", Prog.mcu->uartNeeds.txPin);
        }
    } else if(type == IO_TYPE_UART_RX && Prog.mcu) {
        if(Prog.mcu->uartNeeds.rxPin == 0) {
            strcpy(dest, _("<no UART!>"));
        } else {
            sprintf(dest, "%d", Prog.mcu->uartNeeds.rxPin);
        }
    } else if(type == IO_TYPE_PWM_OUTPUT && Prog.mcu) {
        if(Prog.mcu->pwmNeedsPin == 0) {
            strcpy(dest, _("<no PWM!>"));
        } else {
            sprintf(dest, "%d", Prog.mcu->pwmNeedsPin);
        }
    } else {
        strcpy(dest, "");
    }
}
