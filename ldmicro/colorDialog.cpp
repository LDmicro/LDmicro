//-----------------------------------------------------------------------------
// Copyright 2017 Ihor Nehrutsa
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
// Dialog for setting the colors of LDmicro.
//-----------------------------------------------------------------------------
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <commctrl.h>
#include <commdlg.h>

#include "ldmicro.h"

static HWND ColorDialog;
static HWND ColorList;

//-----------------------------------------------------------------------------
// Don't allow any characters other than 0-9. in the text boxes.
//-----------------------------------------------------------------------------
static LRESULT CALLBACK MyNumberProc(HWND hwnd, UINT msg, WPARAM wParam,
    LPARAM lParam)
{
/*
    if(msg == WM_CHAR) {
        if(hwnd == ConfigBitsTextbox) {
            if(!(ishobdigit(wParam) || wParam == '\b'))
                return 0;
        } else
            if(!(isdigit(wParam) || wParam == '.' || wParam == '\b'))
                return 0;
    }

    LONG_PTR t;
    if(hwnd == CrystalTextbox)
        t = PrevCrystalProc;
    else if(hwnd == ConfigBitsTextbox)
        t = PrevConfigBitsProc;
    else if(hwnd == CycleTextbox)
        t = PrevCycleProc;
    else if(hwnd == BaudTextbox)
        t = PrevBaudProc;
    else
        oops();
*/
//  return CallWindowProc((WNDPROC)t, hwnd, msg, wParam, lParam);
}

static void MakeControls(void)
{
    ColorList = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTBOX, "",
        WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE | WS_VSCROLL |
//      WS_SIZEBOX |
        LBS_NOTIFY, 10, 10, 455, 240, ColorDialog, NULL, Instance, NULL);
    FixedFont(ColorList);
/*
    HWND textLabel = CreateWindowEx(0, WC_STATIC, _("PLC Cycle Time (ms):"),
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT,
        1, 13, 180, 21, ColorDialog, NULL, Instance, NULL);
    NiceFont(textLabel);

    CycleTextbox = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "",
        WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
        185, 12, 75, 21, ColorDialog, NULL, Instance, NULL);
    NiceFont(CycleTextbox);

    HWND TimerLabel = CreateWindowEx(0, WC_STATIC, _("Timer0|1:"),
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT,
        255, 13, 70, 21, ColorDialog, NULL, Instance, NULL);
    NiceFont(TimerLabel);

    TimerTextbox = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "",
        WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
        330, 12, 25, 21, ColorDialog, NULL, Instance, NULL);
    NiceFont(TimerTextbox);

    YPlcCycleDutyCheckbox = CreateWindowEx(0, WC_BUTTON, _("YPlcCycleDuty"),
        WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP | WS_VISIBLE,
        370, 13, 100, 20, ColorDialog, NULL, Instance, NULL);
    NiceFont(YPlcCycleDutyCheckbox);
/*
    WDTECheckbox = CreateWindowEx(0, WC_BUTTON, _("WDT enable"),
        WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP | WS_VISIBLE,
        370, 43, 100, 20, ColorDialog, NULL, Instance, NULL);
    NiceFont(WDTECheckbox);

    HWND textLabel2 = CreateWindowEx(0, WC_STATIC,
        _("MCU Crystal Frequency (MHz):"),
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT,
        1, 43, 180, 21, ColorDialog, NULL, Instance, NULL);
    NiceFont(textLabel2);

    CrystalTextbox = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "",
        WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
        185, 42, 75, 21, ColorDialog, NULL, Instance, NULL);
    NiceFont(CrystalTextbox);

    HWND textLabel2_ = CreateWindowEx(0, WC_STATIC,
        _("PIC Configuration Bits:"),
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_LEFT,
        265, 73, 130, 21, ColorDialog, NULL, Instance, NULL);
    NiceFont(textLabel2_);

    ConfigBitsTextbox = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "",
        WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
        400, 72, 85, 21, ColorDialog, NULL, Instance, NULL);
    NiceFont(ConfigBitsTextbox);

    HWND textLabel3 = CreateWindowEx(0, WC_STATIC, _("UART Baud Rate (bps):"),
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT,
        1, 73, 180, 21, ColorDialog, NULL, Instance, NULL);
    NiceFont(textLabel3);

    BaudTextbox = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "",
        WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
        185, 72, 75, 21, ColorDialog, NULL, Instance, NULL);
    NiceFont(BaudTextbox);

    if(!UartFunctionUsed()) {
        EnableWindow(BaudTextbox, FALSE);
        EnableWindow(textLabel3, FALSE);
    }

    if(Prog.mcu && (Prog.mcu->whichIsa == ISA_ANSIC ||
                    Prog.mcu->whichIsa == ISA_INTERPRETED ||
                    Prog.mcu->whichIsa == ISA_XINTERPRETED ||
                    Prog.mcu->whichIsa == ISA_PASCAL ||
                    Prog.mcu->whichIsa == ISA_NETZER))
    {
        EnableWindow(CrystalTextbox, FALSE);
        EnableWindow(textLabel2, FALSE);
    }

    if(Prog.mcu && (Prog.mcu->whichIsa != ISA_PIC16))
    {
        EnableWindow(ConfigBitsTextbox, FALSE);
        EnableWindow(textLabel2_, FALSE);
//      EnableWindow(WDTECheckbox, FALSE);
    }
*/
    OkButton = CreateWindowEx(0, WC_BUTTON, _("OK"),
        WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE | BS_DEFPUSHBUTTON,
        480, 10, 70, 23, ColorDialog, NULL, Instance, NULL);
    NiceFont(OkButton);

    CancelButton = CreateWindowEx(0, WC_BUTTON, _("Cancel"),
        WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
        480, 40, 70, 23, ColorDialog, NULL, Instance, NULL);
    NiceFont(CancelButton);
