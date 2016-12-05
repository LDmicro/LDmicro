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
// Stuff for editing the schematic, mostly tying in with the cursor somehow.
// Actual manipulation of circuit elements happens in circuit.cpp, though.
// Jonathan Westhues, Oct 2004
//-----------------------------------------------------------------------------
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "ldmicro.h"

// Not all options all available e.g. can't delete the only relay coil in
// a rung, can't insert two coils in series, etc. Keep track of what is
// allowed so we don't corrupt our program.
BOOL CanInsertEnd;
BOOL CanInsertOther;
BOOL CanInsertComment;

// Ladder logic program is laid out on a grid program; this matrix tells
// us which leaf element is in which box on the grid, which allows us
// to determine what element has just been selected when the user clicks
// on something, for example.
ElemLeaf *DisplayMatrix[DISPLAY_MATRIX_X_SIZE][DISPLAY_MATRIX_Y_SIZE];
int DisplayMatrixWhich[DISPLAY_MATRIX_X_SIZE][DISPLAY_MATRIX_Y_SIZE];
ElemLeaf *Selected;
int SelectedWhich;

ElemLeaf DisplayMatrixFiller;

// Cursor within the ladder logic program. The paint routines figure out
// where the cursor should go and calculate the coordinates (in pixels)
// of the rectangle that describes it; then BlinkCursor just has to XOR
// the requested rectangle at periodic intervals.
PlcCursor Cursor;

//-----------------------------------------------------------------------------
// Find the address in the DisplayMatrix of the selected leaf element. Set
// *gx and *gy if we succeed and return TRUE, else return FALSE.
//-----------------------------------------------------------------------------
BOOL FindSelected(int *gx, int *gy)
{
    if(!Selected) return FALSE;
    int i, j;
    for(i = 0; i < DISPLAY_MATRIX_X_SIZE; i++) {
        for(j = 0; j < DISPLAY_MATRIX_Y_SIZE; j++) {
            if(DisplayMatrix[i][j] == Selected) {
              if(SelectedWhich != ELEM_COMMENT)
                while(DisplayMatrix[i+1][j] == Selected) {
                    i++;
                }
                *gx = i;
                *gy = j;
                return TRUE;
            }
        }
    }
    return FALSE;
}

//-----------------------------------------------------------------------------
// Select the item in DisplayMatrix at (gx, gy), and then update everything
// to reflect that. In particular change the enabled state of the menus so
// that those items that do not apply are grayed, and scroll the element in
// to view.
//-----------------------------------------------------------------------------
void SelectElement(int gx, int gy, int state)
{
    if(gx < 0 || gy < 0) {
        if(!FindSelected(&gx, &gy)) {
            return;
        }
    }

    if(Selected) Selected->selectedState = SELECTED_NONE;

    Selected = DisplayMatrix[gx][gy];
    SelectedWhich = DisplayMatrixWhich[gx][gy];

    if(SelectedWhich == ELEM_PLACEHOLDER) {
        state = SELECTED_LEFT;
    }

    if((gy - ScrollYOffset) >= ScreenRowsAvailable()) {
        ScrollYOffset = gy - ScreenRowsAvailable() + 1;
        RefreshScrollbars();
    }
    if((gy - ScrollYOffset) < 0) {
        ScrollYOffset = gy;
        RefreshScrollbars();
    }
    if((gx - ScrollXOffset*POS_WIDTH*FONT_WIDTH) >= ScreenColsAvailable()) {
        ScrollXOffset = gx*POS_WIDTH*FONT_WIDTH - ScreenColsAvailable();
        RefreshScrollbars();
    }
    if((gx - ScrollXOffset*POS_WIDTH*FONT_WIDTH) < 0) {
        ScrollXOffset = gx*POS_WIDTH*FONT_WIDTH;
        RefreshScrollbars();
    }

    ok();
    if(Selected) Selected->selectedState = state;
    ok();

    WhatCanWeDoFromCursorAndTopology();
}

