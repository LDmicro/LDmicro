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
// Load/save the circuit from/to a file in a nice ASCII format.
// Jonathan Westhues, Nov 2004
//-----------------------------------------------------------------------------
#include "stdafx.h"

#include "ldmicro.h"
#include "pcports.h"

char *FrmStrToStr(char *dest);
//void FrmStrToFile(FILE *f, char *str);
char *DelNL(char *str);
char *DelLastNL(char *str);

typedef enum FRMTTag { FRMT_COMMENT, FRMT_01, FRMT_x20 } FRMT;
char *StrToFrmStr(char *dest, const char *str, FRMT frmt);
char *StrToFrmStr(char *dest, const char *src);

ElemSubcktSeries *LoadSeriesFromFile(FileTracker& f);

//-----------------------------------------------------------------------------
// Check a line of text from a saved project file to determine whether it
// contains a leaf element (coil, contacts, etc.). If so, create an element
// for and save that in *any and *which, and return true, else return false.
//-----------------------------------------------------------------------------
static bool LoadLeafFromFile(char *line, void **any, int *which)
{
    ElemLeaf *l = AllocLeaf();
    int       x;

    auto scan_contact_3 = [&]() -> int {
        int negated, set1;
        int res = sscanf(line, "CONTACTS %s %d %d", l->d.contacts.name, &negated, &set1);
        if(res == 3) {
            l->d.contacts.negated = negated != 0;
            l->d.contacts.set1 = set1 != 0;
        }
        return res;
    };
    auto scan_contact_2 = [&]() -> int {
        int negated;
        int res = sscanf(line, "CONTACTS %s %d ", l->d.contacts.name, &negated);
        if(res == 2) {
            l->d.contacts.negated = negated != 0;
        }
        return res;
    };

    auto scan_coil_5 = [&]() -> int {
        int negated, setOnly, resetOnly, ttrigger;
        int res = sscanf(line, "COIL %s %d %d %d %d", l->d.coil.name, &negated, &setOnly, &resetOnly, &ttrigger);
        if(res == 5) {
            l->d.coil.negated = negated != 0;
            l->d.coil.setOnly = setOnly != 0;
            l->d.coil.resetOnly = resetOnly != 0;
            l->d.coil.ttrigger = ttrigger != 0;
        }
        return res;
    };

    auto scan_coil_4 = [&]() -> int {
        int negated, setOnly, resetOnly;
        int res = sscanf(line, "COIL %s %d %d %d", l->d.coil.name, &negated, &setOnly, &resetOnly);
        if(res == 4) {
            l->d.coil.negated = negated != 0;
            l->d.coil.setOnly = setOnly != 0;
            l->d.coil.resetOnly = resetOnly != 0;
        }
        return res;
    };
    bool scanned = true;
    if(memcmp(line, "COMMENT", 7) == 0) {
        FrmStrToStr(l->d.comment.str, &line[8]);
        *which = ELEM_COMMENT;
    } else if(scan_contact_3() == 3) {
        *which = ELEM_CONTACTS;
    } else if(scan_contact_2() == 2) {
        *which = ELEM_CONTACTS;
    } else if(scan_coil_5() == 5) {
        *which = ELEM_COIL;
    } else if(scan_coil_4() == 4) {
        *which = ELEM_COIL;
    } else if(memcmp(line, "PLACEHOLDER", 11) == 0) {
        *which = ELEM_PLACEHOLDER;
    } else if(memcmp(line, "SHORT", 5) == 0) {
        *which = ELEM_SHORT;
    } else if(memcmp(line, "OPEN", 4) == 0) {
        *which = ELEM_OPEN;
    } else if(memcmp(line, "MASTER_RELAY", 12) == 0) {
        *which = ELEM_MASTER_RELAY;
    } else if((sscanf(line, "DELAY %s", l->d.timer.name) == 1)) {
        *which = ELEM_DELAY;
    } else if((sscanf(line, "SLEEP %s %s", l->d.timer.name, l->d.timer.delay) == 2)) {
        *which = ELEM_SLEEP;
    } else if(memcmp(line, "SLEEP", 5) == 0) {
        *which = ELEM_SLEEP;
    } else if(memcmp(line, "CLRWDT", 6) == 0) {
        *which = ELEM_CLRWDT;
    } else if(memcmp(line, "LOCK", 4) == 0) {
        *which = ELEM_LOCK;
    } else if(sscanf(line, "GOTO %s", l->d.doGoto.label) == 1) {
        *which = ELEM_GOTO;
    } else if(sscanf(line, "GOSUB %s", l->d.doGoto.label) == 1) {
        *which = ELEM_GOSUB;
    } else if(memcmp(line, "RETURN", 6) == 0) {
        *which = ELEM_RETURN;
    } else if(sscanf(line, "LABEL %s", l->d.doGoto.label) == 1) {
        *which = ELEM_LABEL;
    } else if(sscanf(line, "SUBPROG %s", l->d.doGoto.label) == 1) {
        *which = ELEM_SUBPROG;
    } else if(sscanf(line, "ENDSUB %s", l->d.doGoto.label) == 1) {
        *which = ELEM_ENDSUB;
    } else if(sscanf(line, "SHIFT_REGISTER %s %d", l->d.shiftRegister.name, &(l->d.shiftRegister.stages)) == 2) {
        *which = ELEM_SHIFT_REGISTER;
    } else if(memcmp(line, "OSR", 3) == 0) {
        *which = ELEM_ONE_SHOT_RISING;
    } else if(memcmp(line, "OSF", 3) == 0) {
        *which = ELEM_ONE_SHOT_FALLING;
    } else if(memcmp(line, "OSL", 3) == 0) {
        *which = ELEM_ONE_SHOT_LOW;
    } else if(memcmp(line, "OSC", 3) == 0) {
        *which = ELEM_OSC;
    } else if(memcmp(line, "NPULSE_OFF", 10) == 0) {
        *which = ELEM_NPULSE_OFF;
    } else if((sscanf(line, "TIME2COUNT %s %s", l->d.timer.name, l->d.timer.delay) == 2)) {
        *which = ELEM_TIME2COUNT;
    } else if((sscanf(line, "TIME2DELAY %s %s", l->d.timer.name, l->d.timer.delay) == 2)) {
        *which = ELEM_TIME2DELAY;

    } else if((sscanf(line, "TON %s %s %d", l->d.timer.name, l->d.timer.delay, &l->d.timer.adjust) == 3)) {
        *which = ELEM_TON;
    } else if((sscanf(line, "TOF %s %s %d", l->d.timer.name, l->d.timer.delay, &l->d.timer.adjust) == 3)) {
        *which = ELEM_TOF;
    } else if((sscanf(line, "RTO %s %s %d", l->d.timer.name, l->d.timer.delay, &l->d.timer.adjust) == 3)) {
        *which = ELEM_RTO;
    } else if((sscanf(line, "RTL %s %s %d", l->d.timer.name, l->d.timer.delay, &l->d.timer.adjust) == 3)) {
        *which = ELEM_RTL;
    } else if((sscanf(line, "TCY %s %s %d", l->d.timer.name, l->d.timer.delay, &l->d.timer.adjust) == 3)) {
        *which = ELEM_TCY;
    } else if((sscanf(line, "THI %s %s %d", l->d.timer.name, l->d.timer.delay, &l->d.timer.adjust) == 3)) {
        *which = ELEM_THI;
    } else if((sscanf(line, "TLO %s %s %d", l->d.timer.name, l->d.timer.delay, &l->d.timer.adjust) == 3)) {
        *which = ELEM_TLO;

    } else if((sscanf(line, "TON %s %s", l->d.timer.name, l->d.timer.delay) == 2)) {
        *which = ELEM_TON;
        if(Prog.LDversion == "0.1")
            l->d.timer.adjust = -1;
        else
            l->d.timer.adjust = 0;
    } else if((sscanf(line, "TOF %s %s", l->d.timer.name, l->d.timer.delay) == 2)) {
        *which = ELEM_TOF;
        if(Prog.LDversion == "0.1")
            l->d.timer.adjust = -1;
        else
            l->d.timer.adjust = 0;
    } else if((sscanf(line, "RTO %s %s", l->d.timer.name, l->d.timer.delay) == 2)) {
        *which = ELEM_RTO;
        if(Prog.LDversion == "0.1")
            l->d.timer.adjust = -1;
        else
            l->d.timer.adjust = 0;
    } else if((sscanf(line, "RTL %s %s", l->d.timer.name, l->d.timer.delay) == 2)) {
        *which = ELEM_RTL;
        l->d.timer.adjust = 0;
    } else if((sscanf(line, "TCY %s %s", l->d.timer.name, l->d.timer.delay) == 2)) {
        *which = ELEM_TCY;
        l->d.timer.adjust = 0;
    } else if((sscanf(line, "THI %s %s", l->d.timer.name, l->d.timer.delay) == 2)) {
        *which = ELEM_THI;
        l->d.timer.adjust = 0;
    } else if((sscanf(line, "TLO %s %s", l->d.timer.name, l->d.timer.delay) == 2)) {
        *which = ELEM_TLO;
        l->d.timer.adjust = 0;

    } else if((sscanf(line, "CTR %s %s %s %c", l->d.counter.name, l->d.counter.max, l->d.counter.init, &l->d.counter.inputKind) == 4)) {
        *which = ELEM_CTR;
    } else if((sscanf(line, "CTC %s %s %s %c", l->d.counter.name, l->d.counter.max, l->d.counter.init, &l->d.counter.inputKind) == 4)) {
        *which = ELEM_CTC;
    } else if((sscanf(line, "CTU %s %s %s %c", l->d.counter.name, l->d.counter.max, l->d.counter.init, &l->d.counter.inputKind) == 4)) {
        *which = ELEM_CTU;
    } else if((sscanf(line, "CTD %s %s %s %c", l->d.counter.name, l->d.counter.max, l->d.counter.init, &l->d.counter.inputKind) == 4)) {
        *which = ELEM_CTD;

    } else if((sscanf(line, "CTR %s %s %s", l->d.counter.name, l->d.counter.max, l->d.counter.init) == 3)) {
        *which = ELEM_CTR;
        l->d.counter.inputKind = '/';
    } else if((sscanf(line, "CTC %s %s %s", l->d.counter.name, l->d.counter.max, l->d.counter.init) == 3)) {
        *which = ELEM_CTC;
        l->d.counter.inputKind = '/';
    } else if((sscanf(line, "CTU %s %s %s", l->d.counter.name, l->d.counter.max, l->d.counter.init) == 3)) {
        *which = ELEM_CTU;
        l->d.counter.inputKind = '/';
    } else if((sscanf(line, "CTD %s %s %s", l->d.counter.name, l->d.counter.max, l->d.counter.init) == 3)) {
        l->d.counter.inputKind = '/';
        *which = ELEM_CTD;

    } else if((sscanf(line, "CTD %s %s", l->d.counter.name, l->d.counter.max) == 2)) {
        strcpy(l->d.counter.init, "0");
        l->d.counter.inputKind = '/';
        *which = ELEM_CTD;
    } else if((sscanf(line, "CTU %s %s", l->d.counter.name, l->d.counter.max) == 2)) {
        strcpy(l->d.counter.init, "0");
        l->d.counter.inputKind = '/';
        *which = ELEM_CTU;
    } else if((sscanf(line, "CTC %s %s", l->d.counter.name, l->d.counter.max) == 2)) {
        strcpy(l->d.counter.init, "0");
        l->d.counter.inputKind = '/';
        *which = ELEM_CTC;

    } else if(sscanf(line, "RES %s", l->d.reset.name) == 1) {
        *which = ELEM_RES;

    } else if(sscanf(line, "MOVE %s %s", l->d.move.dest, l->d.move.src) == 2) {
        *which = ELEM_MOVE;

    } else if(sscanf(line, "BIN2BCD %s %s", l->d.move.dest, l->d.move.src) == 2) {
        *which = ELEM_BIN2BCD;

    } else if(sscanf(line, "BCD2BIN %s %s", l->d.move.dest, l->d.move.src) == 2) {
        *which = ELEM_BCD2BIN;

    } else if(sscanf(line, "OPPOSITE %s %s", l->d.move.dest, l->d.move.src) == 2) {
        *which = ELEM_OPPOSITE;

    } else if(sscanf(line, "SWAP %s %s", l->d.move.dest, l->d.move.src) == 2) {
        *which = ELEM_SWAP;

    } else if(sscanf(line,
                     "BUS %s %s %d %d %d %d %d %d %d %d",
                     l->d.bus.dest,
                     l->d.bus.src,
                     &l->d.bus.PCBbit[7],
                     &l->d.bus.PCBbit[6],
                     &l->d.bus.PCBbit[5],
                     &l->d.bus.PCBbit[4],
                     &l->d.bus.PCBbit[3],
                     &l->d.bus.PCBbit[2],
                     &l->d.bus.PCBbit[1],
                     &l->d.bus.PCBbit[0])
              == (2 + 8)) {
        *which = ELEM_BUS;

    } else if(sscanf(line,
                     "SPI_WR %s %s %s %s %s %s %s %s",
                     l->d.spi.name,
                     l->d.spi.send,
                     l->d.spi.recv,
                     l->d.spi.mode,
                     l->d.spi.modes,
                     l->d.spi.size,
                     l->d.spi.first,
                     l->d.spi.bitrate)
              == 8) {
        l->d.spi.which = ELEM_SPI_WR;
        *which = ELEM_SPI_WR;

    } else if(sscanf(line,
                     "SPI %s %s %s %s %s %s %s %s",
                     l->d.spi.name,
                     l->d.spi.send,
                     l->d.spi.recv,
                     l->d.spi.mode,
                     l->d.spi.modes,
                     l->d.spi.size,
                     l->d.spi.first,
                     l->d.spi.bitrate)
              == 8) {
        l->d.spi.which = ELEM_SPI;
        *which = ELEM_SPI;

    }
    ///// Added by JG
      else if(sscanf(line,
                     "I2C_RD %s %s %s %s %s %s %s %s",
                     l->d.i2c.name,
                     l->d.i2c.send,
                     l->d.i2c.recv,
                     l->d.i2c.mode,
                     l->d.i2c.address,
                     l->d.i2c.registr,
                     l->d.i2c.first,
                     l->d.i2c.bitrate)
              == 8) {
        l->d.i2c.which = ELEM_I2C_RD;
        *which = ELEM_I2C_RD;
    }
      else if(sscanf(line,
                     "I2C_WR %s %s %s %s %s %s %s %s",
                     l->d.i2c.name,
                     l->d.i2c.send,
                     l->d.i2c.recv,
                     l->d.i2c.mode,
                     l->d.i2c.address,
                     l->d.i2c.registr,
                     l->d.i2c.first,
                     l->d.i2c.bitrate)
              == 8) {
        l->d.i2c.which = ELEM_I2C_WR;
        *which = ELEM_I2C_WR;
    }
    /////

     else if(sscanf(line, "7SEGMENTS %s %s %c", l->d.segments.dest, l->d.segments.src, &l->d.segments.common) == 3) {
        l->d.segments.which = ELEM_7SEG;
        *which = ELEM_7SEG;

    } else if(sscanf(line, "9SEGMENTS %s %s %c", l->d.segments.dest, l->d.segments.src, &l->d.segments.common) == 3) {
        l->d.segments.which = ELEM_9SEG;
        *which = ELEM_9SEG;

    } else if(sscanf(line, "14SEGMENTS %s %s %c", l->d.segments.dest, l->d.segments.src, &l->d.segments.common) == 3) {
        l->d.segments.which = ELEM_14SEG;
        *which = ELEM_14SEG;

    } else if(sscanf(line, "16SEGMENTS %s %s %c", l->d.segments.dest, l->d.segments.src, &l->d.segments.common) == 3) {
        l->d.segments.which = ELEM_16SEG;
        *which = ELEM_16SEG;

    } else if(sscanf(line,
                     "STEPPER %s %s %s %d %d %s",
                     l->d.stepper.name,
                     l->d.stepper.max,
                     l->d.stepper.P,
                     &l->d.stepper.nSize,
                     &l->d.stepper.graph,
                     l->d.stepper.coil)
              == 6) {
        *which = ELEM_STEPPER;
    } else if(sscanf(line,
                     "PULSER %s %s %s %s %s",
                     l->d.pulser.P1,
                     l->d.pulser.P0,
                     l->d.pulser.accel,
                     l->d.pulser.counter,
                     l->d.pulser.busy)
              == 5) {
        *which = ELEM_PULSER;
    } else if(sscanf(line, "NPULSE %s %s %s", l->d.Npulse.counter, l->d.Npulse.targetFreq, l->d.Npulse.coil) == 3) {
        *which = ELEM_NPULSE;
    } else if(sscanf(line,
                     "QUAD_ENCOD %s %d %s %s %s %s %c %d",
                     l->d.QuadEncod.counter,
                     &l->d.QuadEncod.int01,
                     l->d.QuadEncod.inputA,
                     l->d.QuadEncod.inputB,
                     l->d.QuadEncod.inputZ,
                     l->d.QuadEncod.dir,
                     &l->d.QuadEncod.inputZKind,
                     &l->d.QuadEncod.countPerRevol)
              == 8) {
        FrmStrToStr(l->d.QuadEncod.inputZ, l->d.QuadEncod.inputZ);
        FrmStrToStr(l->d.QuadEncod.dir, l->d.QuadEncod.dir);
        *which = ELEM_QUAD_ENCOD;
    } else if(sscanf(line, "MOD %s %s %s", l->d.math.dest, l->d.math.op1, l->d.math.op2) == 3) {
        *which = ELEM_MOD;
    } else if(sscanf(line, "SHL %s %s %s", l->d.math.dest, l->d.math.op1, l->d.math.op2) == 3) {
        *which = ELEM_SHL;
    } else if(sscanf(line, "SHR %s %s %s", l->d.math.dest, l->d.math.op1, l->d.math.op2) == 3) {
        *which = ELEM_SHR;
    } else if(sscanf(line, "SR0 %s %s %s", l->d.math.dest, l->d.math.op1, l->d.math.op2) == 3) {
        *which = ELEM_SR0;
    } else if(sscanf(line, "ROL %s %s %s", l->d.math.dest, l->d.math.op1, l->d.math.op2) == 3) {
        *which = ELEM_ROL;
    } else if(sscanf(line, "ROR %s %s %s", l->d.math.dest, l->d.math.op1, l->d.math.op2) == 3) {
        *which = ELEM_ROR;
    } else if(sscanf(line, "AND %s %s %s", l->d.math.dest, l->d.math.op1, l->d.math.op2) == 3) {
        *which = ELEM_AND;
    } else if(sscanf(line, "OR %s %s %s", l->d.math.dest, l->d.math.op1, l->d.math.op2) == 3) {
        *which = ELEM_OR;
    } else if(sscanf(line, "XOR %s %s %s", l->d.math.dest, l->d.math.op1, l->d.math.op2) == 3) {
        *which = ELEM_XOR;
    } else if(sscanf(line, "NOT %s %s", l->d.math.dest, l->d.math.op1) == 2) {
        *which = ELEM_NOT;
    } else if(sscanf(line, "NEG %s %s", l->d.math.dest, l->d.math.op1) == 2) {
        *which = ELEM_NEG;
    } else if(sscanf(line, "ADD %s %s %s", l->d.math.dest, l->d.math.op1, l->d.math.op2) == 3) {
        *which = ELEM_ADD;
    } else if(sscanf(line, "SUB %s %s %s", l->d.math.dest, l->d.math.op1, l->d.math.op2) == 3) {
        *which = ELEM_SUB;
    } else if(sscanf(line, "MUL %s %s %s", l->d.math.dest, l->d.math.op1, l->d.math.op2) == 3) {
        *which = ELEM_MUL;
    } else if(sscanf(line, "DIV %s %s %s", l->d.math.dest, l->d.math.op1, l->d.math.op2) == 3) {
        *which = ELEM_DIV;
    } else
        scanned = false;
    if (scanned) {
        ; //
    } else if(sscanf(line, "SET_BIT %s %s", l->d.move.dest, l->d.move.src) == 2) {
        *which = ELEM_SET_BIT;
    } else if(sscanf(line, "CLEAR_BIT %s %s", l->d.move.dest, l->d.move.src) == 2) {
        *which = ELEM_CLEAR_BIT;
    } else if(sscanf(line, "IF_BIT_SET %s %s", l->d.move.dest, l->d.move.src) == 2) {
        *which = ELEM_IF_BIT_SET;
    } else if(sscanf(line, "IF_BIT_CLEAR %s %s", l->d.move.dest, l->d.move.src) == 2) {
        *which = ELEM_IF_BIT_CLEAR;
#ifdef USE_SFR
    } else if(sscanf(line, "RSFR %s %s", l->d.cmp.op1, l->d.cmp.op2) == 2) {
        *which = ELEM_RSFR;
    } else if(sscanf(line, "WSFR %s %s", l->d.cmp.op1, l->d.cmp.op2) == 2) {
        *which = ELEM_WSFR;
    } else if(sscanf(line, "SSFR %s %s", l->d.cmp.op1, l->d.cmp.op2) == 2) {
        *which = ELEM_SSFR;
    } else if(sscanf(line, "CSFR %s %s", l->d.cmp.op1, l->d.cmp.op2) == 2) {
        *which = ELEM_CSFR;
    } else if(sscanf(line, "TSFR %s %s", l->d.cmp.op1, l->d.cmp.op2) == 2) {
        *which = ELEM_TSFR;
    } else if(sscanf(line, "TCSFR %s %s", l->d.cmp.op1, l->d.cmp.op2) == 2) {
        *which = ELEM_T_C_SFR;
#endif
    } else if(sscanf(line, "EQU %s %s", l->d.cmp.op1, l->d.cmp.op2) == 2) {
        *which = ELEM_EQU;
    } else if(sscanf(line, "NEQ %s %s", l->d.cmp.op1, l->d.cmp.op2) == 2) {
        *which = ELEM_NEQ;
    } else if(sscanf(line, "GRT %s %s", l->d.cmp.op1, l->d.cmp.op2) == 2) {
        *which = ELEM_GRT;
    } else if(sscanf(line, "GEQ %s %s", l->d.cmp.op1, l->d.cmp.op2) == 2) {
        *which = ELEM_GEQ;
    } else if(sscanf(line, "LEQ %s %s", l->d.cmp.op1, l->d.cmp.op2) == 2) {
        *which = ELEM_LEQ;
    } else if(sscanf(line, "LES %s %s", l->d.cmp.op1, l->d.cmp.op2) == 2) {
        *which = ELEM_LES;
    } else if(sscanf(line, "READ_ADC %s %d", l->d.readAdc.name, &l->d.readAdc.refs) == 2) {
        *which = ELEM_READ_ADC;
    } else if(sscanf(line, "READ_ADC %s", l->d.readAdc.name) == 1) {
        l->d.readAdc.refs = 0;
        *which = ELEM_READ_ADC;
    } else if(sscanf(line, "RANDOM %s", l->d.readAdc.name) == 1) {
        *which = ELEM_RANDOM;
    } else if(sscanf(line, "SEED_RANDOM %s %s", l->d.move.dest, l->d.move.src) == 2) {
        *which = ELEM_SEED_RANDOM;
    } else if(sscanf(line,
                     "SET_PWM %s %s %s %s",
                     l->d.setPwm.duty_cycle,
                     l->d.setPwm.targetFreq,
                     l->d.setPwm.name,
                     l->d.setPwm.resolution)
              == 4) {
        *which = ELEM_SET_PWM;
    } else if(sscanf(line, "SET_PWM %s %s %s", l->d.setPwm.duty_cycle, l->d.setPwm.targetFreq, l->d.setPwm.name) == 3) {
        *which = ELEM_SET_PWM;
    } else if(sscanf(line, "SET_PWM %s %s", l->d.setPwm.duty_cycle, l->d.setPwm.targetFreq) == 2) {
        *which = ELEM_SET_PWM;
    } else if(memcmp(line, "UART_RECV_AVAIL", 15) == 0) {
        *which = ELEM_UART_RECV_AVAIL;
    } else if(memcmp(line, "UART_SEND_READY", 15) == 0) {
        *which = ELEM_UART_SEND_READY;
    } else if(memcmp(line, "UART_SEND_BUSY", 14) == 0) {
        *which = ELEM_UART_SEND_READY;
    } else if(memcmp(line, "UART_UDRE", 9) == 0) {
        *which = ELEM_UART_SEND_READY;
    } else if(sscanf(line, "UART_RECVn %s", l->d.uart.name) == 1) {
        l->d.uart.bytes = SizeOfVar(l->d.uart.name);
        *which = ELEM_UART_RECVn;
    } else if([&]()->int{int tmp_bool;auto res = sscanf(line, "UART_RECV %s %d %d", l->d.uart.name, &(l->d.uart.bytes), &tmp_bool);l->d.uart.wait = tmp_bool != 0;return res;}() == 3) {
        *which = ELEM_UART_RECV;
    } else if(sscanf(line, "UART_RECV %s", l->d.uart.name) == 1) {
        l->d.uart.bytes = 1;
        l->d.uart.wait = false;
        *which = ELEM_UART_RECV;
    } else if(sscanf(line, "UART_SENDn %s", l->d.uart.name) == 1) {
        l->d.uart.bytes = SizeOfVar(l->d.uart.name);
        *which = ELEM_UART_SENDn;
    } else if([&]()->int{int tmp_bool;auto res = sscanf(line, "UART_SEND %s %d %d", l->d.uart.name, &(l->d.uart.bytes), &tmp_bool);l->d.uart.wait = tmp_bool != 0;return res;}() == 3) {
        *which = ELEM_UART_SEND;
    } else if(sscanf(line, "UART_SEND %s", l->d.uart.name) == 1) {
        l->d.uart.bytes = 1;
        l->d.uart.wait = false;
        *which = ELEM_UART_SEND;
    } else if(sscanf(line, "PERSIST %s", l->d.persist.var) == 1) {
        *which = ELEM_PERSIST;
    } else if(sscanf(line, "FORMATTED_STRING %s %d", l->d.fmtdStr.var, &x) == 2) {
        if(strcmp(l->d.fmtdStr.var, "(none)") == 0) {
            strcpy(l->d.fmtdStr.var, "");
        }

        char *p = line;
        int   i;
        for(i = 0; i < 3; i++) {
            while(!isspace(*p))
                p++;
            while(isspace(*p))
                p++;
        }
        for(i = 0; i < x; i++) {
            l->d.fmtdStr.string[i] = static_cast<char>(atoi(p));
            if(l->d.fmtdStr.string[i] < 32) {
                l->d.fmtdStr.string[i] = 'X';
            }
            while(!isspace(*p) && *p)
                p++;
            while(isspace(*p) && *p)
                p++;
        }
        l->d.fmtdStr.string[i] = '\0';

        *which = ELEM_FORMATTED_STRING;
    } else if(sscanf(line, "FORMATTED_STRING %s %s", l->d.fmtdStr.var, l->d.fmtdStr.string) == 2) {
        int i = strlen("FORMATTED_STRING") + 1 + strlen(l->d.fmtdStr.var) + 1;

        if(strcmp(l->d.fmtdStr.var, "(none)") == 0) {
            strcpy(l->d.fmtdStr.var, "");
        }
        FrmStrToStr(l->d.fmtdStr.string, &line[i]);
        DelNL(l->d.fmtdStr.string);
        if(strcmp(l->d.fmtdStr.string, "(none)") == 0) {
            strcpy(l->d.fmtdStr.string, "");
        }
        *which = ELEM_FORMATTED_STRING;
    } else if(sscanf(line, "STRING %s %s %d", l->d.fmtdStr.dest, l->d.fmtdStr.var, &x) == 3) {
        if(strcmp(l->d.fmtdStr.dest, "(none)") == 0) {
            strcpy(l->d.fmtdStr.dest, "");
        }
        if(strcmp(l->d.fmtdStr.var, "(none)") == 0) {
            strcpy(l->d.fmtdStr.var, "");
        }

        char *p = line;
        int   i;
        for(i = 0; i < 4; i++) {
            while(!isspace(*p))
                p++;
            while(isspace(*p))
                p++;
        }
        for(i = 0; i < x; i++) {
            l->d.fmtdStr.string[i] = static_cast<char>(atoi(p));
            if(l->d.fmtdStr.string[i] < 32) {
                l->d.fmtdStr.string[i] = 'X';
            }
            while(!isspace(*p) && *p)
                p++;
            while(isspace(*p) && *p)
                p++;
        }
        l->d.fmtdStr.string[i] = '\0';

        *which = ELEM_STRING;
    } else if(sscanf(line, "STRING %s %s %s", l->d.fmtdStr.dest, l->d.fmtdStr.var, l->d.fmtdStr.string) == 3) {
        if(strcmp(l->d.fmtdStr.dest, "(none)") == 0) {
            strcpy(l->d.fmtdStr.dest, "");
        }
        if(strcmp(l->d.fmtdStr.var, "(none)") == 0) {
            strcpy(l->d.fmtdStr.var, "");
        }
        int i = strlen("STRING") + 1 + strlen(l->d.fmtdStr.dest) + 1 + strlen(l->d.fmtdStr.var) + 1;
        FrmStrToStr(l->d.fmtdStr.string, &line[i]);
        DelNL(l->d.fmtdStr.string);
        if(strcmp(l->d.fmtdStr.string, "(none)") == 0) {
            strcpy(l->d.fmtdStr.string, "");
        }
        *which = ELEM_STRING;
    } else if([&]() -> int {
                  int editAsString;
                  int res = sscanf(line,
                                   "LOOK_UP_TABLE %s %s %d %d",
                                   l->d.lookUpTable.dest,
                                   l->d.lookUpTable.index,
                                   &(l->d.lookUpTable.count),
                                   &editAsString);
                  l->d.lookUpTable.editAsString = editAsString != 0;
                  return res;
              }() == 4) {
        char *p = line;
        int   i;
        // First skip over the parts that we already sscanf'd.
        for(i = 0; i < 5; i++) {
            while((!isspace(*p)) && *p)
                p++;
            while(isspace(*p) && *p)
                p++;
        }
        // Then copy over the look-up table entries.
        for(i = 0; i < l->d.lookUpTable.count; i++) {
            l->d.lookUpTable.vals[i] = hobatoi(p);
            while((!isspace(*p)) && *p)
                p++;
            while(isspace(*p) && *p)
                p++;
        }
        sscanf(p, "%s", l->d.lookUpTable.name);
        if(!strlen(l->d.lookUpTable.name))
            sprintf(l->d.lookUpTable.name, "%s%d", l->d.lookUpTable.dest, l->d.lookUpTable.count);
        *which = ELEM_LOOK_UP_TABLE;
    } else if(sscanf(line,
                     "PIECEWISE_LINEAR %s %s %d",
                     l->d.piecewiseLinear.dest,
                     l->d.piecewiseLinear.index,
                     &(l->d.piecewiseLinear.count))
              == 3) {
        char *p = line;
        int   i;
        // First skip over the parts that we already sscanf'd.
        for(i = 0; i < 4; i++) {
            while((!isspace(*p)) && *p)
                p++;
            while(isspace(*p) && *p)
                p++;
        }
        // Then copy over the piecewise linear points.
        for(i = 0; i < l->d.piecewiseLinear.count * 2; i++) {
            l->d.piecewiseLinear.vals[i] = hobatoi(p);
            while((!isspace(*p)) && *p)
                p++;
            while(isspace(*p) && *p)
                p++;
        }
        sscanf(p, "%s", l->d.piecewiseLinear.name);
        *which = ELEM_PIECEWISE_LINEAR;
    } else {
        // that's odd; nothing matched
        CheckFree(l);
        return false;
    }
    *any = l;
    if(*which == ELEM_SET_PWM) {
        if(l->d.setPwm.name[0] != 'P') { // Fix the name, this case will occur when reading old LD files
            memmove(l->d.setPwm.name + 1, l->d.setPwm.name, strlen(l->d.setPwm.name) + 1);
            l->d.setPwm.name[0] = 'P';
        }
        char *s;
        if((s = strchr(l->d.setPwm.targetFreq, '.')) != nullptr) {
            *s = '\0';
        }
    }
    return true;
}

