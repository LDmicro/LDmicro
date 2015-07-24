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
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "ldmicro.h"

// Number of drawing columns (leaf element units) available. We want to
// know this so that we can right-justify the coils.
int ColsAvailable;

// Set when we draw the selected element in the program. If there is no
// element selected then we ought to put the cursor at the top left of
// the screen.
BOOL SelectionActive;

// Is the element currently being drawn highlighted because it is selected?
// If so we must not do further syntax highlighting.
BOOL ThisHighlighted;

#define TOO_LONG _("!!!too long!!!")

#define DM_BOUNDS(gx, gy) { \
        if((gx) >= DISPLAY_MATRIX_X_SIZE || (gx) < 0) oops(); \
        if((gy) >= DISPLAY_MATRIX_Y_SIZE || (gy) < 0) oops(); \
    }

//-----------------------------------------------------------------------------
// The display code is the only part of the program that knows how wide a
// rung will be when it's displayed; so this is the only convenient place to
// warn the user and undo their changes if they created something too wide.
// This is not very clean.
//-----------------------------------------------------------------------------
static BOOL CheckBoundsUndoIfFails(int gx, int gy)
{
    if(gx >= DISPLAY_MATRIX_X_SIZE || gx < 0 ||
       gy >= DISPLAY_MATRIX_Y_SIZE || gy < 0)
    {
        if(CanUndo()) {
            UndoUndo();
            Error(_("Too many elements in subcircuit!"));
            return TRUE;
        }
    }
    return FALSE;
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
        case ELEM_PLACEHOLDER:
        case ELEM_OPEN:
        case ELEM_SHORT:
        case ELEM_CONTACTS:
        case ELEM_TON:
        case ELEM_TOF:
        case ELEM_RTO:
        case ELEM_CTU:
        case ELEM_CTD:
        case ELEM_ONE_SHOT_RISING:
        case ELEM_ONE_SHOT_FALLING:
        case ELEM_EQU:
        case ELEM_NEQ:
        case ELEM_GRT:
        case ELEM_GEQ:
        case ELEM_LES:
        case ELEM_LEQ:
        case ELEM_UART_RECV:
        case ELEM_UART_SEND:
            return 1;

        case ELEM_FORMATTED_STRING:
            return 2;

        case ELEM_COMMENT: {
            if(soFar != 0) oops();

            ElemLeaf *l = (ElemLeaf *)elem;
            char tbuf[MAX_COMMENT_LEN];

            strcpy(tbuf, l->d.comment.str);
            char *b = strchr(tbuf, '\n');

            int len;
            if(b) {
                *b = '\0';
                len = max(strlen(tbuf)-1, strlen(b+1));
            } else {
                len = strlen(tbuf);
            }
            // round up, and allow space for lead-in
            len = (len + 7 + (POS_WIDTH-1)) / POS_WIDTH;
            return max(ColsAvailable, len);
        }
        case ELEM_CTC:
        case ELEM_RES:
        case ELEM_COIL:
        case ELEM_MOVE:
        case ELEM_SHIFT_REGISTER:
        case ELEM_LOOK_UP_TABLE:
        case ELEM_PIECEWISE_LINEAR:
        case ELEM_MASTER_RELAY:
        case ELEM_READ_ADC:
        case ELEM_SET_PWM:
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
            if(ColsAvailable - soFar > 2) {
                return ColsAvailable - soFar;
            } else {
                return 2;
            }

        case ELEM_SERIES_SUBCKT: {
            // total of the width of the members
            int total = 0;
            int i;
            ElemSubcktSeries *s = (ElemSubcktSeries *)elem;
            for(i = 0; i < s->count; i++) {
                total += CountWidthOfElement(s->contents[i].which,
                    s->contents[i].d.any, total+soFar);
            }
            return total;
        }

        case ELEM_PARALLEL_SUBCKT: {
            // greatest of the width of the members
            int max = 0;
            int i;
            ElemSubcktParallel *p = (ElemSubcktParallel *)elem;
            for(i = 0; i < p->count; i++) {
                int w = CountWidthOfElement(p->contents[i].which,
                    p->contents[i].d.any, soFar);
                if(w > max) {
                    max = w;
                }
            }
            return max;
        }

        default:
            oops();
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
        CASE_LEAF
            return 1;

        case ELEM_PARALLEL_SUBCKT: {
            // total of the height of the members
            int total = 0;
            int i;
            ElemSubcktParallel *s = (ElemSubcktParallel *)elem;
            for(i = 0; i < s->count; i++) {
                total += CountHeightOfElement(s->contents[i].which,
                    s->contents[i].d.any);
            }
            return total;
        }

        case ELEM_SERIES_SUBCKT: {
            // greatest of the height of the members
            int max = 0;
            int i;
            ElemSubcktSeries *s = (ElemSubcktSeries *)elem;
            for(i = 0; i < s->count; i++) {
                int w = CountHeightOfElement(s->contents[i].which,
                    s->contents[i].d.any);
                if(w > max) {
                    max = w;
                }
            }
            return max;
        }

        default:
            oops();
    }
}