/*
    char txt[1024*4] = "";
    char explanation[1024*4] = "";

    BOOL b;
    int cycleTimeMin;
    int cycleTimeMax;
    int prescaler;
    int sc;
    int divider;
    if(Prog.mcu && (Prog.mcu->whichIsa == ISA_AVR)) {
        b=CalcAvrTimerPlcCycle(Prog.cycleTime,
            &prescaler,
            &sc,
            &divider,
            &cycleTimeMin,
            &cycleTimeMax);
    } else if(Prog.mcu && (Prog.mcu->whichIsa == ISA_PIC16)) {
        b=CalcPicTimerPlcCycle(Prog.cycleTime,
            &cycleTimeMin,
            &cycleTimeMax);
    }

    if((Prog.mcu && (Prog.mcu->whichIsa == ISA_AVR))
    || (Prog.mcu && (Prog.mcu->whichIsa == ISA_PIC16))) {
        char s1[3];
        char s2[3];
        double _cycleTimeMin = SIprefix(1.0*cycleTimeMin/1e6,s1);
        double _cycleTimeMax = SIprefix(1.0*cycleTimeMax/1e6,s2);
        sprintf(txt,"Available PLC Cycle Time: min=%.6g %ss, max=%d ms (%.6g %ss)\n",
            _cycleTimeMin, s1, cycleTimeMax/1000, _cycleTimeMax, s2);
        strcat(explanation,txt);
        if(b) {
            if(Prog.mcu && (Prog.mcu->whichIsa == ISA_AVR)) {
                double _cycleTimeNow = SIprefix(1.0*prescaler*divider/Prog.mcuClock,s2);
                sprintf(txt,"Fact PLC Cycle Time=%.6g %ss with clocksPerCycle=%d\n",
                    _cycleTimeNow, s2, prescaler*divider);
                strcat(explanation,txt);

                sprintf(txt,"MCU PLC Timer%d: prescaler=%d, divider=%d\n",
                    Prog.cycleTimer, prescaler, divider);
                strcat(explanation,txt);
                sprintf(txt,"\n");
                strcat(explanation,txt);
            }
            double minDelay;
            minDelay = SIprefix(1.0 * Prog.cycleTime / 1000000, s2); //s
            sprintf(txt,"TON,TOF,RTO min Delay=%.6g ms (%.6g %ss)\n", 1.0 * Prog.cycleTime / 1000, minDelay, s2);
            strcat(explanation,txt);

            double maxDelay;
            maxDelay = SIprefix(1.0 * 0x7f * Prog.cycleTime / 1000000, s2); //s
            sprintf(txt,"TON,TOF,RTO  8bit max Delay=%.6g %ss\n", maxDelay, s2);
            strcat(explanation,txt);

            maxDelay = SIprefix(1.0 * 0x7fff * Prog.cycleTime / 1000000, s2); //s
            sprintf(txt,"TON,TOF,RTO 16bit max Delay=%.6g %ss\n", maxDelay, s2);
            strcat(explanation,txt);

            maxDelay = SIprefix(1.0 * 0x7fFFff * Prog.cycleTime / 1000000, s2); //s
            sprintf(txt,"TON,TOF,RTO 24bit max Delay=%.6g %ss\n", maxDelay, s2);
            strcat(explanation,txt);
        }
        sprintf(txt,"\n");
        strcat(explanation,txt);
    }
    if(UartFunctionUsed()) {
        if(Prog.mcu && Prog.mcu->uartNeeds.rxPin != 0) {
            sprintf(txt,
                _("Serial (UART) will use pins %d(RX) and %d(TX).\r\n"),
                Prog.mcu->uartNeeds.rxPin, Prog.mcu->uartNeeds.txPin);
            strcat(explanation,txt);
            strcat(explanation,
                _("Frame format: 8 data, parity - none, 1 stop bit, handshaking - none.\r\n\r\n"));
        } else {
            strcat(explanation,
                _("Please select a micro with a UART.\r\n\r\n"));
        }
    } else {
        strcat(explanation, _("No serial instructions (UART Send/UART Receive) "
            "are in use; add one to program before setting baud rate.\r\n\r\n")
        );
    }

    strcat(explanation,
        _("The cycle time for the 'PLC' runtime generated by LDmicro is user-"
        "configurable. Very short cycle times may not be achievable due "
        "to processor speed constraints, and very long cycle times may not "
        "be achievable due to hardware overflows. Cycle times between 10 ms "
        "and 100 ms will usually be practical.\r\n\r\n"
        "The compiler must know what speed crystal you are using with the "
        "micro to convert between timing in clock cycles and timing in "
        "seconds. A 4 MHz to 20 MHz crystal is typical; check the speed "
        "grade of the part you are using to determine the maximum allowable "
        "clock speed before choosing a crystal."));

    HWND textLabel4 = CreateWindowEx(0, WC_STATIC, explanation,
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
        10, 100, 340 + 200, 800, ColorDialog, NULL, Instance, NULL);
    NiceFont(textLabel4);

    // Measure the explanation string, so that we know how to size our window
    RECT tr, cr;
    int w = 370 + 200;
    HDC hdc = CreateCompatibleDC(NULL);
    SelectObject(hdc, MyNiceFont);
    SetRect(&tr, 0, 0, w, 800);
    DrawText(hdc, explanation, -1, &tr, DT_CALCRECT |
                                        DT_LEFT | DT_TOP | DT_WORDBREAK);
    DeleteDC(hdc);
    int h = 104 + tr.bottom + 10 + 20;
    SetWindowPos(ColorDialog, NULL, 0, 0, w, h, SWP_NOMOVE);
    // h is the desired client height, but SetWindowPos includes title bar;
    // so fix it up by hand
    GetClientRect(ColorDialog, &cr);
    int nh = h + (h - (cr.bottom - cr.top));
    SetWindowPos(ColorDialog, NULL, 0, 0, w, nh, SWP_NOMOVE);

    PrevCycleProc = SetWindowLongPtr(CycleTextbox, GWLP_WNDPROC,
        (LONG_PTR)MyNumberProc);

    PrevCrystalProc = SetWindowLongPtr(CrystalTextbox, GWLP_WNDPROC,
        (LONG_PTR)MyNumberProc);

    PrevConfigBitsProc = SetWindowLongPtr(ConfigBitsTextbox, GWLP_WNDPROC,
        (LONG_PTR)MyNumberProc);

    PrevBaudProc = SetWindowLongPtr(BaudTextbox, GWLP_WNDPROC,
        (LONG_PTR)MyNumberProc);
*/
}

