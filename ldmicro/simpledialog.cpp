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
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>

#include "ldmicro.h"

static HWND SimpleDialog;

#define MAX_BOXES 6

static HWND Textboxes[MAX_BOXES];
static HWND Labels[MAX_BOXES];

static LONG_PTR PrevAlnumOnlyProc[MAX_BOXES];
static LONG_PTR PrevNumOnlyProc[MAX_BOXES];

static BOOL NoCheckingOnBox[MAX_BOXES];

//-----------------------------------------------------------------------------
// Don't allow any characters other than -A-Za-z0-9_ in the box.
//-----------------------------------------------------------------------------
static LRESULT CALLBACK MyAlnumOnlyProc(HWND hwnd, UINT msg, WPARAM wParam,
    LPARAM lParam)
{
    if(msg == WM_CHAR) {
        if(!(isalpha(wParam) || isdigit(wParam) || wParam == '_' ||
            wParam == '@' ||
            wParam == '\b' || wParam == '-' || wParam == '\''))
        {
            return 0;
        }
    }

    int i;
    for(i = 0; i < MAX_BOXES; i++) {
        if(hwnd == Textboxes[i]) {
            return CallWindowProc((WNDPROC)PrevAlnumOnlyProc[i], hwnd, msg,
                wParam, lParam);
        }
    }
    oops();
    return 0;
}

//-----------------------------------------------------------------------------
// Don't allow any characters other than -0-9. in the box.
//-----------------------------------------------------------------------------
static LRESULT CALLBACK MyNumOnlyProc(HWND hwnd, UINT msg, WPARAM wParam,
    LPARAM lParam)
{
    if(msg == WM_CHAR) {
        if(!(ishobdigit(wParam) || wParam == '.' || wParam == '\b'
            || wParam == '\''
            || wParam == '\\'
            || wParam == '-'))
        {
            return 0;
        }
    }

    int i;
    for(i = 0; i < MAX_BOXES; i++) {
        if(hwnd == Textboxes[i]) {
            return CallWindowProc((WNDPROC)PrevNumOnlyProc[i], hwnd, msg,
                wParam, lParam);
        }
    }
    oops();
    return 0;
}

static void MakeControls(int boxes, char **labels, DWORD fixedFontMask)
{
    int i;
    HDC hdc = GetDC(SimpleDialog);
    SelectObject(hdc, MyNiceFont);

    SIZE si;

    int maxLen = 0;
    for(i = 0; i < boxes; i++) {
        GetTextExtentPoint32(hdc, labels[i], strlen(labels[i]), &si);
        if(si.cx > maxLen) maxLen = si.cx;
    }

    int adj;
    if(maxLen > 70) {
        adj = maxLen - 70;
    } else {
        adj = 0;
    }

    for(i = 0; i < boxes; i++) {
        GetTextExtentPoint32(hdc, labels[i], strlen(labels[i]), &si);

        Labels[i] = CreateWindowEx(0, WC_STATIC, labels[i],
            WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
            (80 + adj) - si.cx + 15, 13 + i*30, si.cx, 21,
            SimpleDialog, NULL, Instance, NULL);
        NiceFont(Labels[i]);

        Textboxes[i] = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "",
            WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS |
            WS_VISIBLE,
            80 + 25 + adj, 12 + 30*i, 120 + 535 - adj, 21,
            SimpleDialog, NULL, Instance, NULL);

        if(fixedFontMask & (1 << i)) {
            FixedFont(Textboxes[i]);
        } else {
            NiceFont(Textboxes[i]);
        }
    }
    ReleaseDC(SimpleDialog, hdc);

    OkButton = CreateWindowEx(0, WC_BUTTON, _("OK"),
        WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE | BS_DEFPUSHBUTTON,
        218 + 550, 11, 70, 23, SimpleDialog, NULL, Instance, NULL);
    NiceFont(OkButton);

    CancelButton = CreateWindowEx(0, WC_BUTTON, _("Cancel"),
        WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
        218 + 550, 41, 70, 23, SimpleDialog, NULL, Instance, NULL);
    NiceFont(CancelButton);
}

