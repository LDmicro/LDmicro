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
    rungs.fill(nullptr);
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

PlcProgram::PlcProgram(const PlcProgram& other)
{
    *this = other;
}

PlcProgram& PlcProgram::operator=(const PlcProgram& other)
{
    this->reset();

    cycleTime = other.cycleTime;
    mcuClock = other.mcuClock;
    baudRate = other.baudRate;
    cycleTimer = other.cycleTimer;
    cycleDuty = other.cycleDuty;
    configurationWord = other.configurationWord;
    WDTPSA = other.WDTPSA;
    OPTION = other.OPTION;
    spiRate = other.spiRate;
    i2cRate = other.i2cRate;
    LDversion = other.LDversion;

    io.count = other.io.count;
    std::copy(other.io.assignment, other.io.assignment + other.io.count, io.assignment);

    numRungs = other.numRungs;
    std::copy(other.rungPowered, other.rungPowered + numRungs, rungPowered);
    std::copy(other.rungSimulated, other.rungSimulated + numRungs, rungSimulated);
    std::copy(other.rungSelected, other.rungSelected + numRungs, rungSelected);
    std::copy(other.OpsInRung, other.OpsInRung + numRungs, OpsInRung);
    std::copy(other.HexInRung, other.HexInRung + numRungs, HexInRung);
    for(int i = 0; i < numRungs; ++i) {
        rungs[i] = static_cast<ElemSubcktSeries *>(deepCopy(ELEM_SERIES_SUBCKT, other.rungs[i]));
    }
    setMcu(other.mcu_);
    return *this;
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

    configurationWord = 0;

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
    setMcu(nullptr);
}

void PlcProgram::appendEmptyRung()
{
    if(numRungs >= (MAX_RUNGS - 1))
        THROW_COMPILER_EXCEPTION_FMT(_("Exceeded the limit of %d rungs!"), MAX_RUNGS);
    rungs[numRungs++] = AllocEmptyRung();
}

void PlcProgram::insertEmptyRung(uint32_t idx)
{
    if(static_cast<int>(idx) >= numRungs)
        THROW_COMPILER_EXCEPTION_FMT(_("Invalid rung index %lu!"), idx);
    if(numRungs > (MAX_RUNGS - 1))
        THROW_COMPILER_EXCEPTION_FMT(_("Exceeded the limit of %d rungs!"), MAX_RUNGS);
    memmove(&rungs[idx + 1], &rungs[idx], (numRungs - idx) * sizeof(rungs[0]));
    memmove(&rungSelected[idx + 1], &rungSelected[idx], (numRungs - idx) * sizeof(rungSelected[0]));
    rungs[idx] = AllocEmptyRung();
    numRungs++;
}

void* PlcProgram::deepCopy(int which, const void* any) const
{
    switch(which) {
        CASE_LEAF
        {
            ElemLeaf *leaf = AllocLeaf();
            memcpy(leaf, any, sizeof(ElemLeaf));
            leaf->selectedState = SELECTED_NONE;
            return leaf;
        }
        case ELEM_SERIES_SUBCKT: {
            ElemSubcktSeries *n = AllocSubcktSeries();
            ElemSubcktSeries *s = (ElemSubcktSeries *)any;
            n->count = s->count;
            for(int i = 0; i < s->count; i++) {
                n->contents[i].which = s->contents[i].which;
                n->contents[i].data.any = deepCopy(s->contents[i].which, s->contents[i].data.any);
            }
            return n;
        }
        case ELEM_PARALLEL_SUBCKT: {
            ElemSubcktParallel *n = AllocSubcktParallel();
            ElemSubcktParallel *p = (ElemSubcktParallel *)any;
            n->count = p->count;
            for(int i = 0; i < p->count; i++) {
                n->contents[i].which = p->contents[i].which;
                n->contents[i].data.any = deepCopy(p->contents[i].which, p->contents[i].data.any);
            }
            return n;
        }
        default:
            THROW_COMPILER_EXCEPTION_FMT("Invalid series element, whitch = %i", which);
    }

    return nullptr;
}
