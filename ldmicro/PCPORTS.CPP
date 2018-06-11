//-----------------------------------------------------------------------------
// Copyright 2015 Nehrutsa Ihor
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
//-----------------------------------------------------------------------------
#include "stdafx.h"

#include "ldmicro.h"
#include "pcports.h"
#include "filetracker.hpp"

McuIoPinInfo IoPc[MAX_IO];
int          IoPcCount;
//-----------------------------------------------------------------------------
int MaskToBit(int Mask)
{
    int ret;
    switch(Mask) {
        case 0x01: {
            ret = 0;
            break;
        };
        case 0x02: {
            ret = 1;
            break;
        };
        case 0x04: {
            ret = 2;
            break;
        };
        case 0x08: {
            ret = 3;
            break;
        };
        case 0x10: {
            ret = 4;
            break;
        };
        case 0x20: {
            ret = 5;
            break;
        };
        case 0x40: {
            ret = 6;
            break;
        };
        case 0x80: {
            ret = 7;
            break;
        };
        default: {
            ret = -1;
            break;
        };
    };
    return ret;
}
//-----------------------------------------------------------------------------
void LptDb25PinToRegMask(int Pin, int *Reg, int *Mask)
{
    switch(Pin) {
        case 1: {
            *Reg = LPT_CONTROL_REG;
            *Mask = PAR_CONTROL_STROBE;
            break;
        };
        case 2: {
            *Reg = LPT_DATA_REG;
            *Mask = PAR_DATA0;
            break;
        };
        case 3: {
            *Reg = LPT_DATA_REG;
            *Mask = PAR_DATA1;
            break;
        };
        case 4: {
            *Reg = LPT_DATA_REG;
            *Mask = PAR_DATA2;
            break;
        };
        case 5: {
            *Reg = LPT_DATA_REG;
            *Mask = PAR_DATA3;
            break;
        };
        case 6: {
            *Reg = LPT_DATA_REG;
            *Mask = PAR_DATA4;
            break;
        };
        case 7: {
            *Reg = LPT_DATA_REG;
            *Mask = PAR_DATA5;
            break;
        };
        case 8: {
            *Reg = LPT_DATA_REG;
            *Mask = PAR_DATA6;
            break;
        };
        case 9: {
            *Reg = LPT_DATA_REG;
            *Mask = PAR_DATA7;
            break;
        };
        case 10: {
            *Reg = LPT_STATUS_REG;
            *Mask = PAR_STATUS_NOT_ACK;
            break;
        };
        case 11: {
            *Reg = LPT_STATUS_REG;
            *Mask = PAR_STATUS_NOT_BUSY;
            break;
        };
        case 12: {
            *Reg = LPT_STATUS_REG;
            *Mask = PAR_STATUS_PE;
            break;
        };
        case 13: {
            *Reg = LPT_STATUS_REG;
            *Mask = PAR_STATUS_SLCT;
            break;
        };
        case 14: {
            *Reg = LPT_CONTROL_REG;
            *Mask = PAR_CONTROL_AUTOFD;
            break;
        };
        case 15: {
            *Reg = LPT_STATUS_REG;
            *Mask = PAR_STATUS_NOT_ERROR;
            break;
        };
        case 16: {
            *Reg = LPT_CONTROL_REG;
            *Mask = PAR_CONTROL_NOT_INIT;
            break;
        };
        case 17: {
            *Reg = LPT_CONTROL_REG;
            *Mask = PAR_CONTROL_SLIN;
            break;
        };
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
        case 25: {
            *Reg = 0;
            *Mask = 0;
            break;
        }
        default: {
            *Reg = 0;
            *Mask = 0;
            //oops();
            //Raise Exception.Create('case Pin of');
        };
    };
}
//-----------------------------------------------------------------------------
void ComDb9PinToRegMask(int Pin, int *Reg, int *Mask)
{
    switch(Pin) {
        case DB9TD: {
            *Reg = 3;
            *Mask = TD;
            break;
        };
        case DB9RD: {
            *Reg = 0;
            *Mask = 0;
            break;
        };
        case DB9DTR: {
            *Reg = 4;
            *Mask = DTR;
            break;
        };
        case DB9RTS: {
            *Reg = 4;
            *Mask = RTS;
            break;
        };
        case DB9GND: {
            *Reg = 0;
            *Mask = 0;
            break;
        };
        case DB9CTS: {
            *Reg = 6;
            *Mask = CTS;
            break;
        };
        case DB9DSR: {
            *Reg = 6;
            *Mask = DSR;
            break;
        };
        case DB9RI: {
            *Reg = 6;
            *Mask = RI;
            break;
        };
        case DB9DCD: {
            *Reg = 6;
            *Mask = DCD;
            break;
        };
        default: {
            *Reg = 0;
            *Mask = 0;
            //Raise Exception.Create('case Pin of');
        };
    };
}
//-----------------------------------------------------------------------------
void ComDb25PinToRegMask(int Pin, int *Reg, int *Mask)
{
    switch(Pin) {
        case DB25TD: {
            *Reg = 3;
            *Mask = TD;
            break;
        };
        case DB25RD: {
            *Reg = 0;
            *Mask = 0;
            break;
        };
        case DB25DTR: {
            *Reg = 4;
            *Mask = DTR;
            break;
        };
        case DB25RTS: {
            *Reg = 4;
            *Mask = RTS;
            break;
        };
        case DB25GND: {
            *Reg = 0;
            *Mask = 0;
            break;
        };
        case DB25CTS: {
            *Reg = 6;
            *Mask = CTS;
            break;
        };
        case DB25DSR: {
            *Reg = 6;
            *Mask = DSR;
            break;
        };
        case DB25RI: {
            *Reg = 6;
            *Mask = RI;
            break;
        };
        case DB25DCD: {
            *Reg = 6;
            *Mask = DCD;
            break;
        };
        default: {
            *Reg = 0;
            *Mask = 0;
            //Raise Exception.Create('case Pin of');
        };
    };
}
//-----------------------------------------------------------------------------
bool ParceVar(char *s, char *prt, int *portN, int *Reg, int *Mask, int *Addr)
{
    strcpy(prt, "???");
    *portN = 0;
    *Reg = 0;
    *Mask = 0;
    *Addr = 0;

    char port;
    int  dbPin = 0;
    int  pin = 0;
    int  pinMcu = -1;
    char trans[1024];

    for(int i = 0; i < Prog.io.count; i++) {
        Transliterate(trans, Prog.io.assignment[i].name);
        if(strcmp(trans, s + 3) == 0) {
            pin = Prog.io.assignment[i].pin;
            break;
        };
    };
    if(Prog.mcu && (pin)) {
        for(uint32_t i = 0; i < Prog.mcu->pinCount; i++) {
            if(pin == Prog.mcu->pinInfo[i].pin) {
                pinMcu = i;
                break;
            };
        };
        if(pinMcu >= 0) {
            *Addr = Prog.mcu->pinInfo[pinMcu].addr;
            *portN = Prog.mcu->pinInfo[pinMcu].portN;
            dbPin = Prog.mcu->pinInfo[pinMcu].dbPin;
            port = Prog.mcu->pinInfo[pinMcu].port;
            if(port == 'L') {
                strcpy(prt, "Lpt");
                LptDb25PinToRegMask(dbPin, Reg, Mask);
            } else if(port == 'c') {
                strcpy(prt, "Com");
                ComDb9PinToRegMask(dbPin, Reg, Mask);
            } else if(port == 'C') {
                strcpy(prt, "Com");
                ComDb25PinToRegMask(dbPin, Reg, Mask);
            }
        }
    };
    return pin != 0;
}
//-----------------------------------------------------------------------------
void FillPcPinInfo(McuIoPinInfo *pinInfo)
{
    int Reg, Mask, Bit;
    if(pinInfo->port == 'L') {
        LptDb25PinToRegMask(pinInfo->dbPin, &Reg, &Mask);
        Bit = MaskToBit(Mask);
        pinInfo->bit = Bit;
        sprintf(pinInfo->pinName,
                "%c%s%d%s%dPin%d",
                pinInfo->port,
                "pt",
                pinInfo->portN,
                Reg == LPT_DATA_REG ? "Data"
                                    : Reg == LPT_STATUS_REG ? "Status" : Reg == LPT_CONTROL_REG ? "Control" : "Reg",
                Bit,
                pinInfo->dbPin);
    } else if((pinInfo->port == 'c') || (pinInfo->port == 'C')) {
        if(pinInfo->port == 'c')
            ComDb9PinToRegMask(pinInfo->dbPin, &Reg, &Mask);
        else
            ComDb25PinToRegMask(pinInfo->dbPin, &Reg, &Mask);
        Bit = MaskToBit(Mask);
        pinInfo->bit = Bit;
        sprintf(pinInfo->pinName,
                "%c%s%d%s%dPin%d",
                pinInfo->port,
                "om",
                pinInfo->portN,
                Reg == LCR ? "Lcr" : Reg == MCR ? "Mcr" : Reg == MSR ? "Msr" : "Reg",
                Bit,
                pinInfo->dbPin);
    }
}
//-----------------------------------------------------------------------------
bool SavePcPorts(const char *filename)
{
    BOOL b = FALSE;
    b = WritePrivateProfileString(TEXT("LPT1"), TEXT("ADDR   "), TEXT("0x378"), filename);
    b = WritePrivateProfileString(TEXT("LPT1"), TEXT("DATA   "), TEXT("OUT"), filename);
    b = WritePrivateProfileString(TEXT("LPT1"), TEXT("STATUS "), TEXT(" IN"), filename);
    b = WritePrivateProfileString(TEXT("LPT1"), TEXT("CONTROL"), TEXT(" IN"), filename);

    b = WritePrivateProfileString(TEXT("COM1"), TEXT("ADDR"), TEXT(" 0x3F8"), filename);

    b = WritePrivateProfileString(TEXT("LPT2"), TEXT("ADDR   "), TEXT("0x1028"), filename);
    b = WritePrivateProfileString(TEXT("LPT2"), TEXT("DATA   "), TEXT("OUT"), filename);
    b = WritePrivateProfileString(TEXT("LPT2"), TEXT("STATUS "), TEXT(" IN"), filename);
    b = WritePrivateProfileString(TEXT("LPT2"), TEXT("CONTROL"), TEXT(" IN"), filename);

    b = WritePrivateProfileString(TEXT("Section1"),
                                  TEXT("SecondKey"),
                                  TEXT("4By golly, it works!"),
                                  /*TEXT("appname.ini")*/ filename);
    b = WritePrivateProfileString(TEXT("Section1"),
                                  TEXT("ThirdKey"),
                                  TEXT("4Another test..."),
                                  /*TEXT("appname.ini")*/ filename);

    b = WritePrivateProfileString(TEXT("Section2"),
                                  TEXT("ThirdKey"),
                                  TEXT("4Another test..."),
                                  /*TEXT("appname.ini")*/ filename);
    return b != FALSE;
}
//-----------------------------------------------------------------------------
static const char *IoTypeToPas(int IoType)
{
    if(IoType == IO_TYPE_DIG_OUTPUT)
        return "OutPort";
    if(IoType == IO_TYPE_DIG_INPUT)
        return "InPort";
    return "NotUsePort";
}
//-----------------------------------------------------------------------------
bool LoadPcPorts(const char *filename)
{
    FileTracker f(filename, "r");
    if(!f)
        return false;

    char dest[MAX_PATH];
    strcpy(dest, CurrentCompilePath);
    if(strlen(dest))
        if(dest[strlen(dest) - 1] != '\\')
            strcat(dest, "\\");
    //    strcat(dest, ExtractFileName(filename));
    strcat(dest, filename);
    SetExt(dest, dest, "pas");

    FileTracker fu(dest, "w");
    if(!fu) {
        THROW_COMPILER_EXCEPTION_FMT(_("Couldn't open file '%s'"), dest);
        return false;
    }
    fprintf(fu,
            "unit PcPorts;\n"
            "interface\n"
            "procedure SetPcPortsDir;\n"
            "implementation\n"
            "uses ports_, portIO;\n"
            "procedure SetPcPortsDir;\n"
            "begin\n"
            "                  { bits # 76543210 76543210 76543210 76543210 }\n");

    char  line_[512];
    char  line[512];
    char *pline;
    char  line2[512];
    char  line3[512];
    char  line4[512];
    char *pline4;
    char *c;
    int   portN, db;
    int   i, j, n, Dtype, Stype, Ctype;

    DWORD addr = 0;
    strcpy(line_, "");
    IoPcCount = 0;
    while(fgets(line, sizeof(line), f)) {
        if(line[strlen(line) - 1] == '\n')
            line[strlen(line) - 1] = '\0';
        if((c = strstr(line, "//")))
            c = 0;
        if((c = strstr(line, ";")))
            c = 0;
        _strupr(line);
        pline = line;
        while(isspace(*pline))
            pline++;

        if(strlen(line) && (strspn(line_, line) != 4)) {
            strncpy(line_, line, 4);
            line_[4] = '\0';
            addr = 0;
        }
        strcpy(line2, "");
        strcpy(line3, "");
        strcpy(line4, "");
        pline4 = "";
        if((n = sscanf(pline, "LPT%d %s %s %s", &portN, &line2, &line3, &line4))) {
            if(strlen(line4))
                pline4 = strstr(pline, line4);
            if(n == 3) {
                if(strstr(line2, "ADDR")) {
                    addr = hobatoi(line3);
                    continue;
                }
            }
            Dtype = IO_TYPE_PENDING;
            Stype = IO_TYPE_PENDING;
            Ctype = IO_TYPE_PENDING;
            if(n == 1) {
                Dtype = IO_TYPE_DIG_OUTPUT;
                Stype = IO_TYPE_DIG_INPUT;
                Ctype = IO_TYPE_DIG_OUTPUT;
            } else if(n >= 2) {
                if(strstr(line2, "DATA")) {
                    Dtype = IO_TYPE_DIG_OUTPUT;
                    if(strstr(line3, "IN"))
                        Dtype = IO_TYPE_DIG_INPUT;
                } else if(strstr(line2, "STATUS") || strstr(line2, "STATE")) {
                    Stype = IO_TYPE_DIG_INPUT;
                } else if(strstr(line2, "CONTROL") || strstr(line2, "CTRL")) {
                    Ctype = IO_TYPE_DIG_OUTPUT;
                    if(strstr(line3, "IN")) {
                        Ctype = IO_TYPE_DIG_INPUT;
                    }
                }
            }
            if((portN > 0) && (IoPcCount + 8 < MAX_IO) && (Dtype != IO_TYPE_PENDING)) {
                IoPc[IoPcCount + 0].dbPin = 2;
                IoPc[IoPcCount + 0].ioType = Dtype;
                IoPc[IoPcCount + 0].portN = portN;
                IoPc[IoPcCount + 1].dbPin = 3;
                IoPc[IoPcCount + 1].ioType = Dtype;
                IoPc[IoPcCount + 1].portN = portN;
                IoPc[IoPcCount + 2].dbPin = 4;
                IoPc[IoPcCount + 2].ioType = Dtype;
                IoPc[IoPcCount + 2].portN = portN;
                IoPc[IoPcCount + 3].dbPin = 5;
                IoPc[IoPcCount + 3].ioType = Dtype;
                IoPc[IoPcCount + 3].portN = portN;
                IoPc[IoPcCount + 4].dbPin = 6;
                IoPc[IoPcCount + 4].ioType = Dtype;
                IoPc[IoPcCount + 4].portN = portN;
                IoPc[IoPcCount + 5].dbPin = 7;
                IoPc[IoPcCount + 5].ioType = Dtype;
                IoPc[IoPcCount + 5].portN = portN;
                IoPc[IoPcCount + 6].dbPin = 8;
                IoPc[IoPcCount + 6].ioType = Dtype;
                IoPc[IoPcCount + 6].portN = portN;
                IoPc[IoPcCount + 7].dbPin = 9;
                IoPc[IoPcCount + 7].ioType = Dtype;
                IoPc[IoPcCount + 7].portN = portN;

                for(i = 0; i < 8; i++) {
                    IoPc[IoPcCount].addr = addr;
                    IoPc[IoPcCount].port = 'L';
                    IoPc[IoPcCount].pin = IoPcCount + 1;
                    FillPcPinInfo(&IoPc[IoPcCount]);
                    IoPcCount++;
                }
            }
            if((portN > 0) && (IoPcCount + 5 < MAX_IO) && (Stype != IO_TYPE_PENDING)) {
                IoPc[IoPcCount + 0].dbPin = 15;
                IoPc[IoPcCount + 0].ioType = Stype;
                IoPc[IoPcCount + 0].portN = portN;
                IoPc[IoPcCount + 1].dbPin = 13;
                IoPc[IoPcCount + 1].ioType = Stype;
                IoPc[IoPcCount + 1].portN = portN;
                IoPc[IoPcCount + 2].dbPin = 12;
                IoPc[IoPcCount + 2].ioType = Stype;
                IoPc[IoPcCount + 2].portN = portN;
                IoPc[IoPcCount + 3].dbPin = 10;
                IoPc[IoPcCount + 3].ioType = Stype;
                IoPc[IoPcCount + 3].portN = portN;
                IoPc[IoPcCount + 4].dbPin = 11;
                IoPc[IoPcCount + 4].ioType = Stype;
                IoPc[IoPcCount + 4].portN = portN;

                for(i = 0; i < 5; i++) {
                    IoPc[IoPcCount].addr = addr;
                    if(addr)
                        IoPc[IoPcCount].addr += 2;
                    IoPc[IoPcCount].port = 'L';
                    IoPc[IoPcCount].pin = IoPcCount + 1;
                    FillPcPinInfo(&IoPc[IoPcCount]);
                    IoPcCount++;
                }
            }
            if((portN > 0) && (IoPcCount + 4 < MAX_IO) && (Ctype != IO_TYPE_PENDING)) {
                IoPc[IoPcCount + 0].dbPin = 1;
                IoPc[IoPcCount + 0].ioType = Ctype;
                IoPc[IoPcCount + 0].portN = portN;
                IoPc[IoPcCount + 1].dbPin = 14;
                IoPc[IoPcCount + 1].ioType = Ctype;
                IoPc[IoPcCount + 1].portN = portN;
                IoPc[IoPcCount + 2].dbPin = 16;
                IoPc[IoPcCount + 2].ioType = Ctype;
                IoPc[IoPcCount + 2].portN = portN;
                IoPc[IoPcCount + 3].dbPin = 17;
                IoPc[IoPcCount + 3].ioType = Ctype;
                IoPc[IoPcCount + 3].portN = portN;

                for(i = 0; i < 4; i++) {
                    IoPc[IoPcCount].addr = addr;
                    if(addr)
                        IoPc[IoPcCount].addr += 1;
                    IoPc[IoPcCount].port = 'L';
                    IoPc[IoPcCount].pin = IoPcCount + 1;
                    FillPcPinInfo(&IoPc[IoPcCount]);
                    IoPcCount++;
                }
            }
            if(Dtype)
                fprintf(fu, "  SetALptDir(%d,0,%-7s,' %s DATA');\n", portN, IoTypeToPas(Dtype), pline4);
            if(Stype)
                fprintf(fu, "  SetALptDir(%d,1,%-7s,' %s STATUS');\n", portN, IoTypeToPas(Stype), pline4);
            if(Ctype)
                fprintf(fu, "  SetALptDir(%d,2,%-7s,' %s CONTROL');\n\n", portN, IoTypeToPas(Ctype), pline4);

        } else if((n = sscanf(pline, "COM%d %s %s %s", &portN, &line2, &line3, &line4))) {
            if(strlen(line4))
                pline4 = strstr(pline, line4);
            if(n == 3) {
                if(strstr(line2, "ADDR")) {
                    addr = hobatoi(line3);
                    continue;
                }
            }
            if((portN > 0) && (IoPcCount + 7 < MAX_IO)) {
                db = 9;
                if(strstr(line2, "DB25") || strstr(line3, "DB25"))
                    db = 25;
                if(db == 9) {
                    IoPc[IoPcCount + 0].dbPin = 3;
                    IoPc[IoPcCount + 1].dbPin = 4;
                    IoPc[IoPcCount + 2].dbPin = 7;
                    IoPc[IoPcCount + 3].dbPin = 8;
                    IoPc[IoPcCount + 4].dbPin = 6;
                    IoPc[IoPcCount + 5].dbPin = 9;
                    IoPc[IoPcCount + 6].dbPin = 1;
                } else {
                    IoPc[IoPcCount + 0].dbPin = 2;
                    IoPc[IoPcCount + 1].dbPin = 20;
                    IoPc[IoPcCount + 2].dbPin = 4;
                    IoPc[IoPcCount + 3].dbPin = 5;
                    IoPc[IoPcCount + 4].dbPin = 6;
                    IoPc[IoPcCount + 5].dbPin = 22;
                    IoPc[IoPcCount + 6].dbPin = 8;
                };
                Dtype = IO_TYPE_PENDING;
                Stype = IO_TYPE_PENDING;
                if(n == 1) {
                    Dtype = IO_TYPE_DIG_OUTPUT;
                    Stype = IO_TYPE_DIG_INPUT;
                } else if(strstr(line2, "IN") || strstr(line3, "IN"))
                    Stype = IO_TYPE_DIG_INPUT;
                else if(strstr(line2, "OUT") || strstr(line3, "OUT"))
                    Dtype = IO_TYPE_DIG_OUTPUT;

                IoPc[IoPcCount + 0].ioType = Dtype;
                IoPc[IoPcCount + 0].portN = portN;
                IoPc[IoPcCount + 1].ioType = Dtype;
                IoPc[IoPcCount + 1].portN = portN;
                IoPc[IoPcCount + 2].ioType = Dtype;
                IoPc[IoPcCount + 2].portN = portN;
                IoPc[IoPcCount + 3].ioType = Stype;
                IoPc[IoPcCount + 3].portN = portN;
                IoPc[IoPcCount + 4].ioType = Stype;
                IoPc[IoPcCount + 4].portN = portN;
                IoPc[IoPcCount + 5].ioType = Stype;
                IoPc[IoPcCount + 5].portN = portN;
                IoPc[IoPcCount + 6].ioType = Stype;
                IoPc[IoPcCount + 6].portN = portN;
                j = IoPcCount;
                for(i = 0; i < 7; i++) {
                    if(IoPc[j + i].ioType != IO_TYPE_PENDING) {
                        IoPc[IoPcCount].addr = addr + i;
                        IoPc[IoPcCount].port = db == 9 ? 'c' : 'C';
                        IoPc[IoPcCount].pin = IoPcCount + 1;
                        FillPcPinInfo(&IoPc[IoPcCount]);
                        IoPcCount++;
                    }
                }
            }
            if(Dtype) {
                fprintf(fu, "  SetAComDir(%d,3,%-7s,' %s LCR');\n", portN, IoTypeToPas(Dtype), pline4);
                fprintf(fu, "  SetAComDir(%d,4,%-7s,' %s MCR');\n", portN, IoTypeToPas(Dtype), pline4);
            }
            if(Stype)
                fprintf(fu, "  SetAComDir(%d,6,%-7s,' %s MSR');\n\n", portN, IoTypeToPas(Stype), pline4);
        }
    }
    fprintf(fu, "end;\nend.");
    return true;
}
//-----------------------------------------------------------------------------