//-----------------------------------------------------------------------------
BOOL StaySameElem(int Which)
{
    if( Which == ELEM_RES ||
        Which == ELEM_SET_BIT ||
        Which == ELEM_CLEAR_BIT ||
        Which == ELEM_SHL ||
        Which == ELEM_SHR ||
        Which == ELEM_SR0 ||
        Which == ELEM_ROL ||
        Which == ELEM_ROR ||
        Which == ELEM_AND ||
        Which == ELEM_OR  ||
        Which == ELEM_XOR ||
        Which == ELEM_NOT ||
        Which == ELEM_NEG ||
        Which == ELEM_ADD ||
        Which == ELEM_SUB ||
        Which == ELEM_MUL ||
        Which == ELEM_DIV ||
        Which == ELEM_MOD ||
        Which == ELEM_BIN2BCD ||
        Which == ELEM_BCD2BIN ||
        Which == ELEM_SWAP ||
        Which == ELEM_BUS  ||
        Which == ELEM_7SEG  ||
        Which == ELEM_9SEG  ||
        Which == ELEM_14SEG ||
        Which == ELEM_16SEG ||
        Which == ELEM_READ_ADC ||
        Which == ELEM_SET_PWM ||
        Which == ELEM_MASTER_RELAY ||
        Which == ELEM_SHIFT_REGISTER ||
        Which == ELEM_LOOK_UP_TABLE ||
        Which == ELEM_PIECEWISE_LINEAR ||
        Which == ELEM_PERSIST ||
        Which == ELEM_MOVE)
      return TRUE;
    else
      return FALSE;
}
//-----------------------------------------------------------------------------
BOOL CanChangeOutputElem(int Which)
{
    if( Which == ELEM_COIL ||
/*
        Which == ELEM_CTU ||
        Which == ELEM_CTD ||
        Which == ELEM_CTC ||
        Which == ELEM_CTR ||
*/
        Which == ELEM_SET_PWM ||
        Which == ELEM_STEPPER ||
        Which == ELEM_PULSER ||
        Which == ELEM_NPULSE ||
        Which == ELEM_MASTER_RELAY ||
        Which == ELEM_WSFR ||
        Which == ELEM_PERSIST)
      return TRUE;
    else
      return FALSE;
}
//-----------------------------------------------------------------------------
// Returnn TRUE if this instruction(element) must be the
// rightmost instruction in its rung.
//-----------------------------------------------------------------------------
BOOL EndOfRungElem(int Which)
{
    if( Which == ELEM_COIL ||
        Which == ELEM_RES ||
        Which == ELEM_MOD ||
        Which == ELEM_ADD ||
        Which == ELEM_SUB ||
        Which == ELEM_MUL ||
        Which == ELEM_DIV ||
        Which == ELEM_READ_ADC ||
        Which == ELEM_SET_PWM ||
        Which == ELEM_PWM_OFF ||
        Which == ELEM_NPULSE_OFF ||
        Which == ELEM_MASTER_RELAY ||
        Which == ELEM_SHIFT_REGISTER ||
        Which == ELEM_LOOK_UP_TABLE ||
        Which == ELEM_PIECEWISE_LINEAR ||
        Which == ELEM_PERSIST ||
        Which == ELEM_MOVE)
      return TRUE;
    return FALSE;
}

//-----------------------------------------------------------------------------
// Must be called every time the cursor moves or the cursor stays the same
// but the circuit topology changes under it. Determines what we are allowed
// to do: where coils can be added, etc.
//-----------------------------------------------------------------------------
void WhatCanWeDoFromCursorAndTopology(void)
{
    BOOL canNegate = FALSE, canNormal = FALSE;
    BOOL canResetOnly = FALSE, canSetOnly = FALSE, canTtrigger = FALSE;
    BOOL canPushUp = TRUE, canPushDown = TRUE;

    BOOL canDelete = TRUE;

    int i = RungContainingSelected();
    if(i >= 0) {
        if(i == 0) canPushUp = FALSE;
        if(i == (Prog.numRungs-1)) canPushDown = FALSE;

        if(Prog.rungs[i]->count == 1 &&
            Prog.rungs[i]->contents[0].which == ELEM_PLACEHOLDER)
        {
            canDelete = FALSE;
        }
    }

    CanInsertEnd = FALSE;
    CanInsertOther = TRUE;

    if(Selected && EndOfRungElem(SelectedWhich))
    {
        if(SelectedWhich == ELEM_COIL) {
            canNegate = TRUE;
            canNormal = TRUE;
            canResetOnly = TRUE;
            canSetOnly = TRUE;
            canTtrigger = TRUE;
        }

        if(Selected->selectedState == SELECTED_ABOVE ||
           Selected->selectedState == SELECTED_BELOW)
        {
            CanInsertEnd = TRUE;
            CanInsertOther = FALSE;
        } else if(Selected->selectedState == SELECTED_RIGHT) {
            CanInsertEnd = FALSE;
            CanInsertOther = FALSE;
        }
    } else if(Selected) {
        if(Selected->selectedState == SELECTED_RIGHT ||
            SelectedWhich == ELEM_PLACEHOLDER)
        {
            CanInsertEnd = ItemIsLastInCircuit(Selected);
        }
    }
    if(SelectedWhich == ELEM_CONTACTS) {
        canNegate = TRUE;
        canNormal = TRUE;
    }
    if(SelectedWhich == ELEM_PLACEHOLDER) {
        // a comment must be the only element in its rung, and it will fill
        // the rung entirely
        CanInsertComment = TRUE;
    } else {
        CanInsertComment = FALSE;
    }
    if(SelectedWhich == ELEM_COMMENT) {
        // if there's a comment there already then don't let anything else
        // into the rung
        CanInsertEnd = FALSE;
        CanInsertOther = FALSE;
    }
    SetMenusEnabled(canNegate, canNormal, canResetOnly, canSetOnly, canDelete,
        CanInsertEnd, CanInsertOther, canPushDown, canPushUp, CanInsertComment);
}