//-----------------------------------------------------------------------------
char *strspace(char *str)
{
    while(strlen(str) && isspace(str[0]))
        str++;
    while(strlen(str) && isspace(str[strlen(str) - 1]))
        str[strlen(str) - 1] = '\0';
    return str;
}
//-----------------------------------------------------------------------------
char *strspacer(char *str)
{
    int i = strlen(str) - 1;
    while((i >= 0) && isspace(str[i])) {
        str[i] = 0;
        i--;
    }
    return str;
}
//-----------------------------------------------------------------------------
// Load a parallel subcircuit from a file. We look for leaf nodes using
// LoadLeafFromFile, which we can put directly into the parallel circuit
// that we're building up, or series subcircuits that we can pass to
// LoadSeriesFromFile. Returns the parallel subcircuit built up, or nullptr if
// something goes wrong.
//-----------------------------------------------------------------------------
static ElemSubcktParallel *LoadParallelFromFile(FileTracker& f)
{
    char  line[512];
    void *any;
    int   which;

    ElemSubcktParallel *ret = AllocSubcktParallel();
    int                 cnt = 0;

    for(;;) {
        if(!fgets(line, sizeof(line), f))
            return nullptr;
        if(!strlen(strspace(line)))
            continue;
        char *s = line;
        while(isspace(*s))
            s++;
        //if((*s=='/') && ((++(*s))=='/')) continue;
        if(*s == ';')
            continue;

        if(strcmp(s, "SERIES") == 0) {
            which = ELEM_SERIES_SUBCKT;
            any = LoadSeriesFromFile(f);
            if(!any)
                return nullptr;

        } else if(LoadLeafFromFile(s, &any, &which)) {
            // got it
        } else if(strcmp(s, "END") == 0) {
            ret->count = cnt;
            return ret;
        } else {
            return nullptr;
        }
        ret->contents[cnt].which = which;
        ret->contents[cnt].data.any = any;
        cnt++;
        if(cnt >= MAX_ELEMENTS_IN_SUBCKT)
            return nullptr;
    }
}

