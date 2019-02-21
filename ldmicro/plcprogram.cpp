#include "plcprogram.h"
#include "stdafx.h"
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

PlcProgram::PlcProgram()
{
    memset(rungSelected, ' ', sizeof(rungSelected));
    numRungs = 0;
    cycleTime = 10000;
    mcuClock = 16000000;
    baudRate = 9600;
    io.count = 0;
    cycleTimer = 1;
    cycleDuty = 0;
    configurationWord = 0;
    setMcu(nullptr);
}

PlcProgram::~PlcProgram()
{
    reset();
}

void LoadWritePcPorts();
void PlcProgram::setMcu(McuIoInfo* m)
{
    mcu_ = m;
    if(!mcu_)
        return;

	configurationWord = 0;     ///// Added by JG

    LoadWritePcPorts();

    auto comparePinInfo = [](const McuIoPinInfo& a, const McuIoPinInfo& b) -> bool {
            const char* sa = strlen(a.pinName) > 0 ? a.pinName : "";
            const char* sb = strlen(b.pinName) > 0 ? b.pinName : "";
            return (strcmp(sa, sb) < 0);
        };

    if(mcu_ && (mcu_->core != PC_LPT_COM))
        std::sort(mcu_->pinInfo, mcu_->pinInfo + mcu_->pinCount, comparePinInfo);
}

void PlcProgram::reset()
{
    for(int i = 0; i < numRungs; i++) {
        FreeCircuit(ELEM_SERIES_SUBCKT, rungs[i]);
    }
    memset(rungSelected, ' ', sizeof(rungSelected));
    numRungs = 0;
    cycleTime = 10000;
    mcuClock = 16000000;
    baudRate = 9600;
    io.count = 0;
    cycleTimer = 1;
    cycleDuty = 0;
    configurationWord = 0;
    setMcu(nullptr);
}

bool PlcProgram::appendEmptyRung()
{
    if(numRungs >= (MAX_RUNGS - 1))
        return false;
    rungs[numRungs++] = AllocEmptyRung();
    return true;
}
