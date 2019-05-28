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
static HWND RateTextbox;            ///// Added by JG
static HWND SpeedTextbox;           ///// Added by JG

static LONG_PTR PrevCrystalProc;
static LONG_PTR PrevConfigBitsProc;
static LONG_PTR PrevCycleProc;
static LONG_PTR PrevBaudProc;
static LONG_PTR PrevRateProc;       ///// Added by JG
static LONG_PTR PrevSpeedProc;      ///// Added by JG

//-----------------------------------------------------------------------------
// Don't allow any characters other than 0-9. in the text boxes.
//-----------------------------------------------------------------------------
static LRESULT CALLBACK MyNumberProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if(msg == WM_CHAR) {
        if(hwnd == ConfigBitsTextbox) {
            if(!(ishobdigit(wParam) || wParam == '\b'))
                return 0;
        } else if(!(isdigit(wParam) || wParam == '.' || wParam == '\b'))
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
    else if(hwnd == RateTextbox)    ///// Added by JG
        t = PrevRateProc;           /////
    else if(hwnd == SpeedTextbox)   ///// Added by JG
        t = PrevSpeedProc;          /////

    else
        oops();

    return CallWindowProc((WNDPROC)t, hwnd, msg, wParam, lParam);
}

static void MakeControls()
{
    HWND textLabel = CreateWindowEx(0,
                                    WC_STATIC,
                                    _("PLC Cycle Time (ms):"),
                                    WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT,
                                    1,
                                    13,
                                    180,
                                    21,
                                    ConfDialog,
                                    nullptr,
                                    Instance,
                                    nullptr);
    NiceFont(textLabel);

    CycleTextbox = CreateWindowEx(WS_EX_CLIENTEDGE,
                                  WC_EDIT,
                                  "",
                                  WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
                                  185,
                                  12,
                                  75,
                                  21,
                                  ConfDialog,
                                  nullptr,
                                  Instance,
                                  nullptr);
    NiceFont(CycleTextbox);

    HWND textLabel2 = CreateWindowEx(0,
                                     WC_STATIC,
                                     _("MCU Crystal Frequency (MHz):"),
                                     WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT,
                                     1,
                                     43,
                                     180,
                                     21,
                                     ConfDialog,
                                     nullptr,
                                     Instance,
                                     nullptr);
    NiceFont(textLabel2);

    CrystalTextbox = CreateWindowEx(WS_EX_CLIENTEDGE,
                                    WC_EDIT,
                                    "",
                                    WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
                                    185,
                                    42,
                                    75,
                                    21,
                                    ConfDialog,
                                    nullptr,
                                    Instance,
                                    nullptr);
    NiceFont(CrystalTextbox);

    HWND textLabel3 = CreateWindowEx(0,
                                     WC_STATIC,
                                     _("UART Baud Rate (bps):"),
                                     WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT,
                                     1,
                                     73,
                                     180,
                                     21,
                                     ConfDialog,
                                     nullptr,
                                     Instance,
                                     nullptr);
    NiceFont(textLabel3);

    BaudTextbox = CreateWindowEx(WS_EX_CLIENTEDGE,
                                 WC_EDIT,
                                 "",
                                 WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
                                 185,
                                 72,
                                 75,
                                 21,
                                 ConfDialog,
                                 nullptr,
                                 Instance,
                                 nullptr);
    NiceFont(BaudTextbox);

    ///// SPI rate parameter added by JG
    HWND textLabel5 = CreateWindowEx(0,
                                     WC_STATIC,
                                     _("SPI Rate (Hz):"),
                                     WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT,
                                     10,
                                     103,
                                     150,
                                     21,
                                     ConfDialog,
                                     nullptr,
                                     Instance,
                                     nullptr);
    NiceFont(textLabel5);

    RateTextbox = CreateWindowEx(WS_EX_CLIENTEDGE,
                                 WC_EDIT,
                                 "",
                                 WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
                                 165,
                                 102,
                                 95,
                                 21,
                                 ConfDialog,
                                 nullptr,
                                 Instance,
                                 nullptr);
    NiceFont(RateTextbox);
    /////

    ///// I2C speed parameter added by JG
    HWND textLabel6 = CreateWindowEx(0,
                                     WC_STATIC,
                                     _("I2C Rate (Hz):"),
                                     WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT,
                                     245,
                                     103,
                                     150,
                                     21,
                                     ConfDialog,
                                     nullptr,
                                     Instance,
                                     nullptr);
    NiceFont(textLabel6);

    SpeedTextbox = CreateWindowEx(WS_EX_CLIENTEDGE,
                                 WC_EDIT,
                                 "",
                                 WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
                                 400,
                                 102,
                                 100,
                                 21,
                                 ConfDialog,
                                 nullptr,
                                 Instance,
                                 nullptr);
    NiceFont(SpeedTextbox);
    /////

    HWND TimerLabel = CreateWindowEx(0,
                                     WC_STATIC,
                                     _("Timer0|1:"),
                                     WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT,
                                     255,
                                     13,
                                     70,
                                     21,
                                     ConfDialog,
                                     nullptr,
                                     Instance,
                                     nullptr);
    NiceFont(TimerLabel);

    TimerTextbox = CreateWindowEx(WS_EX_CLIENTEDGE,
                                  WC_EDIT,
                                  "",
                                  WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
                                  330,
                                  12,
                                  25,
                                  21,
                                  ConfDialog,
                                  nullptr,
                                  Instance,
                                  nullptr);
    NiceFont(TimerTextbox);

    YPlcCycleDutyCheckbox = CreateWindowEx(0,
                                           WC_BUTTON,
                                           "YPlcCycleDuty",
                                           WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP | WS_VISIBLE,
                                           370,
                                           13,
                                           100,
                                           20,
                                           ConfDialog,
                                           nullptr,
                                           Instance,
                                           nullptr);
    NiceFont(YPlcCycleDutyCheckbox);

    HWND textLabel2_ = CreateWindowEx(0,
                                      WC_STATIC,
                                      _("PIC Configuration Bits:"),
                                      WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_LEFT,
                                      267,
                                      73,
                                      130,
                                      21,
                                      ConfDialog,
                                      nullptr,
                                      Instance,
                                      nullptr);
    NiceFont(textLabel2_);

    ConfigBitsTextbox = CreateWindowEx(WS_EX_CLIENTEDGE,
                                       WC_EDIT,
                                       "",
                                       WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
                                       400,
                                       72,
                                       85,
                                       21,
                                       ConfDialog,
                                       nullptr,
                                       Instance,
                                       nullptr);
    NiceFont(ConfigBitsTextbox);

    ///// Modified by JG to extend to AVRs & ARMs
    if(!Prog.mcu() || ((Prog.mcu()->whichIsa != ISA_PIC16) && (Prog.mcu()->whichIsa != ISA_AVR) && (Prog.mcu()->whichIsa != ISA_ARM))) {
        EnableWindow(ConfigBitsTextbox, false);
        EnableWindow(textLabel2_, false);
    }

    if(!UartFunctionUsed()) {
        EnableWindow(BaudTextbox, false);
        EnableWindow(textLabel3, false);
    }
    ///// Added by JG
    if(!SpiFunctionUsed()) {
        EnableWindow(RateTextbox, false);
        EnableWindow(textLabel5, false);
    }
    if(!I2cFunctionUsed()) {
        EnableWindow(SpeedTextbox, false);
        EnableWindow(textLabel6, false);
    }
    /////
    // clang-format off
    if(Prog.mcu() && (Prog.mcu()->whichIsa == ISA_INTERPRETED ||
                      Prog.mcu()->whichIsa == ISA_XINTERPRETED ||
                      Prog.mcu()->whichIsa == ISA_NETZER))
    {
        EnableWindow(CrystalTextbox, false);
        EnableWindow(textLabel2, false);
    }
    // clang-format on

    ///// Modified by JG to extend to AVRs & ARMs
    if(Prog.mcu() && (Prog.mcu()->whichIsa != ISA_PIC16) && (Prog.mcu()->whichIsa != ISA_AVR) && (Prog.mcu()->whichIsa != ISA_ARM)) {
        EnableWindow(ConfigBitsTextbox, false);
        EnableWindow(textLabel2_, false);
        //      EnableWindow(WDTECheckbox, false);
    }

    ///// Added by JG
    if(Prog.mcu() && (Prog.mcu()->whichIsa == ISA_ARM)) {
        EnableWindow(TimerTextbox, false);
    }
    /////
    OkButton = CreateWindowEx(0,
                              WC_BUTTON,
                              _("OK"),
                              WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE | BS_DEFPUSHBUTTON,
                              268 + 215,
                              11,
                              70,
                              23,
                              ConfDialog,
                              nullptr,
                              Instance,
                              nullptr);
    NiceFont(OkButton);

    CancelButton = CreateWindowEx(0,
                                  WC_BUTTON,
                                  _("Cancel"),
                                  WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
                                  268 + 215,
                                  41,
                                  70,
                                  23,
                                  ConfDialog,
                                  nullptr,
                                  Instance,
                                  nullptr);
    NiceFont(CancelButton);

    char txt[1024 * 4] = "";
    char explanation[1024 * 4] = "";

    bool b = false;
    if(Prog.mcu() && Prog.cycleTime) {
        if(Prog.mcu()->whichIsa == ISA_AVR) {
            b = CalcAvrPlcCycle(Prog.cycleTime, AvrProgLdLen); // && AvrProgLdLen;
        } else if(Prog.mcu()->whichIsa == ISA_PIC16) {
            b = CalcPicPlcCycle(Prog.cycleTime, PicProgLdLen) && PicProgLdLen;
        }
        char   s1[100];
        char   s2[100];
        double _cycleTimeMin = SIprefix(1.0 * plcTmr.cycleTimeMin / 1e6, s1);
        double _cycleTimeMax = SIprefix(1.0 * plcTmr.cycleTimeMax / 1e6, s2);
        sprintf(txt,
                _("Available PLC Cycle Time: min=%.6g %ss, max=%lld ms (%.6g %ss)\n"),
                _cycleTimeMin,
                s1,
                plcTmr.cycleTimeMax / 1000,
                _cycleTimeMax,
                s2);
        strcat(explanation, txt);
        if(b) {
            sprintf(txt,
                    _("MCU PLC Timer%d: TMR%d=%d, prescaler=%d, softDivisor=%d\n"),
                    Prog.cycleTimer,
                    Prog.cycleTimer,
                    plcTmr.tmr,
                    plcTmr.prescaler,
                    plcTmr.softDivisor);
            strcat(explanation, txt);

            sprintf(txt,
                    _("%030lld=TicksPerCycle\n%030lld=In fact\n"),
                    plcTmr.ticksPerCycle,
                    (long long int)plcTmr.prescaler * (long long int)plcTmr.softDivisor * (long long int)plcTmr.tmr);
            strcat(explanation, txt);

            double _TCycle = SIprefix(1.0 * plcTmr.TCycle, s1);
            double _Fcycle = SIprefix(1.0 * plcTmr.Fcycle, s2);
            sprintf(txt,
                    _("In fact TCycle=%.6g %ss, Fcycle=%.6g %sHz, PLC Cycle deviation=%.3f%%\n"),
                    _TCycle,
                    s1,
                    _Fcycle,
                    s2,
                    1e2 * (1e6 * plcTmr.TCycle - Prog.cycleTime) / Prog.cycleTime);
            strcat(explanation, txt);
        }
        sprintf(txt, "\n");
        strcat(explanation, txt);

        double minDelay;
        minDelay = SIprefix(1.0 * Prog.cycleTime / 1000000, s2); // s
        sprintf(txt, _("TON,TOF,RTO min Delay=%.6g ms (%.6g %ss)\n"), 1.0 * Prog.cycleTime / 1000, minDelay, s2);
        strcat(explanation, txt);

        double maxDelay;
        maxDelay = SIprefix(1.0 * 0x7f * Prog.cycleTime / 1000000, s2); // s
        sprintf(txt, _("TON,TOF,RTO  8bit max Delay=%.6g %ss\n"), maxDelay, s2);
        strcat(explanation, txt);

        maxDelay = SIprefix(1.0 * 0x7fff * Prog.cycleTime / 1000000, s2); // s
        sprintf(txt, _("TON,TOF,RTO 16bit max Delay=%.6g %ss\n"), maxDelay, s2);
        strcat(explanation, txt);

        maxDelay = SIprefix(1.0 * 0x7fFFff * Prog.cycleTime / 1000000, s2); // s
        sprintf(txt, _("TON,TOF,RTO 24bit max Delay=%.6g %ss\n"), maxDelay, s2);
        strcat(explanation, txt);

        sprintf(txt, "\n");
        strcat(explanation, txt);
    }
    if(UartFunctionUsed()) {
        if(Prog.mcu() && Prog.mcu()->uartNeeds.rxPin != 0) {
            sprintf(txt,
                    _("Serial (UART) will use pins %d(RX) and %d(TX).\r\n"),
                    Prog.mcu()->uartNeeds.rxPin,
                    Prog.mcu()->uartNeeds.txPin);
            strcat(explanation, txt);
            strcat(explanation, _("Frame format: 8 data, parity - none, 1 stop bit, handshaking - none.\r\n\r\n"));
        } else {
            strcat(explanation, _("Please select a micro with a UART.\r\n\r\n"));
        }
    } else {
        strcat(explanation,
               _("No serial instructions (UART Send/UART Receive) are in use; add one to program before setting baud rate.\r\n\r\n"));
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

    HWND textLabel4 = CreateWindowEx(0,
                                     WC_STATIC,
                                     explanation,
                                     WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
                                     10,
                                     100+30,                ///// 30 added by JG
                                     340 + 200,
                                     800,
                                     ConfDialog,
                                     nullptr,
                                     Instance,
                                     nullptr);
    NiceFont(textLabel4);

    // Measure the explanation string, so that we know how to size our window
    RECT tr, cr;
    int  w = 370 + 200;
    HDC  hdc = CreateCompatibleDC(nullptr);
    SelectObject(hdc, MyNiceFont);
    SetRect(&tr, 0, 0, w, 800);
    DrawText(hdc, explanation, -1, &tr, DT_CALCRECT | DT_LEFT | DT_TOP | DT_WORDBREAK);
    DeleteDC(hdc);
    int h = 104 + tr.bottom + 10 + 20 + 30;                         ///// 30 added by JG
    SetWindowPos(ConfDialog, nullptr, 0, 0, w, h, SWP_NOMOVE);
    // h is the desired client height, but SetWindowPos includes title bar;
    // so fix it up by hand
    GetClientRect(ConfDialog, &cr);
    int nh = h + (h - (cr.bottom - cr.top));
    SetWindowPos(ConfDialog, nullptr, 0, 0, w, nh, SWP_NOMOVE);

    PrevCycleProc = SetWindowLongPtr(CycleTextbox, GWLP_WNDPROC, (LONG_PTR)MyNumberProc);
    PrevCrystalProc = SetWindowLongPtr(CrystalTextbox, GWLP_WNDPROC, (LONG_PTR)MyNumberProc);
    PrevConfigBitsProc = SetWindowLongPtr(ConfigBitsTextbox, GWLP_WNDPROC, (LONG_PTR)MyNumberProc);
    PrevBaudProc = SetWindowLongPtr(BaudTextbox, GWLP_WNDPROC, (LONG_PTR)MyNumberProc);
    PrevRateProc = SetWindowLongPtr(RateTextbox, GWLP_WNDPROC, (LONG_PTR)MyNumberProc);         ///// Added by JG
    PrevSpeedProc = SetWindowLongPtr(SpeedTextbox, GWLP_WNDPROC, (LONG_PTR)MyNumberProc);       ///// Added by JG
}

void ShowConfDialog()
{
    // The window's height will be resized later, to fit the explanation text.
    ConfDialog = CreateWindowClient(0,
                                    "LDmicroDialog",
                                    _("PLC Configuration"),
                                    WS_OVERLAPPED | WS_SYSMENU,
                                    100,
                                    100,
                                    0,
                                    0,
                                    nullptr,
                                    nullptr,
                                    Instance,
                                    nullptr);

    MakeControls();

    char buf[26];
    sprintf(buf, "%.3f", 1.0 * Prog.cycleTime / 1000); //us show as ms
    SendMessage(CycleTextbox, WM_SETTEXT, 0, (LPARAM)buf);

    sprintf(buf, "%d", Prog.cycleTimer);
    SendMessage(TimerTextbox, WM_SETTEXT, 0, (LPARAM)buf);

    if(Prog.cycleDuty) {
        SendMessage(YPlcCycleDutyCheckbox, BM_SETCHECK, BST_CHECKED, 0);
    }

    sprintf(buf, "%.6f", Prog.mcuClock / 1e6); //Hz show as MHz
    SendMessage(CrystalTextbox, WM_SETTEXT, 0, (LPARAM)buf);

    if(!Prog.configurationWord) {
        if(Prog.mcu())
            Prog.configurationWord = Prog.mcu()->configurationWord;
    }
    if(Prog.configurationWord) {
        sprintf(buf, "0x%llX", Prog.configurationWord);
    }
    ///// Added by JG
    else {
        sprintf(buf, "%i", 0);
    }
    /////
    SendMessage(ConfigBitsTextbox, WM_SETTEXT, 0, (LPARAM)buf);

    sprintf(buf, "%d", Prog.baudRate);
    SendMessage(BaudTextbox, WM_SETTEXT, 0, (LPARAM)buf);

    ///// Added by JG
    sprintf(buf, "%d", Prog.spiRate);
    SendMessage(RateTextbox, WM_SETTEXT, 0, (LPARAM)buf);

    sprintf(buf, "%d", Prog.i2cRate);
    SendMessage(SpeedTextbox, WM_SETTEXT, 0, (LPARAM)buf);

    if ((Prog.mcu()) && (Prog.mcu()->whichIsa == ISA_ARM)) {
        Prog.cycleTimer = 3;        //  ARM uses Timer 3
        sprintf(buf, "%d", 3);
        SendMessage(TimerTextbox, WM_SETTEXT, 0, (LPARAM)buf);
    }
    /////

    EnableWindow(MainWindow, false);
    ShowWindow(ConfDialog, true);
    SetFocus(CycleTextbox);

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

        if(IsDialogMessage(ConfDialog, &msg))
            continue;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if(!DialogCancel) {
        char buf[26];
        SendMessage(CycleTextbox, WM_GETTEXT, (WPARAM)sizeof(buf), (LPARAM)(buf));
        double        dProgCycleTime = 1000.0 * atof(buf);
        long long int ProgCycleTime;

        sprintf(buf, "%.0f", dProgCycleTime);
        ProgCycleTime = hobatoi(buf);

        SendMessage(TimerTextbox, WM_GETTEXT, (WPARAM)sizeof(buf), (LPARAM)(buf));
        if(atoi(buf) == 0)
            Prog.cycleTimer = 0;
        else
            Prog.cycleTimer = 1;

        ///// Added by JG2
        if ((Prog.mcu()) && (Prog.mcu()->whichIsa == ISA_ARM))
            {
            Prog.cycleTimer = 3;        //  ARM uses Timer 3
            }
        /////

        if(SendMessage(YPlcCycleDutyCheckbox, BM_GETSTATE, 0, 0) & BST_CHECKED) {
            Prog.cycleDuty = 1;
        } else {
            Prog.cycleDuty = 0;
        }
        SendMessage(CrystalTextbox, WM_GETTEXT, (WPARAM)sizeof(buf), (LPARAM)(buf));
        ///// Added by JG:  convert '.' to ',' for atof()
        for (size_t i= 0 ; i < strlen(buf) ; i++)
            if (buf[i] == '.') buf[i] = ',';
        /////
        Prog.mcuClock = (int)(1e6 * atof(buf) + 0.5);

        SendMessage(ConfigBitsTextbox, WM_GETTEXT, (WPARAM)sizeof(buf), (LPARAM)(buf));

        if(Prog.mcu() && (Prog.mcu()->whichIsa == ISA_PIC16)) {
            Prog.configurationWord = hobatoi(buf);
            if(!Prog.configurationWord) {
                Error(_("Zero Configuration Word(s) not valid."));
                Prog.configurationWord = Prog.mcu()->configurationWord;
            }
        }

        SendMessage(BaudTextbox, WM_GETTEXT, (WPARAM)sizeof(buf), (LPARAM)(buf));
        Prog.baudRate = atoi(buf);

        ///// Added by JG
        SendMessage(RateTextbox, WM_GETTEXT, (WPARAM)sizeof(buf), (LPARAM)(buf));
        Prog.spiRate = atol(buf);

        SendMessage(SpeedTextbox, WM_GETTEXT, (WPARAM)sizeof(buf), (LPARAM)(buf));
        Prog.i2cRate = atol(buf);
        /////

        if(Prog.mcuClock <= 0) {
            Error(_("Zero crystal frequency not valid; resetting to 16 MHz."));
            Prog.mcuClock = 16000000; //16 MHz
        }

        if(Prog.mcu() && (ProgCycleTime > 0)) {
            if(Prog.mcu()->whichIsa == ISA_AVR) {
                CalcAvrPlcCycle(ProgCycleTime, AvrProgLdLen);
            } else if(Prog.mcu()->whichIsa == ISA_PIC16) {
                CalcPicPlcCycle(ProgCycleTime, PicProgLdLen);
            }
        }

        if(ProgCycleTime == 0) {
            Warning(_("A zero cycle time value is available, but timers (TON, TOF, etc) will not work correctly!"));
            Prog.cycleTime = ProgCycleTime;
            Prog.cycleTimer = -1;
        } else if(ProgCycleTime < 0) {
            Error(_("Negative cycle time is not valid; Reset to 10 ms."));
            Prog.cycleTime = 10000; //us
        } else
            Prog.cycleTime = ProgCycleTime;
    }

    EnableWindow(MainWindow, true);
    SetFocus(MainWindow);
    DestroyWindow(ConfDialog);
    return;
}