//-----------------------------------------------------------------------------
// Same as LoadParallelFromFile, but for a series subcircuit. Thus builds
// a series circuit out of parallel circuits and leaf elements.
//-----------------------------------------------------------------------------
ElemSubcktSeries *LoadSeriesFromFile(FileTracker& f)
{
    char  line[512];
    void *any;
    int   which;

    ElemSubcktSeries *ret = AllocSubcktSeries();
    int               cnt = 0;

    for(;;) {
        if(!fgets(line, sizeof(line), f))
            return nullptr;
        if(!strlen(strspace(line)))
            continue;
        char *s = line;
        while(isspace(*s))
            s++;
        if(*s == ';')
            continue; // NOT for release
        if(strcmp(s, "PARALLEL") == 0) {
            which = ELEM_PARALLEL_SUBCKT;
            any = LoadParallelFromFile(f);
            if(!any)
                return nullptr;
        } else if(strcmp(s, "SERIES") == 0) {
            which = ELEM_SERIES_SUBCKT;
            any = LoadSeriesFromFile(f);
            if(!any)
                return nullptr;
        } else if(LoadLeafFromFile(s, &any, &which)) {
            // got it
        } else if(strcmp(s, "END") == 0) {
            ret->count = cnt;
            return ret;
        } else {
            return nullptr;
        }
        ret->contents[cnt].which = which;
        ret->contents[cnt].data.any = any;
        cnt++;
        if(cnt >= MAX_ELEMENTS_IN_SUBCKT)
            return nullptr;
    }
}

