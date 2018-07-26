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
// Routines for modifying the circuit: add a particular element at a
// particular point, delete the selected element, etc.
// Jonathan Westhues, Oct 2004
//-----------------------------------------------------------------------------
#include "stdafx.h"

#include "ldmicro.h"

//-----------------------------------------------------------------------------
// Like malloc/free, but memsets memory allocated to all zeros. Also TODO some
// checking and something sensible if it fails.
//-----------------------------------------------------------------------------
void *CheckMalloc(size_t n)
{
    void *p = malloc(n);
    if(p)
        memset(p, 0, n);
    return p;
}
void CheckFree(void *p)
{
    free(p);
}

//-----------------------------------------------------------------------------
// Convenience routines for allocating frequently-used data structures.
//-----------------------------------------------------------------------------

ElemLeaf *AllocLeaf()
{
    return (ElemLeaf *)CheckMalloc(sizeof(ElemLeaf));
}
ElemSubcktSeries *AllocSubcktSeries()
{
    return (ElemSubcktSeries *)CheckMalloc(sizeof(ElemSubcktSeries));
}
ElemSubcktParallel *AllocSubcktParallel()
{
    return (ElemSubcktParallel *)CheckMalloc(sizeof(ElemSubcktParallel));
}

//-----------------------------------------------------------------------------
// Routine that does the actual work of adding a leaf element to the left/
// right of or above/below the selected element. If we are adding left/right
// in a series circuit then it's easy; just increase length of that
// subcircuit and stick it in. Same goes for above/below in a parallel
// subcircuit. If we are adding above/below in a series circuit or left/right
// in a parallel circuit then we must create a new parallel (for series) or
// series (for parallel) subcircuit with 2 elements, one for the previously
// selected element and one for the new element. Calls itself recursively on
// all subcircuits. Returns true if it or a child made the addition.
//-----------------------------------------------------------------------------
static bool AddLeafWorker(int which, void *any, int newWhich, ElemLeaf *newElem)
{
    int i;
    switch(which) {
        case ELEM_SERIES_SUBCKT: {
            ElemSubcktSeries *s = (ElemSubcktSeries *)any;
            for(i = 0; i < s->count; i++) {
                if(s->contents[i].data.any == Selected) {
                    break;
                }
                if(s->contents[i].which == ELEM_PARALLEL_SUBCKT) {
                    if(AddLeafWorker(ELEM_PARALLEL_SUBCKT, s->contents[i].data.any, newWhich, newElem)) {
                        return true;
                    }
                }
            }
            if(i == s->count)
                break;
            if(s->contents[i].which == ELEM_PLACEHOLDER) {
                // Special case--placeholders are replaced. They only appear
                // in the empty series subcircuit that I generate for them,
                // so there is no need to consider them anywhere but here.
                // If we copy instead of replacing then the DisplayMatrix
                // tables don't get all messed up.
                memcpy(s->contents[i].data.leaf, newElem, sizeof(ElemLeaf));
                s->contents[i].data.leaf->selectedState = EndOfRungElem(newWhich) ? SELECTED_LEFT : SELECTED_RIGHT;
                CheckFree(newElem);
                s->contents[i].which = newWhich;
                SelectedWhich = newWhich;
                return true;
            }
            if(s->count >= (MAX_ELEMENTS_IN_SUBCKT - 1)) {
                Error(_("Too many elements in subcircuit!"));
                return true;
            }
            switch(Selected->selectedState) {
                case SELECTED_LEFT:
                    memmove(&s->contents[i + 1], &s->contents[i], (s->count - i) * sizeof(s->contents[0]));
                    s->contents[i].data.leaf = newElem;
                    s->contents[i].which = newWhich;
                    (s->count)++;
                    break;

                case SELECTED_RIGHT:
                    memmove(&s->contents[i + 2], &s->contents[i + 1], (s->count - i - 1) * sizeof(s->contents[0]));
                    s->contents[i + 1].data.leaf = newElem;
                    s->contents[i + 1].which = newWhich;
                    (s->count)++;
                    break;

                case SELECTED_BELOW:
                case SELECTED_ABOVE: {
                    ElemSubcktParallel *p = AllocSubcktParallel();
                    p->count = 2;

                    int t;
                    t = (Selected->selectedState == SELECTED_ABOVE) ? 0 : 1;
                    p->contents[t].which = newWhich;
                    p->contents[t].data.leaf = newElem;
                    t = (Selected->selectedState == SELECTED_ABOVE) ? 1 : 0;
                    p->contents[t].which = s->contents[i].which;
                    p->contents[t].data.any = s->contents[i].data.any;

                    s->contents[i].which = ELEM_PARALLEL_SUBCKT;
                    s->contents[i].data.parallel = p;
                    break;
                }
                default:
                    oops();
                    break;
            }
            return true;
            break;
        }
        case ELEM_PARALLEL_SUBCKT: {
            ElemSubcktParallel *p = (ElemSubcktParallel *)any;
            for(i = 0; i < p->count; i++) {
                if(p->contents[i].data.any == Selected) {
                    break;
                }
                if(p->contents[i].which == ELEM_SERIES_SUBCKT) {
                    if(AddLeafWorker(ELEM_SERIES_SUBCKT, p->contents[i].data.any, newWhich, newElem)) {
                        return true;
                    }
                }
            }
            if(i == p->count)
                break;
            if(p->count >= (MAX_ELEMENTS_IN_SUBCKT - 1)) {
                Error(_("Too many elements in subcircuit!"));
                return true;
            }
            switch(Selected->selectedState) {
                case SELECTED_ABOVE:
                    memmove(&p->contents[i + 1], &p->contents[i], (p->count - i) * sizeof(p->contents[0]));
                    p->contents[i].data.leaf = newElem;
                    p->contents[i].which = newWhich;
                    (p->count)++;
                    break;

                case SELECTED_BELOW:
                    memmove(&p->contents[i + 2], &p->contents[i + 1], (p->count - i - 1) * sizeof(p->contents[0]));
                    p->contents[i + 1].data.leaf = newElem;
                    p->contents[i + 1].which = newWhich;
                    (p->count)++;
                    break;

                case SELECTED_LEFT:
                case SELECTED_RIGHT: {
                    ElemSubcktSeries *s = AllocSubcktSeries();
                    s->count = 2;

                    int t;
                    t = (Selected->selectedState == SELECTED_LEFT) ? 0 : 1;
                    s->contents[t].which = newWhich;
                    s->contents[t].data.leaf = newElem;
                    t = (Selected->selectedState == SELECTED_LEFT) ? 1 : 0;
                    s->contents[t].which = p->contents[i].which;
                    s->contents[t].data.any = p->contents[i].data.any;

                    p->contents[i].which = ELEM_SERIES_SUBCKT;
                    p->contents[i].data.series = s;
                    break;
                }
                default:
                    oops();
                    break;
            }
            return true;
            break;
        }
    }

    return false;
}

