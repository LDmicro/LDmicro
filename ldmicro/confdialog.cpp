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
// Dialog for setting the overall PLC parameters. Mostly this relates to
// timing; to set up the timers we need to know the desired cycle time,
// which is configurable, plus the MCU clock (i.e. crystal frequency).
// Jonathan Westhues, Nov 2004
//-----------------------------------------------------------------------------
#include "stdafx.h"

#include "ldmicro.h"

static HWND ConfDialog;

static HWND CrystalTextbox;
static HWND ConfigBitsTextbox;
static HWND CycleTextbox;
static HWND TimerTextbox;
static HWND YPlcCycleDutyCheckbox;
static HWND BaudTextbox;

static LONG_PTR PrevCrystalProc;
static LONG_PTR PrevConfigBitsProc;
static LONG_PTR PrevCycleProc;
static LONG_PTR PrevBaudProc;

//-----------------------------------------------------------------------------
// Don't allow any characters other than 0-9. in the text boxes.
//-----------------------------------------------------------------------------
static LRESULT CALLBACK MyNumberProc(HWND hwnd, UINT msg, WPARAM wParam,
    LPARAM lParam)
{
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

    return CallWindowProc((WNDPROC)t, hwnd, msg, wParam, lParam);
}

static void MakeControls(void)
{
    HWND textLabel = CreateWindowExW(0, WC_STATICW, _("PLC Cycle Time (ms):"),
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT,
        1, 13, 180, 21, ConfDialog, NULL, Instance, NULL);
    NiceFont(textLabel);

    CycleTextbox = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, L"",
        WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
        185, 12, 75, 21, ConfDialog, NULL, Instance, NULL);
    NiceFont(CycleTextbox);

    HWND textLabel2 = CreateWindowExW(0, WC_STATICW,
        _("MCU Crystal Frequency (MHz):"),
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT,
        1, 43, 180, 21, ConfDialog, NULL, Instance, NULL);
    NiceFont(textLabel2);

    CrystalTextbox = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, L"",
        WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
        185, 42, 75, 21, ConfDialog, NULL, Instance, NULL);
    NiceFont(CrystalTextbox);

    HWND textLabel3 = CreateWindowExW(0, WC_STATICW, _("UART Baud Rate (bps):"),
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT,
        1, 73, 180, 21, ConfDialog, NULL, Instance, NULL);
    NiceFont(textLabel3);

    BaudTextbox = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, L"",
        WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
        185, 72, 75, 21, ConfDialog, NULL, Instance, NULL);
    NiceFont(BaudTextbox);

    HWND TimerLabel = CreateWindowExW(0, WC_STATICW, _("Timer0|1:"),
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT,
        255, 13, 70, 21, ConfDialog, NULL, Instance, NULL);
    NiceFont(TimerLabel);

    TimerTextbox = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, L"",
        WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
        330, 12, 25, 21, ConfDialog, NULL, Instance, NULL);
    NiceFont(TimerTextbox);

    YPlcCycleDutyCheckbox = CreateWindowExW(0, WC_BUTTONW, L"YPlcCycleDuty",
        WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP | WS_VISIBLE,
        370, 13, 100, 20, ConfDialog, NULL, Instance, NULL);
    NiceFont(YPlcCycleDutyCheckbox);

    HWND textLabel2_ = CreateWindowExW(0, WC_STATICW,
        _("PIC Configuration Bits:"),
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_LEFT,
        265, 73, 130, 21, ConfDialog, NULL, Instance, NULL);
    NiceFont(textLabel2_);

    ConfigBitsTextbox = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, L"",
        WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
        400, 72, 85, 21, ConfDialog, NULL, Instance, NULL);
    NiceFont(ConfigBitsTextbox);

    if(!Prog.mcu || (Prog.mcu->whichIsa != ISA_PIC16))
    {
        EnableWindow(ConfigBitsTextbox, FALSE);
        EnableWindow(textLabel2_, FALSE);
    }

    if(!UartFunctionUsed()) {
        EnableWindow(BaudTextbox, FALSE);
        EnableWindow(textLabel3, FALSE);
    }

    if(Prog.mcu && (Prog.mcu->whichIsa == ISA_INTERPRETED ||
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
    }

    OkButton = CreateWindowExW(0, WC_BUTTONW, _("OK"),
        WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE | BS_DEFPUSHBUTTON,
        268 + 215, 11, 70, 23, ConfDialog, NULL, Instance, NULL);
    NiceFont(OkButton);

    CancelButton = CreateWindowExW(0, WC_BUTTONW, _("Cancel"),
        WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
        268 + 215, 41, 70, 23, ConfDialog, NULL, Instance, NULL);
    NiceFont(CancelButton);

    wchar_t txt[1024*4] = L"";
    wchar_t explanation[1024*4] = L"";

    BOOL b = FALSE;
    if(Prog.mcu) {
        if(Prog.mcu->whichIsa == ISA_AVR) {
            b=CalcAvrPlcCycle(Prog.cycleTime, AvrProgLdLen); // && AvrProgLdLen;
        } else if(Prog.mcu->whichIsa == ISA_PIC16) {
            b=CalcPicPlcCycle(Prog.cycleTime, PicProgLdLen) && PicProgLdLen;
        }
        wchar_t s1[5];
        wchar_t s2[5];
        double _cycleTimeMin = SIprefix(1.0*plcTmr.cycleTimeMin/1e6, s1);
        double _cycleTimeMax = SIprefix(1.0*plcTmr.cycleTimeMax/1e6, s2);
        swprintf_s(txt,_("Available PLC Cycle Time: min=%.6g %lss, max=%lld ms (%.6g %lss)\n"),
            _cycleTimeMin, s1, plcTmr.cycleTimeMax/1000, _cycleTimeMax, s2);
        wcscat(explanation,txt);
        if(b) {
            swprintf_s(txt,_("MCU PLC Timer%d: TMR%d=%d, prescaler=%d, softDivisor=%d\n"),
                Prog.cycleTimer, Prog.cycleTimer, plcTmr.tmr, plcTmr.prescaler, plcTmr.softDivisor);
            wcscat(explanation,txt);

            swprintf_s(txt,_("%30lld=TicksPerCycle\n%30lld=In fact\n"),
                plcTmr.ticksPerCycle, (long long int)plcTmr.prescaler * (long long int)plcTmr.softDivisor * (long long int)plcTmr.tmr);
            wcscat(explanation,txt);

            double _TCycle = SIprefix(1.0*plcTmr.TCycle, s1);
            double _Fcycle = SIprefix(1.0*plcTmr.Fcycle, s2);
            swprintf_s(txt,_("In fact TCycle=%.6g %lss, Fcycle=%.6g %lsHz, PLC Cycle deviation=%.3f%%\n"), _TCycle, s1, _Fcycle, s2, 1e2*(1e6*plcTmr.TCycle-Prog.cycleTime)/Prog.cycleTime);
            wcscat(explanation,txt);
        }
        swprintf_s(txt,L"\n");
        wcscat(explanation,txt);

        double minDelay;
        minDelay = SIprefix(1.0 * Prog.cycleTime / 1000000, s2); //s
        swprintf_s(txt,_("TON,TOF,RTO min Delay=%.6g ms (%.6g %lss)\n"), 1.0 * Prog.cycleTime / 1000, minDelay, s2);
        wcscat(explanation,txt);

        double maxDelay;
        maxDelay = SIprefix(1.0 * 0x7f * Prog.cycleTime / 1000000, s2); //s
        swprintf_s(txt,_("TON,TOF,RTO  8bit max Delay=%.6g %lss\n"), maxDelay, s2);
        wcscat(explanation,txt);

        maxDelay = SIprefix(1.0 * 0x7fff * Prog.cycleTime / 1000000, s2); //s
        swprintf_s(txt,_("TON,TOF,RTO 16bit max Delay=%.6g %lss\n"), maxDelay, s2);
        wcscat(explanation,txt);

        maxDelay = SIprefix(1.0 * 0x7fFFff * Prog.cycleTime / 1000000, s2); //s
        swprintf_s(txt, _("TON,TOF,RTO 24bit max Delay=%.6g %lss\n"), maxDelay, s2);
        wcscat(explanation,txt);

        swprintf_s(txt,L"\n");
        wcscat(explanation,txt);
    }
    if(UartFunctionUsed()) {
        if(Prog.mcu && Prog.mcu->uartNeeds.rxPin != 0) {
            swprintf_s(txt,
                _("Serial (UART) will use pins %d(RX) and %d(TX).\r\n"),
                Prog.mcu->uartNeeds.rxPin, Prog.mcu->uartNeeds.txPin);
            wcscat(explanation,txt);
            wcscat(explanation, _("Frame format: 8 data, parity - none, 1 stop bit, handshaking - none.\r\n\r\n"));
        } else {
            wcscat(explanation, _("Please select a micro with a UART.\r\n\r\n"));
        }
    } else {
        wcscat(explanation, _("No serial instructions (UART Send/UART Receive) "
            "are in use; add one to program before setting baud rate.\r\n\r\n")
        );
    }

    wcscat(explanation,
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

    HWND textLabel4 = CreateWindowExW(0, WC_STATICW, explanation,
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
        10, 100, 340 + 200, 800, ConfDialog, NULL, Instance, NULL);
    NiceFont(textLabel4);

    // Measure the explanation string, so that we know how to size our window
    RECT tr, cr;
    int w = 370 + 200;
    HDC hdc = CreateCompatibleDC(NULL);
    SelectObject(hdc, MyNiceFont);
    SetRect(&tr, 0, 0, w, 800);
    DrawTextW(hdc, explanation, -1, &tr, DT_CALCRECT | DT_LEFT | DT_TOP | DT_WORDBREAK);
    DeleteDC(hdc);
    int h = 104 + tr.bottom + 10 + 20;
    SetWindowPos(ConfDialog, NULL, 0, 0, w, h, SWP_NOMOVE);
    // h is the desired client height, but SetWindowPos includes title bar;
    // so fix it up by hand
    GetClientRect(ConfDialog, &cr);
    int nh = h + (h - (cr.bottom - cr.top));
    SetWindowPos(ConfDialog, NULL, 0, 0, w, nh, SWP_NOMOVE);

    PrevCycleProc = SetWindowLongPtr(CycleTextbox, GWLP_WNDPROC,
        (LONG_PTR)MyNumberProc);

    PrevCrystalProc = SetWindowLongPtr(CrystalTextbox, GWLP_WNDPROC,
        (LONG_PTR)MyNumberProc);

    PrevConfigBitsProc = SetWindowLongPtr(ConfigBitsTextbox, GWLP_WNDPROC,
        (LONG_PTR)MyNumberProc);

    PrevBaudProc = SetWindowLongPtr(BaudTextbox, GWLP_WNDPROC,
        (LONG_PTR)MyNumberProc);
}

void ShowConfDialog(void)
{
    // The window's height will be resized later, to fit the explanation text.
    ConfDialog = CreateWindowClient(0, L"LDmicroDialog", _("PLC Configuration"),
        WS_OVERLAPPED | WS_SYSMENU,
        100, 100, 0, 0, NULL, NULL, Instance, NULL);

    MakeControls();

    wchar_t buf[26];
    swprintf_s(buf, L"%.3f", 1.0 * Prog.cycleTime / 1000); //us show as ms
    SendMessageW(CycleTextbox, WM_SETTEXT, 0, (LPARAM)buf);

    swprintf_s(buf, L"%d", Prog.cycleTimer);
    SendMessageW(TimerTextbox, WM_SETTEXT, 0, (LPARAM)buf);

    if(Prog.cycleDuty) {
        SendMessage(YPlcCycleDutyCheckbox, BM_SETCHECK, BST_CHECKED, 0);
    }

    swprintf_s(buf, L"%.6f", Prog.mcuClock / 1e6); //Hz show as MHz
    SendMessageW(CrystalTextbox, WM_SETTEXT, 0, (LPARAM)buf);

    if(!Prog.configurationWord) {
        if(Prog.mcu)
            Prog.configurationWord = Prog.mcu->configurationWord;
    }
    swprintf_s(buf, L"");
    if(Prog.configurationWord) {
        swprintf_s(buf, L"0x%llX", Prog.configurationWord);
    }
    SendMessageW(ConfigBitsTextbox, WM_SETTEXT, 0, (LPARAM)buf);

    swprintf_s(buf, L"%d", Prog.baudRate);
    SendMessageW(BaudTextbox, WM_SETTEXT, 0, (LPARAM)buf);

    EnableWindow(MainWindow, FALSE);
    ShowWindow(ConfDialog, TRUE);
    SetFocus(CycleTextbox);

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

        if(IsDialogMessage(ConfDialog, &msg)) continue;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if(!DialogCancel) {
        wchar_t buf[26];
        SendMessageW(CycleTextbox, WM_GETTEXT, (WPARAM)sizeof(buf), (LPARAM)(buf));
        double dProgCycleTime = 1000.0*atof(u8(buf));
        long long int ProgCycleTime;

        swprintf_s(buf,L"%.0f",dProgCycleTime);
        ProgCycleTime = hobatoi(u8(buf));

        SendMessage(TimerTextbox, WM_GETTEXT, (WPARAM)sizeof(buf), (LPARAM)(buf));
        if(atoi(u8(buf)) == 0)
            Prog.cycleTimer = 0;
        else
            Prog.cycleTimer = 1;

        if(SendMessage(YPlcCycleDutyCheckbox, BM_GETSTATE, 0, 0) & BST_CHECKED) {
            Prog.cycleDuty = 1;
        } else {
            Prog.cycleDuty = 0;
        }
        SendMessage(CrystalTextbox, WM_GETTEXT, (WPARAM)sizeof(buf),
            (LPARAM)(buf));
        Prog.mcuClock = (int)(1e6*atof(u8(buf)) + 0.5);

        SendMessage(ConfigBitsTextbox, WM_GETTEXT, (WPARAM)sizeof(buf),
            (LPARAM)(buf));

        if(Prog.mcu && (Prog.mcu->whichIsa == ISA_PIC16)) {
            Prog.configurationWord = hobatoi(u8(buf));
            if(!Prog.configurationWord) {
                Error(_("Zero Configuration Word(s) not valid."));
                Prog.configurationWord = Prog.mcu->configurationWord;
            }
        }

        SendMessage(BaudTextbox, WM_GETTEXT, (WPARAM)sizeof(buf), (LPARAM)(buf));
        Prog.baudRate = atoi(u8(buf));

        if(Prog.mcuClock <= 0) {
            Error(_("Zero crystal frequency not valid; resetting to 16 MHz."));
            Prog.mcuClock = 16000000; //16 MHz
        }

        if(Prog.mcu) {
          if(Prog.mcu->whichIsa == ISA_AVR) {
             CalcAvrPlcCycle(ProgCycleTime, AvrProgLdLen);
          } else if(Prog.mcu->whichIsa == ISA_PIC16) {
             CalcPicPlcCycle(ProgCycleTime, PicProgLdLen);
          }
        }

        if(ProgCycleTime == 0) {
            Error(_(" A zero cycle time value is available, but timers (TON, TOF, etc) will not work correctly!"));
            Prog.cycleTime = ProgCycleTime;
            Prog.cycleTimer = -1;
        } else
        if(ProgCycleTime < 0) {
            Error(_("Negative cycle time is not valid; Reset to 10 ms."));
            Prog.cycleTime = 10000; //us
        } else
            Prog.cycleTime = ProgCycleTime;
    }

    EnableWindow(MainWindow, TRUE);
    SetFocus(MainWindow);
    DestroyWindow(ConfDialog);
    return;
}