//-----------------------------------------------------------------------------
void LoadWritePcPorts()
{
    if(Prog.mcu() && (Prog.mcu()->core == PC_LPT_COM)) {
        //RunningInBatchMode = true;
        char pc[MAX_PATH];
        strcpy(pc, CurrentLdPath);
        if(strlen(pc))
            strcat(pc, "\\");
        strcat(pc, "pcports.cfg");
        if(LoadPcPorts(pc)) {
            for(uint32_t i = 0; i < supportedMcus().size(); i++)
                if(supportedMcus()[i].core == PC_LPT_COM) {
                    supportedMcus()[i].pinInfo = IoPc;
                    supportedMcus()[i].pinCount = IoPcCount;
                }
        } else
            Warning(_("File '%s' not found!"), pc);
        //RunningInBatchMode = false;
    }
}

//-----------------------------------------------------------------------------
void SavePullUpListToFile(FileTracker& f)
{
    for(int i = 0; i < MAX_IO_PORTS; i++) {
        if(IS_MCU_REG(i)) {
            fprintf(f, "    %c%c: 0x%X \n", Prog.mcu()->portPrefix, 'A' + i, Prog.pullUpRegs[i]);
        }
    }
}

//-----------------------------------------------------------------------------
bool LoadPullUpListFromFile(FileTracker& f)
{
    char line[MAX_NAME_LEN];
    char portPrefix;
    char port;
    int i;
    uint32_t pullUpRegs;
    bool Ok;

    while(fgets(line, sizeof(line), f)) {
        if(!strlen(strspace(line)))
            continue;
        if(strcmp(line, "END") == 0) {
            return true;
        }
        Ok = true;
        // Don't internationalize this! It's the file format, not UI.
        if(sscanf(line, "   %c%c: 0x%X", &portPrefix, &port, &pullUpRegs) == 3) {
            i = port-'A';
            if((portPrefix == Prog.mcu()->portPrefix) && (i >= 0) && (i < MAX_IO_PORTS)) {
                Prog.pullUpRegs[i] = pullUpRegs;
            } else {
                Ok = false;
            }
        }
        if(!Ok) {
            THROW_COMPILER_EXCEPTION_FMT(_("Error reading 'PULL-UP LIST' section from .ld file!\nError in line:\n'%s'."), strspacer(line));
        }
    }
    return false;
}
//-----------------------------------------------------------------------------
// Load a project from a saved project description files. This describes the
// program, the target processor, plus certain configuration settings (cycle 
// time, processor clock, etc.). Return true for success, false if anything
// went wrong.
//-----------------------------------------------------------------------------
bool LoadProjectFromFile(const char *filename)
{
    FreeEntireProgram();
    strcpy(CurrentCompileFile, "");

    FileTracker f(filename, "r");
    if(!f.is_open())
        return false;

    strcpy(CurrentLdPath, filename);
    ExtractFileDir(CurrentLdPath);

    char          line[512];
    char          version[512];
    long long int cycle;
    int           crystal, baud;
    long          rate, speed;                          ///// Added by JG
    int           cycleTimer, cycleDuty, wdte;
    unsigned long long int configWord = 0;
    Prog.configurationWord = 0;
    while(fgets(line, sizeof(line), f)) {
        if(!strlen(strspace(line)))
            continue;
        if(strcmp(line, "IO LIST") == 0) {
            if(!LoadIoListFromFile(f)) {
                return false;
            }
        } else if(strcmp(line, "VAR LIST") == 0) {
            if(!LoadVarListFromFile(f)) {
                return false;
            }
        } else if(strcmp(line, "PULL-UP LIST") == 0) {
            if(!LoadPullUpListFromFile(f)) {
                return false;
            }
        } else if(sscanf(line, "LDmicro%s", &version[0])) {
            Prog.LDversion = version;
            if((Prog.LDversion != "0.1") )
                Prog.LDversion = "0.2";
        } else if(sscanf(line, "CRYSTAL=%d", &crystal)) {
            Prog.mcuClock = crystal;
        } else if(sscanf(line,
                         "CYCLE=%lld us at Timer%d, YPlcCycleDuty:%d, ConfigurationWord(s):%llx",
                         &cycle,
                         &cycleTimer,
                         &cycleDuty,
                         &configWord)
                  == 4) {
            Prog.cycleTime = cycle;
            if((cycleTimer != 0) && (cycleTimer != 1))
                cycleTimer = 1;
            Prog.cycleTimer = cycleTimer;
            Prog.cycleDuty = cycleDuty;
            Prog.configurationWord = configWord;
            if(Prog.cycleTime == 0)
                Prog.cycleTimer = -1;
        } else if(sscanf(line,
                         "CYCLE=%lld us at Timer%d, YPlcCycleDuty:%d, WDTE:%d",
                         &cycle,
                         &cycleTimer,
                         &cycleDuty,
                         &wdte)
                  == 4) {
            Prog.cycleTime = cycle;
            if((cycleTimer != 0) && (cycleTimer != 1))
                cycleTimer = 1;
            Prog.cycleTimer = cycleTimer;
            Prog.cycleDuty = cycleDuty;
            if(Prog.cycleTime == 0)
                Prog.cycleTimer = -1;
        } else if(sscanf(line, "CYCLE=%lld us at Timer%d, YPlcCycleDuty:%d", &cycle, &cycleTimer, &cycleDuty) == 3) {
            Prog.cycleTime = cycle;
            if((cycleTimer != 0) && (cycleTimer != 1))
                cycleTimer = 1;
            Prog.cycleTimer = cycleTimer;
            Prog.cycleDuty = cycleDuty;
            if(Prog.cycleTime == 0)
                Prog.cycleTimer = -1;
        } else if(sscanf(line, "CYCLE=%lld", &cycle)) {
            Prog.cycleTime = cycle;
            Prog.cycleTimer = 1;
            Prog.cycleDuty = 0;
            if(Prog.cycleTime == 0)
                Prog.cycleTimer = -1;
        } else if(sscanf(line, "BAUD=%d Hz, RATE=%ld Hz, SPEED=%ld Hz", &baud, &rate, &speed) == 3) {       ///// RATE + SPEED created by JG for SPI & I2C
            Prog.baudRate = baud;
            Prog.spiRate = rate;
            Prog.i2cRate = speed;
        } else if(sscanf(line, "BAUD=%d Hz, RATE=%ld Hz", &baud, &rate) == 2) {     ///// RATE created by JG for SPI
            Prog.baudRate = baud;
            Prog.spiRate = rate;
            Prog.i2cRate = 0;
        } else if(sscanf(line, "BAUD=%d Hz", &baud) == 1) {
            Prog.baudRate = baud;
            Prog.spiRate = 0;
            Prog.i2cRate = 0;
        } else if(memcmp(line, "COMPILED=", 9) == 0) {
            strcpy(CurrentCompileFile, line + 9);

            strcpy(CurrentCompilePath, CurrentCompileFile);
            ExtractFileDir(CurrentCompilePath);
        } else if(memcmp(line, "COMPILER=", 9) == 0) {
            int i = GetMnu(line + 9);
            if(i > 0)
                compile_MNU = i;
        } else if(memcmp(line, "MICRO=", 6) == 0) {
            if(strlen(line) > 6) {
                auto& mcus = supportedMcus();
                auto mcu = std::find_if(std::begin(mcus), std::end(mcus),
                                         [&line](const McuIoInfo& info){ return (strcmp(info.mcuName, line + 6) == 0);});
                if(mcu != std::end(mcus)) {
                    Prog.setMcu(&(*mcu));
                    LoadWritePcPorts();
                } else {
                    Error(_("Microcontroller '%s' not supported.\r\n\r\n"
                            "Defaulting to no selected MCU."),
                          line + 6);
                }
            }
        } else if(strcmp(line, "PROGRAM") == 0) {
            break;
        }
    }

    int rung = -2;

    if(strcmp(line, "PROGRAM") != 0)
        goto failed;

    for(rung = 0;;) {
        if(!fgets(line, sizeof(line), f))
            break;
        if(!strlen(strspace(line)))
            continue;
        if(strstr(line, "RUNG") == 0)
            goto failed;

        ElemSubcktSeries *s = LoadSeriesFromFile(f);
        if(!s)
            goto failed;
        if(rung >= MAX_RUNGS) {
            Error(_("Too many rungs in input file!\nSame rungs not loaded!"));
            break;
        }
        Prog.rungs_[rung] = s;
        rung++;
    }
    Prog.numRungs = rung;

    for(rung = 0; rung < Prog.numRungs; rung++) {
        while(CollapseUnnecessarySubckts(ELEM_SERIES_SUBCKT, Prog.rungs_[rung]))
            ProgramChanged();
    }

    f.close();
    tGetLastWriteTime(filename, (PFILETIME)&LastWriteTime, 1);
    PrevWriteTime = LastWriteTime;
    strcpy(CurrentSaveFile, filename);
    return true;

failed:
    NewProgram();
    Error(
        _("File format error; perhaps this program is for a newer version "
          "of LDmicro?"));
    Error(_("Error in RUNG %d. See error below %s"), rung + 1, line);
    return false;
}