BOOL ShowSimpleDialog(char *title, int boxes, char **labels, DWORD numOnlyMask,
    DWORD alnumOnlyMask, DWORD fixedFontMask, char **dests)
{
    BOOL didCancel;

    if(boxes > MAX_BOXES) oops();

    SimpleDialog = CreateWindowClient(0, "LDmicroDialog", title,
        WS_OVERLAPPED | WS_SYSMENU,
        100, 100, 304 + 550, 15 + 30*(boxes < 2 ? 2 : boxes), NULL, NULL,
        Instance, NULL);

    MakeControls(boxes, labels, fixedFontMask);

    int i;
    for(i = 0; i < boxes; i++) {
        SendMessage(Textboxes[i], WM_SETTEXT, 0, (LPARAM)dests[i]);

        if(numOnlyMask & (1 << i)) {
            PrevNumOnlyProc[i] = SetWindowLongPtr(Textboxes[i], GWLP_WNDPROC,
                (LONG_PTR)MyNumOnlyProc);
        } else // numOnlyMask overpower alnumOnlyMask
        if(alnumOnlyMask & (1 << i)) {
            PrevAlnumOnlyProc[i] = SetWindowLongPtr(Textboxes[i], GWLP_WNDPROC,
                (LONG_PTR)MyAlnumOnlyProc);
        }
    }

    EnableWindow(MainWindow, FALSE);
    ShowWindow(SimpleDialog, TRUE);
    SetFocus(Textboxes[0]);
    SendMessage(Textboxes[0], EM_SETSEL, 0, -1);

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

        if(IsDialogMessage(SimpleDialog, &msg)) continue;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    didCancel = DialogCancel;

    if(!didCancel) {
        for(i = 0; i < boxes; i++) {
            if(NoCheckingOnBox[i]) {
                char get[MAX_NAME_LEN];
                SendMessage(Textboxes[i], WM_GETTEXT, (MAX_NAME_LEN-1), (LPARAM)get);
                strcpy(dests[i], get);
            } else {
                char get[MAX_NAME_LEN];
                SendMessage(Textboxes[i], WM_GETTEXT, (MAX_NAME_LEN-1), (LPARAM)get);

                if( (!strchr(get, '\'')) ||
                    (get[0] == '\'' && get[3] == '\'' && strlen(get)==4 && get[1] == '\\' ) ||
                        (get[0] == '\'' && get[2] == '\'' && strlen(get)==3) )
                {
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

    EnableWindow(MainWindow, TRUE);
    DestroyWindow(SimpleDialog);

    return !didCancel;
}

void ShowTimerDialog(int which, SDWORD *delay, char *name)
{
    char *s;
    switch(which) {
        case ELEM_TCY: s = _("Cyclic On/Off"); break;
        case ELEM_TON: s = _("Turn-On Delay"); break;
        case ELEM_TOF: s = _("Turn-Off Delay"); break;
        case ELEM_RTO: s = _("Retentive Turn-On Delay"); break;
        default: oops(); break;
    }

    char *labels[] = { _("Name:"), _("Delay (ms):") };

    char delBuf[16];
    char nameBuf[MAX_NAME_LEN];
    sprintf(delBuf, "%.3f", (*delay / 1000.0));
    strcpy(nameBuf, name+1);
    char *dests[] = { nameBuf, delBuf };

    if(ShowSimpleDialog(s, 2, labels, (1 << 1), (1 << 0), (1 << 0), dests)) {
        name[0] = 'T';
        strcpy(name+1, nameBuf);
        double del = atof(delBuf);
        long long period = (long long)round(del * 1000 / Prog.cycleTime);// - 1;
        if(del <= 0) {
            Error(_("Delay cannot be zero or negative."));
        } else if(period  < 1)  {
            char *s1 = _("Timer period too short (needs faster cycle time).");
            char s2[1024];
            sprintf(s2, _("Timer '%s'=%.3f ms."), name, del);
            char s3[1024];
            sprintf(s3, _("Minimum available timer period = PLC cycle time = %.3f ms."), 1.0*Prog.cycleTime/1000);
            char *s4 = _("Not available");
            Error("%s\n\r%s %s\r\n%s", s1, s4, s2, s3);
        } else if((period >= long long (1 << (SizeOfVar(name)*8-1)))
                   && (Prog.mcu->portPrefix != 'L')) {
            char *s1 = _("Timer period too long (max 32767 times cycle time); use a "
                "slower cycle time.");
            char s2[1024];
            sprintf(s2, _("Timer 'T%s'=%.3f ms needs %d PLC cycle times."), nameBuf, del/1000, period);
            double maxDelay = 1.0 * ((1 << (SizeOfVar(name)*8-1))-1) * Prog.cycleTime / 1000000; //s
            char s3[1024];
            sprintf(s3, _("Maximum available timer period = %.3f s."), maxDelay);
            Error("%s\r\n%s\r\n%s", s1, s2, s3);
            *delay = (SDWORD)(1000*del + 0.5);
        /*
        if(del > 2140000) { // 2**31/1000, don't overflow signed int
            Error(_("Delay too long; maximum is 2**31 us."));
        } else if(del <= 0) {
            Error(_("Delay cannot be zero or negative."));
        */
        } else {
            *delay = (SDWORD)(1000*del + 0.5);
        }
    }
}

//-----------------------------------------------------------------------------
// Report an error if a constant doesn't fit in 8-16-24 bits.
//-----------------------------------------------------------------------------
static void CheckConstantInRange(char *name, char *str, SDWORD v)
{
    SDWORD val = hobatoi(str);
    if(val != v) oops();
    int radix = getradix(str);

    int sov;
    if(strlen(name)==0)
        sov = 3;
    else if(IsNumber(name)) {
        sov = byteNeeded(hobatoi(name));
        name = "$_tmp_Var";
    } else
        sov = SizeOfVar(name);
    if (sov == 1) {
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
    } else ooops("Constant %s Variable '%s' size=%d value=%d", str, name, sov, v);
}

//-----------------------------------------------------------------------------
// Report an error if a var doesn't fit in 8-16-24 bits.
//-----------------------------------------------------------------------------
void CheckVarInRange(char *name, char *str, SDWORD v)
{
    SDWORD val = hobatoi(str);
    if(val != v) oops();
    int radix = getradix(str);

    int sov = SizeOfVar(name);
    if (sov == 1) {
        if((v < -128 || v > 127) && (radix == 10))
            Error(_("Variable %s=%d out of range: -128 to 127 inclusive."), name, v);
        else if((v < 0 || v > 0xff) && (radix != 10))
            Error(_("Variable %s=0x%X out range : 0 to 0xFF inclusive."), str, v, name);
    } else if((sov == 2) || (sov == 0)){
        if((v < -32768 || v > 32767) && (radix == 10))
            Error(_("Variable %s=%d out of range: -32768 to 32767 inclusive."), name, v);
        else if((v < 0 || v > 0xffff) && (radix != 10))
            Error(_("Variable %s=0x%X out range : 0 to 0xFFFF inclusive."), str, v, name);
    } else if(sov == 3) {
        if((v < -8388608 || v > 8388607) && (radix == 10))
            Error(_("Variable %s=%d out of range: -8388608 to 8388607 inclusive."), name, v);
        else if((v < 0 || v > 0xffffff) && (radix != 10))
            Error(_("Variable %s=0x%X out range : 0 to 0xffFFFF inclusive."), str, v, name);
    } else if(sov == 4) {
        if((v < -2147483648LL || v > 2147483647LL) && (radix == 10))
            Error(_("Variable %s=%d out of range: -2147483648 to 2147483647 inclusive."), name, v);
        else if((v < 0 || v > 0xffffFFFF) && (radix != 10))
            Error(_("Variable %s=0x%X out range : 0 to 0xffffFFFF inclusive."), str, v, name);
    } else ooops("Variable '%s' size=%d value=%d", name, sov, v);
}

//-----------------------------------------------------------------------------
void ShowCounterDialog(int which, char *minV, char *maxV, char *name)
{
    char *title;
    switch(which) {
        case ELEM_CTU:  title = _("Count Up"); break;
        case ELEM_CTD:  title = _("Count Down"); break;
        case ELEM_CTC:  title = _("Circular Counter"); break;
        case ELEM_CTR:  title = _("Circular Counter Reversive"); break;
        default: oops();
    }

    char *labels[] = { _("Name:"),
//     ((which == ELEM_CTC)||(which == ELEM_CTU) ? _("Start value:") : _("Max value:")),
       _("Start value:"),
       (((which == ELEM_CTC) ? _("Max value:") :
        (which == ELEM_CTR) ? _("Min value:") :
        (which == ELEM_CTU ? _("True if >= :") : _("True if > :")))) };
    char *dests[] = { name+1, minV, maxV };
    if(ShowSimpleDialog(title, 3, labels, 0, 0x7, 0x7, dests)) {
      if(IsNumber(minV)){
        SDWORD _minV = hobatoi(minV);
        CheckVarInRange(name, minV, _minV);
      }
      if(IsNumber(maxV)){
        SDWORD _maxV = hobatoi(maxV);
        CheckVarInRange(name, maxV, _maxV);
      }
    }
}
// Special function
void ShowSFRDialog(int which, char *op1, char *op2)
{
    char *title;
    char *l2;
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
    char *labels[] = { _("SFR position:"), l2 };
    char *dests[] = { op1, op2 };
    if(ShowSimpleDialog(title, 2, labels, 0, 0x3, 0x3, dests)) {
        if(which==ELEM_RSFR) {
            if(IsNumber(op2)) {
                Error(_("Read SFR instruction: '%s' not a valid destination."),
                    op2);
            }
        }
    }
}

// Special function

void ShowCmpDialog(int which, char *op1, char *op2)
{
    char *title;
    char *l2;
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
    char *labels[] = { _("'Closed' if:"), l2};
    char *dests[] = { op1, op2};
    if(ShowSimpleDialog(title, 2, labels, 0, 0x7, 0x7, dests)){
        if(IsNumber(op1))
            CheckConstantInRange(op2, op1, hobatoi(op1));
        if(IsNumber(op2))
            CheckConstantInRange(op1, op2, hobatoi(op2));
    };
}

void ShowVarBitDialog(int which, char *dest, char *src)
{
    char *title;
    switch(which) {
        case ELEM_IF_BIT_SET   : title = _("If bit set"); break;
        case ELEM_IF_BIT_CLEAR : title = _("If bit clear"); break;
        case ELEM_SET_BIT      : title = _("Set bit"); break;
        case ELEM_CLEAR_BIT    : title = _("Clear bit"); break;
        default: oops();
    }
    char *labels[] = { _("Variable:"), _("Bit # [0..15]:") };
    char *dests[] = { dest, src };
    if(ShowSimpleDialog(title, 2, labels, 0, 0x3, 0x3, dests)){
    }
}

void ShowMoveDialog(int which, char *dest, char *src)
{
    char *title;
    switch(which) {
        case ELEM_MOVE    : title = _("Move"); break;
        case ELEM_BIN2BCD : title = _("Convert BIN to packed BCD"); break;
        case ELEM_BCD2BIN : title = _("Convert packed BCD to BIN"); break;
        case ELEM_SWAP    : title = _("Swap source and assign to destination"); break;
        default: oops();
    }
    char *labels[] = { _("Destination:="), _("Source:") };
    char *dests[] = { dest, src };
    if(ShowSimpleDialog(title, 2, labels, 0, 0x3, 0x3, dests)){
        if(IsNumber(dest)) {
            Error(_("Move instruction: '%s' not a valid destination."),
                dest);
        }
        if(IsNumber(src))
            CheckConstantInRange(dest, src, hobatoi(src));
    }
}

void ShowBusDialog(ElemLeaf *l)
{
    ElemBus *s = &(l->d.bus);
    char *title = _("BUS tracer");

    char busStr[100];
    char PCBbitStr[100];
    char PCBbitStr2[10];
    strcpy(busStr, "|");
    strcpy(PCBbitStr, "|");
    int i;

        for(i=7; i>=0; i--) {
            sprintf(PCBbitStr2, "%2d|", s->PCBbit[i]);
            strcat(PCBbitStr, PCBbitStr2);
            strcat(busStr, PCBbitStr2);
        }

    char *labels[] = { _("Destination:"), _("Source:"), _("Destination bits:"), _("Source bits:") };
    char *dests[] = { s->dest, busStr, s->src, PCBbitStr};
    if(ShowSimpleDialog(title, 4, labels, 0, 0x3, 0xff, dests)){
        if(IsNumber(s->dest)) {
            Error(_("Bus instruction: '%s' not a valid destination."),
                s->dest);
        }
        if(IsNumber(s->src)) {
            Error(_("Bus instruction: '%s' not a valid source."),
                s->src);
        }

        if(sscanf(PCBbitStr,"|%d|%d|%d|%d|%d|%d|%d|%d|",
             &l->d.bus.PCBbit[7],
             &l->d.bus.PCBbit[6],
             &l->d.bus.PCBbit[5],
             &l->d.bus.PCBbit[4],
             &l->d.bus.PCBbit[3],
             &l->d.bus.PCBbit[2],
             &l->d.bus.PCBbit[1],
             &l->d.bus.PCBbit[0])==8) {
        } else {
        }
    }
}

void ShowSegmentsDialog(ElemLeaf *l)
{
    ElemSegments *s = &(l->d.segments);
    char common[10];
    sprintf(common, "%c", s->common);
    char *s1;
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
        default: oops();
    }
    char *labels[] = { _("Destination:"), _("Source:"), _("Common:Cathode|Anode:")};
    char *dests[] = { s->dest, s->src, common};
    char s2[50];
    sprintf(s2,_("Convert char to %s Segments"), s1);
    if(ShowSimpleDialog(s2, 3, labels, 0, 0x3, 0xff, dests)){
        if(IsNumber(s->dest)) {
            Error(_("Segments instruction: '%s' not a valid destination."),
                s->dest);
        }
        if(IsNumber(s->src))
            CheckConstantInRange(s->dest, s->src, hobatoi(s->src));
        if((common[0]=='A') || (common[0]=='a'))
            s->common = 'A';
        else
            s->common = 'C';
    }
}

void ShowReadAdcDialog(char *name)
{
    char *labels[] = { _("Destination:") };
    char *dests[] = { name };
    ShowSimpleDialog(_("Read A/D Converter"), 1, labels, 0, 0x1, 0x1, dests);
}

void ShowSetPwmDialog(void *e)
{
    ElemSetPwm *s = (ElemSetPwm *)e;
    char *name          = s->name;
    char *duty_cycle    = s->duty_cycle;
    char *targetFreq    = s->targetFreq;
    char *labels[] = { _("Name:"), _("Duty cycle:"), _("Frequency (Hz):")};
    char *dests[] = { name+1, duty_cycle, targetFreq};

    if(ShowSimpleDialog(_("Set PWM Duty Cycle"), 3, labels, 0x4, 0x3, 0x7, dests)) {
        //TODO: check the available range
        double freq = hobatoi(targetFreq);
        if(freq < 0)
            Error(_("'%s' freq < 0"), targetFreq);
        if(freq > Prog.mcuClock)
            Error(_("'%s' freq > %d"), targetFreq, Prog.mcuClock);

        double duty = atof(duty_cycle);
        if(duty < 0.0)
            Error(_("'%s' duty < 0"), duty_cycle);
        if(duty > 100.0)
            Error(_("'%s' duty > 100"), duty_cycle, Prog.mcuClock);
    }
}

void ShowUartDialog(int which, char *name)
{
    char *labels[] = { (which == ELEM_UART_RECV) ? _("Destination:") :
        _("Source:") };
    char *dests[] = { name };

    NoCheckingOnBox[0] = TRUE;
    ShowSimpleDialog((which == ELEM_UART_RECV) ? _("Receive from UART") :
        _("Send to UART"), 1, labels, 0x0, 0x1, 0x1, dests);
    NoCheckingOnBox[0] = FALSE;
}

void ShowMathDialog(int which, char *dest, char *op1, char *op2)
{
    char *l2, *title;
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
        title = _("SHL");
    } else if(which == ELEM_SHR) {
        l2 = ">> Operand2:";
        title = _("SHR");
    } else if(which == ELEM_SR0) {
        l2 = ">> Operand2:";
        title = _("SR0");
    } else if(which == ELEM_ROL) {
        l2 = "rol Operand2:";
        title = _("ROL");
    } else if(which == ELEM_ROR) {
        l2 = "ror Operand2:";
        title = _("ROR");
    } else if(which == ELEM_AND) {
        l2 = "&& Operand2:";
        title = _("AND");
    } else if(which == ELEM_OR) {
        l2 = "| Operand2:";
        title = _("OR");
    } else if(which == ELEM_XOR) {
        l2 = "^ Operand2:";
        title = _("XOR");
    } else if(which == ELEM_NOT) {
        l2 = "~ Operand1:";
        title = _("NOT");
    } else if(which == ELEM_NEG) {
        l2 = "- Operand1:";
        title = _("NEG");
    } else oops();

    NoCheckingOnBox[2] = TRUE;
    BOOL b;
    if((which == ELEM_NOT)
    || (which == ELEM_NEG)) {
        char *labels[] = { _("Destination:="), l2 };
        char *dests[] = { dest, op1};
        b=ShowSimpleDialog(title, 2, labels, 0, 0x7, 0x7, dests);
    } else {
        char *labels[] = { _("Destination:="), _("Operand1:"), l2 };
        char *dests[] = { dest, op1, op2 };
        b=ShowSimpleDialog(title, 3, labels, 0, 0x7, 0x7, dests);
    }
    NoCheckingOnBox[2] = FALSE;
    if(b){
        if(IsNumber(dest)) {
            Error(_("Math instruction: '%s' not a valid destination."), dest);
        }
        if(IsNumber(op1))
            CheckConstantInRange(dest, op1, hobatoi(op1));
        if(IsNumber(op2)){
            CheckConstantInRange(dest, op2, hobatoi(op2));
            if((which == ELEM_SHL) || (which == ELEM_SHR) || (which == ELEM_SR0)
            || (which == ELEM_ROL) || (which == ELEM_ROR)) {
                if((hobatoi(op2)<0) || (BITS_OF_LD_VAR<hobatoi(op2))){
                    Error(_("Shift constant %s=%d out of range: 0 to %d inclusive."), op2, hobatoi(op2), BITS_OF_LD_VAR);
                }
            }
        }
    }
}

void CalcSteps(ElemStepper *s, ResSteps *r)
{
    memset(&(*r),0,sizeof(ResSteps));

    FILE *f;

    double massa=1;
    double k;
    int nSize = s->nSize;
    int graph = s->graph;

    int P = 0;
    if(IsNumber(s->P)){
        P = hobatoi(s->P);
    };

    int max = 0;
    if(IsNumber(s->max)){
        max = hobatoi(s->max);
    };

    f = fopen("acceleration_deceleration.txt", "w");
/*
    if(graph==1){
      k = kProp(nSize);
      makeAccelTable(f, max, P, nSize, &r->T,
      "v=k*t  a=const  s=k*t^2/2  e=m*v^2/2",
      1, 0, &r->n, &r->Psum, &r->shrt, &r->sovElement,
      &Proportional,1, &tsProp,k, &vProp,k, &aProp,k, &eFv, massa);
    }else if(graph==2){
      k = kSqrt2(nSize);
      makeAccelTable(f, max, P, nSize, &r->T,
      "v=k*sqrt(t)  a=k/(2*t^(1/2))  s=k*t^(3/2)/(3/2)  e=m*v^2/2",
      1, 0, &r->n, &r->Psum, &r->shrt, &r->sovElement,
      &Proportional,1, &tsSqrt2,k, &vSqrt2,k, &aSqrt2,k, &eFv, massa);
    }else if(graph==3){
      k = kSqrt3(nSize);
      makeAccelTable(f, max, P, nSize, &r->T,
      "v=k*t^(1/3)  a=k/(3*t^(2/3))  s=k*t^(4/3)/(4/3)  e=m*v^2/2",
      1, 0, &r->n, &r->Psum, &r->shrt, &r->sovElement,
      &Proportional,1, &tsSqrt3,k, &vSqrt3,k, &aSqrt3,k, &eFv, massa);
    }else if(graph==4){
      k = k2(nSize);
      makeAccelTable(f, max, P, nSize, &r->T,
      "v=k*t^2  a=k*2*t  s=k*t^3/3  e=m*v^2/2",
      1, 0, &r->n, &r->Psum, &r->shrt, &r->sovElement,
      &Proportional,1, &ts2,k, &v2,k, &a2,k, &eFv, massa);
    }else if(graph==5){
      k = ksS(nSize);
      makeAccelTable(f, max, P, nSize, &r->T,
      "v=k*t^2  a=k*2*t  s=k*t^3/3  s=i  e=m*v^2/2",
      2, 0, &r->n, &r->Psum, &r->shrt, &r->sovElement,
      &Proportional,1.0, &tsS,k, &vS,k, &aS,k, &eFv, massa);
    }else if(graph==6){
      k = ktS(nSize);
      makeAccelTable(f, max, P, nSize, &r->T,
      "v=k*t^2  a=k*2*t  s=k*t^3/3  t=i  e=m*v^2/2",
      2, 1, &r->n, &r->Psum, &r->shrt, &r->sovElement,
      &stS,k, &Proportional,1, &vS,k, &aS,k, &eFv, massa);
    } else {
      fprintf(f,"Generates %d steps without acceleration/deceleration.", s->max);
    }
*/
    fclose(f);

    s->n = r->n;
}

void ShowStepperDialog(int which, void *e)
{
    ElemStepper *s = (ElemStepper *)e;
    char *name = s->name;
    char *P    = s->P   ;
    char *max  = s->max ;
    char *coil = s->coil;

    char *title;
    title = _("Stepper");
    char *labels[] = { _("Name:"),  _("Counter:"), _("P:"), _("Table size:"),  _("graph:"), _("Pulse to:")};
    char sgraph[128];
    sprintf(sgraph, "%d", s->graph);
    char snSize[128];
    sprintf(snSize, "%d", s->nSize);
    char *dests[] = { name, max, P, snSize, sgraph, coil+1};
    if(ShowSimpleDialog(title, 6, labels, 0, 0xff, 0xff, dests)) {
        s->graph = hobatoi(sgraph);
        s->nSize = hobatoi(snSize);

        char str[MAX_NAME_LEN];
        if(IsNumber(max)) {
            sprintf(str, "C%s%s", s->name, "Dec");
            CheckConstantInRange(str, max, hobatoi(max));
        }
        if(IsNumber(P)) {
            sprintf(str, "C%s%s", s->name, "P");
            CheckConstantInRange(str, P, hobatoi(P));
        }
        if(coil[0]!='Y')
            Error(_("Pulse to: '%s' you must set to output pin 'Y%s' or to internal relay 'R%s'."), coil, coil, coil);

        if(IsNumber(P)){
            double Pt=1.0*Prog.cycleTime*hobatoi(P)/1000000.0;
            char Punits[3];
            double _Pt=SIprefix(Pt, Punits);

            double F=1000000.0/Prog.cycleTime/hobatoi(P);
            char Funits[3];
            double _F=SIprefix(F, Funits);

            char str[1000];
            sprintf(str, "Pmin=%.3f %ss, Fmax=%.3f %sHz", _Pt, Punits, _F, Funits);

            int count=hobatoi(max);

            ResSteps r;
            //memset(&r,0,sizeof(r));
            if(IsNumber(max) && (s->graph)){
                CalcSteps(s, &r);

                double _Psum=SIprefix(Pt*r.Psum, Punits);
                sprintf(str, "%s\n\nAcceleration/Deceleration time=%.3f %ss", str, _Psum, Punits);

                CheckFree(r.T);
            }

            if(IsNumber(max)){
                double Tfull;
                double _Tfull;
                char Tunits[3];
                if(r.n){
                    if(count > (r.n-1)*2)
                        Tfull=Pt*(count-(r.n-1)*2)+Pt*r.Psum*2.0;
                    else
                        Tfull=Pt*r.Psum*2.0;

                    _Tfull=SIprefix(Tfull, Tunits);
                    sprintf(str, "%s\n\nWork time=%.3f %ss", str, _Tfull, Tunits);
                }
                _Tfull=SIprefix(Pt*count, Tunits);
                sprintf(str, "%s\n\nTime without accel/decel=%.3f %ss", str, _Tfull, Tunits);
            }

            MessageBox(MainWindow, str, _("Stepper information"),
                MB_OK | MB_ICONINFORMATION);
        }
    };
}

void ShowPulserDialog(int which, char *P1, char *P0, char *accel, char *counter, char *busy)
{
    char *title;
    title = _("Pulser");

    char *labels[] = { _("Counter:"), _("P1:"), _("P0:"), _("Accel.:"), _("Busy to:")};
    char *dests[] = { counter, P1, P0, accel, busy};
    if(ShowSimpleDialog(title, 5, labels, 0, 0xff, 0xff, dests)) {
        if(IsNumber(P1))
            CheckConstantInRange("", P1,hobatoi(P1));
        if(IsNumber(P0))
            CheckConstantInRange("", P0,hobatoi(P0));
        if(IsNumber(accel))
            CheckConstantInRange("", accel,hobatoi(accel));
        if(IsNumber(counter))
            CheckConstantInRange("", counter,hobatoi(counter));
        if((busy[0]!='Y') && (busy[0]!='R'))
            Error(_("Busy to: '%s' you must set to internal relay 'R%s' or to output pin 'Y%s'."), busy, busy, busy);

        if(IsNumber(P1) && IsNumber(P0)){
            double P1t=(double)Prog.cycleTime*hobatoi(P1)/1000.0;
            double P0t=(double)Prog.cycleTime*hobatoi(P0)/1000.0;
            double P=P1t+P0t;
            char *Punits =  _("ms");

            double F=1000000.0/Prog.cycleTime/(hobatoi(P1)+hobatoi(P0));
            char *Funits;
            if (F<1000.0)
                Funits = _("Hz");
            else {
                F=F/1000.0;
                Funits = _("kHz");
            }
            char str[1000];
            sprintf(str, "P1=%.3f %s, P0=%.3f %s, F=%.3f %s", P1t, Punits, P0t, Punits, F, Funits);

            int count;
            if(IsNumber(counter))
                count=hobatoi(counter);
            double Ta=0;
            int N=0;
            int Na=0;
            if(IsNumber(accel)){
                int a=hobatoi(accel);
                int i,mina,maxa;
                if(a>1){
                    mina=hobatoi(P1)+hobatoi(P0);
                    maxa=mina*a;
                    if(!IsNumber(counter))
                        count= mina+maxa;

                    for(i=maxa;(i>mina) && (N<count);i--){
                      Na+=i;
                      N++;
                      if(hobatoi(P1) != hobatoi(P0)) i--;
                    }
                    Ta=(double)Prog.cycleTime*Na/1000.0;
                    //sprintf(str, "%s\n\nAcceleration time %d cycles=%.3f %s", str, Na, Ta, Punits);
                    sprintf(str, "%s\n\nAcceleration time=%.3f %s", str, Ta, Punits);
                }
            }

            if(IsNumber(counter)){
                double Tfull;
                if(count>N)
                    Tfull=P*(count-N)+Ta;
                else
                    Tfull=Ta;

                char *Tunits;
                if(Tfull<1000.0)
                    Tunits = _("ms");
                else {
                    Tfull/=1000.0;
                    Tunits = _("s");
                }
                sprintf(str, "%s\n\nWork time=%.3f %s", str, Tfull, Tunits);
            }
            MessageBox(MainWindow, str, _("Pulser information"),
                MB_OK | MB_ICONINFORMATION);
        }
    };
}

void ShowNPulseDialog(int which, char *counter, char *targetFreq, char *coil)
{
    char *labels[] = { _("Counter var:"), _("Frequency (Hz):"), "Pulse to:"};
    char *dests[] = { counter, targetFreq, coil};
    if(ShowSimpleDialog(_("Set N Pulse Cycle"), 3, labels, 0x2, 0x1, 0x7, dests)) {
        //TODO: check the available range
        double freq = hobatoi(targetFreq);
        if(freq < 0)
            Error(_("'%s' freq < 0"), targetFreq);
        if(freq > 1000000.0)
            Error(_("'%s' freq > 100000"), targetFreq);
    }
}

void ShowQuadEncodDialog(int which, char *counter, int *int01, char *contactA, char *contactB, char *contactZ, char *error)
{
    char title[100];
    sprintf(title, _("Quad Encod%d"), *int01);

    char _int01[100];
    sprintf(_int01, "%d", *int01);

    char *labels[] = { _("Counter var:"), _("Input A INTs:"), _("Input A:"), _("Input B:"), _("Input Z:"), _("Output Zero(Counter==0):")};
    char *dests[] = { counter, _int01, contactA, contactB, contactZ, error};
{};
    NoCheckingOnBox[4] = TRUE;
    NoCheckingOnBox[5] = TRUE;
    if(ShowSimpleDialog(title, 6, labels, 0x2, 0xff, 0xff, dests)) {
        //TODO: check the available range
        *int01 = hobatoi(_int01);
        if(Prog.mcu)
        if((*int01<0)||(Prog.mcu->interruptCount<=*int01))
            Error(_("Can select only INTs pin."));

    }
    NoCheckingOnBox[4] = FALSE;
    NoCheckingOnBox[5] = FALSE;
}

void ShowSizeOfVarDialog(PlcProgramSingleIo *io)
{
    int sov=SizeOfVar(io->name);
    char sovStr[20];
    sprintf(sovStr, "%d", sov);

    SDWORD val;
    char valStr[MAX_STRING_LEN];
    if(io->type == IO_TYPE_STRING){
        strcpy(valStr,GetSimulationStr(io->name));
    } else {
        val = GetSimulationVariable(io->name, TRUE);
        sprintf(valStr, "%d", val);
    }

    char s[MAX_NAME_LEN];
    sprintf(s, _("Set variable '%s'"), io->name);

    char *labels[] = { _("SizeOfVar:"), _("Simulation value:")};
    char *dests[] = { sovStr, valStr };
    if(ShowSimpleDialog(s, 2, labels, 0x3, 0x0, 0x3, dests)) {
       sov = hobatoi(sovStr);
       sov = 2;
       if((sov <= 0)
       ||((io->type != IO_TYPE_STRING) && (sov>4) && (io->type != IO_TYPE_BCD))
       ||((io->type == IO_TYPE_BCD) && (sov>10))
       ){
           Error(_("Not a reasonable size for a variable."));
       } else {
           SetSizeOfVar(io->name, sov);
       }

       if(io->type == IO_TYPE_STRING){
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

    char *labels[] = { _("Name:"), _("Stages:") };
    char *dests[] = { name, stagesStr };
    ShowSimpleDialog(_("Shift Register"), 2, labels, 0x2, 0x1, 0x3, dests);

    *stages = hobatoi(stagesStr);

    if(*stages <= 0 || *stages >= MAX_SHIFT_REGISTER_STAGES-1) {
        Error(_("Not a reasonable size for a shift register."));
        *stages = 8;
    }
}

void ShowFormattedStringDialog(char *var, char *string)
{
    char *labels[] = { _("Variable:"), _("String:") };
    char *dests[] = { var, string };
    NoCheckingOnBox[0] = TRUE;
    NoCheckingOnBox[1] = TRUE;
    ShowSimpleDialog(_("Formatted String Over UART"), 2, labels, 0x0,
        0x1, 0x3, dests);
    NoCheckingOnBox[0] = FALSE;
    NoCheckingOnBox[1] = FALSE;
}

void ShowStringDialog(char * dest, char *var, char *string)
{
    char *labels[] = { _("Variable list:"), _("Format string:"), _("Dest:") };
    char *dests[] = { var, string, dest };
    NoCheckingOnBox[0] = TRUE;
    NoCheckingOnBox[1] = TRUE;
    NoCheckingOnBox[2] = TRUE;
    ShowSimpleDialog(_("Formatted String"), 3, labels, 0x0,
        0x6, 0x7, dests);
    NoCheckingOnBox[0] = FALSE;
    NoCheckingOnBox[1] = FALSE;
    NoCheckingOnBox[2] = FALSE;
}

void ShowCprintfDialog(int which, void *e)
{
    ElemFormattedString *f = (ElemFormattedString *)e;
    char *var = f->var;
    char *string = f->string;
    char *dest = f->dest;
    char *enable = f->enable;
    char *error = f->error;

    char *labels[] = { _("Variable list:"), _("Format string:"), _("Dest:"), _("Enable:"), _("Error:") };
    char *dests[] = { var, string, dest, enable, error };
    char *s;
    switch(which) {
        case ELEM_CPRINTF:      s = "CPRINTF"; goto cprintf;
        case ELEM_SPRINTF:      s = "SPRINTF"; goto cprintf;
        case ELEM_FPRINTF:      s = "FPRINTF"; goto cprintf;
        case ELEM_PRINTF:       s = "PRINTF"; goto cprintf;
        case ELEM_I2C_CPRINTF:  s = "I2C_PRINTF"; goto cprintf;
        case ELEM_ISP_CPRINTF:  s = "ISP_PRINTF"; goto cprintf;
        case ELEM_UART_CPRINTF: s = "UART_PRINTF"; goto cprintf; {
        cprintf:
            break;
        }
        default: ooops("ELEM_0x%X");
    }
    char str[MAX_NAME_LEN];
    sprintf(str, ("Formatted String over %s"), s);
    NoCheckingOnBox[0] = TRUE;
    NoCheckingOnBox[1] = TRUE;
    NoCheckingOnBox[2] = TRUE;
    NoCheckingOnBox[3] = TRUE;
    NoCheckingOnBox[4] = TRUE;
    ShowSimpleDialog( str, 5, labels, 0x0,
        0x1c, 0xff, dests);
    NoCheckingOnBox[0] = FALSE;
    NoCheckingOnBox[1] = FALSE;
    NoCheckingOnBox[2] = FALSE;
    NoCheckingOnBox[3] = FALSE;
    NoCheckingOnBox[4] = FALSE;
}

void ShowPersistDialog(char *var)
{
    char *labels[] = { _("Variable:") };
    char *dests[] = { var };
    ShowSimpleDialog(_("Make Persistent"), 1, labels, 0, 1, 1, dests);
}
