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
    rungs_.fill(nullptr);
    numRungs = 0;
    cycleTime = 10000;
    mcuClock = 16000000;
    baudRate = 9600;
    spiRate = 1000000;
    i2cRate = 100000;
    io.count = 0;
    cycleTimer = 1;
    cycleDuty = 0;
    configurationWord = 0;
    setMcu(nullptr);
    LDversion = "0.2";
    for(int i = 0; i < MAX_IO_PORTS; i++) {
        pullUpRegs[i] = ~0u; // All input pins try to set Pull-up registers by default.
    }
    WDTPSA = 0;
    OPTION = 0;
    for(int i = 0; i <= MAX_RUNGS; i++) {
        rungPowered[i] = 0;
        rungSimulated[i] = 0;
        rungSelected[i] = 0;
        OpsInRung[i] = 0;
        HexInRung[i] = 0;
    }
	compiler = 0;
}

PlcProgram::PlcProgram(const PlcProgram &other)
{
    *this = other;
}

PlcProgram &PlcProgram::operator=(const PlcProgram &other)
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
    std::copy(other.pullUpRegs, other.pullUpRegs + numRungs, pullUpRegs);
    for(int i = 0; i < numRungs; ++i) {
        rungs_[i] = static_cast<ElemSubcktSeries *>(deepCopy(ELEM_SERIES_SUBCKT, other.rungs_[i]));
    }
    setMcu(other.mcu_);
    return *this;
}

PlcProgram::~PlcProgram()
{
    reset();
}

void LoadWritePcPorts();
void PlcProgram::setMcu(McuIoInfo *m)
{
    mcu_ = m;
    if(!mcu_)
        return;

    // configurationWord = 0; Don't resets configurationWord when select other MCU !!!

    auto comparePinInfo = [](const McuIoPinInfo &a, const McuIoPinInfo &b) -> bool {
        const char *sa = strlen(a.pinName) > 0 ? a.pinName : "";
        const char *sb = strlen(b.pinName) > 0 ? b.pinName : "";
        return (strcmp(sa, sb) < 0);
    };

    if(mcu_ && (mcu_->core != PC_LPT_COM))
        std::sort(mcu_->pinInfo, mcu_->pinInfo + mcu_->pinCount, comparePinInfo);
}

int PlcProgram::mcuPWM() const
{
    if(!mcu_)
        return 0;

    int n = 0;
    if(mcu_->pwmCount) {
        int prevPin = -1;
        for(uint32_t i = 0; i < mcu_->pwmCount; i++) {
            if(mcu_->pwmInfo[i].pin)
                if(mcu_->pwmInfo[i].pin != prevPin)
                    if((mcu_->whichIsa == ISA_PIC16) || (mcu_->pwmInfo[i].timer != cycleTimer))
                        n++;
            prevPin = mcu_->pwmInfo[i].pin;
        }
    }
    return n;
}

int PlcProgram::mcuADC() const
{
    return mcu_ ? mcu_->adcCount : 0;
}

int PlcProgram::mcuSPI() const
{
    return mcu_ ? mcu_->spiCount : 0;
}

int PlcProgram::mcuI2C() const
{
    return mcu_ ? mcu_->i2cCount : 0;
}

int PlcProgram::mcuROM() const
{
    return 1000000; //TODO: fix ROM hardcode
    /*
    if(!mcu_)
        return 0;

    int n = 0;
    for(uint32_t i = 0; i < MAX_ROM_SECTIONS; i++) {
        n += mcu_->rom[i].len;
    }
    return n;
*/
}

int PlcProgram::mcuRAM() const
{
    if(!mcu_)
        return 0;

    int n = 0;
    for(uint32_t i = 0; i < MAX_RAM_SECTIONS; i++) {
        n += mcu_->ram[i].len;
    }
    return n;
}

int PlcProgram::mcuUART() const
{
    if(!mcu_)
        return 0;
    if(mcu_->uartNeeds.rxPin && mcu_->uartNeeds.txPin)
        return 1;
    return 0;
}

void PlcProgram::reset()
{
    for(int i = 0; i < numRungs; i++) {
        FreeCircuit(ELEM_SERIES_SUBCKT, rungs_[i]);
    }
    memset(rungSelected, ' ', sizeof(rungSelected));
    numRungs = 0;
    cycleTime = 10000;
    mcuClock = 16000000;
    baudRate = 9600;
    spiRate = 1000000;
    i2cRate = 100000;
    io.count = 0;
    cycleTimer = 1;
    cycleDuty = 0;
    setMcu(nullptr);
    LDversion = "0.2";
}

void PlcProgram::appendEmptyRung()
{
    if(numRungs >= (MAX_RUNGS - 1))
        THROW_COMPILER_EXCEPTION_FMT(_("Exceeded the limit of %d rungs!"), MAX_RUNGS);
    rungs_[numRungs++] = AllocEmptyRung();
}

void PlcProgram::insertEmptyRung(uint32_t idx)
{
    if(static_cast<int>(idx) >= numRungs)
        THROW_COMPILER_EXCEPTION_FMT(_("Invalid rung index %lu!"), idx);
    if(numRungs > (MAX_RUNGS - 1))
        THROW_COMPILER_EXCEPTION_FMT(_("Exceeded the limit of %d rungs!"), MAX_RUNGS);
    memmove(&rungs_[idx + 1], &rungs_[idx], (numRungs - idx) * sizeof(rungs_[0]));
    memmove(&rungSelected[idx + 1], &rungSelected[idx], (numRungs - idx) * sizeof(rungSelected[0]));
    rungs_[idx] = AllocEmptyRung();
    numRungs++;
}

void *PlcProgram::deepCopy(int which, const void *any) const
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

    //return nullptr;
}