UINT_PTR CALLBACK CCHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    //dbpd(uiMsg)
    return 0;
}

BOOL ChooseClr(DWORD *rgbCurrent)
{
    CHOOSECOLOR cc;

    static COLORREF acrCustClr[16];
    ZeroMemory(&acrCustClr, sizeof(acrCustClr));

    acrCustClr[ 0] = Schemes[MNU_SCHEME_USER & 0xff].bg         ;
    acrCustClr[ 1] = Schemes[MNU_SCHEME_USER & 0xff].def        ;
    acrCustClr[ 2] = Schemes[MNU_SCHEME_USER & 0xff].selected   ;
    acrCustClr[ 3] = Schemes[MNU_SCHEME_USER & 0xff].op         ;
    acrCustClr[ 4] = Schemes[MNU_SCHEME_USER & 0xff].punct      ;
    acrCustClr[ 5] = Schemes[MNU_SCHEME_USER & 0xff].lit        ;
    acrCustClr[ 6] = Schemes[MNU_SCHEME_USER & 0xff].name       ;
    acrCustClr[ 7] = Schemes[MNU_SCHEME_USER & 0xff].rungNum    ;
    acrCustClr[ 8] = Schemes[MNU_SCHEME_USER & 0xff].comment    ;
    acrCustClr[ 9] = Schemes[MNU_SCHEME_USER & 0xff].bus        ;
    acrCustClr[10] = Schemes[MNU_SCHEME_USER & 0xff].simBg      ;
    acrCustClr[11] = Schemes[MNU_SCHEME_USER & 0xff].simRungNum ;
    acrCustClr[12] = Schemes[MNU_SCHEME_USER & 0xff].simOff     ;
    acrCustClr[13] = Schemes[MNU_SCHEME_USER & 0xff].simOn      ;
    acrCustClr[14] = Schemes[MNU_SCHEME_USER & 0xff].simBusLeft ;
    acrCustClr[15] = Schemes[MNU_SCHEME_USER & 0xff].simBusRight;

    ZeroMemory(&cc, sizeof(CHOOSECOLOR));
    cc.lStructSize = sizeof(CHOOSECOLOR);
    cc.hwndOwner = ColorDialog;
    cc.lpCustColors = (LPDWORD) acrCustClr;
    cc.rgbResult = *rgbCurrent;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT /*| CC_ENABLEHOOK*/;
    // cc.lpfnHook = &CCHookProc;

    if(ChooseColor(&cc)==TRUE) {
        *rgbCurrent = cc.rgbResult;
        return TRUE;
    }
    return FALSE;
}