//-----------------------------------------------------------------------------
// Determine the width, in leaf element units, of the widest row of the PLC
// program (i.e. loop over all the rungs and find the widest).
//-----------------------------------------------------------------------------
int ProgCountWidestRow(void)
{
    int i;
    int max = 0;
    int colsTemp = ColsAvailable;
    ColsAvailable = 0;
    for(i = 0; i < Prog.numRungs; i++) {
        int w = CountWidthOfElement(ELEM_SERIES_SUBCKT, Prog.rungs[i], 0);
        if(w > max) {
            max = w;
        }
    }
    ColsAvailable = colsTemp;
    return max;
}

//-----------------------------------------------------------------------------
// Draw a vertical wire one leaf element unit high up from (cx, cy), where cx
// and cy are in charcter units.
//-----------------------------------------------------------------------------
static void VerticalWire(int cx, int cy)
{
    int j;
    for(j = 1; j < POS_HEIGHT; j++) {
        DrawChars(cx, cy + (POS_HEIGHT/2 - j), "|");
    }
    DrawChars(cx, cy + (POS_HEIGHT/2), "+");
    DrawChars(cx, cy + (POS_HEIGHT/2 - POS_HEIGHT), "+");
}

//-----------------------------------------------------------------------------
// Convenience functions for making the text colors pretty, for DrawElement.
//-----------------------------------------------------------------------------
static void NormText(void)
{
    SetTextColor(Hdc, InSimulationMode ? HighlightColours.simOff :
        HighlightColours.def);
    SelectObject(Hdc, FixedWidthFont);
}
static void EmphText(void)
{
    SetTextColor(Hdc, InSimulationMode ? HighlightColours.simOn :
        HighlightColours.selected);
    SelectObject(Hdc, FixedWidthFontBold);
}
static void NameText(void)
{
    if(!InSimulationMode && !ThisHighlighted) {
        SetTextColor(Hdc, HighlightColours.name);
    }
}
static void BodyText(void)
{
    if(!InSimulationMode && !ThisHighlighted) {
        SetTextColor(Hdc, HighlightColours.def);
    }
}
static void PoweredText(BOOL powered)
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
static int FormattedStrlen(char *str)
{
    int l = 0;
    while(*str) {
        if(*str > 10) {
            l++;
        }
        str++;
    }
    return l;
}

//-----------------------------------------------------------------------------
// Draw a string, centred in the space of a single position, with spaces on
// the left and right. Draws on the upper line of the position.
//-----------------------------------------------------------------------------
static void CenterWithSpaces(int cx, int cy, char *str, BOOL powered,
    BOOL isName)
{
    int extra = POS_WIDTH - FormattedStrlen(str);
    PoweredText(powered);
    if(isName) NameText();
    DrawChars(cx + (extra/2), cy + (POS_HEIGHT/2) - 1, str);
    if(isName) BodyText();
}

//-----------------------------------------------------------------------------
// Like CenterWithWires, but for an arbitrary width position (e.g. for ADD
// and SUB, which are double-width).
//-----------------------------------------------------------------------------
static void CenterWithWiresWidth(int cx, int cy, char *str, BOOL before,
    BOOL after, int totalWidth)
{
    int extra = totalWidth - FormattedStrlen(str);

    PoweredText(after);
    DrawChars(cx + (extra/2), cy + (POS_HEIGHT/2), str);

    PoweredText(before);
    int i;
    for(i = 0; i < (extra/2); i++) {
        DrawChars(cx + i, cy + (POS_HEIGHT/2), "-");
    }
    PoweredText(after);
    for(i = FormattedStrlen(str)+(extra/2); i < totalWidth; i++) {
        DrawChars(cx + i, cy + (POS_HEIGHT/2), "-");
    }
}