//-----------------------------------------------------------------------------
// Rub out freed element from the DisplayMatrix, just so we don't confuse
// ourselves too much (or access freed memory)...
//-----------------------------------------------------------------------------
void ForgetFromGrid(void *p)
{
    int i, j;
    for(i = 0; i < DISPLAY_MATRIX_X_SIZE; i++) {
        for(j = 0; j < DISPLAY_MATRIX_Y_SIZE; j++) {
            if(DisplayMatrix[i][j] == p) {
                DisplayMatrix[i][j] = NULL;
            }
        }
    }
    if(Selected == p) {
        Selected = NULL;
    }
}

//-----------------------------------------------------------------------------
// Rub out everything from DisplayMatrix. If we don't do that before freeing
// the program (e.g. when loading a new file) then there is a race condition
// when we repaint.
//-----------------------------------------------------------------------------
void ForgetEverything(void)
{
    memset(DisplayMatrix, 0, sizeof(DisplayMatrix));
    memset(DisplayMatrixWhich, 0, sizeof(DisplayMatrixWhich));
    Selected = NULL;
    SelectedWhich = 0;
}

//-----------------------------------------------------------------------------
// Select the top left element of the program. Returns TRUE if it was able
// to do so, FALSE if not. The latter occurs given a completely empty
// program.
//-----------------------------------------------------------------------------
BOOL MoveCursorTopLeft(void)
{
    int i, j;
    // Let us first try to place it somewhere on-screen, so start at the
    // vertical scroll offset, not the very top (to avoid placing the
    // cursor in a position that would force us to scroll to put it in to
    // view.)
    for(i = 0; i < DISPLAY_MATRIX_X_SIZE; i++) {
        for(j = ScrollYOffset;
            j < DISPLAY_MATRIX_Y_SIZE && j < (ScrollYOffset+16); j++)
        {
            if(VALID_LEAF(DisplayMatrix[i][j])) {
                SelectElement(i, j, SELECTED_LEFT);
                return TRUE;
            }
        }
    }

    // If that didn't work, then try anywhere on the diagram before giving
    // up entirely.
    for(i = 0; i < DISPLAY_MATRIX_X_SIZE; i++) {
        for(j = 0; j < 16; j++) {
            if(VALID_LEAF(DisplayMatrix[i][j])) {
                SelectElement(i, j, SELECTED_LEFT);
                return TRUE;
            }
        }
    }
    return FALSE;
}

//-----------------------------------------------------------------------------
// Move the cursor in response to a keyboard command (arrow keys). Basically
// we move the cursor within the currently selected element until we hit
// the edge (e.g. left edge for VK_LEFT), and then we see if we can select
// a new item that would let us keep going.
//-----------------------------------------------------------------------------
void MoveCursorKeyboard(int keyCode)
{
    if(!Selected || Selected->selectedState == SELECTED_NONE) {
        MoveCursorTopLeft();
        return;
    }

    switch(keyCode) {
        case VK_LEFT: {
            if(!Selected || Selected->selectedState == SELECTED_NONE) {
                break;
            }
            if(Selected->selectedState != SELECTED_LEFT) {
                SelectElement(-1, -1, SELECTED_LEFT);
                break;
            }
            if(SelectedWhich == ELEM_COMMENT) break;
            int i, j;
            if(FindSelected(&i, &j)) {
                i--;
                while(i >= 0 && (!VALID_LEAF(DisplayMatrix[i][j]) ||
                    (DisplayMatrix[i][j] == Selected)))
                {
                    i--;
                }
                if(i >= 0) {
                    SelectElement(i, j, SELECTED_RIGHT);
                }
            }
            break;
        }
        case VK_RIGHT: {
            if(!Selected || Selected->selectedState == SELECTED_NONE) {
                break;
            }
            if(Selected->selectedState != SELECTED_RIGHT) {
                SelectElement(-1, -1, SELECTED_RIGHT);
                break;
            }
            if(SelectedWhich == ELEM_COMMENT) break;
            int i, j;
            if(FindSelected(&i, &j)) {
                i++;
                while(i < DISPLAY_MATRIX_X_SIZE &&
                    !VALID_LEAF(DisplayMatrix[i][j]))
                {
                    i++;
                }
                if(i != DISPLAY_MATRIX_X_SIZE) {
                    SelectElement(i, j, SELECTED_LEFT);
                }
            }
            break;
        }
        case VK_UP: {
            if(!Selected || Selected->selectedState == SELECTED_NONE) {
                break;
            }
            if(Selected->selectedState != SELECTED_ABOVE &&
                SelectedWhich != ELEM_PLACEHOLDER)
            {
                SelectElement(-1, -1, SELECTED_ABOVE);
                break;
            }
            int i, j;
            if(FindSelected(&i, &j)) {
                j--;
                while(j >= 0 && !VALID_LEAF(DisplayMatrix[i][j]))
                    j--;
                if(j >= 0) {
                    SelectElement(i, j, SELECTED_BELOW);
                }
            }
            break;
        }
        case VK_DOWN: {
            if(!Selected || Selected->selectedState == SELECTED_NONE) {
                break;
            }
            if(Selected->selectedState != SELECTED_BELOW &&
                SelectedWhich != ELEM_PLACEHOLDER)
            {
                SelectElement(-1, -1, SELECTED_BELOW);
                break;
            }
            int i, j;
            if(FindSelected(&i, &j)) {
                j++;
                while(j < DISPLAY_MATRIX_Y_SIZE &&
                    !VALID_LEAF(DisplayMatrix[i][j]))
                {
                    j++;
                }
                if(j != DISPLAY_MATRIX_Y_SIZE) {
                    SelectElement(i, j, SELECTED_ABOVE);
                } else if(ScrollYOffsetMax - ScrollYOffset < 3) {
                    // special case: scroll the end marker into view
                    ScrollYOffset = ScrollYOffsetMax;
                    RefreshScrollbars();
                }
            }
            break;
        }
    }
}