void ShowColorDialog(void)
{
    DWORD schemeSave;
    SyntaxHighlightingColours SchemesSave;
    memcpy(&SchemesSave, &Schemes[MNU_SCHEME_USER & 0xff], sizeof(SchemesSave));
    memcpy(&Schemes[MNU_SCHEME_USER & 0xff], &HighlightColours, sizeof(SchemesSave));
    schemeSave = scheme;
    scheme = MNU_SCHEME_USER & 0xff;

    ColorDialog = CreateWindowClient(0, "LDmicroDialog", _("Select color for:"),
        WS_OVERLAPPED | WS_SYSMENU,
        100, 100, 560, 250, NULL, NULL, Instance, NULL);

    MakeControls();
    RECT r;
//  GetClientRect(ColorList, &r);
    GetWindowRect(ColorList, &r);

    int Index = 0;
    SendMessage(ColorList, LB_ADDSTRING, 0, (LPARAM)_("Background"));
    SendMessage(ColorList, LB_ADDSTRING, 0, (LPARAM)_("Default foreground"));
    SendMessage(ColorList, LB_ADDSTRING, 0, (LPARAM)_("Selected element"));
    SendMessage(ColorList, LB_ADDSTRING, 0, (LPARAM)_("OpCode (like OSR, ADD, ...)"));
    SendMessage(ColorList, LB_ADDSTRING, 0, (LPARAM)_("Punctuation, like square or curly braces"));
    SendMessage(ColorList, LB_ADDSTRING, 0, (LPARAM)_("Literal number"));
    SendMessage(ColorList, LB_ADDSTRING, 0, (LPARAM)_("Name of an item"));
    SendMessage(ColorList, LB_ADDSTRING, 0, (LPARAM)_("Rung number"));
    SendMessage(ColorList, LB_ADDSTRING, 0, (LPARAM)_("Comment text"));
    SendMessage(ColorList, LB_ADDSTRING, 0, (LPARAM)_("| The `bus' at the right and left of screen |"));
    SendMessage(ColorList, LB_ADDSTRING, 0, (LPARAM)_("Background, Simulation mode"));
    SendMessage(ColorList, LB_ADDSTRING, 0, (LPARAM)_("Rung number, Simulation mode"));
    SendMessage(ColorList, LB_ADDSTRING, 0, (LPARAM)_("De-energized element, Simulation mode"));
    SendMessage(ColorList, LB_ADDSTRING, 0, (LPARAM)_("Energzied element, Simulation mode"));
    SendMessage(ColorList, LB_ADDSTRING, 0, (LPARAM)_("| The `bus' at the left of the screen, Simulation mode"));
    SendMessage(ColorList, LB_ADDSTRING, 0, (LPARAM)_("The `bus' at the right of the screen |, Simulation mode"));

/*
    char buf[26];
    sprintf(buf, "%.3f", 1.0 * Prog.cycleTime / 1000); //us show as ms
    SendMessage(CycleTextbox, WM_SETTEXT, 0, (LPARAM)buf);

    sprintf(buf, "%d", Prog.cycleTimer);
    SendMessage(TimerTextbox, WM_SETTEXT, 0, (LPARAM)buf);

    if(Prog.cycleDuty) {
        SendMessage(YPlcCycleDutyCheckbox, BM_SETCHECK, BST_CHECKED, 0);
    }
/*
    if(Prog.WDTE) {
        SendMessage(WDTECheckbox, BM_SETCHECK, BST_CHECKED, 0);
    }
*
    sprintf(buf, "%.6f", Prog.mcuClock / 1e6); //Hz show as MHz
    SendMessage(CrystalTextbox, WM_SETTEXT, 0, (LPARAM)buf);

    if(!Prog.configurationWord) {
        if(Prog.mcu)
            Prog.configurationWord = Prog.mcu->configurationWord;
    }
    sprintf(buf, "");
    if(Prog.configurationWord) {
        sprintf(buf, "0x%X", Prog.configurationWord);
    }
    SendMessage(ConfigBitsTextbox, WM_SETTEXT, 0, (LPARAM)buf);

    sprintf(buf, "%d", Prog.baudRate);
    SendMessage(BaudTextbox, WM_SETTEXT, 0, (LPARAM)buf);
*/
    EnableWindow(MainWindow, FALSE);
    ShowWindow(ColorDialog, TRUE);
    SetFocus(ColorList);

    SendMessage(ColorList, LB_SETCURSEL, (WPARAM)Index, 0);

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
        } else if(((msg.message == WM_LBUTTONDBLCLK) || (msg.message == WM_RBUTTONDBLCLK)) && PtInRect(&r, msg.pt)) {
            int sel = SendMessage(ColorList, LB_GETCURSEL, 0, 0);
            switch(sel) {
                case  0: ChooseClr(&Schemes[MNU_SCHEME_USER & 0xff].bg         ); break;
                case  1: ChooseClr(&Schemes[MNU_SCHEME_USER & 0xff].def        ); break;
                case  2: ChooseClr(&Schemes[MNU_SCHEME_USER & 0xff].selected   ); break;
                case  3: ChooseClr(&Schemes[MNU_SCHEME_USER & 0xff].op         ); break;
                case  4: ChooseClr(&Schemes[MNU_SCHEME_USER & 0xff].punct      ); break;
                case  5: ChooseClr(&Schemes[MNU_SCHEME_USER & 0xff].lit        ); break;
                case  6: ChooseClr(&Schemes[MNU_SCHEME_USER & 0xff].name       ); break;
                case  7: ChooseClr(&Schemes[MNU_SCHEME_USER & 0xff].rungNum    ); break;
                case  8: ChooseClr(&Schemes[MNU_SCHEME_USER & 0xff].comment    ); break;
                case  9: ChooseClr(&Schemes[MNU_SCHEME_USER & 0xff].bus        ); break;
                case 10: ChooseClr(&Schemes[MNU_SCHEME_USER & 0xff].simBg      ); break;
                case 11: ChooseClr(&Schemes[MNU_SCHEME_USER & 0xff].simRungNum ); break;
                case 12: ChooseClr(&Schemes[MNU_SCHEME_USER & 0xff].simOff     ); break;
                case 13: ChooseClr(&Schemes[MNU_SCHEME_USER & 0xff].simOn      ); break;
                case 14: ChooseClr(&Schemes[MNU_SCHEME_USER & 0xff].simBusLeft ); break;
                case 15: ChooseClr(&Schemes[MNU_SCHEME_USER & 0xff].simBusRight); break;
                default: oops();
            }
        }
        if(IsDialogMessage(ColorDialog, &msg)) continue;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if(!DialogCancel) {
/*
        char buf[26];
        SendMessage(CycleTextbox, WM_GETTEXT, (WPARAM)sizeof(buf),
            (LPARAM)(buf));
        double dProgCycleTime = 1000.0*atof(buf);
        long long int ProgCycleTime;

        sprintf(buf,"%.0f",dProgCycleTime);
        ProgCycleTime = hobatoi(buf);

        SendMessage(TimerTextbox, WM_GETTEXT, (WPARAM)sizeof(buf), (LPARAM)(buf));
        if(atoi(buf) == 0)
            Prog.cycleTimer = 0;
        else
            Prog.cycleTimer = 1;

        if(SendMessage(YPlcCycleDutyCheckbox, BM_GETSTATE, 0, 0) & BST_CHECKED) {
            Prog.cycleDuty = 1;
        } else {
            Prog.cycleDuty = 0;
        }
/*
        if(SendMessage(WDTECheckbox, BM_GETSTATE, 0, 0) & BST_CHECKED) {
            Prog.WDTE = 1;
        } else {
            Prog.WDTE = 0;
        }
*/
        memcpy(&HighlightColours, &Schemes[MNU_SCHEME_USER & 0xff], sizeof(SchemesSave));
        RefreshControlsToSettings();
    } else {
        scheme = schemeSave;
        memcpy(&Schemes[MNU_SCHEME_USER & 0xff], &SchemesSave, sizeof(SchemesSave));
    }
    EnableWindow(MainWindow, TRUE);
    DestroyWindow(ColorDialog);
    return;
}