//-----------------------------------------------------------------------------
// Helper routine for outputting hierarchical representation of the ladder
// logic: indent on file f, by depth*4 spaces.
//-----------------------------------------------------------------------------
static void Indent(FileTracker& f, int depth)
{
    for(int i = 0; i < depth; i++) {
        fprintf(f, "  ");
    }
}

//-----------------------------------------------------------------------------
// Save an element to a file. If it is a leaf, then output a single line
// describing it and return. If it is a subcircuit, call ourselves
// recursively (with depth+1, so that the indentation is right) to handle
// the members of the subcircuit. Special case for depth=0: we do not
// output the SERIES/END delimiters. This is because the root is delimited
// by RUNG/END markers output elsewhere.
//-----------------------------------------------------------------------------
void SaveElemToFile(FileTracker& f, int which, void *any, int depth, int rung)
{
    ElemLeaf *  l = (ElemLeaf *)any;
    const char *s;
    char        str1[1024];
    char        str2[1024];
    char        str3[1024];
    char        str4[1024];

    Indent(f, depth);

    switch(which) {
        case ELEM_PLACEHOLDER:
            fprintf(f, "PLACEHOLDER\n");
            break;

        case ELEM_COMMENT: {
            fprintf(f, "COMMENT %s\n", StrToFrmStr(str1, l->d.comment.str, FRMT_COMMENT));
            break;
        }
        case ELEM_OPEN:
            fprintf(f, "OPEN\n");
            break;

        case ELEM_SHORT:
            fprintf(f, "SHORT\n");
            break;

        case ELEM_MASTER_RELAY:
            fprintf(f, "MASTER_RELAY\n");
            break;

        case ELEM_SLEEP:
            fprintf(f, "SLEEP %s %s\n", l->d.timer.name, l->d.timer.delay);
            break;

        case ELEM_DELAY:
            fprintf(f, "DELAY %s\n", l->d.timer.name);
            break;

        case ELEM_TIME2DELAY:
            fprintf(f, "TIME2DELAY %s %s\n", l->d.timer.name, l->d.timer.delay);
            break;

        case ELEM_CLRWDT:
            fprintf(f, "CLRWDT\n");
            break;

        case ELEM_LOCK:
            fprintf(f, "LOCK\n");
            break;

        case ELEM_RETURN:
            fprintf(f, "RETURN\n");
            break;

        case ELEM_GOTO:
            fprintf(f, "GOTO %s\n", l->d.doGoto.label);
            break;

        case ELEM_GOSUB:
            fprintf(f, "GOSUB %s\n", l->d.doGoto.label);
            break;

        case ELEM_LABEL:
            fprintf(f, "LABEL %s\n", l->d.doGoto.label);
            break;

        case ELEM_SUBPROG:
            fprintf(f, "SUBPROG %s\n", l->d.doGoto.label);
            break;

        case ELEM_ENDSUB:
            fprintf(f, "ENDSUB %s\n", l->d.doGoto.label);
            break;

        case ELEM_SHIFT_REGISTER:
            fprintf(f, "SHIFT_REGISTER %s %d\n", l->d.shiftRegister.name, l->d.shiftRegister.stages);
            break;

        case ELEM_CONTACTS:
            if(l->d.contacts.name[0] != 'X')
                l->d.contacts.set1 = false;
            fprintf(f, "CONTACTS %s %d %d\n", l->d.contacts.name, l->d.contacts.negated, l->d.contacts.set1);
            break;

        case ELEM_COIL:
            fprintf(f,
                    "COIL %s %d %d %d %d\n",
                    l->d.coil.name,
                    l->d.coil.negated,
                    l->d.coil.setOnly,
                    l->d.coil.resetOnly,
                    l->d.coil.ttrigger);
            break;

        // clang-format off
        case ELEM_TIME2COUNT: s = "TIME2COUNT"; goto timer;
        case ELEM_TCY: s = "TCY"; goto timer;
        case ELEM_TON: s = "TON"; goto timer;
        case ELEM_TOF: s = "TOF"; goto timer;
        case ELEM_RTO: s = "RTO"; goto timer;
        case ELEM_RTL: s = "RTL"; goto timer;
        case ELEM_THI: s = "THI"; goto timer;
        case ELEM_TLO: s = "TLO"; goto timer;
        timer:
            fprintf(f, "%s %s %s %d\n", s, l->d.timer.name, l->d.timer.delay, l->d.timer.adjust);
            break;

        case ELEM_CTU: s = "CTU"; goto counter;
        case ELEM_CTD: s = "CTD"; goto counter;
        case ELEM_CTC: s = "CTC"; goto counter;
        case ELEM_CTR: s = "CTR"; goto counter;
        // clang-format on
        counter:
            fprintf(f, "%s %s %s %s %c\n", s, l->d.counter.name, l->d.counter.max, l->d.counter.init, l->d.counter.inputKind);
            break;

        case ELEM_STEPPER:
            fprintf(f,
                    "STEPPER %s %s %s %d %d %s\n",
                    l->d.stepper.name,
                    l->d.stepper.max,
                    l->d.stepper.P,
                    l->d.stepper.nSize,
                    l->d.stepper.graph,
                    l->d.stepper.coil);
            break;

        case ELEM_PULSER:
            fprintf(f,
                    "PULSER %s %s %s %s %s\n",
                    l->d.pulser.P1,
                    l->d.pulser.P0,
                    l->d.pulser.accel,
                    l->d.pulser.counter,
                    l->d.pulser.busy);
            break;

        case ELEM_NPULSE:
            fprintf(f, "NPULSE %s %s %s\n", l->d.Npulse.counter, l->d.Npulse.targetFreq, l->d.Npulse.coil);
            break;

        case ELEM_QUAD_ENCOD:
            fprintf(f,
                    "QUAD_ENCOD %s %d %s %s %s %s %c %d\n",
                    l->d.QuadEncod.counter,
                    l->d.QuadEncod.int01,
                    l->d.QuadEncod.inputA,
                    l->d.QuadEncod.inputB,
                    StrToFrmStr(str1,l->d.QuadEncod.inputZ),
                    StrToFrmStr(str2,l->d.QuadEncod.dir),
                    l->d.QuadEncod.inputZKind,
                    l->d.QuadEncod.countPerRevol);
            break;

        case ELEM_NPULSE_OFF:
            fprintf(f, "NPULSE_OFF\n");
            break;

        case ELEM_RES:
            fprintf(f, "RES %s\n", l->d.reset.name);
            break;

        case ELEM_MOVE:
            fprintf(f, "MOVE %s %s\n", l->d.move.dest, l->d.move.src);
            break;

        case ELEM_BIN2BCD:
            fprintf(f, "BIN2BCD %s %s\n", l->d.move.dest, l->d.move.src);
            break;

        case ELEM_BCD2BIN:
            fprintf(f, "BCD2BIN %s %s\n", l->d.move.dest, l->d.move.src);
            break;

        case ELEM_OPPOSITE:
            fprintf(f, "OPPOSITE %s %s\n", l->d.move.dest, l->d.move.src);
            break;

        case ELEM_SWAP:
            fprintf(f, "SWAP %s %s\n", l->d.move.dest, l->d.move.src);
            break;

        ///// Added by JG
        case ELEM_SPI_WR:
            fprintf(f,
                    "SPI_WR %s %s %s %s %s %s %s %s\n",
                    l->d.spi.name,
                    l->d.spi.send,
                    l->d.spi.recv,
                    l->d.spi.mode,
                    l->d.spi.modes,
                    l->d.spi.size,
                    l->d.spi.first,
                    l->d.spi.bitrate);
            break;
            /////

        case ELEM_SPI: {
            fprintf(f,
                    "SPI %s %s %s %s %s %s %s %s\n",
                    l->d.spi.name,
                    l->d.spi.send,
                    l->d.spi.recv,
                    l->d.spi.mode,
                    l->d.spi.modes,
                    l->d.spi.size,
                    l->d.spi.first,
                    l->d.spi.bitrate);
            break;
        }

        ///// Added by JG
        case ELEM_I2C_RD:
            fprintf(f,
                    "I2C_RD %s %s %s %s %s %s %s %s\n",
                    l->d.i2c.name,
                    l->d.i2c.send,
                    l->d.i2c.recv,
                    l->d.i2c.mode,
                    l->d.i2c.address,
                    l->d.i2c.registr,
                    l->d.i2c.first,
                    l->d.i2c.bitrate);
            break;

        case ELEM_I2C_WR:
            fprintf(f,
                    "I2C_WR %s %s %s %s %s %s %s %s\n",
                    l->d.i2c.name,
                    l->d.i2c.send,
                    l->d.i2c.recv,
                    l->d.i2c.mode,
                    l->d.i2c.address,
                    l->d.i2c.registr,
                    l->d.i2c.first,
                    l->d.i2c.bitrate);
            break;

            /////

        case ELEM_BUS: {
            fprintf(f, "BUS %s %s", l->d.bus.dest, l->d.bus.src);
            int i;
            for(i = 7; i >= 0; i--)
                fprintf(f, " %d", l->d.bus.PCBbit[i]);
            fprintf(f, "\n");
            break;
        }
        case ELEM_7SEG:
            fprintf(f, "7SEGMENTS %s %s %c\n", l->d.segments.dest, l->d.segments.src, l->d.segments.common);
            break;

        case ELEM_9SEG:
            fprintf(f, "9SEGMENTS %s %s %c\n", l->d.segments.dest, l->d.segments.src, l->d.segments.common);
            break;

        case ELEM_14SEG:
            fprintf(f, "14SEGMENTS %s %s %c\n", l->d.segments.dest, l->d.segments.src, l->d.segments.common);
            break;

        case ELEM_16SEG:
            fprintf(f, "16SEGMENTS %s %s %c\n", l->d.segments.dest, l->d.segments.src, l->d.segments.common);
            break;

        case ELEM_NOT:
            fprintf(f, "NOT %s %s\n", l->d.math.dest, l->d.math.op1);
            break;

        case ELEM_NEG:
            fprintf(f, "NEG %s %s\n", l->d.math.dest, l->d.math.op1);
            break;

        // clang-format off
        case ELEM_ROL: s = "ROL"; goto math;
        case ELEM_ROR: s = "ROR"; goto math;
        case ELEM_SHL: s = "SHL"; goto math;
        case ELEM_SHR: s = "SHR"; goto math;
        case ELEM_SR0: s = "SR0"; goto math;
        case ELEM_AND: s = "AND"; goto math;
        case ELEM_OR : s = "OR" ; goto math;
        case ELEM_XOR: s = "XOR"; goto math;
        case ELEM_MOD: s = "MOD"; goto math;
        case ELEM_ADD: s = "ADD"; goto math;
        case ELEM_SUB: s = "SUB"; goto math;
        case ELEM_MUL: s = "MUL"; goto math;
        case ELEM_DIV: s = "DIV"; goto math;
        // clang-format on
        math:
            fprintf(f, "%s %s %s %s\n", s, l->d.math.dest, l->d.math.op1, l->d.math.op2);
            break;

        case ELEM_SET_BIT:
            fprintf(f, "SET_BIT %s %s\n", l->d.move.dest, l->d.move.src);
            break;

        case ELEM_CLEAR_BIT:
            fprintf(f, "CLEAR_BIT %s %s\n", l->d.move.dest, l->d.move.src);
            break;

        case ELEM_IF_BIT_SET:
            fprintf(f, "IF_BIT_SET %s %s\n", l->d.move.dest, l->d.move.src);
            break;

        case ELEM_IF_BIT_CLEAR:
            fprintf(f, "IF_BIT_CLEAR %s %s\n", l->d.move.dest, l->d.move.src);
            break;

#ifdef USE_SFR
        // Special function
        // clang-format off
        case ELEM_RSFR: s = "RSFR"; goto sfrcmp;
        case ELEM_WSFR: s = "WSFR"; goto sfrcmp;
        case ELEM_SSFR: s = "SSFR"; goto sfrcmp;
        case ELEM_CSFR: s = "CSFR"; goto sfrcmp;
        case ELEM_TSFR: s = "TSFR"; goto sfrcmp;
        case ELEM_T_C_SFR: s = "TCSFR"; goto sfrcmp;
        // clang-format on
        sfrcmp:
            fprintf(f, "%s %s %s\n", s, l->d.cmp.op1, l->d.cmp.op2);
            break;
// Special function
#endif

        // clang-format off
        case ELEM_EQU: s = "EQU"; goto cmp;
        case ELEM_NEQ: s = "NEQ"; goto cmp;
        case ELEM_GRT: s = "GRT"; goto cmp;
        case ELEM_GEQ: s = "GEQ"; goto cmp;
        case ELEM_LES: s = "LES"; goto cmp;
        case ELEM_LEQ: s = "LEQ"; goto cmp;
        // clang-format on
        cmp:
            fprintf(f, "%s %s %s\n", s, l->d.cmp.op1, l->d.cmp.op2);
            break;

        case ELEM_ONE_SHOT_RISING:
            fprintf(f, "OSR\n");
            break;

        case ELEM_ONE_SHOT_FALLING:
            fprintf(f, "OSF\n");
            break;

        case ELEM_ONE_SHOT_LOW:
            fprintf(f, "OSL\n");
            break;

        case ELEM_OSC:
            fprintf(f, "OSC\n");
            break;

        case ELEM_READ_ADC:
            fprintf(f, "READ_ADC %s %d\n", l->d.readAdc.name, l->d.readAdc.refs);
            break;

        case ELEM_RANDOM:
            fprintf(f, "RANDOM %s\n", l->d.readAdc.name);
            break;

        case ELEM_SEED_RANDOM:
            fprintf(f, "SEED_RANDOM %s %s\n", l->d.move.dest, l->d.move.src);
            break;

        case ELEM_SET_PWM:
            fprintf(f,
                    "SET_PWM %s %s %s %s\n",
                    l->d.setPwm.duty_cycle,
                    l->d.setPwm.targetFreq,
                    l->d.setPwm.name,
                    l->d.setPwm.resolution);
            break;

        case ELEM_UART_RECV:
            fprintf(f, "UART_RECV %s %d %d\n", l->d.uart.name, l->d.uart.bytes, l->d.uart.wait);
            break;

        case ELEM_UART_SEND:
            fprintf(f, "UART_SEND %s %d %d\n", l->d.uart.name, l->d.uart.bytes, l->d.uart.wait);
            break;

        case ELEM_UART_RECVn:
            fprintf(f, "UART_RECVn %s\n", l->d.uart.name);
            break;

        case ELEM_UART_SENDn:
            fprintf(f, "UART_SENDn %s\n", l->d.uart.name);
            break;

        case ELEM_UART_SEND_READY:
            fprintf(f, "UART_SEND_READY\n");
            break;

        case ELEM_UART_RECV_AVAIL:
            fprintf(f, "UART_RECV_AVAIL\n");
            break;

        case ELEM_PERSIST:
            fprintf(f, "PERSIST %s\n", l->d.persist.var);
            break;

        case ELEM_CPRINTF:
            s = "CPRINTF";
            goto cprintf;
        case ELEM_SPRINTF:
            s = "SPRINTF";
            goto cprintf;
        case ELEM_FPRINTF:
            s = "FPRINTF";
            goto cprintf;
        case ELEM_PRINTF:
            s = "PRINTF";
            goto cprintf;
        case ELEM_I2C_CPRINTF:
            s = "I2C_PRINTF";
            goto cprintf;
        case ELEM_ISP_CPRINTF:
            s = "ISP_PRINTF";
            goto cprintf;
        case ELEM_UART_CPRINTF:
            s = "UART_PRINTF";
            goto cprintf;
            {
            cprintf:
                fprintf(f,
                        "%s %s %s %s %s %s\n",
                        s,
                        StrToFrmStr(str1, l->d.fmtdStr.var, FRMT_x20),
                        StrToFrmStr(str2, l->d.fmtdStr.string, FRMT_x20),
                        l->d.fmtdStr.dest,
                        StrToFrmStr(str3, l->d.fmtdStr.enable, FRMT_x20), //may be (none)
                        StrToFrmStr(str4, l->d.fmtdStr.error, FRMT_x20)); //may be (none)
                break;
            }
        case ELEM_STRING: {
            int i;
            fprintf(f, "STRING ");
            if(*(l->d.fmtdStr.dest)) {
                fprintf(f, "%s", l->d.fmtdStr.dest);
            } else {
                fprintf(f, "(none)");
            }
            if(*(l->d.fmtdStr.var)) {
                fprintf(f, " %s", l->d.fmtdStr.var);
            } else {
                fprintf(f, " (none)");
            }
            fprintf(f, " %d", strlen(l->d.fmtdStr.string));
            for(i = 0; i < (int)strlen(l->d.fmtdStr.string); i++) {
                fprintf(f, " %d", l->d.fmtdStr.string[i]);
            }
            fprintf(f, "\n");
            break;
        }

        case ELEM_FORMATTED_STRING: {
            int i;
            fprintf(f, "FORMATTED_STRING ");
            if(*(l->d.fmtdStr.var)) {
                fprintf(f, "%s", l->d.fmtdStr.var);
            } else {
                fprintf(f, "(none)");
            }
            fprintf(f, " %d", strlen(l->d.fmtdStr.string));
            for(i = 0; i < (int)strlen(l->d.fmtdStr.string); i++) {
                fprintf(f, " %d", l->d.fmtdStr.string[i]);
            }
            fprintf(f, "\n");
            break;
        }
        case ELEM_LOOK_UP_TABLE: {
            int i;
            fprintf(f,
                    "LOOK_UP_TABLE %s %s %d %d",
                    l->d.lookUpTable.dest,
                    l->d.lookUpTable.index,
                    l->d.lookUpTable.count,
                    l->d.lookUpTable.editAsString);
            for(i = 0; i < l->d.lookUpTable.count; i++) {
                fprintf(f, " %d", l->d.lookUpTable.vals[i]);
            }
            fprintf(f, " %s", l->d.lookUpTable.name);
            fprintf(f, "\n");
            break;
        }
        case ELEM_PIECEWISE_LINEAR: {
            int i;
            fprintf(f,
                    "PIECEWISE_LINEAR %s %s %d",
                    l->d.piecewiseLinear.dest,
                    l->d.piecewiseLinear.index,
                    l->d.piecewiseLinear.count);
            for(i = 0; i < l->d.piecewiseLinear.count * 2; i++) {
                fprintf(f, " %d", l->d.piecewiseLinear.vals[i]);
            }
            fprintf(f, " %s", l->d.piecewiseLinear.name);
            fprintf(f, "\n");
            break;
        }

        case ELEM_SERIES_SUBCKT: {
            ElemSubcktSeries *series = (ElemSubcktSeries *)any;
            if(depth == 0) {
                if(Prog.LDversion == "0.1")
                    fprintf(f, "RUNG\n");
                else
                    fprintf(f, "RUNG %d\n", rung);
                //fprintf(f, "RUNG\n");
            } else {
                fprintf(f, "SERIES\n");
            }
            for(int i = 0; i < series->count; i++) {
                SaveElemToFile(f, series->contents[i].which, series->contents[i].data.any, depth + 1, rung);
            }
            Indent(f, depth);
            fprintf(f, "END\n");
            break;
        }

        case ELEM_PARALLEL_SUBCKT: {
            ElemSubcktParallel *parallel = (ElemSubcktParallel *)any;
            fprintf(f, "PARALLEL\n");
            for(int i = 0; i < parallel->count; i++) {
                SaveElemToFile(f, parallel->contents[i].which, parallel->contents[i].data.any, depth + 1, rung);
            }
            Indent(f, depth);
            fprintf(f, "END\n");
            break;
        }

        default:
            Error("ELEM_0x%x", which);
            break;
    }
}