//-----------------------------------------------------------------------------
static BOOL doReplaceElem(int which, int whichWhere, void *where, int index)
{
    int newWhich;
    switch(which) {
        // group 1 of suitable elements
        case ELEM_SHORT: newWhich = ELEM_OPEN; break;
        case ELEM_OPEN: newWhich = ELEM_SHORT; break;
        // group 2 of suitable elements, etc.
        case ELEM_ONE_SHOT_RISING: newWhich = ELEM_ONE_SHOT_FALLING; break;
        case ELEM_ONE_SHOT_FALLING: newWhich = ELEM_OSC; break;
        case ELEM_OSC: newWhich = ELEM_ONE_SHOT_RISING; break;
        //
        case ELEM_TON: newWhich = ELEM_TOF; break;
        case ELEM_TOF: newWhich = ELEM_RTO; break;
        case ELEM_RTO: newWhich = ELEM_TCY; break;
        case ELEM_TCY: newWhich = ELEM_TON; break;
        //
        case ELEM_EQU: newWhich = ELEM_NEQ; break;
        case ELEM_NEQ: newWhich = ELEM_GEQ; break;
        case ELEM_GEQ: newWhich = ELEM_GRT; break;
        case ELEM_GRT: newWhich = ELEM_LES; break;
        case ELEM_LES: newWhich = ELEM_LEQ; break;
        case ELEM_LEQ: newWhich = ELEM_EQU; break;
        //
        case ELEM_BIN2BCD: newWhich = ELEM_BCD2BIN; break;
        case ELEM_BCD2BIN: newWhich = ELEM_BIN2BCD; break;
        //
        case ELEM_CTU: newWhich = ELEM_CTD; break;
        case ELEM_CTD: newWhich = ELEM_CTC; break;
        case ELEM_CTC: newWhich = ELEM_CTR; break;
        case ELEM_CTR: newWhich = ELEM_CTU; break;
        //
        case ELEM_RSFR: newWhich = ELEM_WSFR; break;
        case ELEM_WSFR: newWhich = ELEM_RSFR; break;
        case ELEM_SSFR: newWhich = ELEM_CSFR; break;
        case ELEM_CSFR: newWhich = ELEM_SSFR; break;
        case ELEM_TSFR: newWhich = ELEM_T_C_SFR; break;
        case ELEM_T_C_SFR: newWhich = ELEM_TSFR; break;
        //
        case ELEM_AND: newWhich = ELEM_OR ; break;
        case ELEM_OR : newWhich = ELEM_XOR; break;
        case ELEM_XOR: newWhich = ELEM_AND; break;
        //
        case ELEM_NOT: newWhich = ELEM_NEG; break;
        case ELEM_NEG: newWhich = ELEM_NOT; break;
        //
        case ELEM_IF_BIT_SET: newWhich = ELEM_IF_BIT_CLEAR; break;
        case ELEM_IF_BIT_CLEAR: newWhich = ELEM_IF_BIT_SET; break;
        //
        case ELEM_SET_BIT: newWhich = ELEM_CLEAR_BIT; break;
        case ELEM_CLEAR_BIT: newWhich = ELEM_SET_BIT; break;
        //
        case ELEM_SHL: newWhich = ELEM_SHR; break;
        case ELEM_SHR: newWhich = ELEM_SR0; break;
        case ELEM_SR0: newWhich = ELEM_ROL; break;
        case ELEM_ROL: newWhich = ELEM_ROR; break;
        case ELEM_ROR: newWhich = ELEM_SHL; break;
        //
        case ELEM_ADD: newWhich = ELEM_SUB; break;
        case ELEM_SUB: newWhich = ELEM_MUL; break;
        case ELEM_MUL: newWhich = ELEM_DIV; break;
        case ELEM_DIV: newWhich = ELEM_MOD; break;
        case ELEM_MOD: newWhich = ELEM_ADD; break;
        //
        case ELEM_7SEG : newWhich = ELEM_9SEG ; break;
        case ELEM_9SEG : newWhich = ELEM_14SEG; break;
        case ELEM_14SEG: newWhich = ELEM_16SEG; break;
        case ELEM_16SEG: newWhich = ELEM_7SEG ; break;
        //
//      case : newWhich = ; break;
        default: newWhich = 0;
    }
    if(newWhich) {
        if(whichWhere == ELEM_SERIES_SUBCKT) {
            ElemSubcktSeries *s = (ElemSubcktSeries *)where;
            s->contents[index].which = newWhich;
        } else {
            ElemSubcktParallel *p = (ElemSubcktParallel *)where;
            p->contents[index].which = newWhich;
        }
        SelectedWhich = newWhich;
        return TRUE;
    }
    return FALSE;
}