//-----------------------------------------------------------------------------
// Add the specified leaf node in the position indicated by the cursor. We
// will search through the entire program using AddLeafWorker to find the
// insertion point, and AddLeafWorker will stick it in at the requested
// location and return true. We return true if it worked, else false.
//-----------------------------------------------------------------------------
static bool AddLeaf(int newWhich, ElemLeaf *newElem)
{
    if(!Selected || Selected->selectedState == SELECTED_NONE)
        return false;

    int i;
    for(i = 0; i < Prog.numRungs; i++) {
        if(AddLeafWorker(ELEM_SERIES_SUBCKT, Prog.rungs[i], newWhich, newElem)) {
            WhatCanWeDoFromCursorAndTopology();
            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
// Routines to allocate memory for a new circuit element (contact, coil, etc.)
// and insert it into the current program with AddLeaf. Fill in some default
// parameters, name etc. when we create the leaf; user can change them later.
//-----------------------------------------------------------------------------
void AddComment(const char *str)
{
    if(!CanInsertComment)
        return;

    ElemLeaf *c = AllocLeaf();
    strcpy(c->d.comment.str, str);

    AddLeaf(ELEM_COMMENT, c);
}

void AddContact(int what)
{
    if(!CanInsertOther)
        return;

    ElemLeaf *c = AllocLeaf();
    if(what == MNU_INSERT_CONT_RELAY) {
        strcpy(c->d.contacts.name, "Rnew");
    } else if(what == MNU_INSERT_CONT_OUTPUT) {
        strcpy(c->d.contacts.name, "Ynew");
    } else {
        strcpy(c->d.contacts.name, "Xnew");
    }
    c->d.contacts.negated = false;

    AddLeaf(ELEM_CONTACTS, c);
}

void AddCoil(int what)
{
    if(!CanInsertEnd)
        return;

    ElemLeaf *c = AllocLeaf();
    if(what == MNU_INSERT_COIL_RELAY) {
        strcpy(c->d.coil.name, "Rnew");
    } else {
        strcpy(c->d.coil.name, "Ynew");
    }
    c->d.coil.negated = false;
    c->d.coil.setOnly = false;
    c->d.coil.resetOnly = false;
    c->d.coil.ttrigger = false;

    AddLeaf(ELEM_COIL, c);
}

void AddDelay()
{
    oops();
    if(!CanInsertOther)
        return;
    ElemLeaf *t = AllocLeaf();
    strcpy(t->d.timer.delay, "10"); // 10 us
    AddLeaf(ELEM_DELAY, t);
}

void AddTimer(int which)
{
    if(!CanInsertOther)
        return;

    ElemLeaf *t = AllocLeaf();
    if((which == ELEM_DELAY) || (which == ELEM_TIME2DELAY)) {
        strcpy(t->d.timer.name, "delay");
        strcpy(t->d.timer.delay, "10"); // 10 us
    } else {
        strcpy(t->d.timer.name, "Tnew");
        strcpy(t->d.timer.delay, "100000"); // 100 ms
    }
    t->d.timer.adjust = 0;
    AddLeaf(which, t);
}

void AddEmpty(int which)
{
    if(!CanInsertOther)
        return;

    ElemLeaf *t = AllocLeaf();
    AddLeaf(which, t);
}

void AddReset()
{
    if(!CanInsertEnd)
        return;

    ElemLeaf *t = AllocLeaf();
    strcpy(t->d.reset.name, "Tnew");
    AddLeaf(ELEM_RES, t);
}

void AddSleep()
{
    if(!CanInsertEnd)
        return;

    ElemLeaf *t = AllocLeaf();
    strcpy(t->d.timer.name, "$Tsleep");
    strcpy(t->d.timer.delay, "1000000"); // 1 s
    AddLeaf(ELEM_SLEEP, t);
}

void AddLock()
{
    if(!CanInsertEnd)
        return;

    ElemLeaf *t = AllocLeaf();
    AddLeaf(ELEM_LOCK, t);
}

void AddClrWdt()
{
    if(!CanInsertEnd)
        return;

    ElemLeaf *t = AllocLeaf();
    AddLeaf(ELEM_CLRWDT, t);
}

void AddGoto(int which)
{
    if(!CanInsertEnd && EndOfRungElem(which))
        return;
    if(!CanInsertOther && !EndOfRungElem(which))
        return;

    ElemLeaf *t = AllocLeaf();
    strcpy(t->d.doGoto.rung, "?");
    AddLeaf(which, t);
}

void AddMasterRelay()
{
    if(!CanInsertEnd)
        return;

    ElemLeaf *t = AllocLeaf();
    AddLeaf(ELEM_MASTER_RELAY, t);
}

void AddShiftRegister()
{
    if(!CanInsertEnd)
        return;

    ElemLeaf *t = AllocLeaf();
    strcpy(t->d.shiftRegister.name, "reg");
    t->d.shiftRegister.stages = 7;
    AddLeaf(ELEM_SHIFT_REGISTER, t);
}

void AddFormattedString()
{
    if(!CanInsertOther)
        return;

    ElemLeaf *t = AllocLeaf();
    strcpy(t->d.fmtdStr.var, "var");
    strcpy(t->d.fmtdStr.string, "value: \\3\\r\\n");
    AddLeaf(ELEM_FORMATTED_STRING, t);
}

void AddString()
{
    if(!CanInsertOther)
        return;

    ElemLeaf *t = AllocLeaf();
    strcpy(t->d.fmtdStr.dest, "dest");
    strcpy(t->d.fmtdStr.string, "fmtstring");
    strcpy(t->d.fmtdStr.var, "var");
    AddLeaf(ELEM_STRING, t);
}

void AddPrint(int code)
{
    if(!CanInsertOther)
        return;

    ElemLeaf *t = AllocLeaf();
    strcpy(t->d.fmtdStr.dest, "dest");
    strcpy(t->d.fmtdStr.string, "fmtString");
    strcpy(t->d.fmtdStr.var, "varsList");
    strcpy(t->d.fmtdStr.enable, "RenableIn");
    strcpy(t->d.fmtdStr.error, "RerrorOut");
    AddLeaf(code, t);
}

void AddLookUpTable()
{
    if(!CanInsertEnd)
        return;

    ElemLeaf *t = AllocLeaf();
    strcpy(t->d.lookUpTable.name, "name");
    strcpy(t->d.lookUpTable.dest, "dest");
    strcpy(t->d.lookUpTable.index, "index");
    t->d.lookUpTable.count = 0;
    t->d.lookUpTable.editAsString = 0;
    AddLeaf(ELEM_LOOK_UP_TABLE, t);
}

void AddPiecewiseLinear()
{
    if(!CanInsertEnd)
        return;

    ElemLeaf *t = AllocLeaf();
    strcpy(t->d.piecewiseLinear.name, "name");
    strcpy(t->d.piecewiseLinear.dest, "yvar");
    strcpy(t->d.piecewiseLinear.index, "xvar");
    t->d.piecewiseLinear.count = 0;
    AddLeaf(ELEM_PIECEWISE_LINEAR, t);
}

void AddMove()
{
    if(!CanInsertEnd)
        return;

    ElemLeaf *t = AllocLeaf();
    strcpy(t->d.move.dest, "dest");
    strcpy(t->d.move.src, "src");
    AddLeaf(ELEM_MOVE, t);
}

void AddBcd(int which)
{
    if(!CanInsertOther)
        return;

    ElemLeaf *t = AllocLeaf();
    strcpy(t->d.move.dest, "dest");
    strcpy(t->d.move.src, "src");
    AddLeaf(which, t);
}

void AddSegments(int which)
{
    if(!CanInsertOther)
        return;

    ElemLeaf *t = AllocLeaf();
    strcpy(t->d.segments.dest, "dest");
    strcpy(t->d.segments.src, "src");
    t->d.segments.common = 'C';
    t->d.segments.which = which;
    AddLeaf(which, t);
}

void AddBus(int which)
{
    if(!CanInsertOther)
        return;

    ElemLeaf *t = AllocLeaf();
    strcpy(t->d.bus.dest, "dest");
    strcpy(t->d.bus.src, "src");
    int i;
    for(i = 0; i < PCBbit_LEN; i++)
        t->d.bus.PCBbit[i] = i;
    AddLeaf(which, t);
}

void AddStepper()
{
    if(!CanInsertOther)
        return;

    ElemLeaf *t = AllocLeaf();
    strcpy(t->d.stepper.name, "step");
    strcpy(t->d.stepper.max, "stepMax");
    strcpy(t->d.stepper.P, "P");
    t->d.stepper.nSize = 100;
    t->d.stepper.n = 0;
    t->d.stepper.graph = 1;
    strcpy(t->d.stepper.coil, "Ystep");
    AddLeaf(ELEM_STEPPER, t);
}

void AddPulser()
{
    if(!CanInsertOther)
        return;

    ElemLeaf *t = AllocLeaf();
    strcpy(t->d.pulser.P1, "P1");
    strcpy(t->d.pulser.P0, "P0");
    strcpy(t->d.pulser.accel, "accel");
    strcpy(t->d.pulser.counter, "counter");
    strcpy(t->d.pulser.busy, "Rbusy");
    AddLeaf(ELEM_PULSER, t);
}

void AddNPulse()
{
    if(!CanInsertOther)
        return;

    if(NPulseFunctionUsed()) {
        Error(_("Can use only one N PULSE element on timer0."));
        return;
    }
    ElemLeaf *t = AllocLeaf();
    strcpy(t->d.Npulse.counter, "counter");
    strcpy(t->d.Npulse.targetFreq, "1000");
    strcpy(t->d.Npulse.coil, "YNpulse");
    AddLeaf(ELEM_NPULSE, t);
}

void AddQuadEncod()
{
    if(!CanInsertOther)
        return;

    uint32_t n = 0;
    if(Prog.mcu) {
        n = QuadEncodFunctionUsed();
        if(n > Prog.mcu->ExtIntCount) {
            //Error(_("Can use only %d INTs on this MCU."), Prog.mcu->ExtIntCount);
            //return;
        }
    }
    ElemLeaf *t = AllocLeaf();
    t->d.QuadEncod.int01 = n;
    sprintf(t->d.QuadEncod.counter, "qCount%d", n);
    sprintf(t->d.QuadEncod.inputA, "XqA%d", n);
    sprintf(t->d.QuadEncod.inputB, "XqB%d", n);
    sprintf(t->d.QuadEncod.inputZ, "XqZ%d", n);
    t->d.QuadEncod.inputZKind = '/';
    t->d.QuadEncod.countPerRevol = 0;
    sprintf(t->d.QuadEncod.dir, "YqDir%d", n);
    AddLeaf(ELEM_QUAD_ENCOD, t);
}

void AddSfr(int which)
{
    if(!CanInsertEnd)
        return;

    ElemLeaf *t = AllocLeaf();
    strcpy(t->d.cmp.op1, "sfr");
#ifdef USE_SFR
    if(which == ELEM_WSFR)
        strcpy(t->d.cmp.op2, "srs");
    else if(which == ELEM_RSFR)
        strcpy(t->d.cmp.op2, "dest");
    else
        strcpy(t->d.cmp.op2, "1");
#endif
    AddLeaf(which, t);
}

void AddMath(int which)
{
    if(!CanInsertEnd && EndOfRungElem(which))
        return;
    if(!CanInsertOther && !EndOfRungElem(which))
        return;

    ElemLeaf *t = AllocLeaf();
    strcpy(t->d.math.dest, "dest");
    strcpy(t->d.math.op1, "src");
    strcpy(t->d.math.op2, "1");
    AddLeaf(which, t);
}

void AddBitOps(int which)
{
    if(!CanInsertEnd && EndOfRungElem(which))
        return;
    if(!CanInsertOther && !EndOfRungElem(which))
        return;

    ElemLeaf *t = AllocLeaf();
    strcpy(t->d.move.dest, "var");
    strcpy(t->d.move.src, "bit");
    AddLeaf(which, t);
}

void AddCmp(int which)
{
    if(!CanInsertOther)
        return;

    ElemLeaf *t = AllocLeaf();
    strcpy(t->d.cmp.op1, "var");
    strcpy(t->d.cmp.op2, "1");
    AddLeaf(which, t);
}

void AddCounter(int which)
{
    //  if(which == ELEM_CTC) {
    //      if(!CanInsertEnd) return;
    //  } else {
    if(!CanInsertOther)
        return;
    //  }

    ElemLeaf *t = AllocLeaf();
    strcpy(t->d.counter.name, "Cnew");
    switch(which) {
        case ELEM_CTU:
            strcpy(t->d.counter.max, "10");
            break;
        case ELEM_CTD:
            strcpy(t->d.counter.max, "-10");
            break;
        case ELEM_CTC:
            strcpy(t->d.counter.max, "9");
            break;
        case ELEM_CTR:
            strcpy(t->d.counter.max, "-9");
            break;
    }
    strcpy(t->d.counter.init, "0");
    t->d.counter.inputKind = '/';
    AddLeaf(which, t);
}

void AddSeedRandom()
{
    if(!CanInsertOther)
        return;

    ElemLeaf *t = AllocLeaf();
    strcpy(t->d.move.dest, "Rand");
    strcpy(t->d.move.src, "newSeed");
    AddLeaf(ELEM_SEED_RANDOM, t);
}

void AddRandom()
{
    if(!CanInsertOther)
        return;

    ElemLeaf *t = AllocLeaf();
    strcpy(t->d.readAdc.name, "Rand");
    AddLeaf(ELEM_RANDOM, t);
}

void AddReadAdc()
{
    if(!CanInsertEnd)
        return;

    if(Prog.mcu) {
        if(!McuADC()) {
            Error(_("No ADC or ADC not supported for selected micro."));
            // return;
        }
        if(AdcFunctionUsed() >= McuADC()) {
            Error(_("No available ADC inputs."));
            // return;
        }
    }
    ElemLeaf *t = AllocLeaf();
    strcpy(t->d.readAdc.name, "ADCnew");
    AddLeaf(ELEM_READ_ADC, t);
}

void AddSetPwm()
{
    if(!CanInsertEnd)
        return;

    if(Prog.mcu) {
        if(!McuPWM()) {
            Error(_("No PWM or PWM not supported for this MCU."));
            // return;
        }
        if(PwmFunctionUsed() >= McuPWM()) {
            Error(_("No available PWM outputs."));
            // return;
        }
    }
    ElemLeaf *t = AllocLeaf();
    strcpy(t->d.setPwm.name, "PWMoutpin");
    strcpy(t->d.setPwm.duty_cycle, "duty_cycle");
    strcpy(t->d.setPwm.targetFreq, "1000");
    AddLeaf(ELEM_SET_PWM, t);
}

void AddUart(int which)
{
    if(!CanInsertOther)
        return;

    if(Prog.mcu) {
        if(!McuUART()) {
            Error(_("No UART or UART not supported for this MCU."));
            // return;
        }
    }
    ElemLeaf *t = AllocLeaf();
    if((which == ELEM_UART_SEND) || (which == ELEM_UART_RECV))
        strcpy(t->d.uart.name, "char");
    else
        strcpy(t->d.uart.name, "var");
    AddLeaf(which, t);
}

void AddSpi(int which)
{
    if(!CanInsertOther)
        return;

    if(Prog.mcu) {
        if(!McuSPI()) {
            Error(_("No SPI or SPI not supported for this MCU."));
            // return;
        }
    }
    ElemLeaf *t = AllocLeaf();
    strcpy(t->d.spi.name, "SPIn");
    strcpy(t->d.spi.mode, "Master");
    strcpy(t->d.spi.send, "send");
    strcpy(t->d.spi.recv, "recv");
    strcpy(t->d.spi.bitrate, "100000");
    strcpy(t->d.spi.modes, "0");
    strcpy(t->d.spi.size, "8");
    strcpy(t->d.spi.first, "MSB");
    AddLeaf(which, t);
}

void AddPersist()
{
    if(!CanInsertEnd)
        return;

    if(Prog.mcu) {
        if(!McuROM()) {
            Error(_("No ROM or ROM not supported for this MCU."));
            // return;
        }
    }
    ElemLeaf *t = AllocLeaf();
    strcpy(t->d.persist.var, "saved");
    AddLeaf(ELEM_PERSIST, t);
}

//-----------------------------------------------------------------------------
// Any subcircuit containing only one element should be collapsed into its
// parent. Call ourselves recursively to do this on every child of the passed
// subcircuit. The passed subcircuit will not be collapsed itself, so that
// the rung subcircuit may contain only one element as a special case. Returns
// true if it or a child made any changes, and for completeness must be
// called iteratively on the root till it doesn't do anything.
//-----------------------------------------------------------------------------
bool CollapseUnnecessarySubckts(int which, void *any)
{
    bool modified = false;

    switch(which) {
        case ELEM_SERIES_SUBCKT: {
            ElemSubcktSeries *s = (ElemSubcktSeries *)any;
            int               i;
            for(i = (s->count - 1); i >= 0; i--) {
                if(s->contents[i].which == ELEM_PARALLEL_SUBCKT) {
                    ElemSubcktParallel *p = s->contents[i].data.parallel;
                    if(p->count == 1) {
                        if(p->contents[0].which == ELEM_SERIES_SUBCKT) {
                            // merge the two series subcircuits
                            ElemSubcktSeries *s2 = p->contents[0].data.series;
                            int               makeSpaces = s2->count - 1;
                            memmove(&s->contents[i + makeSpaces + 1],
                                    &s->contents[i + 1],
                                    (s->count - i - 1) * sizeof(s->contents[0]));
                            memcpy(&s->contents[i], &s2->contents[0], (s2->count) * sizeof(s->contents[0]));
                            s->count += makeSpaces;
                            CheckFree(s2);
                        } else {
                            s->contents[i].which = p->contents[0].which;
                            s->contents[i].data.any = p->contents[0].data.any;
                        }
                        CheckFree(p);
                        modified = true;
                    } else if(p->count == 0) {
                        memmove(&s->contents[i], &s->contents[i + 1], (s->count - i - 1) * sizeof(s->contents[0]));
                        s->count -= 1;
                        CheckFree(p);
                        modified = true;
                    } else {
                        while(CollapseUnnecessarySubckts(ELEM_PARALLEL_SUBCKT, s->contents[i].data.parallel)) {
                            modified = true;
                        }
                    }
                } else if(s->contents[i].which == ELEM_SERIES_SUBCKT) {
                    // move up level
                    ElemSubcktSeries *s2 = s->contents[i].data.series;
                    if((s->count + s2->count) < MAX_ELEMENTS_IN_SUBCKT) {
                        memmove(&s->contents[i + s2->count],
                                &s->contents[i + 1],
                                (s->count - i - 1) * sizeof(s->contents[0]));
                        memcpy(&s->contents[i], &s2->contents[0], (s2->count) * sizeof(s->contents[0]));
                        s->count += s2->count - 1;
                        CheckFree(s2);
                        modified = true;
                    }
                }
                // else a leaf, not a problem
            }
            break;
        }
        case ELEM_PARALLEL_SUBCKT: {
            ElemSubcktParallel *p = (ElemSubcktParallel *)any;
            int                 i;
            for(i = (p->count - 1); i >= 0; i--) {
                if(p->contents[i].which == ELEM_SERIES_SUBCKT) {
                    ElemSubcktSeries *s = p->contents[i].data.series;
                    if(s->count == 1) {
                        if(s->contents[0].which == ELEM_PARALLEL_SUBCKT) {
                            // merge the two parallel subcircuits
                            ElemSubcktParallel *p2 = s->contents[0].data.parallel;
                            int                 makeSpaces = p2->count - 1;
                            memmove(&p->contents[i + makeSpaces + 1],
                                    &p->contents[i + 1],
                                    (p->count - i - 1) * sizeof(p->contents[0]));
                            memcpy(&p->contents[i], &p2->contents[0], (p2->count) * sizeof(p->contents[0]));
                            p->count += makeSpaces;
                            CheckFree(p2);
                        } else {
                            p->contents[i].which = s->contents[0].which;
                            p->contents[i].data.any = s->contents[0].data.any;
                        }
                        CheckFree(s);
                        modified = true;
                    } else if(s->count == 0) {
                        memmove(&p->contents[i], &p->contents[i + 1], (p->count - i - 1) * sizeof(p->contents[0]));
                        p->count -= 1;
                        CheckFree(s);
                        modified = true;
                    } else {
                        while(CollapseUnnecessarySubckts(ELEM_SERIES_SUBCKT, p->contents[i].data.series)) {
                            modified = true;
                        }
                    }
                }
                // else a leaf, not a problem
            }
            break;
        }

        default:
            oops();
            break;
    }

    return modified;
}

//-----------------------------------------------------------------------------
// Delete the selected leaf element from the circuit. Just pull it out of the
// subcircuit that it was in before, and don't worry about creating
// subcircuits with only one element; we will clean that up later in a second
// pass. Returns true if it or a child (calls self recursively) found and
// removed the element.
//-----------------------------------------------------------------------------
static bool DeleteSelectedFromSubckt(int which, void *any)
{
    switch(which) {
        case ELEM_SERIES_SUBCKT: {
            ElemSubcktSeries *s = (ElemSubcktSeries *)any;
            int               i;
            for(i = 0; i < s->count; i++) {
                if(s->contents[i].data.any == Selected) {
                    ForgetFromGrid(s->contents[i].data.any);
                    CheckFree(s->contents[i].data.any);
                    memmove(&s->contents[i], &s->contents[i + 1], (s->count - i - 1) * sizeof(s->contents[0]));
                    (s->count)--;
                    return true;
                }
                if(s->contents[i].which == ELEM_PARALLEL_SUBCKT) {
                    if(DeleteSelectedFromSubckt(ELEM_PARALLEL_SUBCKT, s->contents[i].data.any)) {
                        return true;
                    }
                }
            }
            break;
        }
        case ELEM_PARALLEL_SUBCKT: {
            ElemSubcktParallel *p = (ElemSubcktParallel *)any;
            int                 i;
            for(i = 0; i < p->count; i++) {
                if(p->contents[i].data.any == Selected) {
                    ForgetFromGrid(p->contents[i].data.any);
                    CheckFree(p->contents[i].data.any);
                    memmove(&p->contents[i], &p->contents[i + 1], (p->count - i - 1) * sizeof(p->contents[0]));
                    (p->count)--;
                    return true;
                }
                if(p->contents[i].which == ELEM_SERIES_SUBCKT) {
                    if(DeleteSelectedFromSubckt(ELEM_SERIES_SUBCKT, p->contents[i].data.any)) {
                        return true;
                    }
                }
            }
            break;
        }
        default:
            oops();
            break;
    }
    return false;
}

//-----------------------------------------------------------------------------
static bool DeleteAnyFromSubckt(int which, void *any, int anyWhich, void *anyToDelete, bool IsEnd)
{
    bool res = false;
    switch(anyWhich) {
        case ELEM_SERIES_SUBCKT: {
            ElemSubcktSeries *s = (ElemSubcktSeries *)any;
            int               i;
            for(i = (s->count - 1); i >= 0; i--)
                DeleteAnyFromSubckt(
                    ELEM_SERIES_SUBCKT, anyToDelete, s->contents[i].which, s->contents[i].data.any, IsEnd);
            break;
        }
        case ELEM_PARALLEL_SUBCKT: {
            ElemSubcktParallel *p = (ElemSubcktParallel *)any;
            int                 i;
            for(i = (p->count - 1); i >= 0; i--)
                DeleteAnyFromSubckt(
                    ELEM_PARALLEL_SUBCKT, anyToDelete, p->contents[i].which, p->contents[i].data.any, IsEnd);
            break;
        }
        default:
            if(EndOfRungElem(anyWhich) == static_cast<bool>(IsEnd))
                if(Selected != anyToDelete) {
                    ElemLeaf *saveSelected = Selected;
                    Selected = (ElemLeaf *)anyToDelete; // HOOK
                    if((res = DeleteSelectedFromSubckt(which, any))) {
                        while(CollapseUnnecessarySubckts(which, any))
                            ;
                        //dbp("DeleteAny %d", IsEnd);
                    }
                    Selected = saveSelected; // RESTORE
                }
            break;
    }
    return res;
}

//-----------------------------------------------------------------------------
// Delete the selected item from the program. Just call
// DeleteSelectedFromSubckt on every rung till we find it.
//-----------------------------------------------------------------------------
void DeleteSelectedFromProgram()
{
    if(!Selected || Selected->selectedState == SELECTED_NONE)
        return;
    int i = RungContainingSelected();
    if(i < 0)
        return;

    if(Prog.rungs[i]->count == 1 && Prog.rungs[i]->contents[0].which != ELEM_PARALLEL_SUBCKT) {
        Prog.rungs[i]->contents[0].which = ELEM_PLACEHOLDER;
        SelectedWhich = ELEM_PLACEHOLDER;
        Selected->selectedState = SELECTED_LEFT;
        WhatCanWeDoFromCursorAndTopology();
        return;
    }

    int gx, gy;
    if(!FindSelected(&gx, &gy)) {
        gx = 0;
        gy = 0;
    }

    if(DeleteSelectedFromSubckt(ELEM_SERIES_SUBCKT, Prog.rungs[i])) {
        while(CollapseUnnecessarySubckts(ELEM_SERIES_SUBCKT, Prog.rungs[i]))
            ;
        WhatCanWeDoFromCursorAndTopology();
        MoveCursorNear(&gx, &gy);
        return;
    }
}

//-----------------------------------------------------------------------------
// Free a circuit and all of its subcircuits. Calls self recursively to do
// so.
//-----------------------------------------------------------------------------
void FreeCircuit(int which, void *any)
{
    switch(which) {
        case ELEM_SERIES_SUBCKT: {
            ElemSubcktSeries *s = (ElemSubcktSeries *)any;
            int               i;
            for(i = 0; i < s->count; i++) {
                FreeCircuit(s->contents[i].which, s->contents[i].data.any);
            }
            CheckFree(s);
            break;
        }
        case ELEM_PARALLEL_SUBCKT: {
            ElemSubcktParallel *p = (ElemSubcktParallel *)any;
            int                 i;
            for(i = 0; i < p->count; i++) {
                FreeCircuit(p->contents[i].which, p->contents[i].data.any);
            }
            CheckFree(p);
            break;
        }
            CASE_LEAF
            ForgetFromGrid(any);
            CheckFree(any);
            break;

        default:
            oops();
            break;
    }
}

//-----------------------------------------------------------------------------
// Free the entire program.
//-----------------------------------------------------------------------------
void FreeEntireProgram()
{
    ForgetEverything();

    int i;
    for(i = 0; i < Prog.numRungs; i++) {
        FreeCircuit(ELEM_SERIES_SUBCKT, Prog.rungs[i]);
    }
    memset(Prog.rungSelected, ' ', sizeof(Prog.rungSelected));
    Prog.numRungs = 0;
    Prog.cycleTime = 10000;
    Prog.mcuClock = 16000000;
    Prog.baudRate = 9600;
    Prog.io.count = 0;
    Prog.cycleTimer = 1;
    Prog.cycleDuty = 0;
    Prog.configurationWord = 0;
    SetMcu(nullptr);

    WipeIntMemory();
}

//-----------------------------------------------------------------------------
// Returns true if the given subcircuit contains the given leaf.
//-----------------------------------------------------------------------------
static bool ContainsElem(int which, void *any, ElemLeaf *seek)
{
    switch(which) {
        case ELEM_SERIES_SUBCKT: {
            ElemSubcktSeries *s = (ElemSubcktSeries *)any;
            int               i;
            for(i = 0; i < s->count; i++) {
                if(ContainsElem(s->contents[i].which, s->contents[i].data.any, seek)) {
                    return true;
                }
            }
            break;
        }
        case ELEM_PARALLEL_SUBCKT: {
            ElemSubcktParallel *p = (ElemSubcktParallel *)any;
            int                 i;
            for(i = 0; i < p->count; i++) {
                if(ContainsElem(p->contents[i].which, p->contents[i].data.any, seek)) {
                    return true;
                }
            }
            break;
        }
            CASE_LEAF
            if(any == seek)
                return true;
            break;

        case ELEM_PADDING:
            break;

        default:
            ooops("which=%d", which);
    }
    return false;
}

//-----------------------------------------------------------------------------
// Use ContainsElem to find the rung containing the cursor.
//-----------------------------------------------------------------------------
int RungContainingSelected()
{
    int i;
    for(i = 0; i < Prog.numRungs; i++) {
        if(ContainsElem(ELEM_SERIES_SUBCKT, Prog.rungs[i], Selected)) {
            return i;
        }
    }

    return -1;
}

//-----------------------------------------------------------------------------
// Delete the rung i
//-----------------------------------------------------------------------------
void DeleteRungI(int i)
{
    if(i < 0)
        return;

    FreeCircuit(ELEM_SERIES_SUBCKT, Prog.rungs[i]);
    (Prog.numRungs)--;
    memmove(&Prog.rungs[i], &Prog.rungs[i + 1], (Prog.numRungs - i) * sizeof(Prog.rungs[0]));
    memmove(&Prog.rungSelected[i], &Prog.rungSelected[i + 1], (Prog.numRungs - i) * sizeof(Prog.rungSelected[0]));
}

//-----------------------------------------------------------------------------
// Delete the rung that contains the cursor.
//-----------------------------------------------------------------------------
void DeleteSelectedRung()
{
    if(Prog.numRungs <= 1) {
        Error(_("Cannot delete rung; program must have at least one rung."));
        return;
    }

    int  gx, gy;
    bool foundCursor;
    foundCursor = FindSelected(&gx, &gy);

    int i = RungContainingSelected();
    if(i < 0)
        return;

    DeleteRungI(i);

    if(foundCursor)
        MoveCursorNear(&gx, &gy);

    WhatCanWeDoFromCursorAndTopology();
}

//-----------------------------------------------------------------------------
// Allocate a new `empty' rung, with only a single relay coil at the end. All
// the UI code assumes that rungs always have a coil in them, so it would
// add a lot of nasty special cases to create rungs totally empty.
//-----------------------------------------------------------------------------
static ElemSubcktSeries *AllocEmptyRung()
{
    ElemSubcktSeries *s = AllocSubcktSeries();
    s->count = 1;
    s->contents[0].which = ELEM_PLACEHOLDER;
    ElemLeaf *l = AllocLeaf();
    s->contents[0].data.leaf = l;

    return s;
}

//-----------------------------------------------------------------------------
static void NullDisplayMatrix(int from, int to)
{
    return;
    int i, j;
    for(j = from; j < to; j++) {
        for(i = 0; i < DISPLAY_MATRIX_X_SIZE; i++) {
            if(DisplayMatrixWhich[i][j] == ELEM_COMMENT) {
                DisplayMatrix[i][j] = nullptr;
                DisplayMatrixWhich[i][j] = ELEM_NULL;
            }
        }
    }
    for(j = 0; j < DISPLAY_MATRIX_Y_SIZE; j++) {
        for(i = 0; i < DISPLAY_MATRIX_X_SIZE; i++) {
            if(DisplayMatrixWhich[i][j] == ELEM_COMMENT) {
                DisplayMatrix[i][j] = nullptr;
                DisplayMatrixWhich[i][j] = ELEM_NULL;
            }
        }
    }
}
//-----------------------------------------------------------------------------
// Insert a rung before rung i.
//-----------------------------------------------------------------------------
void InsertRungI(int i)
{
    if(i < 0)
        return;

    if(Prog.numRungs >= (MAX_RUNGS - 1)) {
        Error(_("Too many rungs!"));
        return;
    }

    memmove(&Prog.rungs[i + 1], &Prog.rungs[i], (Prog.numRungs - i) * sizeof(Prog.rungs[0]));
    memmove(&Prog.rungSelected[i + 1], &Prog.rungSelected[i], (Prog.numRungs - i) * sizeof(Prog.rungSelected[0]));
    Prog.rungs[i] = AllocEmptyRung();
    (Prog.numRungs)++;
    NullDisplayMatrix(i, i + 1 + 1);
}

//-----------------------------------------------------------------------------
// Insert a rung either before or after the rung that contains the cursor.
//-----------------------------------------------------------------------------
void InsertRung(bool afterCursor)
{
    if(Prog.numRungs >= (MAX_RUNGS - 1)) {
        Error(_("Too many rungs!"));
        return;
    }

    int i = RungContainingSelected();
    if(i < 0)
        return;

    if(afterCursor)
        i++;
    InsertRungI(i);

    WhatCanWeDoFromCursorAndTopology();
}

//-----------------------------------------------------------------------------
// Swap the row containing the selected element with the one under it, or do
// nothing if the rung is the last in the program.
//-----------------------------------------------------------------------------
void PushRungDown()
{
    int i = RungContainingSelected();
    if(i == (Prog.numRungs - 1))
        return;

    int HeightBefore = 0;
    int j;
    for(j = 0; j < i; j++)
        HeightBefore += CountHeightOfElement(ELEM_SERIES_SUBCKT, Prog.rungs[j]);
    int HeightNow = CountHeightOfElement(ELEM_SERIES_SUBCKT, Prog.rungs[i]);
    int HeightDown = CountHeightOfElement(ELEM_SERIES_SUBCKT, Prog.rungs[i + 1]);

    ElemSubcktSeries *temp = Prog.rungs[i];
    Prog.rungs[i] = Prog.rungs[i + 1];
    Prog.rungs[i + 1] = temp;

    NullDisplayMatrix(HeightBefore, HeightBefore + HeightNow + HeightDown);

    WhatCanWeDoFromCursorAndTopology();
    ScrollSelectedIntoViewAfterNextPaint = true;
}

//-----------------------------------------------------------------------------
// Swap the row containing the selected element with the one above it, or do
// nothing if the rung is the last in the program.
//-----------------------------------------------------------------------------
void PushRungUp()
{
    int i = RungContainingSelected();
    if(i == 0)
        return;

    int HeightBefore = 0;
    int j;
    for(j = 0; j < i - 1; j++)
        HeightBefore += CountHeightOfElement(ELEM_SERIES_SUBCKT, Prog.rungs[j]);
    int HeightUp = CountHeightOfElement(ELEM_SERIES_SUBCKT, Prog.rungs[i - 1]);
    int HeightNow = CountHeightOfElement(ELEM_SERIES_SUBCKT, Prog.rungs[i]);

    ElemSubcktSeries *temp = Prog.rungs[i];
    Prog.rungs[i] = Prog.rungs[i - 1];
    Prog.rungs[i - 1] = temp;

    NullDisplayMatrix(HeightBefore, HeightBefore + HeightUp + HeightNow);

    WhatCanWeDoFromCursorAndTopology();
    ScrollSelectedIntoViewAfterNextPaint = true;
}

//-----------------------------------------------------------------------------
// Start a new project. Give them one rung, with a coil (that they can
// never delete) and nothing else.
//-----------------------------------------------------------------------------
void NewProgram()
{
    UndoFlush();
    FreeEntireProgram();

    Prog.numRungs = 1;
    Prog.rungs[0] = AllocEmptyRung();
}

//-----------------------------------------------------------------------------
// Worker for ItemIsLastInCircuit. Basically we look for seek in the given
// circuit, trying to see whether it occupies the last position in a series
// subcircuit (which may be in a parallel subcircuit that is in the last
// position of a series subcircuit that may be in a parallel subcircuit that
// etc.)
//-----------------------------------------------------------------------------
static void LastInCircuit(int which, void *any, ElemLeaf *seek, bool *found, bool *andItemAfter)
{
    switch(which) {
        case ELEM_PARALLEL_SUBCKT: {
            ElemSubcktParallel *p = (ElemSubcktParallel *)any;
            int                 i;
            for(i = 0; i < p->count; i++) {
                LastInCircuit(p->contents[i].which, p->contents[i].data.any, seek, found, andItemAfter);
                if(*found)
                    return;
            }
            break;
        }
        case ELEM_SERIES_SUBCKT: {
            ElemSubcktSeries *s = (ElemSubcktSeries *)any;
            LastInCircuit(
                s->contents[s->count - 1].which, s->contents[s->count - 1].data.any, seek, found, andItemAfter);
            break;
        }
        default:
            if(*found)
                *andItemAfter = true;
            if(any == seek)
                *found = true;
            break;
    }
}

//-----------------------------------------------------------------------------
// Is an item the last one in the circuit (i.e. does one of its terminals go
// to the rightmost bus bar)? We need to know this because that is the only
// circumstance in which it is okay to insert a coil, RES, etc. after
// something
//-----------------------------------------------------------------------------
bool ItemIsLastInCircuit(ElemLeaf *item)
{
    int i = RungContainingSelected();
    if(i < 0)
        return false;

    bool found = false;
    bool andItemAfter = false;

    LastInCircuit(ELEM_SERIES_SUBCKT, Prog.rungs[i], item, &found, &andItemAfter);

    if(found)
        return !andItemAfter;
    return false;
}

//-----------------------------------------------------------------------------
// Returns true if the subcircuit contains any of the given instruction
// types (ELEM_....), else false.
//-----------------------------------------------------------------------------
/*
bool ContainsWhich(int which, void *any, int seek1, int seek2, int seek3)
{
    switch(which) {
        case ELEM_PARALLEL_SUBCKT: {
            ElemSubcktParallel *p = (ElemSubcktParallel *)any;
            int i;
            for(i = 0; i < p->count; i++) {
                if(ContainsWhich(p->contents[i].which, p->contents[i].data.any,
                    seek1, seek2, seek3))
                {
                    return true;
                }
            }
            break;
        }
        case ELEM_SERIES_SUBCKT: {
            ElemSubcktSeries *s = (ElemSubcktSeries *)any;
            int i;
            for(i = 0; i < s->count; i++) {
                if(ContainsWhich(s->contents[i].which, s->contents[i].data.any,
                    seek1, seek2, seek3))
                {
                    return true;
                }
            }
            break;
        }
        default:
            if(which == seek1 || which == seek2 || which == seek3) {
                return true;
            }
            break;
    }
    return false;
}

bool ContainsWhich(int which, void *any, int seek1, int seek2)
{
    return ContainsWhich(which, any, seek1, seek2, -1);
}

bool ContainsWhich(int which, void *any, int seek1)
{
    return ContainsWhich(which, any, seek1, -1, -1);
}
*/
ElemLeaf *ContainsWhich(int which, void *any, int seek1, int seek2, int seek3)
{
    ElemLeaf *l;
    switch(which) {
        case ELEM_PARALLEL_SUBCKT: {
            ElemSubcktParallel *p = (ElemSubcktParallel *)any;
            int                 i;
            for(i = 0; i < p->count; i++) {
                if(l = ContainsWhich(p->contents[i].which, p->contents[i].data.any, seek1, seek2, seek3)) {
                    return l;
                }
            }
            break;
        }
        case ELEM_SERIES_SUBCKT: {
            ElemSubcktSeries *s = (ElemSubcktSeries *)any;
            int               i;
            for(i = 0; i < s->count; i++) {
                if(l = ContainsWhich(s->contents[i].which, s->contents[i].data.any, seek1, seek2, seek3)) {
                    return l;
                }
            }
            break;
        }
        default:
            if(which == seek1 || which == seek2 || which == seek3) {
                return (ElemLeaf *)any;
            }
            break;
    }
    return nullptr;
}

ElemLeaf *ContainsWhich(int which, void *any, int seek1, int seek2)
{
    return ContainsWhich(which, any, seek1, seek2, -1);
}

ElemLeaf *ContainsWhich(int which, void *any, int seek1)
{
    return ContainsWhich(which, any, seek1, -1, -1);
}

//-----------------------------------------------------------------------------
static bool _FindRung(int which, void *any, int seek, char *name)
{
    switch(which) {
        case ELEM_PARALLEL_SUBCKT: {
            ElemSubcktParallel *p = (ElemSubcktParallel *)any;
            int                 i;
            for(i = 0; i < p->count; i++)
                if(_FindRung(p->contents[i].which, p->contents[i].data.any, seek, name))
                    return true;
            break;
        }
        case ELEM_SERIES_SUBCKT: {
            ElemSubcktSeries *s = (ElemSubcktSeries *)any;
            int               i;
            for(i = 0; i < s->count; i++)
                if(_FindRung(s->contents[i].which, s->contents[i].data.any, seek, name))
                    return true;
            break;
        }
        case ELEM_SUBPROG:
        case ELEM_ENDSUB:
        case ELEM_LABEL: {
            if(which == seek) {
                ElemLeaf *leaf = (ElemLeaf *)any;
                ElemGoto *e = &leaf->d.doGoto;
                if(strcmp(e->rung, name) == 0)
                    return true;
            }
            break;
        }
        default:
            if(which == seek)
                return true; // ???
            break;
    }
    return false;
}

int FindRung(int seek, char *name)
{
    int i;
    for(i = 0; i < Prog.numRungs; i++)
        if(_FindRung(ELEM_SERIES_SUBCKT, Prog.rungs[i], seek, name))
            return i;
    return -1;
}

int FindRungLast(int seek, char *name)
{
    int i;
    for(i = Prog.numRungs - 1; i >= 0; i--)
        if(_FindRung(ELEM_SERIES_SUBCKT, Prog.rungs[i], seek, name))
            return i;
    return -1;
}

//-----------------------------------------------------------------------------
// Returns number of the given instruction
// types (ELEM_....) in the subcircuit.
//-----------------------------------------------------------------------------
static int CountWhich_(int which, void *any, int seek1, int seek2, int seek3, char *name)
{
    int n = 0;
    int i;
    switch(which) {
        case ELEM_PARALLEL_SUBCKT: {
            ElemSubcktParallel *p = (ElemSubcktParallel *)any;
            for(i = 0; i < p->count; i++)
                n += CountWhich_(p->contents[i].which, p->contents[i].data.any, seek1, seek2, seek3, name);
            break;
        }
        case ELEM_SERIES_SUBCKT: {
            ElemSubcktSeries *s = (ElemSubcktSeries *)any;
            for(i = 0; i < s->count; i++)
                n += CountWhich_(s->contents[i].which, s->contents[i].data.any, seek1, seek2, seek3, name);
            break;
        }
        default:
            if(which == seek1 || which == seek2 || which == seek3) {
                if(!name || !strlen(name))
                    n++;
                else {
                    ElemLeaf *leaf = (ElemLeaf *)any;
                    ElemCoil *c = &leaf->d.coil; // !!!
                    if(strcmp(c->name, name) == 0)
                        n++;
                }
            }
            break;
    }
    return n;
}

int CountWhich(int seek1, int seek2, int seek3, char *name)
{
    int n = 0;
    for(int i = 0; i < Prog.numRungs; i++)
        n += CountWhich_(ELEM_SERIES_SUBCKT, Prog.rungs[i], seek1, seek2, seek3, name);
    return n;
}

int CountWhich(int seek1, int seek2, char *name)
{
    return CountWhich(seek1, seek2, -1, name);
}

int CountWhich(int seek1, char *name)
{
    return CountWhich(seek1, -1, -1, name);
}

int CountWhich(int seek1)
{
    return CountWhich(seek1, -1, -1, nullptr);
}

bool DelayUsed()
{
    int i;
    for(i = 0; i < Prog.numRungs; i++) {
        if(ContainsWhich(ELEM_SERIES_SUBCKT, Prog.rungs[i], ELEM_DELAY, -1, -1)) {
            return true;
        }
    }
    return false;
}

bool TablesUsed()
{
    int i;
    for(i = 0; i < Prog.numRungs; i++) {
        if((ContainsWhich(
               ELEM_SERIES_SUBCKT, Prog.rungs[i], ELEM_LOOK_UP_TABLE, ELEM_PIECEWISE_LINEAR, ELEM_SHIFT_REGISTER))
           || (ContainsWhich(ELEM_SERIES_SUBCKT, Prog.rungs[i], ELEM_FORMATTED_STRING, ELEM_7SEG, ELEM_9SEG))
           || (ContainsWhich(ELEM_SERIES_SUBCKT, Prog.rungs[i], ELEM_14SEG, ELEM_16SEG, ELEM_QUAD_ENCOD))) {
            return true;
        }
    }
    return false;
}
/* function moved to intcode.cpp
//-----------------------------------------------------------------------------
// Are either of the UART functions (send or recv) used? Need to know this
// to know whether we must receive their pins.
//-----------------------------------------------------------------------------
bool UartFunctionUsed()
{
    int i;
    for(i = 0; i < Prog.numRungs; i++) {
        if((ContainsWhich(ELEM_SERIES_SUBCKT, Prog.rungs[i],
            ELEM_UART_RECV, ELEM_UART_SEND, ELEM_FORMATTED_STRING))
        ||(ContainsWhich(ELEM_SERIES_SUBCKT, Prog.rungs[i],
            ELEM_UART_SEND_READY, -1, -1)))
        {
            return true;
        }
    }
    return false;
}
*/
//-----------------------------------------------------------------------------
// Is the PWM function used? Need to know this to know whether we must reserve
// the pin.
//-----------------------------------------------------------------------------
/*
int PwmFunctionUsed()
{
    int n = 0;
    int i;
    for(i = 0; i < Prog.numRungs; i++) {
        if(ContainsWhich(ELEM_SERIES_SUBCKT, Prog.rungs[i], ELEM_SET_PWM,
            -1, -1))
        {
            n++;
        }
    }
    return n;
}
/**/
int PwmFunctionUsed()
{
    return CountWhich(ELEM_SET_PWM);
}
/**/
//-----------------------------------------------------------------------------
/*
int _AdcFunctionUsed()
{
    int n = 0;
    int i;
    for(i = 0; i < Prog.numRungs; i++) {
        if(ContainsWhich(ELEM_SERIES_SUBCKT, Prog.rungs[i], ELEM_READ_ADC,
            -1, -1))
        {
            n++;
        }
    }
    return n;
}
*/
int AdcFunctionUsed()
{
    return CountWhich(ELEM_READ_ADC);
}
//-----------------------------------------------------------------------------
uint32_t QuadEncodFunctionUsed()
{
    return CountWhich(ELEM_QUAD_ENCOD);
}
//-----------------------------------------------------------------------------
bool NPulseFunctionUsed()
{
    for(int i = 0; i < Prog.numRungs; i++) {
        if(ContainsWhich(ELEM_SERIES_SUBCKT, Prog.rungs[i], ELEM_NPULSE, -1, -1)) {
            return true;
        }
    }
    return false;
}
//-----------------------------------------------------------------------------
bool EepromFunctionUsed()
{
    for(int i = 0; i < Prog.numRungs; i++) {
        if(ContainsWhich(ELEM_SERIES_SUBCKT, Prog.rungs[i], ELEM_PERSIST, -1, -1)) {
            return true;
        }
    }
    return false;
}
//-----------------------------------------------------------------------------
bool SleepFunctionUsed()
{
    for(int i = 0; i < Prog.numRungs; i++) {
        if(ContainsWhich(ELEM_SERIES_SUBCKT, Prog.rungs[i], ELEM_SLEEP, -1, -1)) {
            return true;
        }
    }
    return false;
}
//-----------------------------------------------------------------------------
// copy the selected rung temporar, InsertRung and
// save in the new rung temp
//-----------------------------------------------------------------------------
const char *CLP = "ldmicro.tmp";
void        CopyRungDown()
{
    int               i = RungContainingSelected();
    char              line[512];
    ElemSubcktSeries *temp = Prog.rungs[i];

    //FILE *f = fopen(CLP, "w+TD");
    FILE *f = fopen(CLP, "w+");
    if(!f) {
        Error(_("Couldn't open file '%s'"), CLP);
        return;
    }
    //fprintf(f, "RUNG\n");
    SaveElemToFile(f, ELEM_SERIES_SUBCKT, temp, 0, i);
    //fprintf(f, "END\n");

    rewind(f);
    fgets(line, sizeof(line), f);
    if(strstr(line, "RUNG"))
        if(temp = LoadSeriesFromFile(f)) {
            InsertRung(true);
            Prog.rungs[i + 1] = temp;
        }
    fclose(f);

    WhatCanWeDoFromCursorAndTopology();
    ScrollSelectedIntoViewAfterNextPaint = true;
}

//-----------------------------------------------------------------------------
void CutRung()
{
    int i;

    FILE *f = fopen(CLP, "w+");
    if(!f) {
        Error(_("Couldn't open file '%s'"), CLP);
        return;
    }
    int SelN = 0;
    for(i = 0; i < Prog.numRungs; i++)
        if(Prog.rungSelected[i] == '*')
            SelN++;
    if(!SelN) {
        i = RungContainingSelected();
        if(i >= 0)
            Prog.rungSelected[i] = '*';
    }
    for(i = (Prog.numRungs - 1); i >= 0; i--)
        if(Prog.rungSelected[i] == '*') {
            SaveElemToFile(f, ELEM_SERIES_SUBCKT, Prog.rungs[i], 0, i);
            DeleteRungI(i);
        }
    fclose(f);

    if(Prog.numRungs == 0) {
        Prog.numRungs = 1;
        Prog.rungs[0] = AllocEmptyRung();
    }

    WhatCanWeDoFromCursorAndTopology();
    //ScrollSelectedIntoViewAfterNextPaint = true;
}

//-----------------------------------------------------------------------------
void CopyRung()
{
    FILE *f = fopen(CLP, "w+");
    if(!f) {
        Error(_("Couldn't open file '%s'"), CLP);
        return;
    }
    int i;
    int SelN = 0;
    for(i = 0; i < Prog.numRungs; i++)
        if(Prog.rungSelected[i] == '*')
            SelN++;
    if(!SelN) {
        i = RungContainingSelected();
        if(i >= 0)
            Prog.rungSelected[i] = '*';
    }
    for(i = 0; i < Prog.numRungs; i++)
        if(Prog.rungSelected[i] > '*') {
            Prog.rungSelected[i] = ' ';
        } else if(Prog.rungSelected[i] == '*') {
            SaveElemToFile(f, ELEM_SERIES_SUBCKT, Prog.rungs[i], 0, i);
            Prog.rungSelected[i] = 'R';
        }
    fclose(f);
}

//-----------------------------------------------------------------------------
void CopyElem()
{
    if(!Selected)
        return;

    FILE *f = fopen(CLP, "w+");
    if(!f) {
        Error(_("Couldn't open file '%s'"), CLP);
        return;
    }
    int i;
    int SelN = 0;
    for(i = 0; i < Prog.numRungs; i++)
        if(Prog.rungSelected[i] == '*')
            SelN++;
    if(!SelN) {
        i = RungContainingSelected();
        if(i >= 0)
            Prog.rungSelected[i] = '*';
    }
    for(i = 0; i < Prog.numRungs; i++)
        if(Prog.rungSelected[i] > '*') {
            Prog.rungSelected[i] = ' ';
        } else if(Prog.rungSelected[i] == '*') {
            fprintf(f, "RUNG\n");
            SaveElemToFile(f, SelectedWhich, Selected, 0, 0);
            fprintf(f, "END\n");
            if(EndOfRungElem(SelectedWhich))
                Prog.rungSelected[i] = 'E';
            else
                Prog.rungSelected[i] = 'L';
        }

    fclose(f);
}

//-----------------------------------------------------------------------------
void PasteRung(int PasteInTo)
{
    if(PasteInTo)
        if(!(CanInsertEnd || CanInsertOther))
            return;

    int j = RungContainingSelected();
    if(j < 0)
        j = 0;

    if(PasteInTo == 0)
        if(Selected && ((Selected->selectedState == SELECTED_BELOW) || (Selected->selectedState == SELECTED_RIGHT))) {
            j++;
        }

    ElemSubcktSeries *temp;

    FILE *f = fopen(CLP, "r");
    if(!f) {
        Error(_("Couldn't open file '%s'"), CLP);
        Error(_("You must Select rungs, then Copy or Cut, then Paste."));
        return;
    }
    int  i;
    char line[512];
    int  rung;

    for(rung = 0; rung == 0;) {
        if(!fgets(line, sizeof(line), f))
            break;
        if(strstr(line, "RUNG"))
            if(temp = LoadSeriesFromFile(f)) {
                if(SelectedWhich == ELEM_PLACEHOLDER) {
                    Prog.rungs[j] = temp;
                    rung = 1;
                } else if(PasteInTo == 0) {
                    //insert rungs from file
                    InsertRungI(j);
                    Prog.rungs[j] = temp;
                    j++;
                } else if(PasteInTo == 1) {
                    if(j >= Prog.numRungs)
                        j = Prog.numRungs - 1;
                    //insert rung INTO series
                    bool doCollapse = false;
                    if(Selected && (Selected->selectedState != SELECTED_NONE)) {
                        if(!EndOfRungElem(SelectedWhich) || (Selected->selectedState == SELECTED_LEFT))
                            if(!ItemIsLastInCircuit(Selected) || (Selected->selectedState == SELECTED_LEFT)) {
                                doCollapse = false;
                                for(i = temp->count - 1; i >= 0; i--) {
                                    if(DeleteAnyFromSubckt(ELEM_SERIES_SUBCKT,
                                                           temp,
                                                           temp->contents[i].which,
                                                           temp->contents[i].data.any,
                                                           true))
                                        doCollapse = true;
                                }
                                if(doCollapse)
                                    while(CollapseUnnecessarySubckts(ELEM_SERIES_SUBCKT, temp))
                                        ;
                            }
                        if(EndOfRungElem(SelectedWhich) && CanInsertEnd
                           && ((Selected->selectedState == SELECTED_BELOW)
                               || (Selected->selectedState == SELECTED_ABOVE))) {
                            doCollapse = false;
                            for(i = temp->count - 1; i >= 0; i--) {
                                if(DeleteAnyFromSubckt(ELEM_SERIES_SUBCKT,
                                                       temp,
                                                       temp->contents[i].which,
                                                       temp->contents[i].data.any,
                                                       false))
                                    doCollapse = true;
                            }
                            if(doCollapse)
                                while(CollapseUnnecessarySubckts(ELEM_SERIES_SUBCKT, temp))
                                    ;
                        }
                        if(temp->count > 0) {
                            if(AddLeaf(ELEM_SERIES_SUBCKT, (ElemLeaf *)temp)) {
                                while(CollapseUnnecessarySubckts(ELEM_SERIES_SUBCKT, Prog.rungs[j])) {
                                    ProgramChanged();
                                }
                            }
                        }
                    }
                } else
                    oops();
            }
    }
    for(i = 0; i < Prog.numRungs; i++) {
        if(Prog.rungSelected[i] != ' ')
            Prog.rungSelected[i] = ' ';
    }
    fclose(f);

    WhatCanWeDoFromCursorAndTopology();
}

//-----------------------------------------------------------------------------
static void RenameSet1_(int which, void *any, int which_elem, char *name, char *new_name, bool set1)
{
    switch(which) {
        case ELEM_PARALLEL_SUBCKT: {
            ElemSubcktParallel *p = (ElemSubcktParallel *)any;
            for(int i = 0; i < p->count; i++)
                RenameSet1_(p->contents[i].which, p->contents[i].data.any, which_elem, name, new_name, set1);
            break;
        }
        case ELEM_SERIES_SUBCKT: {
            ElemSubcktSeries *s = (ElemSubcktSeries *)any;
            for(int i = 0; i < s->count; i++)
                RenameSet1_(s->contents[i].which, s->contents[i].data.any, which_elem, name, new_name, set1);
            break;
        }
        case ELEM_COIL: {
            ElemLeaf *leaf = (ElemLeaf *)any;
            ElemCoil *c = &leaf->d.coil;
            if(strcmp(c->name, name) == 0) {
                if(new_name && strlen(new_name))
                    //if(which_elem == ELEM_COIL) ???
                    if(new_name[0] == name[0])
                        strcpy(c->name, new_name);
            }
            break;
        }
        case ELEM_CONTACTS: {
            ElemLeaf *    leaf = (ElemLeaf *)any;
            ElemContacts *c = &leaf->d.contacts;
            if(strcmp(c->name, name) == 0) {
                if(new_name && strlen(new_name))
                    strcpy(c->name, new_name);
                if(which == ELEM_CONTACTS)
                    c->set1 = set1;
            }
            break;
        }
        default:
            break;
    }
}

//-----------------------------------------------------------------------------
void RenameSet1(int which, char *name, char *new_name, bool set1)
{
    for(int i = 0; i < Prog.numRungs; i++)
        RenameSet1_(ELEM_SERIES_SUBCKT, Prog.rungs[i], which, name, new_name, set1);
}

//-----------------------------------------------------------------------------
static void *FindElem_(int which, void *any, int which_elem, char *name)
{
    void *res;
    switch(which) {
        case ELEM_PARALLEL_SUBCKT: {
            ElemSubcktParallel *p = (ElemSubcktParallel *)any;
            for(int i = 0; i < p->count; i++) {
                res = FindElem_(p->contents[i].which, p->contents[i].data.any, which_elem, name);
                if(res)
                    return res;
            }
            break;
        }
        case ELEM_SERIES_SUBCKT: {
            ElemSubcktSeries *s = (ElemSubcktSeries *)any;
            for(int i = 0; i < s->count; i++) {
                res = FindElem_(s->contents[i].which, s->contents[i].data.any, which_elem, name);
                if(res)
                    return res;
            }
            break;
        }
        case ELEM_CTD:
        case ELEM_CTU:
        case ELEM_CTC:
        case ELEM_CTR: {
            ElemLeaf *leaf = (ElemLeaf *)any;
            ElemCounter *c = &leaf->d.counter;
            if(strcmp(c->name, name) == 0) {
                return c;
            }
            break;
        }
        default: {
            if(which == which_elem)
                 oops();
            return nullptr;
            break;
        }
    }
    return nullptr;
}

//-----------------------------------------------------------------------------
void *FindElem(int which, char *name)
{
    void *res;
    for(int i = 0; i < Prog.numRungs; i++) {
        res = FindElem_(ELEM_SERIES_SUBCKT, Prog.rungs[i], which, name);
        if(res)
            return res;
    }
    return nullptr;
}