//-----------------------------------------------------------------------------
// Save the program in memory to the given file. Returns true for success,
// false otherwise.
//-----------------------------------------------------------------------------
bool SaveProjectToFile(char *filename, int code)
{
    if(code == MNU_SAVE_02)
        Prog.LDversion = "0.2";
    else if(code == MNU_SAVE_01)
        Prog.LDversion = "0.1";

    FileTracker f(filename, "w");
    if(!f)
        return false;

    fprintf(f, "LDmicro%s\n", Prog.LDversion.c_str());
    if(Prog.mcu()) {
        fprintf(f, "MICRO=%s\n", Prog.mcu()->mcuName);
    }
    fprintf(f,
            "CYCLE=%lld us at Timer%d, YPlcCycleDuty:%d, ConfigurationWord(s):0x%llX\n",
            Prog.cycleTime,
            Prog.cycleTimer,
            Prog.cycleDuty,
            Prog.configurationWord);
    fprintf(f, "CRYSTAL=%d Hz\n", Prog.mcuClock);
    fprintf(f, "BAUD=%d Hz, RATE=%d Hz, SPEED=%d Hz\n", Prog.baudRate, Prog.spiRate, Prog.i2cRate);
    if(strlen(CurrentCompileFile) > 0) {
        fprintf(f, "COMPILED=%s\n", CurrentCompileFile);
    }
    if(Prog.LDversion != "0.1") {
        if(compile_MNU > 0)
            fprintf(f, "COMPILER=%s\n", GetMnuCompilerName(compile_MNU));

        fprintf(f, "\n");
        fprintf(f, "PULL-UP LIST\n");
        SavePullUpListToFile(f);
        fprintf(f, "END\n");

		fprintf(f, "\n");
        fprintf(f, "VAR LIST\n");
        SaveVarListToFile(f);
        fprintf(f, "END\n");
    }

    fprintf(f, "\n");
    // list extracted from schematic, but the pin assignments are not
    fprintf(f, "IO LIST\n");
    SaveIoListToFile(f);
    fprintf(f, "END\n");

    fprintf(f, "\n");
    fprintf(f, "PROGRAM\n");

    for(int i = 0; i < Prog.numRungs; i++) {
        SaveElemToFile(f, ELEM_SERIES_SUBCKT, Prog.rungs(i), 0, i + 1);
    }

    fflush(f);
    f.close();
    tGetLastWriteTime(filename, (PFILETIME)&LastWriteTime, 1);
    PrevWriteTime = LastWriteTime;
    return true;
}

