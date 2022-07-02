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
// All the simple dialogs that just require a row of text boxes: timer name
// and delay, counter name and count, move and arithmetic destination and
// operands. Try to reuse code a bit.
// Jonathan Westhues, Nov 2004
//-----------------------------------------------------------------------------
#include "stdafx.h"

#include "ldmicro.h"

static HWND SimpleDialog;

#define MAX_BOXES MAX_IO_PORTS

static HWND Textboxes[MAX_BOXES];
static HWND Labels[MAX_BOXES];
static HWND ComboBox[MAX_BOXES];

static LONG_PTR PrevAlnumOnlyProc[MAX_BOXES];
static LONG_PTR PrevNumOnlyProc[MAX_BOXES];

static bool NoCheckingOnBox[MAX_BOXES];

#define MAX_COMBO_STRINGS 16
typedef struct comboRecordTag {
    int         n;                      // 0 <= n < MAX_COMBO_STRINGS
    const char *str[MAX_COMBO_STRINGS]; // array MAX_COMBO_STRINGS of pointers of char
} comboRecord;

//-----------------------------------------------------------------------------
// Don't allow any characters other than -A-Za-z0-9_ in the box.
//-----------------------------------------------------------------------------
static LRESULT CALLBACK MyAlnumOnlyProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if(msg == WM_CHAR) {
        if(!(isalpha(wParam) || isdigit(wParam) || wParam == '_' || wParam == '@' || wParam == '#' || wParam == '\b' || wParam == '-' || wParam == '\'')) {
            return 0;
        }
    }

    for(int i = 0; i < MAX_BOXES; i++) {
        if(hwnd == Textboxes[i]) {
            return CallWindowProc((WNDPROC)PrevAlnumOnlyProc[i], hwnd, msg, wParam, lParam);
        }
    }
    oops();
    // return 0;
}

//-----------------------------------------------------------------------------
// Don't allow any characters other than -0-9. in the box.
//-----------------------------------------------------------------------------
static LRESULT CALLBACK MyNumOnlyProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if(msg == WM_CHAR) {
        if(!(ishobdigit(wParam) || wParam == '.' || wParam == '\b' || wParam == ' ' || wParam == '\'' || wParam == '\\' || wParam == '-')) {
            return 0;
        }
    }

    for(int i = 0; i < MAX_BOXES; i++) {
        if(hwnd == Textboxes[i]) {
            return CallWindowProc((WNDPROC)PrevNumOnlyProc[i], hwnd, msg, wParam, lParam);
        }
    }
    oops();
    // return 0;
}

static void MakeControls(int labs, const char **labels, int boxes, char **dests, DWORD fixedFontMask, int combo, comboRecord *combos)
{
    int i, j;
    HDC hdc = GetDC(SimpleDialog);
    SelectObject(hdc, MyNiceFont);

    SIZE si;

    int maxLen = 0;
    for(i = 0; i < boxes /*labs*/; i++) {
        GetTextExtentPoint32(hdc, labels[i], strlen(labels[i]), &si);
        if(si.cx > maxLen)
            maxLen = si.cx;
    }

    int adj;
    if(maxLen > 70) {
        adj = maxLen - 70;
    } else {
        adj = 0;
    }

    for(i = 0; i < labs; i++) {
        GetTextExtentPoint32(hdc, labels[i], strlen(labels[i]), &si);

        Labels[i] = CreateWindowEx(0, WC_STATIC, labels[i], WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE, (i < boxes ? (80 + adj) - si.cx : 0) + 15, 13 + i * 30, si.cx, 21, SimpleDialog, nullptr, Instance, nullptr);
        NiceFont(Labels[i]);
    }

    for(i = 0; i < boxes; i++) {
        Textboxes[i] =
            CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE, 80 + 25 + adj, 12 + 30 * i, 120 + 535 - adj, 21, SimpleDialog, nullptr, Instance, nullptr);

        if(fixedFontMask & (1 << i)) {
            FixedFont(Textboxes[i]);
        } else {
            NiceFont(Textboxes[i]);
        }
    }

    for(i = 0; i < combo; i++) {
        if(combos[i].n) {
            //EnableWindow(Textboxes[i], false);
            ShowWindow(Textboxes[i], SW_HIDE);

            ComboBox[i] = CreateWindowEx(WS_EX_CLIENTEDGE,
                                         WC_COMBOBOX,
                                         "",
                                         WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE | CBS_DROPDOWN | CBS_HASSTRINGS | WS_OVERLAPPED,
                                         80 + 25 + adj,
                                         12 + 30 * i,
                                         120 + 535 - adj,
                                         21,
                                         SimpleDialog,
                                         nullptr,
                                         Instance,
                                         nullptr);

            if(fixedFontMask & (1 << i)) {
                FixedFont(ComboBox[i]);
            } else {
                NiceFont(ComboBox[i]);
            }

            int ItemIndex = 0;
            for(j = 0; j < combos[i].n; j++) {
                SendMessage(ComboBox[i], (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)combos[i].str[j]);
                if(dests[i] && strlen(dests[i]))
                    if(strstr(combos[i].str[j], dests[i]))
                        ItemIndex = j;
            }
            SendMessage(ComboBox[i], CB_SETCURSEL, (WPARAM)ItemIndex, (LPARAM)0);
        }
    }

    ReleaseDC(SimpleDialog, hdc);

    OkButton = CreateWindowEx(0, WC_BUTTON, _("OK"), WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE | BS_DEFPUSHBUTTON, 218 + 550, 11, 70, 23, SimpleDialog, nullptr, Instance, nullptr);
    NiceFont(OkButton);

    CancelButton = CreateWindowEx(0, WC_BUTTON, _("Cancel"), WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE, 218 + 550, 41, 70, 23, SimpleDialog, nullptr, Instance, nullptr);
    NiceFont(CancelButton);
}

