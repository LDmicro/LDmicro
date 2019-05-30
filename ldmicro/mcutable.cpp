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
// The table of supported MCUs, used to determine where the IOs are, what
// instruction set, what init code, etc.
// Jonathan Westhues, Oct 2004
//-----------------------------------------------------------------------------
#include "stdafx.h"

#include "mcutable.hpp"
#include "mcutable.h"

std::vector<McuIoInfo> &supportedMcus()
{
    return SupportedMcus;
}

void FillPcPinInfo(McuIoPinInfo *pinInfo);
bool fillPcPinInfos()
{
    for(uint32_t i = 0; i < arraylen(PcCfg); i++)
        FillPcPinInfo(&PcCfg[i]);
    return true;
}