//-----------------------------------------------------------------------------
// Draw a string, centred in the space of a single position, with en dashes on
// the left and right coloured according to the powered state. Draws on the
// middle line.
//-----------------------------------------------------------------------------
static void CenterWithWires(int cx, int cy, char *str, BOOL before, BOOL after)
{
    CenterWithWiresWidth(cx, cy, str, before, after, POS_WIDTH);
}

//-----------------------------------------------------------------------------
// Draw an end of line element (coil, RES, MOV, etc.). Special things about
// an end of line element: we must right-justify it.
//-----------------------------------------------------------------------------
static BOOL DrawEndOfLine(int which, ElemLeaf *leaf, int *cx, int *cy,
    BOOL poweredBefore)
{
    int cx0 = *cx, cy0 = *cy;

    BOOL poweredAfter = leaf->poweredAfter;

    int thisWidth;
    switch(which) {
        case ELEM_ADD:
        case ELEM_SUB:
        case ELEM_MUL:
        case ELEM_DIV:
            thisWidth = 2;
            break;

        default:
            thisWidth = 1;
            break;
    }

    NormText();
    PoweredText(poweredBefore);
    while(*cx < (ColsAvailable-thisWidth)*POS_WIDTH) {
        int gx = *cx/POS_WIDTH;
        int gy = *cy/POS_HEIGHT;

        if(CheckBoundsUndoIfFails(gx, gy)) return FALSE;

        if(gx >= DISPLAY_MATRIX_X_SIZE) oops();
        DM_BOUNDS(gx, gy);
        DisplayMatrix[gx][gy] = PADDING_IN_DISPLAY_MATRIX;
        DisplayMatrixWhich[gx][gy] = ELEM_PADDING;

        int i;
        for(i = 0; i < POS_WIDTH; i++) {
            DrawChars(*cx + i, *cy + (POS_HEIGHT/2), "-");
        }
        *cx += POS_WIDTH;
        cx0 += POS_WIDTH;
    }

    if(leaf == Selected && !InSimulationMode) {
        EmphText();
        ThisHighlighted = TRUE;
    } else {
        ThisHighlighted = FALSE;
    }

    switch(which) {
        case ELEM_CTC: {
            char buf[256];
            ElemCounter *c = &leaf->d.counter;
            sprintf(buf, "{\x01""CTC\x02 0:%d}", c->max);

            CenterWithSpaces(*cx, *cy, c->name, poweredAfter, TRUE);
            CenterWithWires(*cx, *cy, buf, poweredBefore, poweredAfter);
            break;
        }
        case ELEM_RES: {
            ElemReset *r = &leaf->d.reset;
            CenterWithSpaces(*cx, *cy, r->name, poweredAfter, TRUE);
            CenterWithWires(*cx, *cy, "{RES}", poweredBefore, poweredAfter);
            break;
        }
        case ELEM_READ_ADC: {
            ElemReadAdc *r = &leaf->d.readAdc;
            CenterWithSpaces(*cx, *cy, r->name, poweredAfter, TRUE);
            CenterWithWires(*cx, *cy, "{READ ADC}", poweredBefore,
                poweredAfter);
            break;
        }
        case ELEM_SET_PWM: {
            ElemSetPwm *s = &leaf->d.setPwm;
            CenterWithSpaces(*cx, *cy, s->name, poweredAfter, TRUE);
            char l[50];
            if(s->targetFreq >= 100000) {
                sprintf(l, "{PWM %d kHz}", (s->targetFreq+500)/1000);
            } else if(s->targetFreq >= 10000) {
                sprintf(l, "{PWM %.1f kHz}", s->targetFreq/1000.0);
            } else if(s->targetFreq >= 1000) {
                sprintf(l, "{PWM %.2f kHz}", s->targetFreq/1000.0);
            } else {
                sprintf(l, "{PWM %d Hz}", s->targetFreq);
            }
            CenterWithWires(*cx, *cy, l, poweredBefore,
                poweredAfter);
            break;
        }
        case ELEM_PERSIST:
            CenterWithSpaces(*cx, *cy, leaf->d.persist.var, poweredAfter, TRUE);
            CenterWithWires(*cx, *cy, "{PERSIST}", poweredBefore, poweredAfter);
            break;

        case ELEM_MOVE: {
            char top[256];
            char bot[256];
            ElemMove *m = &leaf->d.move;

            if((strlen(m->dest) > (POS_WIDTH - 9)) ||
               (strlen(m->src) > (POS_WIDTH - 9)))
            {
                CenterWithWires(*cx, *cy, TOO_LONG, poweredBefore,
                    poweredAfter);
                break;
            }

            strcpy(top, "{            }");
            memcpy(top+1, m->dest, strlen(m->dest));
            top[strlen(m->dest) + 3] = ':';
            top[strlen(m->dest) + 4] = '=';

            strcpy(bot, "{         \x01MOV\x02}");
            memcpy(bot+2, m->src, strlen(m->src));

            CenterWithSpaces(*cx, *cy, top, poweredAfter, FALSE);
            CenterWithWires(*cx, *cy, bot, poweredBefore, poweredAfter);
            break;
        }
        case ELEM_MASTER_RELAY:
            CenterWithWires(*cx, *cy, "{MASTER RLY}", poweredBefore,
                poweredAfter);
            break;

        case ELEM_SHIFT_REGISTER: {
            char bot[MAX_NAME_LEN+20];
            memset(bot, ' ', sizeof(bot));
            bot[0] = '{';
            sprintf(bot+2, "%s0..%d", leaf->d.shiftRegister.name,
                leaf->d.shiftRegister.stages-1);
            bot[strlen(bot)] = ' ';
            bot[13] = '}';
            bot[14] = '\0';
            CenterWithSpaces(*cx, *cy, "{\x01SHIFT REG\x02   }",
                poweredAfter, FALSE);
            CenterWithWires(*cx, *cy, bot, poweredBefore, poweredAfter);
            break;
        }
        case ELEM_PIECEWISE_LINEAR:
        case ELEM_LOOK_UP_TABLE: {
            char top[MAX_NAME_LEN+20], bot[MAX_NAME_LEN+20];
            char *dest, *index, *str;
            if(which == ELEM_PIECEWISE_LINEAR) {
                dest = leaf->d.piecewiseLinear.dest;
                index = leaf->d.piecewiseLinear.index;
                str = "PWL";
            } else {
                dest = leaf->d.lookUpTable.dest;
                index = leaf->d.lookUpTable.index;
                str = "LUT";
            }
            memset(top, ' ', sizeof(top));
            top[0] = '{';
            sprintf(top+2, "%s :=", dest);
            top[strlen(top)] = ' ';
            top[13] = '}';
            top[14] = '\0';
            CenterWithSpaces(*cx, *cy, top, poweredAfter, FALSE);
            memset(bot, ' ', sizeof(bot));
            bot[0] = '{';
            sprintf(bot+2, " %s[%s]", str, index);
            bot[strlen(bot)] = ' ';
            bot[13] = '}';
            bot[14] = '\0';
            CenterWithWires(*cx, *cy, bot, poweredBefore, poweredAfter);
            break;
        }
        case ELEM_COIL: {
            char buf[4];
            ElemCoil *c = &leaf->d.coil;

            buf[0] = '(';
            if(c->negated) {
                buf[1] = '/';
            } else if(c->setOnly) {
                buf[1] = 'S';
            } else if(c->resetOnly) {
                buf[1] = 'R';
            } else {
                buf[1] = ' ';
            }
            buf[2] = ')';
            buf[3] = '\0';

            CenterWithSpaces(*cx, *cy, c->name, poweredAfter, TRUE);
            CenterWithWires(*cx, *cy, buf, poweredBefore, poweredAfter);
            break;
        }
        case ELEM_DIV:
        case ELEM_MUL:
        case ELEM_SUB:
        case ELEM_ADD: {
            char top[POS_WIDTH*2-3+2];
            char bot[POS_WIDTH*2-3];

            memset(top, ' ', sizeof(top)-1);
            top[0] = '{';

            memset(bot, ' ', sizeof(bot)-1);
            bot[0] = '{';

            int lt = 1;
            if(which == ELEM_ADD) {
                memcpy(top+lt, "\x01""ADD\x02", 5);
            } else if(which == ELEM_SUB) {
                memcpy(top+lt, "\x01SUB\x02", 5);
            } else if(which == ELEM_MUL) {
                memcpy(top+lt, "\x01MUL\x02", 5);
            } else if(which == ELEM_DIV) {
                memcpy(top+lt, "\x01""DIV\x02", 5);
            } else oops();

            lt += 7;
            memcpy(top+lt, leaf->d.math.dest, strlen(leaf->d.math.dest));
            lt += strlen(leaf->d.math.dest) + 2;
            top[lt++] = ':';
            top[lt++] = '=';

            int lb = 2;
            memcpy(bot+lb, leaf->d.math.op1, strlen(leaf->d.math.op1));
            lb += strlen(leaf->d.math.op1) + 1;
            if(which == ELEM_ADD) {
                bot[lb++] = '+';
            } else if(which == ELEM_SUB) {
                bot[lb++] = '-';
            } else if(which == ELEM_MUL) {
                bot[lb++] = '*';
            } else if(which == ELEM_DIV) {
                bot[lb++] = '/';
            } else oops();
            lb++;
            memcpy(bot+lb, leaf->d.math.op2, strlen(leaf->d.math.op2));
            lb += strlen(leaf->d.math.op2);

            int l = max(lb, lt - 2);
            top[l+2] = '}'; top[l+3] = '\0';
            bot[l] = '}'; bot[l+1] = '\0';

            int extra = 2*POS_WIDTH - FormattedStrlen(top);
            PoweredText(poweredAfter);
            DrawChars(*cx + (extra/2), *cy + (POS_HEIGHT/2) - 1, top);
            CenterWithWiresWidth(*cx, *cy, bot, poweredBefore, poweredAfter,
                2*POS_WIDTH);

            *cx += POS_WIDTH;

            break;
        }
        default:
            oops();
            break;
    }

    *cx += POS_WIDTH;

    return poweredAfter;
}