//---------------------------------------------------------------------------
char *StrToFrmStr(char *dest, const char* src, FRMT frmt)
{
    if((src == nullptr) || (strlen(src) == 0)) {
        strcpy(dest, "(none)");
        return dest;
    }

    strcpy(dest, "");
    int i;
    if((frmt == FRMT_01) && (Prog.LDversion == "0.1")) {
        char str[1024];
        sprintf(str, " %d", strlen(src));
        strcat(dest, str);
        for(i = 0; i < (int)strlen(src); i++) {
            sprintf(str, " %d", src[i]);
            strcat(dest, str);
        }
    } else {
        for(i = 0; i < (int)strlen(src); i++) {
            if((frmt == FRMT_x20) && (src[i] == ' ')) {
                strcat(dest, "\\x20");
                //          } else if(src[i] == '\'') {
                //              strcat(dest, "\\\'");
                //          } else if(src[i] == '\"') {
                //              strcat(dest, "\\\"");
                //          } else if(src[i] == '\?') {
                //              strcat(dest, "\\\?");
            } else if(src[i] == '\\') {
                strcat(dest, "\\\\");
            } else if(src[i]
                      == 0x07) { //(alert) Produces an audible or visible alert without changing the active position.
                strcat(dest, "\\a");
            } else if(src[i]
                      == '\b') { //(backspace) Moves the active position to the previous position on the current line.
                strcat(dest, "\\b");
            } else if(src[i]
                      == 0x1B) { //Escape character
                strcat(dest, "\\e");
            } else if(
                src[i]
                == '\f') { //(form feed) Moves the active position to the initial position at the start of the next logical page.
                strcat(dest, "\\f");
            } else if(src[i] == '\n') { //(new line) Moves the active position to the initial position of the next line.
                strcat(dest, "\\n");
            } else if(
                src[i]
                == '\r') { //(carriage return) Moves the active position to the initial position of the current line.
                strcat(dest, "\\r");
            } else if(
                src[i]
                == '\t') { //(horizontal tab) Moves the active position to the next horizontal tabulation position on the current line.
                strcat(dest, "\\t");
            } else if(
                src[i]
                == '\v') { //(vertical tab) Moves the active position to the initial position of the next vertical tabulation position.
                strcat(dest, "\\v");
            } else {
                strncat(dest, &src[i], 1);
            }
        }
    }
    return dest;
}
char *StrToFrmStr(char *dest, const char *src)
{
    return StrToFrmStr(dest, src, FRMT_x20);
}