//-----------------------------------------------------------------------------
static BOOL ReplaceElem(int which, void *any, ElemLeaf *seek,
                        int whichWhere, void *where, int index)
{
    switch(which) {
        case ELEM_SERIES_SUBCKT: {
            ElemSubcktSeries *s = (ElemSubcktSeries *)any;
            int i;
            for(i = 0; i < s->count; i++)
                if(ReplaceElem(s->contents[i].which, s->contents[i].d.any,
                               seek, ELEM_SERIES_SUBCKT, s, i))
                    return TRUE;
            break;
        }
        case ELEM_PARALLEL_SUBCKT: {
            ElemSubcktParallel *p = (ElemSubcktParallel *)any;
            int i;
            for(i = 0; i < p->count; i++)
                if(ReplaceElem(p->contents[i].which, p->contents[i].d.any,
                               seek, ELEM_PARALLEL_SUBCKT, p, i))
                    return TRUE;
            break;
        }
        CASE_LEAF
            if(any == seek)
                return doReplaceElem(which, whichWhere, where, index);
            break;

        case ELEM_PADDING:
            break;

        default:
            ooops("which=%d",which);
    }
    return FALSE;
}

//-----------------------------------------------------------------------------
// Replace the selected element on a suitable element at the cursor position.
//-----------------------------------------------------------------------------
BOOL ReplaceSelectedElement(void)
{
    if(!Selected/* || Selected->selectedState == SELECTED_NONE*/) return FALSE;

    int i;
    for(i = 0; i < Prog.numRungs; i++)
        if(ReplaceElem(ELEM_SERIES_SUBCKT, Prog.rungs[i], Selected, 0, 0, 0))
            return TRUE;
    return FALSE;
}

