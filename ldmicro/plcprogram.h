
#ifndef __PLCPROGRAM_H
#define __PLCPROGRAM_H

#include <array>
#include "circuit.h"
#include "mcutable.hpp"
#include "compilercommon.hpp"

typedef struct McuIoInfoTag McuIoInfo;

typedef struct ModbusAddr {
    uint8_t  Slave;
    uint16_t Address;
} ModbusAddr_t;

struct PlcProgramSingleIo {
    char name[MAX_NAME_LEN];
/*More convenient sort order in IOlist*/
// clang-format off
#define IO_TYPE_NO              0
#define IO_TYPE_PENDING         0
#define IO_TYPE_GENERAL         1
#define IO_TYPE_PERSIST         2
#define IO_TYPE_COUNTER         5
#define IO_TYPE_INT_INPUT       6
#define IO_TYPE_DIG_INPUT       7
#define IO_TYPE_DIG_OUTPUT      8
#define IO_TYPE_READ_ADC        9
#define IO_TYPE_UART_TX         10
#define IO_TYPE_UART_RX         11
#define IO_TYPE_PWM_OUTPUT      12
#define IO_TYPE_INTERNAL_RELAY  13
#define IO_TYPE_TCY             14
#define IO_TYPE_RTO             15
#define IO_TYPE_RTL             16
#define IO_TYPE_TON             17
#define IO_TYPE_TOF             18
#define IO_TYPE_THI             19
#define IO_TYPE_TLO             20
#define IO_TYPE_MODBUS_CONTACT  21
#define IO_TYPE_MODBUS_COIL     22
#define IO_TYPE_MODBUS_HREG     23
#define IO_TYPE_PORT_INPUT      24 // 8bit PORT for in data  - McuIoInfo.inputRegs
#define IO_TYPE_PORT_OUTPUT     25 // 8bit PORT for out data - McuIoInfo.oututRegs
#define IO_TYPE_MCU_REG         26 // 8bit register in/out data as McuIoInfo.dirRegs
#define IO_TYPE_BCD             27 // unpacked, max 10 byte
#define IO_TYPE_STRING          28
//#define IO_TYPE_STRING_LITERAL  29
#define IO_TYPE_TABLE_IN_FLASH  30 // max limited (size of flsh - progSize)
#define IO_TYPE_VAL_IN_FLASH    31 //
#define IO_TYPE_SPI_MOSI        32
#define IO_TYPE_SPI_MISO        33
#define IO_TYPE_SPI_SCK         34
#define IO_TYPE_SPI__SS         35
#define IO_TYPE_I2C_SCL         36  ///// Added by JG
#define IO_TYPE_I2C_SDA         37  /////
    int32_t    type;
#define NO_PIN_ASSIGNED         0
    // clang-format on
    int32_t    pin;
    ModbusAddr modbus;
};

class PlcProgram {
  public:
    PlcProgram();
    PlcProgram(const PlcProgram &other);
    ~PlcProgram();
    void             setMcu(McuIoInfo *mcu);
    const McuIoInfo *mcu() const
    {
        return mcu_;
    }
    int               mcuPWM() const;
    int               mcuADC() const;
    int               mcuSPI() const;
    int               mcuI2C() const;
    int               mcuUART() const;
    int               mcuROM() const;
    int               mcuRAM() const;
    void              reset();
    ElemSubcktSeries *rungs(uint32_t idx)
    {
        return rungs_[idx];
    }
    void appendEmptyRung();
    void insertEmptyRung(uint32_t idx);

  public:
    PlcProgram &operator=(const PlcProgram &other);

  private:
    void *deepCopy(int which, const void *any) const;

  public:
    struct {
        PlcProgramSingleIo assignment[MAX_IO];
        int32_t            count;
    } io;
    int64_t  cycleTime;                // us
    int32_t  cycleTimer;               // 1 or 0
    uint32_t pullUpRegs[MAX_IO_PORTS]; // A is 0, J is 9 // PIC, AVR, ARM, ...
                                       //  uint32_t      pullDnRegs[MAX_IO_PORTS]; // A is 0, J is 9 // ARM
    int64_t configurationWord;         // only PIC
    uint8_t WDTPSA;                    // only for PIC
    uint8_t OPTION;                    // only for PIC10Fxxx
#define YPlcCycleDuty "YPlcCycleDuty"
    int32_t   cycleDuty; // if true, "YPlcCycleDuty" pin set to 1 at begin and to 0 at end of PLC cycle
    int32_t   mcuClock;  // Hz
    int32_t   baudRate;  // Hz
    int32_t   spiRate;   // Hz          Added by JG
    int32_t   i2cRate;   // Hz          Added by JG
    NameArray LDversion;

    std::array<ElemSubcktSeries *, MAX_RUNGS> rungs_; // TODO: move to private:

    int32_t  numRungs;
    bool     rungPowered[MAX_RUNGS + 1]; // [MAX_RUNGS + 1] for Label after last rung
    bool     rungSimulated[MAX_RUNGS + 1];
    char     rungSelected[MAX_RUNGS + 1];
    uint32_t OpsInRung[MAX_RUNGS + 1];
    uint32_t HexInRung[MAX_RUNGS + 1];

  private:
    McuIoInfo *mcu_;
};

#endif // PLCPROGRAM_H