//-----------------------------------------------------------------------------
char *FrmStrToStr(char *dest, const char *src)
{
    const char *s;
    if(src)
        s = src;
    else
        s = dest;

    if(strcmp(s, "(none)") == 0) {
        strcpy(dest, "");
        return dest;
    }

    int i = 0;
    while(*s) {
        if(*s == '\\') {
            if((s[1] == 'x') && (s[2] == '2') && (s[3] == '0')) {
                dest[i++] = ' ';
                s += 3;
            } else if((s[1] == '\\') && (s[2] == 'x') && (s[3] == '2') && (s[4] == '0')) {
                dest[i++] = ' ';
                s += 4;
            } else if(s[1] == '\'') {
                dest[i++] = '\'';
                s++;
            } else if(s[1] == '\"') {
                dest[i++] = '\"';
                s++;
            } else if(s[1] == '\?') {
                dest[i++] = '\?';
                s++;
            } else if(s[1] == '\\') {
                dest[i++] = '\\';
                s++;
            } else if(s[1] == 'a') {
                dest[i++] = 0x07;
                s++;
            } else if(s[1] == 'b') {
                dest[i++] = '\b';
                s++;
            } else if(s[1] == 'e') {
                dest[i++] = 0x1B;
                s++;
            } else if(s[1] == 'f') {
                dest[i++] = '\f';
                s++;
            } else if(s[1] == 'n') {
                dest[i++] = '\n';
                s++;
            } else if(s[1] == 'r') {
                dest[i++] = '\r';
                s++;
            } else if(s[1] == 't') {
                dest[i++] = '\t';
                s++;
            } else if(s[1] == 'v') {
                dest[i++] = '\v';
                s++;
            } else {
                // that is odd
                dest[i++] = *s;
            }
        } else {
            dest[i++] = *s;
        }
        s++;
    }
    dest[i++] = '\0';
    return dest;
}

char *FrmStrToStr(char *dest)
{
    return FrmStrToStr(dest, nullptr);
}
//-----------------------------------------------------------------------------
char *DelNL(char *str)
{
    char *s = str;
    int   i = 0;
    while(*s) {
        if(*s != '\n')
            str[i++] = *s;
        s++;
    }
    str[i++] = '\0';
    return str;
}
//-----------------------------------------------------------------------------
char *DelLastNL(char *str)
{
    if(str[strlen(str) - 1] == '\n')
        str[strlen(str) - 1] = '\0';
    return str;
}