//-----------------------------------------------------------------------------
// Edit the selected element. Pop up the appropriate modal dialog box to do
// this.
//-----------------------------------------------------------------------------
void EditSelectedElement(void)
{
    if(!Selected || Selected->selectedState == SELECTED_NONE) return;

    switch(SelectedWhich) {
        case ELEM_COMMENT:
            ShowCommentDialog(Selected->d.comment.str);
            break;

        case ELEM_CONTACTS:
            ShowContactsDialog(&(Selected->d.contacts.negated),&(Selected->d.contacts.set1),
                Selected->d.contacts.name);
            break;

        case ELEM_COIL:
            ShowCoilDialog(&(Selected->d.coil.negated),
                &(Selected->d.coil.setOnly), &(Selected->d.coil.resetOnly), &(Selected->d.coil.ttrigger),
                Selected->d.coil.name);
            break;

        case ELEM_TCY:
        case ELEM_TON:
        case ELEM_TOF:
        case ELEM_RTO:
            ShowTimerDialog(SelectedWhich, &(Selected->d.timer.delay),
                Selected->d.timer.name);
            break;

        case ELEM_CTR:
        case ELEM_CTU:
        case ELEM_CTD:
        case ELEM_CTC:
            ShowCounterDialog(SelectedWhich, Selected->d.counter.init, Selected->d.counter.max,
                Selected->d.counter.name);
            break;

        // Special function
        case ELEM_RSFR:
        case ELEM_WSFR:
        case ELEM_SSFR:
        case ELEM_CSFR:
        case ELEM_TSFR:
        case ELEM_T_C_SFR:
            ShowSFRDialog(SelectedWhich, Selected->d.sfr.sfr,
                Selected->d.sfr.op);
            break;
        // Special function

        case ELEM_IF_BIT_SET  :
        case ELEM_IF_BIT_CLEAR:
        case ELEM_SET_BIT  :
        case ELEM_CLEAR_BIT:
            ShowVarBitDialog(SelectedWhich, Selected->d.move.dest, Selected->d.move.src);
            break;

        case ELEM_EQU:
        case ELEM_NEQ:
        case ELEM_GRT:
        case ELEM_GEQ:
        case ELEM_LES:
        case ELEM_LEQ:
            ShowCmpDialog(SelectedWhich, Selected->d.cmp.op1,
                Selected->d.cmp.op2);
            break;

        case ELEM_ADD:
        case ELEM_SUB:
        case ELEM_MUL:
        case ELEM_DIV:
        case ELEM_MOD:
        case ELEM_SHL:
        case ELEM_SHR:
        case ELEM_SR0:
        case ELEM_ROL:
        case ELEM_ROR:
        case ELEM_AND:
        case ELEM_OR :
        case ELEM_XOR:
        case ELEM_NOT:
        case ELEM_NEG:
            ShowMathDialog(SelectedWhich, Selected->d.math.dest,
                Selected->d.math.op1, Selected->d.math.op2);
            break;

        case ELEM_STEPPER:
            ShowStepperDialog(SelectedWhich, &Selected->d);
            break;

        case ELEM_PULSER:
            ShowPulserDialog(SelectedWhich, Selected->d.pulser.P1, Selected->d.pulser.P0, Selected->d.pulser.accel, Selected->d.pulser.counter, Selected->d.pulser.busy);
            break;

        case ELEM_NPULSE:
            ShowNPulseDialog(SelectedWhich, Selected->d.Npulse.counter, Selected->d.Npulse.targetFreq, Selected->d.Npulse.coil);
            break;

        case ELEM_QUAD_ENCOD:
            ShowQuadEncodDialog(SelectedWhich, Selected->d.QuadEncod.counter, &(Selected->d.QuadEncod.int01), Selected->d.QuadEncod.contactA, Selected->d.QuadEncod.contactB, Selected->d.QuadEncod.contactZ, Selected->d.QuadEncod.zero);
            break;

        case ELEM_7SEG:
        case ELEM_9SEG:
        case ELEM_14SEG:
        case ELEM_16SEG:
            ShowSegmentsDialog(Selected);
            break;

        case ELEM_BUS:
            ShowBusDialog(Selected);
            break;

        case ELEM_RES:
            ShowResetDialog(Selected->d.reset.name);
            break;

        case ELEM_BIN2BCD:
        case ELEM_BCD2BIN:
        case ELEM_SWAP:
        case ELEM_MOVE:
            ShowMoveDialog(SelectedWhich, Selected->d.move.dest, Selected->d.move.src);
            break;

        case ELEM_SET_PWM:
            ShowSetPwmDialog(&Selected->d);
            break;

        case ELEM_READ_ADC:
            ShowReadAdcDialog(Selected->d.readAdc.name+1);
            break;

        case ELEM_UART_RECV:
        case ELEM_UART_SEND:
            ShowUartDialog(SelectedWhich, Selected->d.uart.name);
            break;

        case ELEM_PERSIST:
            ShowPersistDialog(Selected->d.persist.var);
            break;

        case ELEM_SHIFT_REGISTER:
            ShowShiftRegisterDialog(Selected->d.shiftRegister.name,
                &(Selected->d.shiftRegister.stages));
            break;

        case ELEM_STRING:
            ShowStringDialog(Selected->d.fmtdStr.dest, Selected->d.fmtdStr.var,
                Selected->d.fmtdStr.string);
            break;

        case ELEM_FORMATTED_STRING:
            ShowFormattedStringDialog(Selected->d.fmtdStr.var,
                Selected->d.fmtdStr.string);
            break;

        case ELEM_PIECEWISE_LINEAR:
            ShowPiecewiseLinearDialog(Selected);
            break;

        case ELEM_LOOK_UP_TABLE:
            ShowLookUpTableDialog(Selected);
            break;
    }
}