//-----------------------------------------------------------------------------
// Draw a leaf element. Special things about a leaf: no need to recurse
// further, and we must put it into the display matrix.
//-----------------------------------------------------------------------------
static BOOL DrawLeaf(int which, ElemLeaf *leaf, int *cx, int *cy,
    BOOL poweredBefore)
{
    int cx0 = *cx, cy0 = *cy;
    BOOL poweredAfter = leaf->poweredAfter;

    switch(which) {
        case ELEM_COMMENT: {
            char tbuf[MAX_COMMENT_LEN];
            char tlbuf[MAX_COMMENT_LEN+8];

            strcpy(tbuf, leaf->d.comment.str);
            char *b = strchr(tbuf, '\n');

            if(b) {
                if(b[-1] == '\r') b[-1] = '\0';
                *b = '\0';
                sprintf(tlbuf, "\x03 ; %s\x02", tbuf);
                DrawChars(*cx, *cy + (POS_HEIGHT/2) - 1, tlbuf);
                sprintf(tlbuf, "\x03 ; %s\x02", b+1);
                DrawChars(*cx, *cy + (POS_HEIGHT/2), tlbuf);
            } else {
                sprintf(tlbuf, "\x03 ; %s\x02", tbuf);
                DrawChars(*cx, *cy + (POS_HEIGHT/2) - 1, tlbuf);
            }

            *cx += ColsAvailable*POS_WIDTH;
            break;
        }
        case ELEM_PLACEHOLDER: {
            NormText();
            CenterWithWiresWidth(*cx, *cy, "--", FALSE, FALSE, 2);
            *cx += POS_WIDTH;
            break;
        }
        case ELEM_CONTACTS: {
            char buf[4];
            ElemContacts *c = &leaf->d.contacts;

            buf[0] = ']';
            buf[1] = c->negated ? '/' : ' ';
            buf[2] = '[';
            buf[3] = '\0';

            CenterWithSpaces(*cx, *cy, c->name, poweredAfter, TRUE);
            CenterWithWires(*cx, *cy, buf, poweredBefore, poweredAfter);

            *cx += POS_WIDTH;
            break;
        }
        {
            char *s;
            case ELEM_EQU:
                s = "=="; goto cmp;
            case ELEM_NEQ:
                s = "/="; goto cmp;
            case ELEM_GRT:
                s = ">"; goto cmp;
            case ELEM_GEQ:
                s = ">="; goto cmp;
            case ELEM_LES:
                s = "<"; goto cmp;
            case ELEM_LEQ:
                s = "<="; goto cmp;
cmp:
                char s1[POS_WIDTH+10], s2[POS_WIDTH+10];
                int l1, l2, lmax;

                l1 = 2 + 1 + strlen(s) + strlen(leaf->d.cmp.op1);
                l2 = 2 + 1 + strlen(leaf->d.cmp.op2);
                lmax = max(l1, l2);

                if(lmax < POS_WIDTH) {
                    memset(s1, ' ', sizeof(s1));
                    s1[0] = '[';
                    s1[lmax-1] = ']';
                    s1[lmax] = '\0';
                    strcpy(s2, s1);
                    memcpy(s1+1, leaf->d.cmp.op1, strlen(leaf->d.cmp.op1));
                    memcpy(s1+strlen(leaf->d.cmp.op1)+2, s, strlen(s));
                    memcpy(s2+2, leaf->d.cmp.op2, strlen(leaf->d.cmp.op2));
                } else {
                    strcpy(s1, "");
                    strcpy(s2, TOO_LONG);
                }

                CenterWithSpaces(*cx, *cy, s1, poweredAfter, FALSE);
                CenterWithWires(*cx, *cy, s2, poweredBefore, poweredAfter);

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
        case ELEM_ONE_SHOT_FALLING: {
            char *s1, *s2;
            if(which == ELEM_ONE_SHOT_RISING) {
                s1 = "      _ ";
                s2 = "[\x01OSR\x02_/ ]";
            } else if(which == ELEM_ONE_SHOT_FALLING) {
                s1 = "    _   ";
                s2 = "[\x01OSF\x02 \\_]";
            } else oops();

            CenterWithSpaces(*cx, *cy, s1, poweredAfter, FALSE);
            CenterWithWires(*cx, *cy, s2, poweredBefore, poweredAfter);

            *cx += POS_WIDTH;
            break;
        }
        case ELEM_CTU:
        case ELEM_CTD: {
            char *s;
            if(which == ELEM_CTU)
                s = "\x01""CTU\x02";
            else if(which == ELEM_CTD)
                s = "\x01""CTD\x02";
            else oops();

            char buf[256];
            ElemCounter *c = &leaf->d.counter;
            sprintf(buf, "[%s >=%d]", s, c->max);

            CenterWithSpaces(*cx, *cy, c->name, poweredAfter, TRUE);
            CenterWithWires(*cx, *cy, buf, poweredBefore, poweredAfter);

            *cx += POS_WIDTH;
            break;
        }
        case ELEM_RTO:
        case ELEM_TON:
        case ELEM_TOF: {
            char *s;
            if(which == ELEM_TON)
                s = "\x01TON\x02";
            else if(which == ELEM_TOF)
                s = "\x01TOF\x02";
            else if(which == ELEM_RTO)
                s = "\x01RTO\x02";
            else oops();

            char buf[256];
            ElemTimer *t = &leaf->d.timer;
            if(t->delay >= 1000*1000) {
                sprintf(buf, "[%s %.3f s]", s, t->delay/1000000.0);
            } else if(t->delay >= 100*1000) {
                sprintf(buf, "[%s %.1f ms]", s, t->delay/1000.0);
            } else {
                sprintf(buf, "[%s %.2f ms]", s, t->delay/1000.0);
            }

            CenterWithSpaces(*cx, *cy, t->name, poweredAfter, TRUE);
            CenterWithWires(*cx, *cy, buf, poweredBefore, poweredAfter);

            *cx += POS_WIDTH;
            break;
        }
        case ELEM_FORMATTED_STRING: {
            // Careful, string could be longer than fits in our space.
            char str[POS_WIDTH*2];
            memset(str, 0, sizeof(str));
            char *srcStr = leaf->d.fmtdStr.string;
            memcpy(str, srcStr, min(strlen(srcStr), POS_WIDTH*2 - 7));

            char bot[100];
            sprintf(bot, "{\"%s\"}", str);

            int extra = 2*POS_WIDTH - strlen(leaf->d.fmtdStr.var);
            PoweredText(poweredAfter);
            NameText();
            DrawChars(*cx + (extra/2), *cy + (POS_HEIGHT/2) - 1,
                leaf->d.fmtdStr.var);
            BodyText();

            CenterWithWiresWidth(*cx, *cy, bot, poweredBefore, poweredAfter,
                2*POS_WIDTH);
            *cx += 2*POS_WIDTH;
            break;
        }
        case ELEM_UART_RECV:
        case ELEM_UART_SEND:
            CenterWithWires(*cx, *cy,
                (which == ELEM_UART_RECV) ? "{UART RECV}" : "{UART SEND}",
                poweredBefore, poweredAfter);
            CenterWithSpaces(*cx, *cy, leaf->d.uart.name, poweredAfter, TRUE);
            *cx += POS_WIDTH;
            break;

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

    int gx = cx0/POS_WIDTH;
    int gy = cy0/POS_HEIGHT;
    if(CheckBoundsUndoIfFails(gx, gy)) return FALSE;
    DM_BOUNDS(gx, gy);

    DisplayMatrix[gx][gy] = leaf;
    DisplayMatrixWhich[gx][gy] = which;

    int xadj = 0;
    switch(which) {
        case ELEM_ADD:
        case ELEM_SUB:
        case ELEM_MUL:
        case ELEM_DIV:
        case ELEM_FORMATTED_STRING:
            DM_BOUNDS(gx-1, gy);
            DisplayMatrix[gx-1][gy] = leaf;
            DisplayMatrixWhich[gx-1][gy] = which;
            xadj = POS_WIDTH*FONT_WIDTH;
            break;
    }

    if(which == ELEM_COMMENT) {
        int i;
        for(i = 0; i < ColsAvailable; i++) {
            DisplayMatrix[i][gy] = leaf;
            DisplayMatrixWhich[i][gy] = ELEM_COMMENT;
        }
        xadj = (ColsAvailable-1)*POS_WIDTH*FONT_WIDTH;
    }

    int x0 = X_PADDING + cx0*FONT_WIDTH;
    int y0 = Y_PADDING + cy0*FONT_HEIGHT;

    if(leaf->selectedState != SELECTED_NONE && leaf == Selected) {
        SelectionActive = TRUE;
    }
    switch(leaf->selectedState) {
        case SELECTED_LEFT:
            Cursor.left = x0 + FONT_WIDTH - 4 - xadj;
            Cursor.top = y0 - FONT_HEIGHT/2;
            Cursor.width = 2;
            Cursor.height = POS_HEIGHT*FONT_HEIGHT;
            break;

        case SELECTED_RIGHT:
            Cursor.left = x0 + (POS_WIDTH-1)*FONT_WIDTH - 5;
            Cursor.top = y0 - FONT_HEIGHT/2;
            Cursor.width = 2;
            Cursor.height = POS_HEIGHT*FONT_HEIGHT;
            break;

        case SELECTED_ABOVE:
            Cursor.left = x0 + FONT_WIDTH/2 - xadj;
            Cursor.top = y0 - 2;
            Cursor.width = (POS_WIDTH-2)*FONT_WIDTH + xadj;
            Cursor.height = 2;
            break;

        case SELECTED_BELOW:
            Cursor.left = x0 + FONT_WIDTH/2 - xadj;
            Cursor.top = y0 + (POS_HEIGHT-1)*FONT_HEIGHT +
                FONT_HEIGHT/2 - 2;
            Cursor.width = (POS_WIDTH-2)*(FONT_WIDTH) + xadj;
            Cursor.height = 2;
            break;

        default:
            break;
    }

    return poweredAfter;
}

//-----------------------------------------------------------------------------
// Draw a particular subcircuit with its top left corner at *cx and *cy (in
// characters). If it is a leaf element then just print it and return; else
// loop over the elements of the subcircuit and call ourselves recursively.
// At the end updates *cx and *cy.
//
// In simulation mode, returns TRUE the circuit is energized after the given
// element, else FALSE. This is needed to colour all the wires correctly,
// since the colouring indicates whether a wire is energized.
//-----------------------------------------------------------------------------
BOOL DrawElement(int which, void *elem, int *cx, int *cy, BOOL poweredBefore)
{
    BOOL poweredAfter;

    int cx0 = *cx, cy0 = *cy;
    ElemLeaf *leaf = (ElemLeaf *)elem;

    SetBkColor(Hdc, InSimulationMode ? HighlightColours.simBg :
        HighlightColours.bg);
    NormText();

    if(elem == Selected && !InSimulationMode) {
        EmphText();
        ThisHighlighted = TRUE;
    } else {
        ThisHighlighted = FALSE;
    }

    switch(which) {
        case ELEM_SERIES_SUBCKT: {
            int i;
            ElemSubcktSeries *s = (ElemSubcktSeries *)elem;
            poweredAfter = poweredBefore;
            for(i = 0; i < s->count; i++) {
                poweredAfter = DrawElement(s->contents[i].which,
                    s->contents[i].d.any, cx, cy, poweredAfter);
            }
            break;
        }
        case ELEM_PARALLEL_SUBCKT: {
            int i;
            ElemSubcktParallel *p = (ElemSubcktParallel *)elem;
            int widthMax = CountWidthOfElement(which, elem, (*cx)/POS_WIDTH);
            int heightMax = CountHeightOfElement(which, elem);

            poweredAfter = FALSE;

            int lowestPowered = -1;
            int downBy = 0;
            for(i = 0; i < p->count; i++) {
                BOOL poweredThis;

                poweredThis = DrawElement(p->contents[i].which,
                    p->contents[i].d.any, cx, cy, poweredBefore);

                if(InSimulationMode) {
                    if(poweredThis) poweredAfter = TRUE;
                    PoweredText(poweredThis);
                }

                while((*cx - cx0) < widthMax*POS_WIDTH) {
                    int gx = *cx/POS_WIDTH;
                    int gy = *cy/POS_HEIGHT;

                    if(CheckBoundsUndoIfFails(gx, gy)) return FALSE;

                    DM_BOUNDS(gx, gy);
                    DisplayMatrix[gx][gy] = PADDING_IN_DISPLAY_MATRIX;
                    DisplayMatrixWhich[gx][gy] = ELEM_PADDING;

                    char buf[256];
                    int j;
                    for(j = 0; j < POS_WIDTH; j++) {
                        buf[j] = '-';
                    }
                    buf[j] = '\0';
                    DrawChars(*cx, *cy + (POS_HEIGHT/2), buf);
                    *cx += POS_WIDTH;
                }

                *cx = cx0;
                int justDrewHeight = CountHeightOfElement(p->contents[i].which,
                    p->contents[i].d.any);
                *cy += POS_HEIGHT*justDrewHeight;

                downBy += justDrewHeight;
                if(poweredThis) {
                    lowestPowered = downBy - 1;
                }
            }
            *cx = cx0 + POS_WIDTH*widthMax;
            *cy = cy0;

            int j;
            BOOL needWire;

            if(*cx/POS_WIDTH != ColsAvailable) {
                needWire = FALSE;
                for(j = heightMax - 1; j >= 1; j--) {
                    if(j <= lowestPowered) PoweredText(poweredAfter);
                    if(DisplayMatrix[*cx/POS_WIDTH - 1][*cy/POS_HEIGHT + j]) {
                        needWire = TRUE;
                    }
                    if(needWire) VerticalWire(*cx - 1, *cy + j*POS_HEIGHT);
                }
                // stupid special case
                if(lowestPowered == 0 && InSimulationMode) {
                    EmphText();
                    DrawChars(*cx - 1, *cy + (POS_HEIGHT/2), "+");
                }
            }

            PoweredText(poweredBefore);
            needWire = FALSE;
            for(j = heightMax - 1; j >= 1; j--) {
                if(DisplayMatrix[cx0/POS_WIDTH][*cy/POS_HEIGHT + j]) {
                    needWire = TRUE;
                }
                if(needWire) VerticalWire(cx0 - 1, *cy + j*POS_HEIGHT);
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
    int i;
    char *str = "[END]";
    int lead = (POS_WIDTH - strlen(str))/2;
    ThisHighlighted = TRUE;
    for(i = 0; i < lead; i++) {
        DrawChars(cx + i, cy + (POS_HEIGHT/2), "-");
    }
    DrawChars(cx + i, cy + (POS_HEIGHT/2), str);
    i += strlen(str);
    for(; i < ColsAvailable*POS_WIDTH; i++) {
        DrawChars(cx + i, cy + (POS_HEIGHT/2), "-");
    }
}