///// prototype modified by JG with extra default parameter rdonly
static bool ShowSimpleDialog(const char *title, int labs, const char **labels, DWORD numOnlyMask, DWORD alnumOnlyMask, DWORD fixedFontMask, int boxes, char **dests, int combo = 0, comboRecord *combos = nullptr,
                             long rdOnly = 0)
{
    bool didCancel = false;

    try { //// try... catch added by JG

        if(labs > MAX_BOXES)
            oops();
        if(boxes > MAX_BOXES)
            oops();
        if(boxes > labs)
            oops();
        if(combo > MAX_BOXES)
            oops();
        if(combo > boxes)
            oops();

        SimpleDialog =
            CreateWindowClient(0, "LDmicroDialog", title, WS_OVERLAPPED | WS_SYSMENU, 100, 100, 304 + 550, 15 + 30 * (std::max(boxes, labs) < 2 ? 2 : std::max(boxes, labs)), nullptr, nullptr, Instance, nullptr);

        MakeControls(labs, labels, boxes, dests, fixedFontMask, combo, combos);

        int i;
        for(i = 0; i < boxes; i++) {
            SendMessage(Textboxes[i], WM_SETTEXT, 0, (LPARAM)dests[i]); ///// fill boxes with current settings
            ///// added by JG to make some fields read-only (max= 32 boxes)
            if(rdOnly & (1 << i))
                SendMessage(Textboxes[i], EM_SETREADONLY, TRUE, 0);
            /////

            if(numOnlyMask & (1 << i)) {
                PrevNumOnlyProc[i] = SetWindowLongPtr(Textboxes[i], GWLP_WNDPROC, (LONG_PTR)MyNumOnlyProc);
            } else // numOnlyMask overpower alnumOnlyMask
                if(alnumOnlyMask & (1 << i)) {
                PrevAlnumOnlyProc[i] = SetWindowLongPtr(Textboxes[i], GWLP_WNDPROC, (LONG_PTR)MyAlnumOnlyProc);
            }
        }

        EnableWindow(MainWindow, false);
        ShowWindow(SimpleDialog, true);
        SetFocus(Textboxes[0]);
        SendMessage(Textboxes[0], EM_SETSEL, 0, -1);

        MSG   msg;
        DWORD ret;
        DialogDone = false;
        DialogCancel = false;
        while(((ret = GetMessage(&msg, nullptr, 0, 0)) != 0) && !DialogDone) {
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

            if(IsDialogMessage(SimpleDialog, &msg))
                continue;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        didCancel = DialogCancel;
        if(!didCancel) {
            for(i = 0; i < combo; i++) {
                if(combos[i].n) {
                    if(combos[i].n > MAX_COMBO_STRINGS)
                        oops();
                    int ItemIndex = SendMessage(ComboBox[i], CB_GETCURSEL, 0, 0);
                    SendMessage(Textboxes[i], WM_SETTEXT, 0, (LPARAM)combos[i].str[ItemIndex]);
                }
            }
            for(i = 0; i < boxes; i++) {
                if(NoCheckingOnBox[i]) {
                    char get[MAX_NAME_LEN];
                    SendMessage(Textboxes[i], WM_GETTEXT, (MAX_NAME_LEN - 1), (LPARAM)get);
                    strcpy(dests[i], get);
                } else {
                    char get[MAX_NAME_LEN];
                    SendMessage(Textboxes[i], WM_GETTEXT, (MAX_NAME_LEN - 1), (LPARAM)get);

                    if((!strchr(get, '\'')) || (get[0] == '\'' && get[3] == '\'' && strlen(get) == 4 && get[1] == '\\') || (get[0] == '\'' && get[2] == '\'' && strlen(get) == 3)) {
                        if(strlen(get) == 0) {
                            Error(_("Empty textbox; not permitted."));
                        } else {
                            strcpy(dests[i], get);
                        }
                    } else {
                        Error(_("Bad use of quotes: <%s>"), get);
                    }
                }
            }
        }
        EnableWindow(MainWindow, true);
        SetFocus(MainWindow);
        DestroyWindow(SimpleDialog);

    } catch(...) { ///// Try... catch added by JG

        ///// Added by JG to save work in case of big bug
        srand((unsigned int)time(nullptr));
        char fname[20];
        sprintf(fname, "tmpfile_%4.4d.ld", rand() % 10000);
        SaveProjectToFile(fname, MNU_SAVE_02);
        /////

        abortHandler(EXCEPTION_EXECUTE_HANDLER);
    };

    return !didCancel;
}
/*
//as default : labels = boxes
static bool ShowSimpleDialog(const char *title, int boxes, const char **labels, DWORD numOnlyMask, DWORD alnumOnlyMask, DWORD fixedFontMask, char **dests)
{
    return ShowSimpleDialog(title, boxes, labels, numOnlyMask, alnumOnlyMask, fixedFontMask, boxes, dests, 0, nullptr);
}

//coment : labels > boxes
static bool ShowSimpleDialog(const char *title, int labs, const char **labels, DWORD numOnlyMask, DWORD alnumOnlyMask, DWORD fixedFontMask, int boxes, char **dests)
{
    return ShowSimpleDialog(title, labs, labels, numOnlyMask, alnumOnlyMask, fixedFontMask, boxes, dests, 0, nullptr);
}
*/
void ShowTimerDialog(int which, ElemLeaf *l)
{
    ElemTimer *e = &(l->d.timer);
    char *     name = e->name;
    char *     delay = e->delay;
    int *      adjust = &(e->adjust);

    char buf1[1024];
    if(which == ELEM_TIME2DELAY) {
        char s[100];
        strcpy(buf1, _("Achievable DELAY values (us): "));
        long long T = 0, T0 = 0;
        int       i, n = 0;
        for(i = 1;; i++) {
            if(Prog.mcu()) {
                if(Prog.mcu()->whichIsa == ISA_AVR) {
                    T = i; // to long long
                    T = (T * 4 + 1) * 1000000 / Prog.mcuClock;
                } else if(Prog.mcu()->whichIsa == ISA_PIC16) {
                    T = i; // to long long
                    T = (T * 6 + 10) * 4000000 / Prog.mcuClock;
                } else if(Prog.mcu()->whichIsa == ISA_PIC18) {
                    T = i; // to long long
                    T = (T * 6 + 10) * 4000000 / Prog.mcuClock;
                }
                if(T != T0) {
                    T0 = T;
                    sprintf(s, "%lld, ", T);
                    strcat(buf1, s);
                    n++;
                    if(n >= 5)
                        break;
                }
            }
        }
        if(Prog.mcu()) {
            if(Prog.mcu()->whichIsa == ISA_AVR) {
                T = 0x10000; // to long long
                T = (T * 4 + 1) * 1000000 / Prog.mcuClock;
            } else if(Prog.mcu()->whichIsa == ISA_PIC16) {
                T = 0xffff; // to long long
                T = (T * 6 + 10) * 4000000 / Prog.mcuClock;
            } else if(Prog.mcu()->whichIsa == ISA_PIC18) {
                T = 0xffff; // to long long
                T = (T * 6 + 10) * 4000000 / Prog.mcuClock;
            }
            sprintf(s, "..., %lld", T);
            strcat(buf1, s);
        }
    }
    char buf2[100];
    sprintf(buf2, _("** Total timer delay (ms) = Delay (ms) + Adjust * PLC cycle time (%.3f ms)"),
            1.0 * Prog.cycleTime / 1000); //us show as ms
    const char *labels[] = {
        _("Name:"), which == ELEM_TIME2DELAY ? _("Delay (us):") : _("Delay (ms):"), which == ELEM_TIME2DELAY ? buf1 : _("Adjust:"), _("* Adjust default = 0 (LDmicro v3.5.3), typical = -1 (LDmicro v2.3)"), buf2};

    char adjustBuf[16];
    char delBuf[16];
    char nameBuf[MAX_NAME_LEN];
    if(IsNumber(delay)) {
        sprintf(delBuf, "%.3f", hobatoi(delay) / 1000.0);
    } else {
        strcpy(delBuf, delay);
    }
    sprintf(adjustBuf, "%d", (*adjust));
    strcpy(nameBuf, name + 1);
    char *dests[] = {nameBuf, delBuf, adjustBuf};

    int         labs = arraylen(labels);
    int         boxes = arraylen(dests);
    const char *s;
    // clang-format off
    switch(which) {
        case ELEM_TIME2DELAY: s = _("TIME to DELAY converter"); labs = 3; boxes = 2; sprintf(delBuf, "%d", *delay); strcpy(nameBuf, name); break;
        case ELEM_TIME2COUNT: s = _("TIME to COUNTER converter"); labs = 2; boxes = 2; break;
        case ELEM_TCY:        s = _("Cyclic On/Off"); break;
        case ELEM_TON:        s = _("Turn-On Delay"); break;
        case ELEM_TOF:        s = _("Turn-Off Delay"); break;
        case ELEM_THI:        s = _("High Level Delay"); break;
        case ELEM_TLO:        s = _("Low Level Delay"); break;
        case ELEM_RTO:        s = _("Retentive Turn-On Delay"); break;
        case ELEM_RTL:        s = _("Retentive Turn-On Delay If Low Input"); break;
        default: oops(); break;
    }
    // clang-format on
    if(ShowSimpleDialog(s, labs, labels, (1 << 2), (3 << 0), (7 << 0), boxes, dests)) {
        *adjust = atoi(adjustBuf);
        double  delay_ms;
        int32_t delay_us;
        if(which == ELEM_TIME2DELAY) {
            delay_us = (int32_t)round(atof(delBuf));
            strcpy(name, nameBuf);

            if(delay_us > 0)
                strcpy(delay, delBuf);
            //*delay = (SDWORD)delay_us;
        } else {
            name[0] = 'T';
            strcpy(name + 1, nameBuf);
            if(IsNumber(delBuf)) {
                delay_ms = atof(delBuf);
                delay_us = (int32_t)round(delay_ms * 1000.0);

                if(delay_us > LONG_MAX) {
                    Error(_("Timer period too long.\n\rMaximum possible value is: 2^31 us = 2147483647 us = 2147,48 s = 35.79 min"));
                    delay_us = LONG_MAX;
                }

                int32_t period;
                if(Prog.cycleTime <= 0) {
                    Warning(_("PLC Cycle Time is '0'. TON, TOF, RTO, RTL, TCY timers does not work correctly!"));
                    period = 1;
                } else {
                    period = TestTimerPeriod(name, delay_us, *adjust);
                }
                if(period > 0)
                    sprintf(delay, "%d", delay_us);
            } else {
                strcpy(delay, delBuf);
            }
        }
    }
}

void ShowSleepDialog(ElemLeaf *l)
{
    ElemTimer *e = &(l->d.timer);
    char *     name = e->name;
    char *     delay = e->delay;

    const char *s;
    s = _("Sleep Delay");

    const char *labels[] = {/*_("Name:"),*/ _("Delay (s):")};

    char delBuf[16];
    char nameBuf[MAX_NAME_LEN];
    sprintf(delBuf, "%.3f", (*delay / 1000000.0));
    strcpy(nameBuf, name + 1);
    char *dests[] = {/*nameBuf,*/ delBuf};

    if(ShowSimpleDialog(s, arraylen(labels), labels, (1 << 1), (1 << 0), (1 << 0), arraylen(dests), dests)) {
        name[0] = 'T';
        strcpy(name + 1, nameBuf);
        double  del = atof(delBuf);
        int64_t period = (int64_t)round(del / 1000 / 18); // 18 ms
        if(del <= 0) {
            Error(_("Delay cannot be zero or negative."));
        } else if(period < 1) {
            const char *s1 = _("Timer period too short (needs faster cycle time).");
            char        s2[1024];
            sprintf(s2, _("Timer '%s'=%.3f ms."), name, del);
            char s3[1024];
            sprintf(s3, _("Minimum available timer period = PLC cycle time = %.3f ms."), 1.0 * Prog.cycleTime / 1000);
            const char *s4 = _("Not available");
            Error("%s\n\r%s %s\r\n%s", s1, s4, s2, s3);
        } else if((period >= ((int64_t)1 << ((int64_t)(SizeOfVar(name) * 8 - 1)))) && (Prog.mcu()->whichIsa != ISA_PC)) {
            const char *s1 = _("Timer period too long (max 32767 times cycle time); use a slower cycle time.");
            char        s2[1024];
            sprintf(s2, _("Timer 'T%s'=%.3f ms needs %d PLC cycle times."), nameBuf, del / 1000, period);
            double maxDelay = 1.0 * ((1 << (SizeOfVar(name) * 8 - 1)) - 1) * Prog.cycleTime / 1000000; //s
            char   s3[1024];
            sprintf(s3, _("Maximum available timer period = %.3f s."), maxDelay);
            Error("%s\r\n%s\r\n%s", s1, s2, s3);
            //*delay = (int32_t)(1000000 * del + 0.5);
            strcpy(delay, delBuf);
        } else {
            //*delay = (int32_t)(1000000 * del + 0.5);
            strcpy(delay, delBuf);
        }
    }
}

void ShowDelayDialog(ElemLeaf *l)
{
    ElemTimer *e = &(l->d.timer);
    char *     name = e->name;
    char       s[100] = "";
    char       buf1[1024];
    strcpy(buf1, _("Achievable DELAY values (us): "));
    long long T = 0, T0 = 0;
    int       i, n = 0;
    if(Prog.mcu() && ((Prog.mcu()->whichIsa == ISA_AVR) || (Prog.mcu()->whichIsa == ISA_PIC16) || (Prog.mcu()->whichIsa == ISA_PIC18))) {
        for(i = 0;; i++) {
            if(Prog.mcu()->whichIsa == ISA_AVR) {
                T = i * 1000000 / Prog.mcuClock;
            } else if(Prog.mcu()->whichIsa == ISA_PIC16) {
                T = i * 4000000 / Prog.mcuClock;
            } else if(Prog.mcu()->whichIsa == ISA_PIC18) {
                T = i * 4000000 / Prog.mcuClock;
            }
            if(T != T0) {
                T0 = T;
                sprintf(s, "%lld, ", T);
                strcat(buf1, s);
                n++;
                if(n >= 5)
                    break;
            }
        }
    }
    if(Prog.mcu()) {
        if(Prog.mcu()->whichIsa == ISA_AVR) {
            T = 0x10000; // to long long
            T = (T * 4 + 1) * 1000000 / Prog.mcuClock;
        } else if(Prog.mcu()->whichIsa == ISA_PIC16) {
            T = 0xffff; // to long long
            T = (T * 6 + 10) * 4000000 / Prog.mcuClock;
        } else if(Prog.mcu()->whichIsa == ISA_PIC18) {
            T = 0xffff; // to long long
            T = (T * 6 + 10) * 4000000 / Prog.mcuClock;
        }
        sprintf(s, "..., %lld", T);
        strcat(buf1, s);
    }
    const char *labels[] = {_("Delay (us):"), buf1};

    char delBuf[16];
    sprintf(delBuf, "%s", name);
    char *dests[] = {delBuf};

    if(ShowSimpleDialog(_("Delay, us"), arraylen(labels), labels, (0 << 0), (1 << 0), (1 << 0), arraylen(dests), dests)) {
        if(IsNumber(delBuf)) {
            int32_t del = hobatoi(delBuf);
            if(del <= 0) {
                Error(_("Delay cannot be zero or negative."));
            } else {
                strcpy(name, delBuf);
            }
        } else {
            strcpy(name, delBuf);
        }
    }
}

//-----------------------------------------------------------------------------
// Report an error if a constant doesn't fit in 8-16-24 bits.
//-----------------------------------------------------------------------------
static void CheckConstantInRange(const char *name, const char *str, int32_t v, int32_t loRange = INT32_MIN, int32_t hiRange = INT32_MAX)
{
    int32_t val = hobatoi(str);
    if(val != v)
        oops();
    int radix = getradix(str);

    int sov;
    if(strlen(name) == 0)
        sov = 3;
    else if(IsNumber(name)) {
        sov = byteNeeded(hobatoi(name));
        name = "$_tmp_Var";
    } else
        sov = SizeOfVar(name);
    if(sov == 1) {
        if((v < -128 || v > 127) && (radix == 10))
            Error(_("Constant %s=%d out of variable '%s' range : -128 to 127 inclusive."), str, v, name);
        else if((v < 0 || v > 0xff) && (radix != 10))
            Error(_("Constant %s=%d out of variable '%s' range : 0 to 255 inclusive."), str, v, name);
    } else if((sov == 2) || (sov == 0)) {
        if((v < -32768 || v > 32767) && (radix == 10))
            Error(_("Constant %s=%d out of variable '%s' range: -32768 to 32767 inclusive."), str, v, name);
        else if((v < 0 || v > 0xffff) && (radix != 10))
            Error(_("Constant %s=%d out of variable '%s' range : 0 to 65535 inclusive."), str, v, name);
    } else if(sov == 3) {
        if((v < -8388608 || v > 8388607) && (radix == 10))
            Error(_("Constant %s=%d out of variable '%s' range: -8388608 to 8388607 inclusive."), str, v, name);
        else if((v < 0 || v > 0xffffff) && (radix != 10))
            Error(_("Constant %s=%d out of variable '%s' range : 0 to 16777215 inclusive."), str, v, name);
    } else if(sov == 4) {
        if((v < -2147483648LL || v > 2147483647) && (radix == 10))
            Error(_("Constant %s=%d out of variable '%s' range: -2147483648 to 2147483647 inclusive."), str, v, name);
        else if((DWORD(v) < 0 || DWORD(v) > 0xFFFFffff) && (radix != 10))
            Error(_("Constant %s=%d out of variable '%s' range : 0 to 4294967295(0xFFFFffff) inclusive."), str, v, name);
    } else
        ooops("Constant %s Variable '%s' size=%d value=%d", str, name, sov, v);
    if(v < loRange)
        Error(_("Constant %s=%d value '%s' is less than the lower range: %d."), str, v, name, loRange);
    if(v > hiRange)
        Error(_("Constant %s=%d value '%s' is greater than the upper range: %d."), str, v, name, loRange);
}

//-----------------------------------------------------------------------------
// Report an error if a var doesn't fit in 8-16-24 bits.
//-----------------------------------------------------------------------------
void CheckVarInRange(char *name, char *str, int32_t v)
{
    int32_t val = hobatoi(str);
    if(val != v)
        oops();
    int radix = getradix(str);

    int sov = SizeOfVar(name);
    if(sov == 1) {
        if((v < -128 || v > 127) && (radix == 10))
            Warning(_("Variable %s=%d out of range: -128 to 127 inclusive."), name, v);
        else if((v < 0 || v > 0xff) && (radix != 10))
            Warning(_("Variable %s=0x%X out of range: 0 to 0xFF inclusive."), str, v, name);
    } else if((sov == 2) || (sov == 0)) {
        if((v < -32768 || v > 32767) && (radix == 10))
            Warning(_("Variable %s=%d out of range: -32768 to 32767 inclusive."), name, v);
        else if((v < 0 || v > 0xffff) && (radix != 10))
            Warning(_("Variable %s=0x%X out of range: 0 to 0xFFFF inclusive."), str, v, name);
    } else if(sov == 3) {
        if((v < -8388608 || v > 8388607) && (radix == 10))
            Warning(_("Variable %s=%d out of range: -8388608 to 8388607 inclusive."), name, v);
        else if((v < 0 || v > 0xffffff) && (radix != 10))
            Warning(_("Variable %s=0x%X out of range: 0 to 0xffFFFF inclusive."), str, v, name);
    } else if(sov == 4) {
        if((v < -2147483648LL || v > 2147483647LL) && (radix == 10))
            Warning(_("Variable %s=%d out of range: -2147483648 to 2147483647 inclusive."), name, v);
        else if((DWORD(v) < 0 || DWORD(v) > 0xffffFFFF) && (radix != 10))
            Warning(_("Variable %s=0x%X out of range: 0 to 0xffffFFFF inclusive."), str, v, name);
    } else
        ooops("Variable '%s' size=%d value=%d", name, sov, v);
}

//-----------------------------------------------------------------------------
void ShowCounterDialog(int which, ElemLeaf *l)
{
    ElemCounter *e = &(l->d.counter);
    char *       minV = e->init;
    char *       maxV = e->max;
    char *       name = e->name;
    char         inputKind[MAX_NAME_LEN];
    sprintf(inputKind, "%c", e->inputKind);

    const char *title;
    switch(which) {
        case ELEM_CTU:
            title = _("Count Up");
            break;
        case ELEM_CTD:
            title = _("Count Down");
            break;
        case ELEM_CTC:
            title = _("Circular Counter, incremental");
            break;
        case ELEM_CTR:
            title = _("Circular Counter Reversive, decremental");
            break;
        default:
            oops();
    }

    const char *labels[] = {_("Name:"),
                            //     ((which == ELEM_CTC)||(which == ELEM_CTU) ? _("Start value:") : _("Max value:")),
                            _("Start value:"),
                            (((which == ELEM_CTC)   ? _("Max value:")
                              : (which == ELEM_CTR) ? _("Min value:")
                                                    : (which == ELEM_CTU ? _("True if >= :") : _("True if > :")))),
                            _("Input kind:")};
    char *      dests[] = {name + 1, minV, maxV, inputKind};
    if(ShowSimpleDialog(title, arraylen(labels), labels, 0, 0x7, 0x7, arraylen(dests), dests)) {
        if(IsNumber(minV)) {
            int32_t _minV = hobatoi(minV);
            CheckVarInRange(name, minV, _minV);
        }
        if(IsNumber(maxV)) {
            int32_t _maxV = hobatoi(maxV);
            CheckVarInRange(name, maxV, _maxV);
        }
        if((inputKind[0] == '/') || (inputKind[0] == '\\') || (inputKind[0] == 'o') || (inputKind[0] == '-'))
            e->inputKind = inputKind[0];
        else
            Error(_("Only the characters '-o/\\' are available!"));
    }
}

//-----------------------------------------------------------------------------
#ifdef USE_SFR
// Special function
void ShowSFRDialog(int which, char *op1, char *op2)
{
    const char *title;
    const char *l2 = nullptr;
    switch(which) {
        case ELEM_RSFR:
            title = _("Read From SFR");
            l2 = "read to";
            break;

        case ELEM_WSFR:
            title = _("Write To SFR");
            l2 = "write";
            break;

        case ELEM_SSFR:
            title = _("Set Bit In SFR");
            l2 = "set bit";
            break;

        case ELEM_CSFR:
            title = _("Clear Bit In SFR");
            l2 = "clear bit";
            break;

        case ELEM_TSFR:
            title = _("Test if Bit Set In SFR");
            l2 = "test bit";
            break;

        case ELEM_T_C_SFR:
            title = _("Test if Bit Clear In SFR");
            l2 = "test bit";
            break;

        default:
            oops();
    }
    const char *labels[] = {_("SFR position:"), l2};
    char *      dests[] = {op1, op2};
    if(ShowSimpleDialog(title, arraylen(labels), labels, 0, 0x3, 0x3, arraylen(dests), dests)) {
        if(which == ELEM_RSFR) {
            if(IsNumber(op2)) {
                Error(_("Read SFR instruction: '%s' not a valid destination."), op2);
            }
        }
    }
}
// Special function
#endif

//-----------------------------------------------------------------------------
void ShowCmpDialog(int which, char *op1, char *op2)
{
    const char *title;
    const char *l2 = nullptr;
    switch(which) {
        case ELEM_EQU:
            title = _("If Equals");
            l2 = "== :";
            break;

        case ELEM_NEQ:
            title = _("If Not Equals");
            l2 = "!= :";
            break;

        case ELEM_GRT:
            title = _("If Greater Than");
            l2 = "> :";
            break;

        case ELEM_GEQ:
            title = _("If Greater Than or Equal To");
            l2 = ">= :";
            break;

        case ELEM_LES:
            title = _("If Less Than");
            l2 = "< :";
            break;

        case ELEM_LEQ:
            title = _("If Less Than or Equal To");
            l2 = "<= :";
            break;

        default:
            oops();
    }
    const char *labels[] = {_("'Closed' if:"), l2};
    char *      dests[] = {op1, op2};
    if(ShowSimpleDialog(title, arraylen(labels), labels, 0, 0x7, 0x7, arraylen(dests), dests)) {
        if(IsNumber(op1))
            CheckConstantInRange(op2, op1, hobatoi(op1));
        if(IsNumber(op2))
            CheckConstantInRange(op1, op2, hobatoi(op2));
    };
}

void ShowVarBitDialog(int which, char *dest, char *src)
{
    const char *title;
    switch(which) {
        case ELEM_IF_BIT_SET:
            title = _("If bit set");
            break;
        case ELEM_IF_BIT_CLEAR:
            title = _("If bit clear");
            break;
        case ELEM_SET_BIT:
            title = _("Set bit");
            break;
        case ELEM_CLEAR_BIT:
            title = _("Clear bit");
            break;
        default:
            oops();
    }
    char s[100];
    sprintf(s, _("Bit # [0..%d]:"), SizeOfVar(dest) * 8 - 1);
    const char *labels[] = {_("Variable:"), s};
    char *      dests[] = {dest, src};
    if(ShowSimpleDialog(title, arraylen(labels), labels, 0, 0x3, 0x3, arraylen(dests), dests)) {
    }
}

void ShowMoveDialog(int which, char *dest, char *src)
{
    const char *title;
    switch(which) {
        case ELEM_MOVE:
            title = _("Move");
            break;
        case ELEM_BIN2BCD:
            title = _("Convert BIN to packed BCD");
            break;
        case ELEM_BCD2BIN:
            title = _("Convert packed BCD to BIN");
            break;
        case ELEM_SWAP:
            title = _("Swap source and assign to destination");
            break;
        case ELEM_OPPOSITE:
            title = _("Opposite source and assign to destination");
            break;
        case ELEM_SEED_RANDOM:
            title = _("Seed Random : $seed_...");
            break;
        default:
            oops();
    }
    const char *labels[] = {_("Destination:"), _("Source:")};
    char *      dests[] = {dest, src};
    if(ShowSimpleDialog(title, arraylen(labels), labels, 0, 0x3, 0x3, arraylen(dests), dests)) {
        if(IsNumber(dest)) {
            Error(_("Move instruction: '%s' not a valid destination."), dest);
        }
        if(IsNumber(src))
            CheckConstantInRange(dest, src, hobatoi(src));
    }
}

void ShowBusDialog(ElemLeaf *l)
{
    ElemBus *   s = &(l->d.bus);
    const char *title = _("BUS tracer");

    char busStr[100];
    char PCBbitStr[100];
    char PCBbitStr2[10];
    strcpy(busStr, "|");
    strcpy(PCBbitStr, "|");

    for(int i = 7; i >= 0; i--) {
        sprintf(PCBbitStr2, "%2d|", i);
        strcat(busStr, PCBbitStr2);
        sprintf(PCBbitStr2, "%2d|", s->PCBbit[i]);
        strcat(PCBbitStr, PCBbitStr2);
    }

    const char *labels[] = {_("Destination:"), _("Destination bits:"), _("Source:"), _("Source bits:")};
    char *      dests[] = {s->dest, PCBbitStr, s->src, busStr};
    if(ShowSimpleDialog(title, arraylen(labels), labels, 0, 0x3, 0xff, arraylen(dests), dests, 0, nullptr, 0x08)) {
        if(IsNumber(s->dest)) {
            Error(_("Bus instruction: '%s' not a valid destination."), s->dest);
        }
        if(IsNumber(s->src)) {
            Error(_("Bus instruction: '%s' not a valid source."), s->src);
        }

        if(sscanf(PCBbitStr,
                  "|%d|%d|%d|%d|%d|%d|%d|%d|",
                  &l->d.bus.PCBbit[7],
                  &l->d.bus.PCBbit[6],
                  &l->d.bus.PCBbit[5],
                  &l->d.bus.PCBbit[4],
                  &l->d.bus.PCBbit[3],
                  &l->d.bus.PCBbit[2],
                  &l->d.bus.PCBbit[1],
                  &l->d.bus.PCBbit[0])
           == 8) {
        } else {
        }
    }
}

void ShowSpiDialog(ElemLeaf *l)
{
    ElemSpi *   s = &(l->d.spi);
    const char *title = _("SPI - Serial Peripheral Interface");

    const char *labels[] = {_("SPI Name:"), _("SPI Mode:"), _("Send variable:"), _("Receive to variable:"), _("Bit Rate (Hz):"), _("Data Modes (CPOL, CPHA):"), _("Data Size:"), _("Data Order:"), _("Drive _SS:")};

    if(l->d.spi.which == ELEM_SPI_WR) {
        labels[2] = _("Send string:");
        NoCheckingOnBox[2] = true;
    }

    char *dests[] = {s->name, s->mode, s->send, s->recv, s->bitrate, s->modes, s->size, s->first, s->_ss};

    comboRecord comboRec[] = {{0, {nullptr}},
                              {2, {"Master", "Slave"}},
                              {0, {nullptr}},
                              {0, {nullptr}},
                              {0, {nullptr}},
                              {4, {"0b00", "0b01", "0b10", "0b11"}},
                              {0, {nullptr}},
                              {2, {"MSB_FIRST", "LSB_FIRST"}},
                              {2, {"many: each SPI transfer", "ones: before and after full variable"}}};

    if(Prog.mcu()) {
        if(Prog.mcu()->spiCount) {
            comboRec[0].n = Prog.mcu()->spiCount;
            for(int i = 0; i < comboRec[0].n; i++) {
                comboRec[0].str[i] = Prog.mcu()->spiInfo[i].name;
            }
        }

        if(Prog.mcu()->whichIsa == ISA_ARM) {

        } else {
            // Bit Rate (Hz):
            char buf[128];
            int  m;
            if(Prog.mcu()->whichIsa == ISA_AVR) {
                m = 2;
                comboRec[4].n = 7;
            } else if(Prog.mcu()->whichIsa == ISA_PIC16) {
                m = 4;
                comboRec[4].n = 3;
            } else if(Prog.mcu()->whichIsa == ISA_PIC18) {
                m = 4;
                comboRec[4].n = 3;
            } else
                oops();
            for(int i = 0; i < comboRec[4].n; i++) {
                double f = 1.0 * Prog.mcuClock / (m * xPowerY(m, i));
                double t = 1.0 * 1000 * SizeOfVar(s->send) * 8 / f;
                //sprintf(buf,"%15.3fHz,T_ss=%.3fms", f, t);
                sprintf(buf, "%15.3f Hz, T_ss = %.3f ms", f, t);
                comboRec[4].str[i] = (const char *)CheckMalloc(strlen(buf) + 1);
                strcpy(const_cast<char *>(comboRec[4].str[i]), buf);
            }
        }
    }

    if(Prog.mcu()) {
        /*
    ///// Added by JG
    if ((Prog.mcu()->whichIsa == ISA_ARM) || (Prog.mcu()->whichIsa == ISA_AVR))
    {
        if (l->d.spi.which == ELEM_SPI)
            ShowSimpleDialog(title, arraylen(labels), labels, 0, 0x000F, -1, arraylen(dests), dests, 0, nullptr, 0x00F2);
        if (l->d.spi.which == ELEM_SPI_WR)
        {
            strcpy(dests[3], "-");
            ShowSimpleDialog(title, arraylen(labels), labels, 0, 0x000F, -1, arraylen(dests), dests, 0, nullptr, 0x00FA);
        }
    }
    else
    {
    /////
*/
        if(ShowSimpleDialog(title, arraylen(labels), labels, bit(6), 0x0000, 0xFFFFFFFF, arraylen(dests), dests, arraylen(comboRec), comboRec)) {
            //TODO: check the available range
            if(IsNumber(s->recv)) {
                Error(_("'%s' not a valid destination. There must be a general variable."), s->recv);
            }
            if((Prog.mcu()->whichIsa == ISA_AVR) || (Prog.mcu()->whichIsa == ISA_PIC16)) {
                if(hobatoi(s->size) != 8) {
                    Error(_("The valid value for the 'Data Size' is 8."));
                    strcpy(s->size, "8");
                }
            }
        }
        for(int i = 0; i < comboRec[4].n; i++) {
            CheckFree(const_cast<char *>(comboRec[4].str[i]));
        }
    }
}

void ShowI2cDialog(ElemLeaf *l)
{
    ElemI2c *   s = &(l->d.i2c);
    const char *title = _("I2C - Two Wire Interface");

    const char *labels[] = {_("I2C Name:"), _("I2C Mode:"), _("Send variable:"), _("Receive to variable:"), _("Bit Rate (Hz):"), _("I2C Address:"), _("I2C Register:"), _("Data Order:")};

    if(l->d.i2c.which == ELEM_I2C_RD) {
        NoCheckingOnBox[2] = true; ///// A VOIR
    }

    char *dests[] = {s->name, s->mode, s->send, s->recv, s->bitrate, s->address, s->registr, s->first};

    comboRecord comboRec[] = {{0, {nullptr}}, {2, {"Master", "Slave"}}, {0, {nullptr}}, {0, {nullptr}}, {0, {nullptr}}, {1, {"0"}}, {1, {"0"}}, {2, {"MSB_FIRST", "LSB_FIRST"}}};

    if(Prog.mcu()) {
        if(Prog.mcu()->i2cCount) {
            comboRec[0].n = Prog.mcu()->i2cCount;
            for(int i = 0; i < comboRec[0].n; i++) {
                comboRec[0].str[i] = Prog.mcu()->i2cInfo[i].name;
            }
        }
    }

    if(Prog.mcu()) {
        if((Prog.mcu()->whichIsa == ISA_ARM) || (Prog.mcu()->whichIsa == ISA_AVR) || (Prog.mcu()->whichIsa == ISA_PIC16) || (Prog.mcu()->whichIsa == ISA_PIC18)) {
            if(l->d.i2c.which == ELEM_I2C_RD) { // no send
                strcpy(dests[2], "-");
                ShowSimpleDialog(title, arraylen(labels), labels, 0, 0x000F, 0xFFFFFFFF, arraylen(dests), dests, 0, nullptr, 0x0096);
            }
            if(l->d.i2c.which == ELEM_I2C_WR) { // no recv
                strcpy(dests[3], "-");
                ShowSimpleDialog(title, arraylen(labels), labels, 0, 0x000F, 0xFFFFFFFF, arraylen(dests), dests, 0, nullptr, 0x009A);
            }
        } else {
            if(ShowSimpleDialog(title, arraylen(labels), labels, 0x0004, 0x0003, 0xFFFFFFFF, arraylen(dests), dests, arraylen(comboRec), comboRec)) {
                //TODO: check the available range
            }
        }
    }
}

void ShowModbusDialog(ElemLeaf *l)
{
    ElemModbus *s = &(l->d.modbus);
    const char *title = _("MODBUS - Plc Communication");
    const char *labels[] = {_("Modbus Name:"), _("Modbus UART:"), _("Modbus Speed:"), _("Modbus Timout:"), _("Mode (S/R/E/D):"), _("LookUp table:"), _("Count:")};
    char *      dests[] = {s->name, s->uart, s->speed, s->timout, s->mode, s->lut, s->count};
    comboRecord comboRec[] = {{0, {nullptr}}, {2, {"Master", "Slave"}}, {0, {nullptr}}, {0, {nullptr}}, {0, {nullptr}}, {1, {"0"}}, {1, {"0"}}};

    if(Prog.mcu()) {
        if((Prog.mcu()->whichIsa == ISA_ARM) || (Prog.mcu()->whichIsa == ISA_AVR) || (Prog.mcu()->whichIsa == ISA_PIC16) || (Prog.mcu()->whichIsa == ISA_PIC18)) {
            ShowSimpleDialog(title, 7, labels, 0x000E, 0x0000, 0xFFFFFFFF, 7, dests, 0, nullptr, 0x0000);
        }
    }
}
/////

void ShowSegmentsDialog(ElemLeaf *l)
{
    ElemSegments *s = &(l->d.segments);
    char          common[10];
    sprintf(common, "%c", s->common);
    const char *s1;
    switch(s->which) {
        case ELEM_7SEG:
            s1 = "7";
            break;
        case ELEM_9SEG:
            s1 = "9";
            break;
        case ELEM_14SEG:
            s1 = "14";
            break;
        case ELEM_16SEG:
            s1 = "16";
            break;
        default:
            oops();
    }
    const char *labels[] = {_("Destination:"), _("Source:"), _("Common:Cathode|Anode:")};
    char *      dests[] = {s->dest, s->src, common};
    char        s2[50];
    sprintf(s2, _("Convert char to %s Segments"), s1);
    if(ShowSimpleDialog(s2, arraylen(labels), labels, 0, 0x3, 0xff, arraylen(dests), dests)) {
        if(IsNumber(s->dest)) {
            Error(_("Segments instruction: '%s' not a valid destination."), s->dest);
        }
        if(IsNumber(s->src))
            CheckConstantInRange(s->dest, s->src, hobatoi(s->src));
        if((common[0] == 'A') || (common[0] == 'a'))
            s->common = 'A';
        else
            s->common = 'C';
    }
}

void ShowReadAdcDialog(char *name, int *refs)
{
    const char *labels[] = {_("Destination:"), _("REFS:")};
    char        srefs[100];
    sprintf(srefs, "%d", *refs);
    char *dests[] = {name, srefs};
    if(ShowSimpleDialog(_("Read A/D Converter"), arraylen(labels), labels, 0, 0x1, 0x1, arraylen(dests), dests)) {
        // TODO check the ranges
        *refs = hobatoi(srefs);
    }
}

void ShowGotoDialog(int which, char *name)
{
    const char *labels[] = {_("Destination rung(label):")};
    char *      dests[] = {name};

    const char *s;
    switch(which) {
        case ELEM_GOTO:
            s = _("Goto rung number or label");
            labels[0] = _("Destination rung(label):");
            break;
        case ELEM_GOSUB:
            s = _("Call SUBPROG rung number or label");
            labels[0] = _("Destination rung(label):");
            break;
        case ELEM_LABEL:
            s = _("Define LABEL name");
            labels[0] = _("LABEL name:");
            break;
        case ELEM_SUBPROG:
            s = _("Define SUBPROG name");
            labels[0] = _("SUBPROG name:");
            break;
        case ELEM_ENDSUB:
            s = _("Define ENDSUB name");
            labels[0] = _("ENDSUB name:");
            break;
        default:
            oops();
    }
    ShowSimpleDialog(s, arraylen(labels), labels, 0, 0x1, 0x1, arraylen(dests), dests);
}

void ShowRandomDialog(char *name)
{
    const char *labels[] = {_("Destination:")};
    char *      dests[] = {name};
    ShowSimpleDialog(_("Random value"), arraylen(labels), labels, 0, 0x1, 0x1, arraylen(dests), dests);
}

void ShowSetPwmDialog(void *e)
{
    ElemSetPwm *s = (ElemSetPwm *)e;
    char *      name = s->name;
    char *      duty_cycle = s->duty_cycle;
    char *      targetFreq = s->targetFreq;
    char *      resolution = s->resolution;

    const char *labels[] = {_("Name:"), _("Duty cycle:"), _("Frequency (Hz):"), _("Resolution:")};
    char *      dests[] = {name + 1, duty_cycle, targetFreq, resolution};
    comboRecord comboRec[] = {{0, {nullptr}}, {0, {nullptr}}, {0, {nullptr}}, {4, {"0-100% (6.7 bits)", "0-256  (8 bits)", "0-512  (9 bits)", "0-1024 (10 bits)"}}};

    NoCheckingOnBox[3] = true;
    if(ShowSimpleDialog(_("Set PWM Duty Cycle"), arraylen(labels), labels, 0x4, 0x3, 0x7, arraylen(dests), dests, arraylen(comboRec), comboRec)) {
        //TODO: check the available range
        double freq = hobatoi(targetFreq);
        if(freq < 0)
            Error(_("'%s' freq < 0"), targetFreq);
        if(freq > Prog.mcuClock)
            Error(_("'%s' freq > %d"), targetFreq, Prog.mcuClock);

        int resol, TOP;
        getResolution(resolution, &resol, &TOP);
        if(resol == 7)
            TOP = 99;
        TOP++;

        double duty = atof(duty_cycle);
        if(duty < 0.0)
            Error(_("'%s' duty < 0"), duty_cycle);
        if(duty > TOP)
            Error(_("'%s' duty > %d"), duty_cycle, TOP);
    }
    NoCheckingOnBox[3] = false;
}

void ShowUartDialog(int which, ElemLeaf *l)
{
    ElemUart *e = &(l->d.uart);
    char      bytes[MAX_NAME_LEN];
    sprintf(bytes, "%d", e->bytes);
    char wait[MAX_NAME_LEN];
    sprintf(wait, "%d", e->wait);

    const char *labels[] = {(which == ELEM_UART_RECV) ? _("Destination:") : _("Source:"),
                            (which == ELEM_UART_RECV) ? _("Number of bytes to receive:") : _("Number of bytes to send:"),
                            (which == ELEM_UART_RECV) ? _("Wait until all bytes are received:") : _("Wait until all bytes are sent:")};
    char *dests[] = {e->name, bytes, wait};

    NoCheckingOnBox[0] = true;
    if(ShowSimpleDialog((which == ELEM_UART_RECV) ? _("Receive from UART") : _("Send to UART"), arraylen(labels), labels, 0x0, 0x1, 0x1, arraylen(dests), dests)) {
        e->bytes = std::max(1, std::min(SizeOfVar(e->name), static_cast<int>(hobatoi(bytes))));
        e->wait = hobatoi(wait) ? 1 : 0;
    };
    NoCheckingOnBox[0] = false;
}

void ShowMathDialog(int which, char *dest, char *op1, char *op2)
{
    const char *l2, *title;
    if(which == ELEM_ADD) {
        l2 = "+ Operand2:";
        title = _("Add");
    } else if(which == ELEM_SUB) {
        l2 = "- Operand2:";
        title = _("Subtract");
    } else if(which == ELEM_MUL) {
        l2 = "* Operand2:";
        title = _("Multiply");
    } else if(which == ELEM_DIV) {
        l2 = "/ Operand2:";
        title = _("Divide");
    } else if(which == ELEM_MOD) {
        l2 = "% Operand2:";
        title = _("Divide Remainder");
    } else if(which == ELEM_SHL) {
        l2 = "<< Operand2:";
        title = "SHL";
    } else if(which == ELEM_SHR) {
        l2 = ">> Operand2:";
        title = "SHR";
    } else if(which == ELEM_SR0) {
        l2 = ">> Operand2:";
        title = "SR0";
    } else if(which == ELEM_ROL) {
        l2 = "rol Operand2:";
        title = "ROL";
    } else if(which == ELEM_ROR) {
        l2 = "ror Operand2:";
        title = "ROR";
    } else if(which == ELEM_AND) {
        l2 = "&& Operand2:";
        title = "AND";
    } else if(which == ELEM_OR) {
        l2 = "| Operand2:";
        title = "OR";
    } else if(which == ELEM_XOR) {
        l2 = "^ Operand2:";
        title = "XOR";
    } else if(which == ELEM_NOT) {
        l2 = "~ Operand1:";
        title = "NOT";
    } else if(which == ELEM_NEG) {
        l2 = "- Operand1:";
        title = "NEG";
    } else
        oops();

    NoCheckingOnBox[2] = true;
    bool b;
    if((which == ELEM_NOT) || (which == ELEM_NEG)) {
        const char *labels[] = {_("Destination:="), l2};
        char *      dests[] = {dest, op1};
        b = ShowSimpleDialog(title, arraylen(labels), labels, 0, 0x7, 0x7, arraylen(dests), dests);
    } else {
        const char *labels[] = {_("Destination:="), _("Operand1:"), l2};
        char *      dests[] = {dest, op1, op2};
        b = ShowSimpleDialog(title, arraylen(labels), labels, 0, 0x7, 0x7, arraylen(dests), dests);
    }
    NoCheckingOnBox[2] = false;
    if(b) {
        if(IsNumber(dest)) {
            Error(_("Math instruction: '%s' not a valid destination."), dest);
        }
        if(IsNumber(op1))
            CheckConstantInRange(dest, op1, hobatoi(op1));
        if(IsNumber(op2)) {
            CheckConstantInRange(dest, op2, hobatoi(op2));
            if((which == ELEM_SHL) || (which == ELEM_SHR) || (which == ELEM_SR0) || (which == ELEM_ROL) || (which == ELEM_ROR)) {
                if((hobatoi(op2) < 0) || (SizeOfVar(op1) * 8 < hobatoi(op2))) {
                    Error(_("Shift constant %s=%d out of range of the '%s' variable: 0 to %d inclusive."), op2, hobatoi(op2), op1, SizeOfVar(op1) * 8);
                }
            }
        }
    }
}

void ShowStepperDialog(void *e)
{
    ElemStepper *s = (ElemStepper *)e;
    char *       name = s->name;
    char *       P = s->P;
    char *       max = s->max;
    char *       coil = s->coil;

    const char *title;
    title = _("Stepper");
    const char *labels[] = {_("Name:"), _("Counter:"), "P:", _("Table size:"), _("Graph:"), _("Pulse to:")};
    char        sgraph[128];
    sprintf(sgraph, "%d", s->graph);
    char snSize[128];
    sprintf(snSize, "%d", s->nSize);
    char *dests[] = {name, max, P, snSize, sgraph, coil + 1};
    if(ShowSimpleDialog(title, 6, labels, 0, 0xff, 0xff, arraylen(dests), dests)) {
        s->graph = hobatoi(sgraph);
        s->nSize = hobatoi(snSize);

        char nm[MAX_NAME_LEN];
        if(IsNumber(max)) {
            sprintf(nm, "C%s%s", s->name, "Dec");
            CheckConstantInRange(nm, max, hobatoi(max));
        }
        if(IsNumber(P)) {
            sprintf(nm, "C%s%s", s->name, "P");
            CheckConstantInRange(nm, P, hobatoi(P));
        }
        if(coil[0] != 'Y')
            Error(_("Pulse to: '%s' you must set to output pin 'Y%s' or to internal relay 'R%s'."), coil, coil, coil);

        if(IsNumber(P)) {
            double Pt = 1.0 * Prog.cycleTime * hobatoi(P) / 1000000.0;
            char   Punits[3];
            double _Pt = SIprefix(Pt, Punits);

            double F = 1000000.0 / Prog.cycleTime / hobatoi(P);
            char   Funits[3];
            double _F = SIprefix(F, Funits);

            char str[1000];
            char str2[1000];
            sprintf(str, "Pmin=%.3f %ss, Fmax=%.3f %sHz", _Pt, Punits, _F, Funits);

            int count = hobatoi(max);

            ResSteps r;
            memset(&r, 0, sizeof(r));
            if(IsNumber(max) && (s->graph)) {
                CalcSteps(s, &r);

                double _Psum = SIprefix(Pt * r.Psum, Punits);
                sprintf(str2, "\n\nAcceleration/Deceleration time=%.3f %ss", _Psum, Punits);
                strcat(str, str2);

                CheckFree(r.T);
            }

            if(IsNumber(max)) {
                double Tfull;
                double _Tfull;
                char   Tunits[3];
                if(r.n) {
                    if(count > (r.n - 1) * 2)
                        Tfull = Pt * (count - (r.n - 1) * 2) + Pt * r.Psum * 2.0;
                    else
                        Tfull = Pt * r.Psum * 2.0;

                    _Tfull = SIprefix(Tfull, Tunits);
                    sprintf(str2, "\n\nWork time=%.3f %ss", _Tfull, Tunits);
                    strcat(str, str2);
                }
                _Tfull = SIprefix(Pt * count, Tunits);
                sprintf(str2, "\n\nTime without accel/decel=%.3f %ss", _Tfull, Tunits);
                strcat(str, str2);
            }

            MessageBox(MainWindow, str, _("Stepper information"), MB_OK | MB_ICONINFORMATION);
        }
    };
}

void ShowPulserDialog(ElemLeaf *l)
{
    ElemPulser *e = &(l->d.pulser);
    char *      P1 = e->P1;
    char *      P0 = e->P0;
    char *      accel = e->accel;
    char *      counter = e->counter;
    char *      coil = e->coil;

    const char *title;
    title = _("Pulser");

    const char *labels[] = {_("Pulse counter:"), "Duration of 1:", "Duration of 0:", _("Acc/Decel Factor:"), _("Pulse to:"), _("Duration is in Tcycles, Acceleration/Deceleration Factor is a multiplier of Durations")};
    char *      dests[] = {counter, P1, P0, accel, coil};
    if(ShowSimpleDialog(title, 6, labels, 0, 0xff, 0xff, 5, dests)) {
        if(IsNumber(P1))
            CheckConstantInRange("Duration of 1", P1, hobatoi(P1), 1);
        if(IsNumber(P0))
            CheckConstantInRange("Duration of 0", P0, hobatoi(P0), 1);
        if(IsNumber(accel))
            CheckConstantInRange("Acc/Decel Factor", accel, hobatoi(accel), 1);
        if(IsNumber(counter))
            CheckConstantInRange("Pulse to", counter, hobatoi(counter), 1);
        if((coil[0] != 'Y') && (coil[0] != 'R'))
            Error(_("Pulse to: '%s' you must set to internal relay 'R%s' or to output pin 'Y%s'."), coil, coil, coil);

        if(IsNumber(P1) && IsNumber(P0)) {
            double      P1t = (double)Prog.cycleTime * hobatoi(P1) / 1000.0;
            double      P0t = (double)Prog.cycleTime * hobatoi(P0) / 1000.0;
            double      P = P1t + P0t;
            const char *Punits = _("ms");

            double      F = 1000000.0 / Prog.cycleTime / (hobatoi(P1) + hobatoi(P0));
            const char *Funits;
            if(F < 1000.0)
                Funits = _("Hz");
            else {
                F = F / 1000.0;
                Funits = _("kHz");
            }
            char str[1000];
            char str2[1000];
            sprintf(str, "P1=%.3f %s, P0=%.3f %s, F=%.3f %s", P1t, Punits, P0t, Punits, F, Funits);

            int count = -1;
            if(IsNumber(counter))
                count = hobatoi(counter);
            double Ta = 0;
            int    N = 0;
            int    Na = 0;
            if(IsNumber(accel)) {
                int a = hobatoi(accel);
                int i, mina, maxa;
                if(a > 1) {
                    mina = hobatoi(P1) + hobatoi(P0);
                    maxa = mina * a;
                    if(!IsNumber(counter))
                        count = mina + maxa;

                    for(i = maxa; (i > mina) && (N < count); i--) {
                        Na += i;
                        N++;
                        if(hobatoi(P1) != hobatoi(P0))
                            i--;
                    }
                    Ta = (double)Prog.cycleTime * Na / 1000.0;
                    sprintf(str2, _("\n\nAcceleration time=%.3f %s"), Ta, Punits);
                    strcat(str, str2);
                }
            }

            if(IsNumber(counter)) {
                double Tfull;
                if(count > N)
                    Tfull = P * (count - N) + Ta;
                else
                    Tfull = Ta;

                const char *Tunits;
                if(Tfull < 1000.0)
                    Tunits = _("ms");
                else {
                    Tfull /= 1000.0;
                    Tunits = _("s");
                }
                sprintf(str2, "\n\nWork time=%.3f %s", Tfull, Tunits);
                strcat(str, str2);
            }
            MessageBox(MainWindow, str, _("Pulser information"), MB_OK | MB_ICONINFORMATION);
        }
    };
}

void ShowNPulseDialog(char *counter, char *targetFreq, char *coil)
{
    const char *labels[] = {_("Counter var:"), _("Frequency (Hz):"), _("Pulse to:")};
    char *      dests[] = {counter, targetFreq, coil};
    if(ShowSimpleDialog(_("Set N Pulse Cycle"), arraylen(labels), labels, 0x2, 0x1, 0x7, arraylen(dests), dests)) {
        //TODO: check the available range
        double freq = hobatoi(targetFreq);
        if(freq < 0)
            Error(_("'%s' freq < 0"), targetFreq);
        if(freq > 1000000.0)
            Error(_("'%s' freq > 100000"), targetFreq);
    }
}

void ShowQuadEncodDialog(ElemLeaf *l)
{
    ElemQuadEncod *q = &(l->d.QuadEncod);
    char *         counter = q->counter;
    int *          int01 = &(q->int01);
    char *         inputA = q->inputA;
    char *         inputB = q->inputB;
    char *         inputZ = q->inputZ;
    char *         dir = q->dir;
    char           inputKind[MAX_NAME_LEN];
    sprintf(inputKind, "%c", q->inputZKind);

    char countPerRevol[MAX_NAME_LEN];
    sprintf(countPerRevol, "%d", q->countPerRevol);

    char title[100];
    sprintf(title, "%s", _("Quad Encoder") /*, *int01*/);

    char _int01[100];
    sprintf(_int01, "%d", *int01);

    const char *labels[] = {_("Counter var:"),
                            //_("Input A INTs:"),
                            _("Input A:"),
                            _("Input B:"),
                            _("Input Z:"),
                            _("Input Z kind:"),
                            _("Count per revol:"),
                            _("Output Dir:")};
    char *      dests[] = {counter, /*_int01, */ &inputA[1], &inputB[1], &inputZ[1], inputKind, countPerRevol, dir};
    NoCheckingOnBox[3] = true;
    NoCheckingOnBox[6] = true;
    if(strlen(inputZ) <= 1)
        inputZ[1] = '\0';
    if(strlen(dir) <= 1)
        dir[1] = '\0';
    if(ShowSimpleDialog(title, 7, labels, 0x20, 0xef, 0xff, arraylen(dests), dests)) {
        inputA[0] = 'X';
        inputB[0] = 'X';
        if(strlen(&inputZ[1]))
            inputZ[0] = 'X';
        else
            inputZ[0] = '\0';
        if(strlen(dir)) {
            if((dir[0] != 'Y') && (dir[0] != 'R')) {
                Warning(_("Only 'Y-Outpur Pin' or 'R-Inrenal Relay' are allowed for Output Dir parameter '%s'"), dir);
                char s[MAX_NAME_LEN] = "Y";
                strcat(s, dir);
                strcpy(dir, s);
            }
        }
        //TODO: check the available range
        int32_t val;
        /*
        val = hobatoi(_int01);
        if(Prog.mcu())
            if((val < 0) || (static_cast<SDWORD>(Prog.mcu()->ExtIntCount) <= val))
                Error(_("Can select only INTs pin."));
        */
        val = hobatoi(countPerRevol);
        q->countPerRevol = val;

        if((inputKind[0] == '/') || (inputKind[0] == '\\') || (inputKind[0] == 'o') || (inputKind[0] == '-'))
            q->inputZKind = inputKind[0];
        else
            Error(_("Only the characters '-o/\\' are available!"));
    }
    NoCheckingOnBox[3] = false;
    NoCheckingOnBox[6] = false;
}

void ShowSizeOfVarDialog(PlcProgramSingleIo *io)
{
    int  sov = SizeOfVar(io->name);
    char sovStr[20];
    sprintf(sovStr, "%d", sov);

    int32_t val;
    char    valStr[MAX_STRING_LEN];
    if(io->type == IO_TYPE_STRING) {
        strcpy(valStr, GetSimulationStr(io->name));
    } else {
        val = GetSimulationVariable(io->name, true);
        sprintf(valStr, "%d", val);
    }

    char s[MAX_NAME_LEN];
    sprintf(s, _("Set variable '%s'"), io->name);

    const char *labels[2];
    char *      dests[2];

    if(InSimulationMode) {
        labels[0] = _("Simulation value:");
        labels[1] = _("SizeOfVar:");
        dests[0] = valStr;
        dests[1] = sovStr;
    } else {
        labels[0] = _("SizeOfVar:");
        labels[1] = _("Simulation value:");
        dests[0] = sovStr;
        dests[1] = valStr;
    }
    if(ShowSimpleDialog(s, arraylen(labels), labels, 0x3, 0x0, 0x3, arraylen(dests), dests)) {
        sov = hobatoi(sovStr);
        if((sov <= 0) || ((io->type != IO_TYPE_STRING) && (sov > 4) && (io->type != IO_TYPE_BCD)) || ((io->type == IO_TYPE_BCD) && (sov > 10))) {
            Error(_("Not a reasonable size for a variable."));
        } else {
            SetSizeOfVar(io->name, sov);
        }

        if(io->type == IO_TYPE_STRING) {
            SetSimulationStr(io->name, valStr);
        } else {
            val = hobatoi(valStr);
            SetSimulationVariable(io->name, val);
        }
    }
}

void ShowShiftRegisterDialog(char *name, int *stages)
{
    char stagesStr[20];
    sprintf(stagesStr, "%d", *stages);

    const char *labels[] = {_("Name:"), _("Stages:")};
    char *      dests[] = {name, stagesStr};
    ShowSimpleDialog(_("Shift Register"), arraylen(labels), labels, 0x2, 0x1, 0x3, arraylen(dests), dests);

    *stages = hobatoi(stagesStr);

    if(*stages <= 0 || *stages >= MAX_SHIFT_REGISTER_STAGES - 1) {
        Error(_("Not a reasonable size for a shift register."));
        *stages = 8;
    }
}

void ShowWrDialog(int which, ElemLeaf *l)
{
    ElemFormattedString *e = &(l->d.fmtdStr);
    const char *         labels[] = {IsString(e->string) ? _("String:") : _("String variable:"), _("Wait until all bytes are sent:")};
    char                 wait[MAX_NAME_LEN];
    sprintf(wait, "%d", e->wait);
    char *dests[] = {e->string, wait};
    //NoCheckingOnBox[0] = true;
    if(ShowSimpleDialog(_("String Variable Over UART"), arraylen(labels), labels, 0x2, 0x1, 0x3, arraylen(dests), dests)) {
        e->wait = hobatoi(wait) ? 1 : 0;
    }
    //NoCheckingOnBox[0] = false;
};

void ShowFormattedStringDialog(char *var, char *string)
{
    const char *labels[] = {_("Variable:"), _("String:")};
    char *      dests[] = {var, string};
    NoCheckingOnBox[0] = true;
    NoCheckingOnBox[1] = true;
    ShowSimpleDialog(_("Formatted String Over UART"), arraylen(labels), labels, 0x0, 0x1, 0x3, arraylen(dests), dests);
    NoCheckingOnBox[0] = false;
    NoCheckingOnBox[1] = false;
}

void ShowStringDialog(char *dest, char *var, char *string)
{
    const char *labels[] = {_("Dest:"), _("Format string:"), _("Variable:")};
    char *      dests[] = {dest, string, var};
    NoCheckingOnBox[0] = true;
    NoCheckingOnBox[1] = true;
    NoCheckingOnBox[2] = true;
    ShowSimpleDialog(_("Formatted String: sprintf(dest, frmtStr, var)"), arraylen(labels), labels, 0x0, 0x0, 0x7, arraylen(dests), dests);
    NoCheckingOnBox[0] = false;
    NoCheckingOnBox[1] = false;
    NoCheckingOnBox[2] = false;
}

void ShowFrmtStToCharDialog(ElemLeaf *l)
{
    ElemFormattedString *e = &(l->d.fmtdStr);
    const char *         labels[] = {_("Dest Char:"), IsString(e->string) ? _("String:") : _("String variable:"), _("Variable:")};
    char *               dests[] = {e->dest, e->string, e->var};
    NoCheckingOnBox[0] = true;
    NoCheckingOnBox[1] = true;
    NoCheckingOnBox[2] = true;
    if(ShowSimpleDialog(_("Formatted String To Char"), arraylen(labels), labels, 0x0, 0x0, 0x7, arraylen(dests), dests)) {
        //
    }
    NoCheckingOnBox[0] = false;
    NoCheckingOnBox[1] = false;
    NoCheckingOnBox[2] = false;
};

void ShowVarToCharDialog(ElemLeaf *l)
{
    ElemFormattedString *e = &(l->d.fmtdStr);
    const char *         labels[] = {_("Dest Char:"), _("Variable:")};
    char *               dests[] = {e->dest, e->var};
    //NoCheckingOnBox[0] = true;
    //NoCheckingOnBox[1] = true;
    if(ShowSimpleDialog(_("Variable To Char"), arraylen(labels), labels, 0x0, 0x3, 0x3, arraylen(dests), dests)) {
        //
    }
    //NoCheckingOnBox[0] = false;
    //NoCheckingOnBox[1] = false;
};

void ShowPersistDialog(char *var)
{
    const char *labels[] = {_("Variable:")};
    char *      dests[] = {var};
    ShowSimpleDialog(_("Make Persistent"), arraylen(labels), labels, 0, 1, 1, arraylen(dests), dests);
}

void ShowPullUpDialog()
{
    char *   labels[MAX_IO_PORTS + 2];
    char *   dests[MAX_IO_PORTS];
    int      n = 0;
    uint32_t mask = 0xFF;
    if(Prog.mcu()->whichIsa == ISA_ARM)
        mask = 0xFFFF;
    for(int i = 0; i < MAX_IO_PORTS; i++) {
        if(IS_MCU_REG(i)) {
            labels[n] = (char *)CheckMalloc(20);
            sprintf(labels[n], "Port %c%c:", Prog.mcu()->portPrefix, 'A' + i);
            dests[n] = (char *)CheckMalloc(20);
            sprintf(dests[n], "0x%X", Prog.pullUpRegs[i] & mask);
            n++;
        }
    }
    labels[n] = (char *)_("*Attention: Not all ports have a pull-up resistor. See datasheets of the controller for details.");
    labels[n + 1] = (char *)_("*PIC only: if PORT RB value is not 0, Weak Pull-ups will be enabled for all port B pins via RBPU."); /// To translate (modified)

    if(ShowSimpleDialog(_("Set Pull-up input resistors"), n + 2, (const char **)labels, 0xFFFF, 0, 0xFFFF, n, dests)) {
        int port = 0;
        for(int i = 0; i < MAX_IO_PORTS; i++) {
            if(IS_MCU_REG(i)) {
                Prog.pullUpRegs[i] = hobatoi(dests[port]);
                port++;
            }
        }
    }

    for(int i = 0; i < n; i++) {
        CheckFree(labels[i]);
        CheckFree(dests[i]);
    }
}