//-----------------------------------------------------------------------------
// Edit the element under the mouse cursor. This will typically be the
// selected element, since the first click would have selected it. If there
// is no element under the mouser cursor then do nothing; do not edit the
// selected element (which is elsewhere on screen), as that would be
// confusing.
//-----------------------------------------------------------------------------
void EditElementMouseDoubleclick(int x, int y)
{
    x += ScrollXOffset;

    y += FONT_HEIGHT/2;

    int gx = (x - X_PADDING)/(POS_WIDTH*FONT_WIDTH);
    int gy = (y - Y_PADDING)/(POS_HEIGHT*FONT_HEIGHT);

    gy += ScrollYOffset;

    if(InSimulationMode) {
        ElemLeaf *l = DisplayMatrix[gx][gy];
        if(l && DisplayMatrixWhich[gx][gy] == ELEM_CONTACTS) {
            char *name = l->d.contacts.name;
            if((name[0] != 'Y')
            && (name[0] != 'M')) {
                SimulationToggleContact(name);
            }
        } else if(l && DisplayMatrixWhich[gx][gy] == ELEM_READ_ADC) {
            ShowAnalogSliderPopup(l->d.readAdc.name);
        }
    } else {
        if(DisplayMatrix[gx][gy] == Selected) {
            EditSelectedElement();
        }
    }
}

//-----------------------------------------------------------------------------
// Move the cursor in response to a mouse click. If they clicked in a leaf
// element box then figure out which edge they're closest too (with appropriate
// fudge factors to make all edges equally easy to click on) and put the
// cursor there.
//-----------------------------------------------------------------------------
void MoveCursorMouseClick(int x, int y)
{
    x += ScrollXOffset;

    y += FONT_HEIGHT/2;

    int gx0 = (x - X_PADDING)/(POS_WIDTH*FONT_WIDTH);
    int gy0 = (y - Y_PADDING)/(POS_HEIGHT*FONT_HEIGHT);

    int gx = gx0;
    int gy = gy0 + ScrollYOffset;

    int dognail = 0;

    if(!VALID_LEAF(DisplayMatrix[gx][gy])) {
        int dyUp = 0;
        int dyDn = 0;
        while ( (gy-dyUp > 0) && (!VALID_LEAF(DisplayMatrix[gx][gy-dyUp])) ) {
            dyUp++;
        }
        while ( (gy+dyDn < DISPLAY_MATRIX_Y_SIZE-1) && (!VALID_LEAF(DisplayMatrix[gx][gy+dyDn])) ) {
            dyDn++;
        }
        // a dog-nail
        if(dyDn<dyUp) {
            if(VALID_LEAF(DisplayMatrix[gx][gy+dyDn])) {
                gy += dyDn;
                dognail = SELECTED_ABOVE;
            }
        } else {
            if(VALID_LEAF(DisplayMatrix[gx][gy-dyUp])) {
                gy -= dyUp;
                dognail = SELECTED_BELOW;
            }
        }
    }

    if(VALID_LEAF(DisplayMatrix[gx][gy])) {
        int i, j;
        for(i = 0; i < DISPLAY_MATRIX_X_SIZE; i++) {
            for(j = 0; j < DISPLAY_MATRIX_Y_SIZE; j++) {
                if(DisplayMatrix[i][j])
                    DisplayMatrix[i][j]->selectedState = SELECTED_NONE;
            }
        }
        int dx = x - (gx0*POS_WIDTH*FONT_WIDTH + X_PADDING);
        int dy = y - (gy0*POS_HEIGHT*FONT_HEIGHT + Y_PADDING);

        int dtop = dy;
        int dbottom = POS_HEIGHT*FONT_HEIGHT - dy;
        int dleft = dx;
        int dright = POS_WIDTH*FONT_WIDTH - dx;

        int extra = 1;
        if(DisplayMatrixWhich[gx][gy] == ELEM_COMMENT) {
            dleft += gx*POS_WIDTH*FONT_WIDTH;
            dright += (ColsAvailable - gx - 1)*POS_WIDTH*FONT_WIDTH;
            extra = ColsAvailable;
        } else {
            if((gx > 0) && (DisplayMatrix[gx-1][gy] == DisplayMatrix[gx][gy])) {
                dleft += POS_WIDTH*FONT_WIDTH;
                extra = 2;
            }
            if((gx < (DISPLAY_MATRIX_X_SIZE-1)) &&
                (DisplayMatrix[gx+1][gy] == DisplayMatrix[gx][gy]))
            {
                dright += POS_WIDTH*FONT_WIDTH;
                extra = 2;
            }
        }

        int decideX = (dright - dleft);
        int decideY = (dtop - dbottom);

        decideY = decideY*3*extra;

        int state;
        if(abs(decideY) > abs(decideX)) {
            if(dognail) {
                state = dognail;
            } else if(decideY > 0) {
                state = SELECTED_BELOW;
            } else {
                state = SELECTED_ABOVE;
            }
        } else {
            if(decideX > 0) {
                state = SELECTED_LEFT;
            } else {
                state = SELECTED_RIGHT;
            }
        }
        SelectElement(gx, gy, state);
    }
}

