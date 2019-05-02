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
// Routines for drawing the ladder diagram as a schematic on screen. This
// includes the stuff to figure out where we should draw each leaf (coil,
// contact, timer, ...) element on screen and how we should connect them up
// with wires.
// Jonathan Westhues, Oct 2004
//-----------------------------------------------------------------------------
#include "stdafx.h"

#include "ldmicro.h"

// Number of drawing columns (leaf element units) available. We want to
// know this so that we can right-justify the coils.
int ColsAvailable;

// Set when we draw the selected element in the program. If there is no
// element selected then we ought to put the cursor at the top left of
// the screen.
bool SelectionActive;

// Is the element currently being drawn highlighted because it is selected?
// If so we must not do further syntax highlighting.
bool ThisHighlighted;

#define TOO_LONG _("!!!too long!!!")

#define DM_BOUNDS(gx, gy)                             \
    {                                                 \
        if((gx) >= DISPLAY_MATRIX_X_SIZE || (gx) < 0) \
            ooops("DISPLAY_MATRIX_X_SIZE gx=%d", gx); \
        if((gy) >= DISPLAY_MATRIX_Y_SIZE || (gy) < 0) \
            ooops("DISPLAY_MATRIX_Y_SIZE gy=%d", gy); \
    }

//-----------------------------------------------------------------------------
// The display code is the only part of the program that knows how wide a
// rung will be when it's displayed; so this is the only convenient place to
// warn the user and undo their changes if they created something too wide.
// This is not very clean.
//-----------------------------------------------------------------------------
static bool CheckBoundsUndoIfFails(int gx, int gy)
{
    if(gx >= DISPLAY_MATRIX_X_SIZE || gx < 0 || //
       gy >= DISPLAY_MATRIX_Y_SIZE || gy < 0) {
        if(CanUndo()) {
            UndoUndo();
            Error(_("Too many elements in subcircuit!"));
            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
// Determine the width, in leaf element units, of a particular subcircuit.
// The width of a leaf is 1, the width of a series circuit is the sum of
// of the widths of its members, and the width of a parallel circuit is
// the maximum of the widths of its members.
//-----------------------------------------------------------------------------
static int CountWidthOfElement(int which, void *elem, int soFar)
{
    switch(which) {
        case ELEM_PADDING:
        case ELEM_PLACEHOLDER:
        case ELEM_OPEN:
        case ELEM_SHORT:
        case ELEM_CONTACTS:
        case ELEM_TIME2COUNT:
        case ELEM_TIME2DELAY:
        case ELEM_TCY:
        case ELEM_TON:
        case ELEM_TOF:
        case ELEM_RTL:
        case ELEM_RTO:
        case ELEM_THI:
        case ELEM_TLO:
        case ELEM_CTU:
        case ELEM_CTD:
        case ELEM_CTC: // as Leaf
        case ELEM_CTR:
        case ELEM_ONE_SHOT_RISING:
        case ELEM_ONE_SHOT_FALLING:
        case ELEM_ONE_SHOT_LOW:
        case ELEM_OSC:
        case ELEM_EQU:
        case ELEM_NEQ:
        case ELEM_GRT:
        case ELEM_GEQ:
        case ELEM_LES:
        case ELEM_LEQ:
        case ELEM_UART_RECV:
        case ELEM_UART_RECVn:
        case ELEM_UART_RECV_AVAIL:
        case ELEM_UART_SEND:
        case ELEM_UART_SENDn:
        case ELEM_UART_SEND_READY:
#ifdef USE_SFR
        case ELEM_RSFR:
        case ELEM_WSFR:
        case ELEM_SSFR:
        case ELEM_CSFR:
        case ELEM_TSFR:
        case ELEM_T_C_SFR:
#endif
        case ELEM_BIN2BCD:
        case ELEM_BCD2BIN:
        case ELEM_SWAP:
        case ELEM_OPPOSITE:
        case ELEM_SPI:
        case ELEM_SPI_WR:       ///// Added by JG
        case ELEM_I2C_RD:       ///// Added by JG
        case ELEM_I2C_WR:       ///// Added by JG
        case ELEM_BUS:
        case ELEM_7SEG:
        case ELEM_9SEG:
        case ELEM_14SEG:
        case ELEM_16SEG:
        case ELEM_ROL:
        case ELEM_ROR:
        case ELEM_SHL:
        case ELEM_SHR:
        case ELEM_SR0:
        case ELEM_AND:
        case ELEM_OR:
        case ELEM_XOR:
        case ELEM_RANDOM:
        case ELEM_SEED_RANDOM:
        case ELEM_DELAY:
        case ELEM_LABEL:
        case ELEM_SUBPROG:
        case ELEM_ENDSUB:
        case ELEM_SET_BIT:
        case ELEM_CLEAR_BIT:
        case ELEM_IF_BIT_SET:
        case ELEM_IF_BIT_CLEAR:
            return 1;

        case ELEM_QUAD_ENCOD:
        case ELEM_NPULSE:
        case ELEM_PULSER:
        case ELEM_STEPPER:
        case ELEM_STRING:
        case ELEM_CPRINTF:
        case ELEM_SPRINTF:
        case ELEM_FPRINTF:
        case ELEM_PRINTF:
        case ELEM_I2C_CPRINTF:
        case ELEM_ISP_CPRINTF:
        case ELEM_UART_CPRINTF:
        case ELEM_FORMATTED_STRING:
            return 2;

        case ELEM_COMMENT: {
            //if(soFar != 0) oops();

            ElemLeaf *l = (ElemLeaf *)elem;
            char      tbuf[MAX_COMMENT_LEN];

            strcpy(tbuf, l->d.comment.str);
            char *b = strchr(tbuf, '\n');

            int len;
            if(b) {
                *b = '\0';
                len = std::max(strlen(tbuf) - 1, strlen(b + 1));
            } else {
                len = strlen(tbuf);
            }
            // round up, and allow space for lead-in
            len = (len + 7 + (POS_WIDTH - 1)) / POS_WIDTH;
            return std::min(ScreenColsAvailable() - soFar, std::max(ColsAvailable, len));
            //return std::max(ColsAvailable, len);
        }
        case ELEM_RES:
        case ELEM_COIL:
        case ELEM_MOVE:
        case ELEM_NOT:
        case ELEM_NEG:
        case ELEM_SHIFT_REGISTER:
        case ELEM_LOOK_UP_TABLE:
        case ELEM_PIECEWISE_LINEAR:
        case ELEM_MASTER_RELAY:
        case ELEM_SLEEP:
        case ELEM_CLRWDT:
        case ELEM_LOCK:
        case ELEM_GOTO:
        case ELEM_GOSUB:
        case ELEM_RETURN:
        case ELEM_READ_ADC:
        case ELEM_SET_PWM:
        //case ELEM_PWM_OFF:
        case ELEM_NPULSE_OFF:
        case ELEM_PERSIST:
            if(ColsAvailable - soFar > 1) {
                return ColsAvailable - soFar;
            } else {
                return 1;
            }

        case ELEM_ADD:
        case ELEM_SUB:
        case ELEM_MUL:
        case ELEM_DIV:
        case ELEM_MOD:
            if(ColsAvailable - soFar > 1) {
                return ColsAvailable - soFar;
            } else {
                return 1;
            }

        case ELEM_SERIES_SUBCKT: {
            // total of the width of the members
            int               total = 0;
            ElemSubcktSeries *s = (ElemSubcktSeries *)elem;
            for(int i = 0; i < s->count; i++) {
                total += CountWidthOfElement(s->contents[i].which, s->contents[i].data.any, total + soFar);
            }
            return total;
        }

        case ELEM_PARALLEL_SUBCKT: {
            // greatest of the width of the members
            int                 max = 0;
            ElemSubcktParallel *p = (ElemSubcktParallel *)elem;
            for(int i = 0; i < p->count; i++) {
                int w = CountWidthOfElement(p->contents[i].which, p->contents[i].data.any, soFar);
                if(w > max) {
                    max = w;
                }
            }
            return max;
        }

        default:
            ooops("ELEM_0x%X", which);
            //return 0;
    }
}

//-----------------------------------------------------------------------------
// Determine the height, in leaf element units, of a particular subcircuit.
// The height of a leaf is 1, the height of a parallel circuit is the sum of
// of the heights of its members, and the height of a series circuit is the
// maximum of the heights of its members. (This is the dual of the width
// case.)
//-----------------------------------------------------------------------------
int CountHeightOfElement(int which, void *elem)
{
    switch(which) {
        //case ELEM_PADDING:
        CASE_LEAF
        return 1;

        case ELEM_PARALLEL_SUBCKT: {
            // total of the height of the members
            int                 total = 0;
            ElemSubcktParallel *s = (ElemSubcktParallel *)elem;
            for(int i = 0; i < s->count; i++) {
                total += CountHeightOfElement(s->contents[i].which, s->contents[i].data.any);
            }
            return total;
        }

        case ELEM_SERIES_SUBCKT: {
            // greatest of the height of the members
            int               max = 0;
            ElemSubcktSeries *s = (ElemSubcktSeries *)elem;
            for(int i = 0; i < s->count; i++) {
                int w = CountHeightOfElement(s->contents[i].which, s->contents[i].data.any);
                if(w > max) {
                    max = w;
                }
            }
            return max;
        }

        default:
            ooops("ELEM_0x%X", which);
            //return 0;
    }
}

//-----------------------------------------------------------------------------
// Determine the width, in leaf element units, of the widest row of the PLC
// program (i.e. loop over all the rungs and find the widest).
//-----------------------------------------------------------------------------
int ProgCountWidestRow()
{
    int max = 0;
    int colsTemp = ColsAvailable;
    ColsAvailable = 0;
    for(int i = 0; i < Prog.numRungs; i++) {
        int w = CountWidthOfElement(ELEM_SERIES_SUBCKT, Prog.rungs(i), 0);
        if(w > max) {
            max = w;
        }
    }
    ColsAvailable = colsTemp;
    return max;
}
//-----------------------------------------------------------------------------
int ProgCountRows()
{
    int totalHeight = 0;
    for(int i = 0; i < Prog.numRungs; i++) {
        totalHeight += CountHeightOfElement(ELEM_SERIES_SUBCKT, Prog.rungs(i));
    }
    // // //totalHeight += 1; // without EndRung !
    return totalHeight;
}
//-----------------------------------------------------------------------------
// Draw a vertical wire one leaf element unit high up from (cx, cy), where cx
// and cy are in charcter units.
//-----------------------------------------------------------------------------
static void VerticalWire(int cx, int cy)
{
    if(cx >= 0) {
        for(int j = 1; j < POS_HEIGHT; j++) {
            DrawChars(cx, cy + (POS_HEIGHT / 2 - j), "|");
        }
        DrawChars(cx, cy + (POS_HEIGHT / 2), "+");
        DrawChars(cx, cy + (POS_HEIGHT / 2 - POS_HEIGHT), "+");
    }
}

//-----------------------------------------------------------------------------
// Convenience functions for making the text colors pretty, for DrawElement.
//-----------------------------------------------------------------------------
static void NormText()
{
    SetTextColor(Hdc, InSimulationMode ? HighlightColours.simOff : HighlightColours.def);
    SelectObject(Hdc, FixedWidthFont);
}
static void EmphText()
{
    SetTextColor(Hdc, InSimulationMode ? HighlightColours.simOn : HighlightColours.selected);
    SelectObject(Hdc, FixedWidthFontBold);
}
static void NameText()
{
    if(!InSimulationMode && !ThisHighlighted) {
        SetTextColor(Hdc, HighlightColours.name);
    }
}
static void BodyText()
{
    if(!InSimulationMode && !ThisHighlighted) {
        SetTextColor(Hdc, HighlightColours.def);
    }
}
static void PoweredText(bool powered)
{
    if(InSimulationMode) {
        if(powered)
            EmphText();
        else
            NormText();
    }
}

//-----------------------------------------------------------------------------
// Count the length of a string, in characters. Nonstandard because the
// string may contain special characters to indicate formatting (syntax
// highlighting).
//-----------------------------------------------------------------------------
static int FormattedStrlen(const char *str)
{
    int l = 0;
    while(*str) {
        //if(*str > 10) {
        if(WORD(*str) >= 10) { // WORD typecast allow national charset in comments
            l++;
        }
        str++;
    }
    return l;
}

//-----------------------------------------------------------------------------
static void CenterWithSpacesWidth(int cx, int cy, const char *str, bool before, bool after, bool isName, int totalWidth,
                                  int which)
{
    int extra = totalWidth - FormattedStrlen(str);
    if(which == ELEM_COIL)
        PoweredText(before);
    else
        PoweredText(after);
    if(isName)
        NameText();
    DrawChars(cx + (extra / 2), cy + (POS_HEIGHT / 2) - 1, str);
    if(isName)
        BodyText();
}

static void CenterWithSpacesWidth(int cx, int cy, char *str, bool powered, bool isName, int totalWidth)
{
    CenterWithSpacesWidth(cx, cy, str, powered, powered, isName, totalWidth, 0);
}

//-----------------------------------------------------------------------------
// Draw a string, centred in the space of a single position, with spaces on
// the left and right. Draws on the upper line of the position.
//-----------------------------------------------------------------------------
static void CenterWithSpaces(int cx, int cy, const char *str, bool powered, bool isName)
{
    CenterWithSpacesWidth(cx, cy, str, powered, powered, isName, POS_WIDTH, 0);
}

//-----------------------------------------------------------------------------
// Like CenterWithWires, but for an arbitrary width position (e.g. for ADD
// and SUB, which are double-width).
//-----------------------------------------------------------------------------
static void CenterWithWiresWidth(int cx, int cy, const char *str, bool before, bool after, int totalWidth, int which)
{
    int extra = totalWidth - FormattedStrlen(str);

    PoweredText(before);
    for(int i = 0; i < (extra / 2); i++) {
        DrawChars(cx + i, cy + (POS_HEIGHT / 2), "-");
    }

    if(which != ELEM_COIL)
        PoweredText(after);
    DrawChars(cx + (extra / 2), cy + (POS_HEIGHT / 2), str);

    PoweredText(after);
    for(int i = FormattedStrlen(str) + (extra / 2); i < totalWidth; i++) {
        DrawChars(cx + i, cy + (POS_HEIGHT / 2), "-");
    }
}

//-----------------------------------------------------------------------------
// Draw a string, centred in the space of a single position, with en dashes on
// the left and right coloured according to the powered state. Draws on the
// middle line.
//-----------------------------------------------------------------------------
static void CenterWithWiresWidth(int cx, int cy, const char *str, bool before, bool after, int totalWidth)
{
    CenterWithWiresWidth(cx, cy, str, before, after, totalWidth, 0);
}

static void CenterWithWires(int cx, int cy, const char *str, bool before, bool after)
{
    CenterWithWiresWidth(cx, cy, str, before, after, POS_WIDTH, 0);
}

//-----------------------------------------------------------------------------
#define BUF_LEN 256
static char *formatWidth(char *buf, size_t totalWidth, const char *str1, const char *leftStr, const char *centerStr,
                         const char *rightStr, const char *str5)
{
    if(totalWidth >= POS_WIDTH)
        totalWidth--;

    // FULL lengths with unvisible
    size_t L1 = strlen(str1);
    size_t Ll = strlen(leftStr);
    size_t Lc = strlen(centerStr);
    size_t Lr = strlen(rightStr);
    size_t L5 = strlen(str5);

    // visible lengths
    size_t l1 = FormattedStrlen(str1);
    size_t ll = FormattedStrlen(leftStr);
    size_t lc = FormattedStrlen(centerStr);
    size_t lr = FormattedStrlen(rightStr);
    size_t l5 = FormattedStrlen(str5);

    size_t space1 = 0;
    size_t space2 = 0;

    if(totalWidth < (l1 + ll + lc + lr + l5)) {
        if((ll > lc) && (ll > lr)) {
            Ll = totalWidth - (l1 + lc + lr + l5) + (Ll - ll);
        } else if((lc > ll) && (lc > lr)) {
            Lc = totalWidth - (l1 + ll + lr + l5) + (Lc - lc);
        } else if((lr > ll) && (lr > lc)) {
            Lr = totalWidth - (l1 + ll + lc + l5) + (Lr - lr);
        } else {
            Lc = totalWidth - (l1 + ll + lr + l5) + (Lc - lc);
        }
    } else {
        space2 = (totalWidth - (l1 + ll + lc + lr + l5)) / 2;
        space1 = totalWidth - (l1 + ll + lc + lr + l5) - space2;
    }

    buf[0] = '\0';
    int bufLen = 0;

    if(L1) {
        strncat(buf, str1, L1);
        bufLen += L1;
        buf[bufLen] = '\0';
    }

    if(Ll) {
        strncat(buf, leftStr, Ll);
        bufLen += Ll;
        buf[bufLen] = '\0';
        if(Ll < strlen(leftStr))
            buf[bufLen - 1] = '~';
    }

    if(space1) {
        strncat(buf, "                                                ", space1);
        bufLen += space1;
        buf[bufLen] = '\0';
    }

    if(Lc) {
        strncat(buf, centerStr, Lc);
        bufLen += Lc;
        buf[bufLen] = '\0';
        if(Lc < strlen(centerStr))
            buf[bufLen - 1] = '~';
    }

    if(space2) {
        strncat(buf, "                                                ", space2);
        bufLen += space2;
        buf[bufLen] = '\0';
    }

    if(Lr) {
        strncat(buf, rightStr, Lr);
        bufLen += Lr;
        buf[bufLen] = '\0';
        if(Lr < strlen(rightStr))
            buf[bufLen - 1] = '~';
    }

    if(L5) {
        strncat(buf, str5, L5);
        bufLen += L5;
        buf[bufLen] = '\0';
    }

    // if(totalWidth==POS_WIDTH-1)
    //    //dbp("buf %d %d %d >%s<", Ll, ll, Ll-ll, buf);

    return buf;
}

//-----------------------------------------------------------------------------
void DrawWire(int *cx, int *cy, char c)
{
    char buf[POS_WIDTH + 1];
    memset(buf, c, POS_WIDTH);
    buf[POS_WIDTH] = '\0';

    DrawChars(*cx, *cy + (POS_HEIGHT / 2), buf);
    *cx += POS_WIDTH;
}

//-----------------------------------------------------------------------------
// Draw an end of line element (coil, RES, MOV, etc.). Special things about
// an end of line element: we must right-justify it.
//-----------------------------------------------------------------------------
static bool DrawEndOfLine(int which, ElemLeaf *leaf, int *cx, int *cy, bool poweredBefore)
{
    if(!EndOfRungElem(which)) {
        ooops("which=%d", which);
    }
    int cx0 = *cx, cy0 = *cy;

    bool poweredAfter = leaf->poweredAfter;
    bool workingNow = leaf->workingNow;

    int thisWidth;
    switch(which) {
        case ELEM_ADD:
        case ELEM_SUB:
        case ELEM_MUL:
        case ELEM_DIV:
        case ELEM_MOD:
            thisWidth = 1;
            break;

        default:
            thisWidth = 1;
            break;
    }

    NormText();
    PoweredText(poweredBefore);
    while(*cx < (ColsAvailable - thisWidth) * POS_WIDTH) {
        int gx = *cx / POS_WIDTH;
        int gy = *cy / POS_HEIGHT;

        if(CheckBoundsUndoIfFails(gx, gy))
            return false;

        if(gx >= DISPLAY_MATRIX_X_SIZE)
            oops();
        DM_BOUNDS(gx, gy);
        DisplayMatrix[gx][gy].data.leaf = PADDING_IN_DISPLAY_MATRIX;
        DisplayMatrix[gx][gy].which = ELEM_PADDING;

        DrawWire(cx, cy, '-');
        cx0 += POS_WIDTH;
    }

    if((leaf == Selected.data.leaf) && (!InSimulationMode)) {
        EmphText();
        ThisHighlighted = true;
    } else {
        ThisHighlighted = false;
    }

    static char top[BUF_LEN];
    static char bot[BUF_LEN];
    static char s1[BUF_LEN];
    static char s2[BUF_LEN];
    static char s3[BUF_LEN];
    switch(which) {
        /* //moved to DrawLeaf
        case ELEM_CTC: {
            char buf[256];
            ElemCounter *c = &leaf->d.counter;
            sprintf(buf, "{\x01""CTC\x02 0:%d}", c->max);

            CenterWithSpaces(*cx, *cy, c->name, poweredAfter, true);
            CenterWithWires(*cx, *cy, buf, poweredBefore, poweredAfter);
            break;
        }
        */
        case ELEM_RES: {
            ElemReset *r = &leaf->d.reset;
            CenterWithSpaces(*cx, *cy, formatWidth(top, POS_WIDTH, "", "", r->name, "", ""), poweredAfter, true);
            CenterWithWires(*cx, *cy, "{RES}", poweredBefore, poweredAfter);
            break;
        }
        case ELEM_READ_ADC: {
            ElemReadAdc *r = &leaf->d.readAdc;
            sprintf(s2, "%s", r->name);
            CenterWithSpaces(*cx, *cy, formatWidth(top, POS_WIDTH, "", "", s2, "", ""), poweredAfter, true);
            CenterWithWires(*cx, *cy, "{READ ADC}", poweredBefore, poweredAfter);
            break;
        }
        //case ELEM_PWM_OFF:
        //    CenterWithWires(*cx, *cy, "{PWM OFF}", poweredBefore, poweredAfter);
        //    break;
        case ELEM_NPULSE_OFF:
            CenterWithWires(*cx, *cy, "{NPULSE OFF}", poweredBefore, poweredAfter);
            break;
        case ELEM_SET_PWM: {
            ElemSetPwm *s = &leaf->d.setPwm;
            double      s_targetFreq = SIprefix(hobatoi(s->targetFreq), s1);
            sprintf(s2, "%.6g %sHz", s_targetFreq, s1);

            formatWidth(top, POS_WIDTH, "", s->duty_cycle, " ", s2, "");
            formatWidth(bot, POS_WIDTH, "{", "PWM", " ", s->name, "}");

            CenterWithSpaces(*cx, *cy, top, poweredAfter, true);
            CenterWithWires(*cx, *cy, bot, poweredBefore, poweredAfter);
            break;
        }
        case ELEM_PERSIST:
            sprintf(s2, "%s", leaf->d.persist.var);
            CenterWithSpaces(*cx, *cy, formatWidth(top, POS_WIDTH, "", "", s2, "", ""), poweredAfter, true);
            CenterWithWires(*cx, *cy, "{PERSIST}", poweredBefore, poweredAfter);
            break;

        case ELEM_MOVE: {
            ElemMove *m = &leaf->d.move;
            formatWidth(top,
                        POS_WIDTH,
                        "{\x01"
                        "MOV\x02 ",
                        "",
                        "",
                        m->dest,
                        ":=}");
            formatWidth(bot, POS_WIDTH, "{", "", "", m->src, "}");

            CenterWithSpaces(*cx, *cy, top, poweredAfter, false);
            CenterWithWires(*cx, *cy, bot, poweredBefore, poweredAfter);
            break;
        }

        case ELEM_MASTER_RELAY:
            CenterWithWires(*cx, *cy, "{MASTER RLY}", poweredBefore, workingNow);
            break;

        case ELEM_SLEEP: {
#ifdef SLEEP_DELAY
            double d = 1;
            // SIprefix(1.0 * leaf->d.timer.delay / 1000000.0, s1);
            sprintf(s2, "%.3g %ss", d, s1);
            sprintf(bot, "{SLEEP %s}", s2);
#else
            sprintf(bot, "{SLEEP}");
#endif
            CenterWithWires(*cx, *cy, bot, poweredBefore, poweredAfter);
            break;
        }
        case ELEM_CLRWDT:
            CenterWithWires(*cx, *cy, "{CLRWDT}", poweredBefore, poweredAfter);
            break;

        case ELEM_LOCK:
            CenterWithWires(*cx, *cy, "{LOCK}", poweredBefore, poweredAfter);
            break;

        case ELEM_GOTO:
            CenterWithSpaces(
                *cx, *cy, formatWidth(top, POS_WIDTH, "", "", leaf->d.doGoto.label, "", ""), poweredAfter, true);
            CenterWithWires(*cx, *cy, "{GOTO}", poweredBefore, poweredAfter);
            break;

        case ELEM_GOSUB:
            CenterWithSpaces(
                *cx, *cy, formatWidth(top, POS_WIDTH, "", "", leaf->d.doGoto.label, "", ""), poweredAfter, true);
            CenterWithWires(*cx, *cy, "{GOSUB}", poweredBefore, poweredAfter);
            break;

        case ELEM_RETURN:
            CenterWithWires(*cx, *cy, "{RETURN}", poweredBefore, poweredAfter);
            break;

        case ELEM_SHIFT_REGISTER: {
            sprintf(s2, "%s", leaf->d.shiftRegister.name);
            sprintf(s3, "0..%d", leaf->d.shiftRegister.stages - 1);

            int len = std::min(POS_WIDTH, std::max(1 + 9 + 1, static_cast<int>(1 + strlen(s1) + strlen(s2))));

            formatWidth(top,
                        len,
                        "{",
                        "\x01"
                        "SHIFT REG\x02",
                        "",
                        "",
                        "}");
            formatWidth(bot, len, "{", s2, "", s3, "}");

            CenterWithSpaces(*cx, *cy, top, poweredAfter, false);
            CenterWithWires(*cx, *cy, bot, poweredBefore, poweredAfter);
            break;
        }
        case ELEM_PIECEWISE_LINEAR:
        case ELEM_LOOK_UP_TABLE: {
            const char *name, *dest, *index, *str;
            if(which == ELEM_PIECEWISE_LINEAR) {
                str = "PWL";
                name = leaf->d.piecewiseLinear.name;
                dest = leaf->d.piecewiseLinear.dest;
                index = leaf->d.piecewiseLinear.index;
            } else {
                str = "LUT";
                name = leaf->d.lookUpTable.name;
                dest = leaf->d.lookUpTable.dest;
                index = leaf->d.lookUpTable.index;
            }
            sprintf(s2, "%s", dest);
            formatWidth(top, POS_WIDTH, "{", s2, "", "", ":=}");

            sprintf(s1,
                    "\x01"
                    "%s\x02 ",
                    str);
            sprintf(s2, "%s", name);
            sprintf(s3, "[%s]", index);
            formatWidth(bot, POS_WIDTH, "{", s1, s2, s3, "}");

            CenterWithSpaces(*cx, *cy, top, poweredAfter, false);
            CenterWithWires(*cx, *cy, bot, poweredBefore, poweredAfter);
            break;
        }
        case ELEM_COIL: {
            ElemCoil *c = &leaf->d.coil;

            sprintf(top, "%s", c->name);
            //          CenterWithSpacesWidth(*cx, *cy, top, poweredBefore, poweredAfter, true, POS_WIDTH, ELEM_COIL);
            CenterWithSpacesWidth(*cx, *cy, top, workingNow, poweredAfter, true, POS_WIDTH, ELEM_COIL);

            bot[0] = '(';
            if(c->negated) {
                bot[1] = '/';
            } else if(c->setOnly) {
                bot[1] = 'S';
            } else if(c->resetOnly) {
                bot[1] = 'R';
            } else if(c->ttrigger) {
                bot[1] = 'T';
            } else {
                bot[1] = ' ';
            }
            bot[2] = ')';
            bot[3] = '\0';
            CenterWithWiresWidth(*cx, *cy, bot, poweredBefore, poweredAfter, POS_WIDTH, ELEM_COIL);
            break;
        }
            // clang-format off
        const char *s;
        const char *z;
        case ELEM_DIV: s = "\x01""DIV\x02"; z="/";  goto math;
        case ELEM_MOD: s = "\x01""MOD\x02"; z="%";  goto math;
        case ELEM_MUL: s = "\x01""MUL\x02"; z="*";  goto math;
        case ELEM_SUB: s = "\x01""SUB\x02"; z="-";  goto math;
        case ELEM_ADD: s = "\x01""ADD\x02"; z="+";  goto math;
        // clang-format on
        math : {
            int w = ((which == ELEM_NOT) || (which == ELEM_NEG)) ? 1 : 1;
            sprintf(s1, "%s ", s);
            sprintf(s2, "%s", leaf->d.math.dest);
            formatWidth(top, w * POS_WIDTH, "{", s1, "", s2, ":=}");
            if((which == ELEM_NOT) || (which == ELEM_NEG)) {
                formatWidth(bot, POS_WIDTH, "{", "", z, leaf->d.math.op1, "}");
            } else {
                formatWidth(bot, /*2**/ POS_WIDTH, "{", leaf->d.math.op1, z, leaf->d.math.op2, "}");
            }
            CenterWithSpacesWidth(*cx, *cy, top, poweredAfter, true, w * POS_WIDTH);
            CenterWithWiresWidth(*cx, *cy, bot, poweredBefore, poweredAfter, w * POS_WIDTH);

            *cx += (w - 1) * POS_WIDTH;

            break;
        }
        default:
            ooops("which=%d cx0=%d *cx=%d cy0=%d *cy=%d", which, cx0, *cx, cy0, *cy);
            break;
    }

    *cx += POS_WIDTH;

    return poweredAfter;
}

//-----------------------------------------------------------------------------
// Draw a leaf element. Special things about a leaf: no need to recurse
// further, and we must put it into the display matrix.
//-----------------------------------------------------------------------------
static bool DrawLeaf(int which, ElemLeaf *leaf, int *cx, int *cy, bool poweredBefore)
{
    //  if(EndOfRungElem(which))
    //      ooops("which=%d",which);
    int  cx0 = *cx, cy0 = *cy;
    bool poweredAfter = leaf->poweredAfter;
    bool workingNow = leaf->workingNow;

    static char top[BUF_LEN];
    static char bot[BUF_LEN];
    static char s1[BUF_LEN];
    static char s2[BUF_LEN];
    static char s3[BUF_LEN];
    const char *s;

    switch(which) {
        case ELEM_COMMENT: {
            size_t maxl = ColsAvailable * POS_WIDTH - *cx - 2;
            char   tbuf[MAX_COMMENT_LEN];
            char   tlbuf[MAX_COMMENT_LEN + 8];

            strcpy(tbuf, leaf->d.comment.str);
            char *b = strchr(tbuf, '\n');

            if(b) {
                if(b[-1] == '\r')
                    b[-1] = '\0';
                *b = '\0';
                sprintf(tlbuf, "\x03 ; %s\x02", tbuf);
                if(strlen(tlbuf) > maxl) {
                    tlbuf[maxl] = '~';
                    tlbuf[maxl + 1] = '\0';
                }
                DrawChars(*cx, *cy + (POS_HEIGHT / 2) - 1, tlbuf);
                sprintf(tlbuf, "\x03 ; %s\x02", b + 1);
                if(strlen(tlbuf) > maxl) {
                    tlbuf[maxl] = '~';
                    tlbuf[maxl + 1] = '\0';
                }
                DrawChars(*cx, *cy + (POS_HEIGHT / 2), tlbuf);
            } else {
                sprintf(tlbuf, "\x03 ; %s\x02", tbuf);
                if(strlen(tlbuf) > maxl) {
                    tlbuf[maxl] = '~';
                    tlbuf[maxl + 1] = '\0';
                }
                DrawChars(*cx, *cy + (POS_HEIGHT / 2) - 1, tlbuf);
            }

            //*cx += ColsAvailable*POS_WIDTH;
            *cx += (maxl + 2);
            break;
        }
        case ELEM_PLACEHOLDER: {
            NormText();
            CenterWithWiresWidth(*cx, *cy, "--", false, false, 2);
            *cx += POS_WIDTH;
            break;
        }
        case ELEM_CONTACTS: {
            ElemContacts *c = &leaf->d.contacts;
            /*
            bot[0] = ']';
            bot[1] = c->negated ? '/' : ' ';
            bot[2] = '[';
            bot[3] = '\0';
            */
            formatWidth(top, POS_WIDTH, "", "", c->name, "", "");
            //          CenterWithSpaces(*cx, *cy, top, poweredAfter, true);
            CenterWithSpacesWidth(*cx, *cy, top, workingNow, poweredAfter, true, POS_WIDTH, ELEM_COIL);

            sprintf(bot, "%c]%c[-", ((c->name[0] == 'X') && (c->set1)) ? '^' : '-', c->negated ? '/' : ' ');
            CenterWithWires(*cx, *cy, bot, poweredBefore, poweredAfter);

            *cx += POS_WIDTH;
            break;
        }
        case ELEM_BIN2BCD: {
            ElemMove *m = &leaf->d.move;
            formatWidth(top,
                        POS_WIDTH,
                        "{\x01"
                        "BIN2BCD\x02 ",
                        "",
                        "",
                        m->dest,
                        ":=}");
            formatWidth(bot, POS_WIDTH, "{", "", "", m->src, "}");

            CenterWithSpaces(*cx, *cy, top, poweredAfter, false);
            CenterWithWires(*cx, *cy, bot, poweredBefore, poweredAfter);

            *cx += POS_WIDTH;
            break;
        }
        case ELEM_BCD2BIN: {
            ElemMove *m = &leaf->d.move;
            formatWidth(top,
                        POS_WIDTH,
                        "{\x01"
                        "BCD2BIN\x02 ",
                        "",
                        "",
                        m->dest,
                        ":=}");
            formatWidth(bot, POS_WIDTH, "{", "", "", m->src, "}");

            CenterWithSpaces(*cx, *cy, top, poweredAfter, false);
            CenterWithWires(*cx, *cy, bot, poweredBefore, poweredAfter);

            *cx += POS_WIDTH;
            break;
        }

            // clang-format off
        {
        const char *s;
        const char *z;
        case ELEM_ROL: s = "\x01""ROL\x02"; z="rol"; goto bitwise;
        case ELEM_ROR: s = "\x01""ROR\x02"; z="ror"; goto bitwise;
        case ELEM_SHL: s = "\x01""SHL\x02"; z="<<"; goto bitwise;
        case ELEM_SHR: s = "\x01""SHR\x02"; z=">>"; goto bitwise;
        case ELEM_SR0: s = "\x01""SR0\x02"; z="sr0"; goto bitwise;
        case ELEM_AND: s = "\x01""AND\x02"; z="&";  goto bitwise;
        case ELEM_OR : s = "\x01""OR\x02" ; z="|";  goto bitwise;
        case ELEM_XOR: s = "\x01""XOR\x02"; z="^";  goto bitwise;
        case ELEM_NOT: s = "\x01""NOT\x02"; z="~";  goto bitwise;
        case ELEM_NEG: s = "\x01""NEG\x02"; z="-";  goto bitwise;
        bitwise : {
            sprintf(s1, "%s ", s);
            sprintf(s2, "%s", leaf->d.math.dest);
            formatWidth(top, POS_WIDTH, "{", s1, "", s2, ":=}");
            if((which == ELEM_NOT) || (which == ELEM_NEG)) {
              formatWidth(bot, POS_WIDTH, "{", "", z, leaf->d.math.op1, "}");
            } else {
              formatWidth(bot, POS_WIDTH, "{", leaf->d.math.op1, z, leaf->d.math.op2, "}");
            }
            CenterWithSpaces(*cx, *cy, top, poweredAfter, false);
            CenterWithWires(*cx, *cy, bot, poweredBefore, poweredAfter);

            *cx += POS_WIDTH;
            break;
        }
        }
            // clang-format on

            {
                const char *s;

                case ELEM_SET_BIT:
                    s = "\x01"
                        "SET Bit\x02 ";
                    goto bits;
                case ELEM_CLEAR_BIT:
                    s = "\x01"
                        "CLR Bit\x02 ";
                    goto bits;
                bits:
                    ElemMove *m = &leaf->d.move;

                    formatWidth(top, POS_WIDTH, "{", "", "", m->dest, "}");
                    formatWidth(bot, POS_WIDTH, "{", s, "", m->src, "}");

                    CenterWithSpaces(*cx, *cy, top, poweredAfter, false);
                    CenterWithWires(*cx, *cy, bot, poweredBefore, poweredAfter);

                    *cx += POS_WIDTH;
                    break;
            }

            {
                const char *s;

                case ELEM_IF_BIT_SET:
                    s = "\x01"
                        "if Bit SET\x02 ";
                    goto ifbits;
                case ELEM_IF_BIT_CLEAR:
                    s = "\x01"
                        "if Bit CLR\x02 ";
                    goto ifbits;
                ifbits:
                    ElemMove *m = &leaf->d.move;

                    formatWidth(top, POS_WIDTH, "[", "", "", m->dest, "]");
                    formatWidth(bot, POS_WIDTH, "[", s, "", m->src, "]");

                    CenterWithSpaces(*cx, *cy, top, poweredAfter, false);
                    CenterWithWires(*cx, *cy, bot, poweredBefore, poweredAfter);

                    *cx += POS_WIDTH;
                    break;
            }

        case ELEM_SWAP: {
            ElemMove *m = &leaf->d.move;
            formatWidth(top, POS_WIDTH, "{", "", "", m->dest, ":=}");
            formatWidth(bot,
                        POS_WIDTH,
                        "{\x01"
                        "SWAP\x02 ",
                        "",
                        "",
                        m->src,
                        "}");

            CenterWithSpaces(*cx, *cy, top, poweredAfter, false);
            CenterWithWires(*cx, *cy, bot, poweredBefore, poweredAfter);

            *cx += POS_WIDTH;
            break;
        }
        case ELEM_OPPOSITE: {
            ElemMove *m = &leaf->d.move;
            formatWidth(top, POS_WIDTH, "{", "", "", m->dest, ":=}");
            formatWidth(bot,
                        POS_WIDTH,
                        "{\x01"
                        "OPPOSITE\x02 ",
                        "",
                        "",
                        m->src,
                        "}");

            CenterWithSpaces(*cx, *cy, top, poweredAfter, false);
            CenterWithWires(*cx, *cy, bot, poweredBefore, poweredAfter);

            *cx += POS_WIDTH;
            break;
        }
#ifdef USE_SFR
            {
                const char *s;
                    // clang-format off
            case ELEM_RSFR:    s = "Read";   goto sfrcmp;
            case ELEM_WSFR:    s = "Write";  goto sfrcmp;
            case ELEM_SSFR:    s = "Sbit";   goto sfrcmp;
            case ELEM_CSFR:    s = "Cbit";   goto sfrcmp;
            case ELEM_TSFR:    s = "IsBitS"; goto sfrcmp;
            case ELEM_T_C_SFR: s = "IsBitC"; goto sfrcmp;
                // clang-format on
                sfrcmp:
                    int l1, l2, lmax;

                    l1 = 2 + 1 + strlen(s) + strlen(leaf->d.cmp.op1);
                    l2 = 2 + 1 + strlen(leaf->d.cmp.op2);
                    lmax = std::max(l1, l2);

                    if(lmax < POS_WIDTH) {
                        memset(s1, ' ', sizeof(s1));
                        s1[0] = '?';
                        s1[lmax - 1] = '?';
                        s1[lmax] = '\0';
                        strcpy(s2, s1);
                        memcpy(s1 + 1, leaf->d.cmp.op1, strlen(leaf->d.cmp.op1));
                        memcpy(s1 + strlen(leaf->d.cmp.op1) + 2, s, strlen(s));
                        memcpy(s2 + 2, leaf->d.cmp.op2, strlen(leaf->d.cmp.op2));
                    } else {
                        strcpy(s1, "");
                        strcpy(s2, TOO_LONG);
                    }

                    CenterWithSpaces(*cx, *cy, s1, poweredAfter, false);
                    CenterWithWires(*cx, *cy, s2, poweredBefore, poweredAfter);

                    *cx += POS_WIDTH;
                    break;
            }
#endif
            {
                // clang-format off
            const char *s;
            case ELEM_EQU: s = "=="; goto cmp;
            case ELEM_NEQ: s = "!="; goto cmp;
            case ELEM_GRT: s = ">" ; goto cmp;
            case ELEM_GEQ: s = ">="; goto cmp;
            case ELEM_LES: s = "<" ; goto cmp;
            case ELEM_LEQ: s = "<="; goto cmp;
            cmp:
                    // clang-format on
                    int len = std::min(
                        static_cast<size_t>(POS_WIDTH),
                        std::max(1 + strlen(leaf->d.cmp.op1) + 1 + strlen(s) + 1, 1 + strlen(leaf->d.cmp.op2) + 1));

                    sprintf(s1, "%s", leaf->d.cmp.op1);
                    sprintf(s2, "%s", leaf->d.cmp.op2);

                    formatWidth(top, len, "[", s1, "", s, "]");
                    formatWidth(bot, len, "[", "", "", s2, "]");
                    CenterWithSpaces(*cx, *cy, top, poweredAfter, false);
                    CenterWithWires(*cx, *cy, bot, poweredBefore, poweredAfter);

                    *cx += POS_WIDTH;
                    break;
            }
        case ELEM_OPEN:
            CenterWithWires(*cx, *cy, "+      +", poweredBefore, poweredAfter);
            *cx += POS_WIDTH;
            break;

        case ELEM_SHORT:
            CenterWithWires(*cx, *cy, "+------+", poweredBefore, poweredAfter);
            *cx += POS_WIDTH;
            break;

        case ELEM_ONE_SHOT_RISING:
        case ELEM_ONE_SHOT_LOW:
        case ELEM_ONE_SHOT_FALLING: {
            const char *s1, *s2;
            if(which == ELEM_ONE_SHOT_RISING) {
                s1 = "    _     _    ";
                s2 = "[_/ \x01"
                     "OSR\x02_/ \\_]";
            } else if(which == ELEM_ONE_SHOT_FALLING) {
                s1 = "  _       _    ";
                s2 = "[ \\_\x01"
                     "OSF\x02_/ \\_]";
            } else if(which == ELEM_ONE_SHOT_LOW) {
                s1 = "  _     _   _ ";
                s2 = "[ \\_\x01"
                     "OSL\x02 \\_/ ]";
            } else
                oops();

            CenterWithSpaces(*cx, *cy, s1, poweredAfter, false);
            CenterWithWires(*cx, *cy, s2, poweredBefore, poweredAfter);

            *cx += POS_WIDTH;
            break;
        }
        case ELEM_OSC: {
            const char *s1, *s2;
            s1 = "   _     _   _  ";
            s2 = "[_/ \x01"
                 "OSC\x02_/ \\_/ \\]";

            CenterWithSpaces(*cx, *cy, s1, poweredAfter, false);
            CenterWithWires(*cx, *cy, s2, poweredBefore, poweredAfter);

            *cx += POS_WIDTH;
            break;
        }
        case ELEM_QUAD_ENCOD: {
            ElemQuadEncod *m = &leaf->d.QuadEncod;

            sprintf(s2, "~~[%s %s", m->inputA, m->inputB);
            sprintf(s3, "%s]%s", m->dir, strlen(m->dir)?"-":" ");
            formatWidth(top, 2 * POS_WIDTH, s2, "", "", "", s3);

            sprintf(s1, "-%c[%s", strlen(m->inputZ)?m->inputZKind:'-', m->inputZ);
            sprintf(s2, " \x01QUAD ENCOD\x02 ");
            formatWidth(bot, 2 * POS_WIDTH, s1, "", s2, m->counter, "]^");

            CenterWithSpacesWidth(*cx, *cy, top, poweredAfter, false, 2 * POS_WIDTH);
            CenterWithWiresWidth(*cx, *cy, bot, poweredBefore, poweredAfter, 2 * POS_WIDTH);
            *cx += 2 * POS_WIDTH;
            break;
        }
        case ELEM_STEPPER: {
            ElemStepper *m = &leaf->d.stepper;

            sprintf(s2, "%s %s", m->name, m->max);
            sprintf(s3, " %s", m->coil);
            formatWidth(top,
                        2 * POS_WIDTH,
                        "[\x01"
                        "STEPPER\x02 ",
                        s2,
                        "",
                        s3,
                        "]->");
            sprintf(s2, "%s", m->P);
            sprintf(s3, " %d %d", m->nSize, m->graph);
            formatWidth(bot, 2 * POS_WIDTH, "[", s2, "", s3, "]--");

            CenterWithSpacesWidth(*cx, *cy, top, poweredAfter, false, 2 * POS_WIDTH);
            CenterWithWiresWidth(*cx, *cy, bot, poweredBefore, poweredAfter, 2 * POS_WIDTH);
            *cx += 2 * POS_WIDTH;
            break;
        }
        case ELEM_PULSER: {
            ElemPulser *m = &leaf->d.pulser;

            sprintf(s2, "%s", m->counter);
            sprintf(s3, "%s", m->busy);
            formatWidth(top,
                        2 * POS_WIDTH,
                        "[\x01"
                        "PULSER\x02 ",
                        s2,
                        "",
                        s3,
                        "]->");

            sprintf(s2, "%s %s %s", m->P1, m->P0, m->accel);
            formatWidth(bot, 2 * POS_WIDTH, "[", "", s2, "", "]--");

            CenterWithSpacesWidth(*cx, *cy, top, poweredAfter, false, 2 * POS_WIDTH);
            CenterWithWiresWidth(*cx, *cy, bot, poweredBefore, poweredAfter, 2 * POS_WIDTH);
            *cx += 2 * POS_WIDTH;
            break;
        }
        case ELEM_NPULSE: {
            ElemNPulse *m = &leaf->d.Npulse;

            sprintf(s2, "%s", m->counter);
            sprintf(s3, "%s", m->coil);
            formatWidth(top,
                        2 * POS_WIDTH,
                        "[\x01"
                        "N PULSE\x02 ",
                        s2,
                        "",
                        s3,
                        "]->");

            double m_targetFreq = SIprefix(hobatoi(m->targetFreq), s1);
            sprintf(s2, "%.6g %sHz", m_targetFreq, s1);
            formatWidth(bot, 2 * POS_WIDTH, "[", "", s2, "", "]--");

            CenterWithSpacesWidth(*cx, *cy, top, poweredAfter, false, 2 * POS_WIDTH);
            CenterWithWiresWidth(*cx, *cy, bot, poweredBefore, poweredAfter, 2 * POS_WIDTH);
            *cx += 2 * POS_WIDTH;
            break;
        }
        case ELEM_CTR:
        case ELEM_CTC: {
            const char *s;
            if(which == ELEM_CTC)
                s = "\x01"
                    "CTC\x02 ";
            else if(which == ELEM_CTR)
                s = "\x01"
                    "CTR\x02 ";
            else
                oops();

            ElemCounter *c = &leaf->d.counter;
            sprintf(s2, "%s", c->name);
            CenterWithSpaces(*cx, *cy, formatWidth(top, POS_WIDTH, "", "", s2, "", ""), poweredAfter, true);

            sprintf(s1, "%s:%s", c->init, c->max);
            int l = strlen(s1);
            if(l > POS_WIDTH - 7)
                l = POS_WIDTH - 7;
            formatWidth(s2, l, "", "", s1, "", "");
            /*
            if(which == ELEM_CTC)
                sprintf(bot,
                        "[\x01"
                        "CTC\x02 %s]",
                        s2);
            else
                sprintf(bot,
                        "[\x01"
                        "CTR\x02 %s]",
                        s2);
            */
            sprintf(bot, "%c[%s %s]", c->inputKind, s, s2);
            CenterWithWires(*cx, *cy, bot, poweredBefore, poweredAfter);

            *cx += POS_WIDTH;
            break;
        }
        case ELEM_CTU:
        case ELEM_CTD: {
            const char *s;
            if(which == ELEM_CTU)
                s = "\x01"
                    "CTU\x02";
            else if(which == ELEM_CTD)
                s = "\x01"
                    "CTD\x02";
            else
                oops();

            ElemCounter *c = &leaf->d.counter;
            sprintf(s2, "%s:%s", c->name, c->init);
            CenterWithSpaces(*cx, *cy, formatWidth(top, POS_WIDTH, "", "", s2, "", ""), poweredAfter, true);

            int l = strlen(c->max);
            if(which == ELEM_CTD) {
                if(l > POS_WIDTH - 7)
                    l = POS_WIDTH - 7;
            } else {
                if(l > POS_WIDTH - 8)
                    l = POS_WIDTH - 8;
            }
            formatWidth(s2, l, "", "", c->max, "", "");
            if(which == ELEM_CTD)
                sprintf(bot, "%c[%s>%s]", c->inputKind, s, s2);
            else
                sprintf(bot, "%c[%s>=%s]", c->inputKind, s, s2);
            CenterWithWires(*cx, *cy, bot, poweredBefore, poweredAfter);

            *cx += POS_WIDTH;
            break;
        }
        case ELEM_TIME2DELAY:
        case ELEM_TIME2COUNT:
        case ELEM_TCY:
        case ELEM_RTL:
        case ELEM_RTO:
        case ELEM_THI:
        case ELEM_TLO:
        case ELEM_TON:
        case ELEM_TOF: {
            const char *s0;
            const char *s;
            // clang-format off
            if(which == ELEM_TON) {
                s0 = "";
                s = "\x01""TON\x02";
            } else if(which == ELEM_TOF) {
                s0 = "";
                s = "\x01""TOF\x02";
            } else if(which == ELEM_THI) {
                s0 = "";
                s = "\x01""THI\x02";
            } else if(which == ELEM_TLO) {
                s0 = "o";
                s = "\x01""TLO\x02";
            } else if(which == ELEM_RTO) {
                s0 = "";
                s = "\x01""RTO\x02";
            } else if(which == ELEM_RTL) {
                s0 = "o";
                s = "\x01""RTL\x02";
            } else if(which == ELEM_TCY) {
                s0 = "";
                s = "\x01""TCY\x02";
            } else if(which == ELEM_TIME2COUNT) {
                s0 = "";
                s = "\x01""T2CNT\x02";
            } else if(which == ELEM_TIME2DELAY) {
                s = "\x01""T2DLY\x02";
            } else
                oops();
            // clang-format on

            ElemTimer *t = &leaf->d.timer;
            CenterWithSpaces(*cx, *cy, t->name, poweredAfter, true);

            //s0 = ""; // restore to original
            if(IsNumber(t->delay)) {
              double d = SIprefix(hobatoi(t->delay) / 1000000.0, s2);
              sprintf(bot, "%s[%s %.6g %ss]", s0, s, d, s2);
            } else {
              sprintf(bot, "%s[%s %s]", s0, s, t->delay);
            }
            CenterWithWires(*cx, *cy, bot, poweredBefore, poweredAfter);

            *cx += POS_WIDTH;
            break;
        }
        case ELEM_RANDOM:
            CenterWithSpaces(
                *cx, *cy, formatWidth(top, POS_WIDTH, "", "", leaf->d.readAdc.name, "", ""), poweredAfter, true);
            CenterWithWires(*cx, *cy, "{RAND}", poweredBefore, poweredAfter);
            *cx += POS_WIDTH;
            break;

        case ELEM_DELAY: {
            ElemTimer *t = &leaf->d.timer;
            if(IsNumber(t->name))
                sprintf(s1, "%s us", t->name);
            else
                sprintf(s1, "%s", t->name);
            CenterWithSpaces(*cx, *cy, formatWidth(top, POS_WIDTH, "", "", s1, "", ""), poweredAfter, true);
            CenterWithWires(*cx, *cy, "[DELAY]", poweredBefore, poweredAfter);
            *cx += POS_WIDTH;
            break;
        }

        case ELEM_LABEL: {
            CenterWithSpaces(
                *cx, *cy, formatWidth(top, POS_WIDTH, "", "", leaf->d.doGoto.label, "", ""), poweredAfter, true);
            CenterWithWires(*cx, *cy, "[LABEL]", poweredBefore, poweredAfter);
            *cx += POS_WIDTH;
            break;
        }

        case ELEM_SUBPROG: {
            CenterWithSpaces(
                *cx, *cy, formatWidth(top, POS_WIDTH, "", "", leaf->d.doGoto.label, "", ""), poweredAfter, true);
            CenterWithWires(*cx, *cy, "[SUBPROG]", poweredBefore, poweredAfter);
            *cx += POS_WIDTH;
            break;
        }

        case ELEM_ENDSUB: {
            CenterWithSpaces(
                *cx, *cy, formatWidth(top, POS_WIDTH, "", "", leaf->d.doGoto.label, "", ""), poweredAfter, true);
            CenterWithWires(*cx, *cy, "[ENDSUB]", poweredBefore, poweredAfter);
            *cx += POS_WIDTH;
            break;
        }

        case ELEM_SEED_RANDOM: {
            ElemMove *m = &leaf->d.move;
            formatWidth(top,
                        POS_WIDTH,
                        "{\x01"
                        "SRAND\x02 ",
                        "",
                        "",
                        m->dest,
                        "}");
            formatWidth(bot, POS_WIDTH, "{", "$seed", ":=", m->src, "}");

            CenterWithSpaces(*cx, *cy, top, poweredAfter, false);
            CenterWithWires(*cx, *cy, bot, poweredBefore, poweredAfter);
            *cx += POS_WIDTH;
            break;
        }

        case ELEM_STRING: {
            // Careful, string could be longer than fits in our space.
            sprintf(s1, "%s", leaf->d.fmtdStr.dest);
            formatWidth(top, 2 * POS_WIDTH, "{", "", s1, "", ":=}");

            sprintf(s1, "\"%s\",", leaf->d.fmtdStr.string);
            sprintf(s2, "%s", leaf->d.fmtdStr.var);
            formatWidth(bot, 2 * POS_WIDTH, "{", s1, "", s2, "}");

            CenterWithSpacesWidth(*cx, *cy, top, poweredBefore, true, 2 * POS_WIDTH);
            CenterWithWiresWidth(*cx, *cy, bot, poweredBefore, poweredAfter, 2 * POS_WIDTH);
            *cx += 2 * POS_WIDTH;
            break;
        }

        // clang-format off
        case ELEM_CPRINTF:      s = "CPRN"; goto cprintf;
        case ELEM_SPRINTF:      s = "SPRN"; goto cprintf;
        case ELEM_FPRINTF:      s = "FPRN"; goto cprintf;
        case ELEM_PRINTF:       s = "PRN";  goto cprintf;
        case ELEM_I2C_CPRINTF:  s = "I2C";  goto cprintf;
        case ELEM_ISP_CPRINTF:  s = "ISP";  goto cprintf;
        case ELEM_UART_CPRINTF: s = "UART"; goto cprintf; {
            // clang-format on
            cprintf:
                sprintf(s1, "->%s{", leaf->d.fmtdStr.enable);
                sprintf(s2, "%s %s:=", s, leaf->d.fmtdStr.dest);
                sprintf(s3, "}%s->", leaf->d.fmtdStr.error);

                formatWidth(top, 2 * POS_WIDTH, s1, "", s2, "", s3);

                sprintf(s1, "\"%s\",", leaf->d.fmtdStr.string);
                sprintf(s2, "%s", leaf->d.fmtdStr.var);
                formatWidth(bot, 2 * POS_WIDTH, "{", s1, "", s2, "}");

                CenterWithSpacesWidth(*cx, *cy, top, poweredBefore, true, 2 * POS_WIDTH);
                CenterWithWiresWidth(*cx, *cy, bot, poweredBefore, poweredAfter, 2 * POS_WIDTH);
                *cx += 2 * POS_WIDTH;
                break;
            }

        case ELEM_FORMATTED_STRING: {
            // Careful, string could be longer than fits in our space.
            char str[POS_WIDTH * 2];
            memset(str, 0, sizeof(str));
            char *srcStr = leaf->d.fmtdStr.string;
            memcpy(str, srcStr, std::min(strlen(srcStr), static_cast<size_t>(POS_WIDTH * 2 - 7)));

            sprintf(bot, "{\"%s\"}", str);

            PoweredText(poweredAfter);
            NameText();
            DrawChars(
                *cx, *cy + (POS_HEIGHT / 2) - 1, formatWidth(top, 2 * POS_WIDTH, "", "", leaf->d.fmtdStr.var, "", ""));
            BodyText();

            CenterWithWiresWidth(*cx, *cy, bot, poweredBefore, poweredAfter, 2 * POS_WIDTH);
            *cx += 2 * POS_WIDTH;
            break;
        }
        case ELEM_UART_RECV:
        case ELEM_UART_SEND:
            CenterWithWires(
                *cx, *cy, (which == ELEM_UART_RECV) ? "{UART RECV}" : "{UART SEND}", poweredBefore, poweredAfter);

            sprintf(s2, "%s", leaf->d.uart.name);
            CenterWithSpaces(*cx, *cy, formatWidth(top, POS_WIDTH, "", "", s2, "", ""), poweredAfter, true);
            *cx += POS_WIDTH;
            break;

        case ELEM_UART_RECVn:
        case ELEM_UART_SENDn:
            CenterWithWires(
                *cx, *cy, (which == ELEM_UART_RECVn) ? "{UART RECVn}" : "{UART SENDn}", poweredBefore, poweredAfter);

            sprintf(s2, "%s", leaf->d.uart.name);
            CenterWithSpaces(*cx, *cy, formatWidth(top, POS_WIDTH, "", "", s2, "", ""), poweredAfter, true);
            *cx += POS_WIDTH;
            break;

        case ELEM_UART_SEND_READY:
            CenterWithSpaces(*cx, *cy, " Is ready? ", poweredAfter, false);
            CenterWithWires(*cx, *cy, "[UART SEND]", poweredBefore, poweredAfter);
            *cx += POS_WIDTH;
            break;

        case ELEM_UART_RECV_AVAIL:
            CenterWithSpaces(*cx, *cy, " Is avail? ", poweredAfter, false);
            CenterWithWires(*cx, *cy, "[UART RECV]", poweredBefore, poweredAfter);
            *cx += POS_WIDTH;
            break;

        case ELEM_SPI: {
            ElemSpi *m = &leaf->d.spi;
            formatWidth(top,
                        POS_WIDTH,
                        "{\x01"
                        "SPI\x02 ",
                        "",
                        "",
                        m->name,
                        "}");
            formatWidth(bot, POS_WIDTH, "{->", m->recv, "", m->send, "->}");

            CenterWithSpaces(*cx, *cy, top, poweredAfter, false);
            CenterWithWires(*cx, *cy, bot, poweredBefore, poweredAfter);
            *cx += POS_WIDTH;
            break;
        }

        ///// Created by JG
        case ELEM_SPI_WR: {
            ElemSpi *m = &leaf->d.spi;
            formatWidth(top,
                        POS_WIDTH,
                        "{\x01"
                        "SPI_WR\x02 ",
                        "",
                        "",
                        m->name,
                        "}");
            formatWidth(bot, POS_WIDTH, "{", "", "\"", m->send, "\"->}");

            CenterWithSpaces(*cx, *cy, top, poweredAfter, false);
            CenterWithWires(*cx, *cy, bot, poweredBefore, poweredAfter);
            *cx += POS_WIDTH;
            break;
        }

        case ELEM_I2C_RD: {
            ElemI2c *m = &leaf->d.i2c;
            formatWidth(top,
                        POS_WIDTH,
                        "{\x01"
                        "I2C_RD\x02 ",
                        "",
                        "",
                        m->name,
                        "}");
            formatWidth(bot, POS_WIDTH, "{->", m->recv, m->address, m->registr, "}");

            CenterWithSpaces(*cx, *cy, top, poweredAfter, false);
            CenterWithWires(*cx, *cy, bot, poweredBefore, poweredAfter);
            *cx += POS_WIDTH;
            break;
        }

        case ELEM_I2C_WR: {
            ElemI2c *m = &leaf->d.i2c;
            formatWidth(top,
                        POS_WIDTH,
                        "{\x01"
                        "I2C_WR\x02 ",
                        "",
                        "",
                        m->name,
                        "}");
            formatWidth(bot, POS_WIDTH, "{", m->address, m->registr, m->send, "->}");

            CenterWithSpaces(*cx, *cy, top, poweredAfter, false);
            CenterWithWires(*cx, *cy, bot, poweredBefore, poweredAfter);
            *cx += POS_WIDTH;
            break;
        }
        /////

        case ELEM_BUS: {
            ElemBus *m = &leaf->d.bus;
            formatWidth(top,
                        POS_WIDTH,
                        "{\x01"
                        "BUS\x02 ",
                        "",
                        "",
                        m->dest,
                        ":=}");
            formatWidth(bot, POS_WIDTH, "{", "", "", m->src, "}");

            CenterWithSpaces(*cx, *cy, top, poweredAfter, false);
            CenterWithWires(*cx, *cy, bot, poweredBefore, poweredAfter);
            *cx += POS_WIDTH;
            break;
        }

            {
                // clang-format off
        const char *s;
        case ELEM_7SEG:  s = "7";  goto xseg;
        case ELEM_9SEG:  s = "9";  goto xseg;
        case ELEM_14SEG: s = "14"; goto xseg;
        case ELEM_16SEG: s = "16"; goto xseg;
                // clang-format on
                xseg:
                    ElemSegments *m = &leaf->d.segments;
                    if(m->common == COMMON_CATHODE)
                        sprintf(s3, "C");
                    else
                        sprintf(s3, "A");
                    sprintf(s2,
                            "{\x01"
                            "%sSEG\x02 ",
                            s);
                    formatWidth(top, POS_WIDTH, s2, "", "", m->dest, ":=}");
                    formatWidth(bot, POS_WIDTH, "{", s3, "", m->src, "}");

                    CenterWithSpaces(*cx, *cy, top, poweredAfter, false);
                    CenterWithWires(*cx, *cy, bot, poweredBefore, poweredAfter);
                    *cx += POS_WIDTH;
                    break;
            }

        default:
            poweredAfter = DrawEndOfLine(which, leaf, cx, cy, poweredBefore);
            break;
    }

    // And now we can enter the element into the display matrix so that the
    // UI routines know what element is at position (gx, gy) when the user
    // clicks there, and so that we know where to put the cursor if this
    // element is selected.

    // Don't use original cx0, as an end of line element might be further
    // along than that.
    cx0 = *cx - POS_WIDTH;

    int gx = cx0 / POS_WIDTH;
    int gy = cy0 / POS_HEIGHT;
    if(CheckBoundsUndoIfFails(gx, gy))
        return false;
    DM_BOUNDS(gx, gy);

    DisplayMatrix[gx][gy].data.leaf = leaf;
    DisplayMatrix[gx][gy].which = which;

    int xadj = 0;
    switch(which) {
        case ELEM_QUAD_ENCOD:
        case ELEM_NPULSE:
        case ELEM_PULSER:
        case ELEM_STEPPER:
        case ELEM_STRING:
        case ELEM_CPRINTF:
        case ELEM_SPRINTF:
        case ELEM_FPRINTF:
        case ELEM_PRINTF:
        case ELEM_I2C_CPRINTF:
        case ELEM_ISP_CPRINTF:
        case ELEM_UART_CPRINTF:
        case ELEM_FORMATTED_STRING:
            DM_BOUNDS(gx - 1, gy);
            DisplayMatrix[gx - 1][gy].data.leaf = leaf;
            DisplayMatrix[gx - 1][gy].which = which;
            xadj = POS_WIDTH * FONT_WIDTH;
            break;
    }

    if(which == ELEM_COMMENT) {
        int len = 0;
        for(int i = 0; i < ColsAvailable; i++) {
            if((DisplayMatrix[i][gy].which <= ELEM_PLACEHOLDER) || true // 2.3
               || (DisplayMatrix[i][gy].which == ELEM_COMMENT)) {
                DisplayMatrix[i][gy].data.leaf = leaf;
                DisplayMatrix[i][gy].which = ELEM_COMMENT;
                len++;
            }
        }
        //xadj = (ColsAvailable-1)*POS_WIDTH*FONT_WIDTH;
        xadj = (len - 1) * POS_WIDTH * FONT_WIDTH;
    }

    int x0 = X_PADDING + cx0 * FONT_WIDTH;
    int y0 = Y_PADDING + cy0 * FONT_HEIGHT;

    if((leaf->selectedState != SELECTED_NONE) && (leaf == Selected.data.leaf)) {
        SelectionActive = true;
    }
    switch(leaf->selectedState) {
        case SELECTED_LEFT:
            Cursor.left = x0 + FONT_WIDTH - 7 - xadj;
            Cursor.top = y0 - FONT_HEIGHT / 2;
            Cursor.width = 2;
            Cursor.height = POS_HEIGHT * FONT_HEIGHT;
            break;

        case SELECTED_RIGHT:
            Cursor.left = x0 + (POS_WIDTH - 1) * FONT_WIDTH - 2;
            Cursor.top = y0 - FONT_HEIGHT / 2;
            Cursor.width = 2;
            Cursor.height = POS_HEIGHT * FONT_HEIGHT;
            break;

        case SELECTED_ABOVE:
            Cursor.left = x0 + FONT_WIDTH / 2 - xadj;
            Cursor.top = y0 - 3;
            Cursor.width = (POS_WIDTH - 2) * FONT_WIDTH + xadj;
            Cursor.height = 2;
            break;

        case SELECTED_BELOW:
            Cursor.left = x0 + FONT_WIDTH / 2 - xadj;
            Cursor.top = y0 + (POS_HEIGHT - 1) * FONT_HEIGHT + FONT_HEIGHT / 2 - 2;
            Cursor.width = (POS_WIDTH - 2) * (FONT_WIDTH) + xadj;
            Cursor.height = 2;
            break;

        default:
            break;
    }

    return poweredAfter;
}

//-----------------------------------------------------------------------------
static bool HasEndOfRungElem(int which, void *elem)
{
    // ElemLeaf *leaf = (ElemLeaf *)elem;
    switch(which) {
        case ELEM_SERIES_SUBCKT: {
            ElemSubcktSeries *s = (ElemSubcktSeries *)elem;
            return HasEndOfRungElem(s->contents[s->count - 1].which, s->contents[s->count - 1].data.any);
            break;
        }
        case ELEM_PARALLEL_SUBCKT: {
            ElemSubcktParallel *p = (ElemSubcktParallel *)elem;
            for(int i = 0; i < p->count; i++) {
                if(HasEndOfRungElem(p->contents[i].which, p->contents[i].data.any))
                    return true;
            }
            break;
        }
        default:
            return EndOfRungElem(which);
            break;
    }
    return false;
}

//-----------------------------------------------------------------------------
// Draw a particular subcircuit with its top left corner at *cx and *cy (in
// characters). If it is a leaf element then just print it and return; else
// loop over the elements of the subcircuit and call ourselves recursively.
// At the end updates *cx and *cy.
//
// In simulation mode, returns true the circuit is energized after the given
// element, else false. This is needed to colour all the wires correctly,
// since the colouring indicates whether a wire is energized.
//-----------------------------------------------------------------------------
bool DrawElement(int which, void *elem, int *cx, int *cy, bool poweredBefore/*, int cols*/)
{
    bool poweredAfter;

    int       cx0 = *cx, cy0 = *cy;
    ElemLeaf *leaf = (ElemLeaf *)elem;

    SetBkColor(Hdc, InSimulationMode ? HighlightColours.simBg : HighlightColours.bg);
    NormText();

    if((leaf == Selected.data.leaf) && (!InSimulationMode)) {
        EmphText();
        ThisHighlighted = true;
    } else {
        ThisHighlighted = false;
    }

    switch(which) {
        case ELEM_SERIES_SUBCKT: {
            ElemSubcktSeries *s = (ElemSubcktSeries *)elem;
            poweredAfter = poweredBefore;
            for(int i = 0; i < s->count; i++) {
                poweredAfter = DrawElement(s->contents[i].which, s->contents[i].data.any, cx, cy, poweredAfter/*, 0*/);
            }
            break;
			/*
            if(cols) {
                // Draw wire to the right bus
                if((s->contents[s->count - 1].which == ELEM_COMMENT) || //
                   (s->contents[s->count - 1].which == ELEM_PLACEHOLDER))
                    break;
                if(HasEndOfRungElem(s->contents[s->count - 1].which, s->contents[s->count - 1].data.any))
                    break;
                int width = CountWidthOfElement(which, elem, (*cx) / POS_WIDTH);
                for(int i = width; i < cols; i++)
                    DrawWire(cx, cy, '-');
            }
            break;
			*/
        }
        case ELEM_PARALLEL_SUBCKT: {
            ElemSubcktParallel *p = (ElemSubcktParallel *)elem;
            int                 widthMax = CountWidthOfElement(which, elem, (*cx) / POS_WIDTH);
            int                 heightMax = CountHeightOfElement(which, elem);

            poweredAfter = false;

            int lowestPowered = -1;
            int downBy = 0;
            for(int i = 0; i < p->count; i++) {
                bool poweredThis;

                poweredThis = DrawElement(p->contents[i].which, p->contents[i].data.any, cx, cy, poweredBefore/*, 0*/);

                if(InSimulationMode) {
                    if(poweredThis)
                        poweredAfter = true;
                    PoweredText(poweredThis);
                }

                while((*cx - cx0) < widthMax * POS_WIDTH) {
                    int gx = *cx / POS_WIDTH;
                    int gy = *cy / POS_HEIGHT;

                    if(CheckBoundsUndoIfFails(gx, gy))
                        return false;

                    DM_BOUNDS(gx, gy);
                    DisplayMatrix[gx][gy].data.leaf = PADDING_IN_DISPLAY_MATRIX;
                    DisplayMatrix[gx][gy].which = ELEM_PADDING;

                    DrawWire(cx, cy, '-');
                }

                *cx = cx0;
                int justDrewHeight = CountHeightOfElement(p->contents[i].which, p->contents[i].data.any);
                *cy += POS_HEIGHT * justDrewHeight;

                downBy += justDrewHeight;
                if(poweredThis) {
                    lowestPowered = downBy - 1;
                }
            }
            *cx = cx0 + POS_WIDTH * widthMax;
            *cy = cy0;

            bool needWire;

            if(*cx / POS_WIDTH != ColsAvailable) {
                needWire = false;
                for(int j = heightMax - 1; j >= 1; j--) {
                    if(j <= lowestPowered)
                        PoweredText(poweredAfter);
                    if(DisplayMatrix[*cx / POS_WIDTH - 1][*cy / POS_HEIGHT + j].any()) {
                        needWire = true;
                    }
                    if(needWire)
                        VerticalWire(*cx - 1, *cy + j * POS_HEIGHT);
                }
                // stupid special case
                if(lowestPowered == 0 && InSimulationMode) {
                    EmphText();
                    DrawChars(*cx - 1, *cy + (POS_HEIGHT / 2), "+");
                }
            }

            PoweredText(poweredBefore);
            needWire = false;
            for(int j = heightMax - 1; j >= 1; j--) {
                if(DisplayMatrix[cx0 / POS_WIDTH][*cy / POS_HEIGHT + j].any()) {
                    needWire = true;
                }
                if(needWire)
                    VerticalWire(cx0 - 1, *cy + j * POS_HEIGHT);
            }

            break;
        }
        default:
            poweredAfter = DrawLeaf(which, leaf, cx, cy, poweredBefore);
            break;
    }

    NormText();
    return poweredAfter;
}

//-----------------------------------------------------------------------------
// Draw the rung that signals the end of the program. Kind of useless but
// do it anyways, for convention.
//-----------------------------------------------------------------------------
void DrawEndRung(int cx, int cy)
{
    CenterWithWires(cx, cy, "[END]", false, false);
    cx += POS_WIDTH;
    for(int i = 1; i < ColsAvailable; i++)
        DrawWire(&cx, &cy, '-');
}