//-----------------------------------------------------------------------------
// Place the cursor as near to the given point on the grid as possible. Used
// after deleting an element, for example.
//-----------------------------------------------------------------------------
BOOL MoveCursorNear(int *gx, int *gy)
{
    int out = 0;

    if(*gx < DISPLAY_MATRIX_X_SIZE) {
        if(VALID_LEAF(DisplayMatrix[*gx][*gy])) {
            SelectElement(*gx, *gy, SELECTED_BELOW);
            return FindSelected(&*gx, &*gy);
        }
    }

    for(out = 0; out < 8; out++) {
        if(*gx - out >= 0) {
            if(VALID_LEAF(DisplayMatrix[*gx-out][*gy])) {
                SelectElement(*gx-out, *gy, SELECTED_RIGHT);
                return FindSelected(&*gx, &*gy);
            }
        }
        if(*gx + out < DISPLAY_MATRIX_X_SIZE) {
            if(VALID_LEAF(DisplayMatrix[*gx+out][*gy])) {
                SelectElement(*gx+out, *gy, SELECTED_LEFT);
                return FindSelected(&*gx, &*gy);
            }
        }
        if(*gy - out >= 0) {
            if(VALID_LEAF(DisplayMatrix[*gx][*gy-out])) {
                SelectElement(*gx, *gy-out, SELECTED_BELOW);
                return FindSelected(&*gx, &*gy);
            }
        }
        if(*gy + out < DISPLAY_MATRIX_Y_SIZE) {
            if(VALID_LEAF(DisplayMatrix[*gx][*gy+out])) {
                SelectElement(*gx, *gy+out, SELECTED_ABOVE);
                return FindSelected(&*gx, &*gy);
            }
        }

        if(out == 1) {
            // Now see if we have a straight shot to the right; might be far
            // if we have to go up to a coil or other end of line element.
            int across;
            for(across = 1; *gx+across < DISPLAY_MATRIX_X_SIZE; across++) {
                if(VALID_LEAF(DisplayMatrix[*gx+across][*gy])) {
                    SelectElement(*gx+across, *gy, SELECTED_LEFT);
                    return FindSelected(&*gx, &*gy);
                }
                if(!DisplayMatrix[*gx+across][*gy]) break;
            }
        }
    }

    //MoveCursorTopLeft();
    int tgx=*gx, tgy=*gy;
    BOOL f=FindSelected(&tgx, &tgy);
    return f;
}

//-----------------------------------------------------------------------------
// Negate the selected item, if this is meaningful.
//-----------------------------------------------------------------------------
void NegateSelected(void)
{
    if(Selected->d.contacts.negated) {
        MakeNormalSelected();
        return;
    }
    switch(SelectedWhich) {
        case ELEM_CONTACTS:
            Selected->d.contacts.negated = TRUE;
            break;

        case ELEM_COIL: {
            ElemCoil *c = &Selected->d.coil;
            c->negated = TRUE;
            c->resetOnly = FALSE;
            c->setOnly = FALSE;
            c->ttrigger = FALSE;
            break;
        }
        default:
            break;
    }
}

//-----------------------------------------------------------------------------
// Make the item selected normal: not negated, not set/reset only.
//-----------------------------------------------------------------------------
void MakeNormalSelected(void)
{
    if(!Selected->d.contacts.negated) {
        NegateSelected();
        return;
    }
    switch(SelectedWhich) {
        case ELEM_CONTACTS:
            Selected->d.contacts.negated = FALSE;
            break;

        case ELEM_COIL: {
            ElemCoil *c = &Selected->d.coil;
            c->negated = FALSE;
            c->setOnly = FALSE;
            c->resetOnly = FALSE;
            c->ttrigger = FALSE;
            break;
        }
        default:
            break;
    }
}

//-----------------------------------------------------------------------------
// Make the selected item set-only, if it is a coil.
//-----------------------------------------------------------------------------
void MakeSetOnlySelected(void)
{
    if(SelectedWhich != ELEM_COIL) return;

    ElemCoil *c = &Selected->d.coil;
    c->setOnly = TRUE;
    c->resetOnly = FALSE;
    c->negated = FALSE;
    c->ttrigger = FALSE;
}

//-----------------------------------------------------------------------------
// Make the selected item reset-only, if it is a coil.
//-----------------------------------------------------------------------------
void MakeResetOnlySelected(void)
{
    if(SelectedWhich != ELEM_COIL) return;

    ElemCoil *c = &Selected->d.coil;
    c->resetOnly = TRUE;
    c->setOnly = FALSE;
    c->negated = FALSE;
    c->ttrigger = FALSE;
}

//-----------------------------------------------------------------------------
// Make the selected item T-trigger, if it is a coil.
//-----------------------------------------------------------------------------
void MakeTtriggerSelected(void)
{
    if(SelectedWhich != ELEM_COIL) return;

    ElemCoil *c = &Selected->d.coil;
    c->ttrigger = TRUE;
    c->resetOnly = FALSE;
    c->setOnly = FALSE;
    c->negated = FALSE;
}

