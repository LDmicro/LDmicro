//-----------------------------------------------------------------------------
// Copyright 2007 Jonathan Westhues
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
// Write the program as ANSI C source. This is very simple, because the
// intermediate code structure is really a lot like C. Someone else will be
// responsible for calling us with appropriate timing.
// Jonathan Westhues, Oct 2004
//-----------------------------------------------------------------------------
#include "stdafx.h"

#include "ldmicro.h"
#include "intcode.h"

#define USE_STRING_LITERAL // --- //

namespace {
    std::unordered_set<std::string> variables;
    bool                            all_arduino_pins_are_mapped;
} // namespace

static int mcu_ISA = -1;
static int compiler_variant = -1;

static int TIMER_Used = 0; // for Cycle timer variable
static int UART_Used = -1; // and UART #
static int I2C_Used = -1;  // and I2C #
static int SPI_Used = -1;  // and SPI # or SPI Port

static int SPI_MOSI = 0; // and SPI pins (for AVRs)
static int SPI_MISO = 0;
static int SPI_SCK = 0;
static int SPI_SS = 0;

#define MAX_ADC_C 256
static int ADC_Used[MAX_ADC_C]; // used ADC #
static int ADC_Chan[MAX_ADC_C]; // ADC channel

#define MAX_PWM_C 256
static int  PWM_Used[MAX_PWM_C];  // used PWM #
static long PWM_Freq[MAX_PWM_C];  // PWM Frequency
static int  PWM_Resol[MAX_PWM_C]; // PWM Resolution
static int  PWM_MaxCs[MAX_PWM_C]; // PWM Max prediv choices
static int  countpwm = 0;
/////

//-----------------------------------------------------------------------------
// Have we seen a variable before? If not then no need to generate code for
// it, otherwise we will have to make a declaration, and mark it as seen.
//-----------------------------------------------------------------------------
static bool SeenVariable(const char *name)
{
    if(IsNumber(name))
        return true;

    if(variables.count(name))
        return true;

    variables.emplace(name);
    return false;
}

//-----------------------------------------------------------------------------
// Turn an internal symbol into a C name; only trick is that internal symbols
// use $ for symbols that the int code generator needed for itself, so map
// that into something okay for C.
//-----------------------------------------------------------------------------
#define ASBIT 1
#define ASINT 2
#define ASSTR 3
static const char *MapSym(const char *str, int how = ASINT);
static const char *MapSym(const char *str, int how)
// TODO: Use GetVariableType() instead 'how'
{
    if(!str)
        return nullptr;
    if(strlen(str) == 0)
        return nullptr;

    // can be called as parameters of another function up to 16 times
    static char AllRets[16][MAX_NAME_LEN + 30];
    static int  RetCnt;
    RetCnt = (RetCnt + 1) & 15;
    char *ret = AllRets[RetCnt];

    // The namespace for bit and integer variables is distinct.
    char bit_int;
    if(how == ASBIT) {
        bit_int = 'b';
    } else if(how == ASINT) {
        bit_int = 'i';
    } else if(how == ASSTR) {
        bit_int = 's';
    } else {
        THROW_COMPILER_EXCEPTION(_("Can't assign prefix."));
    }

    // User and internal symbols are distinguished.
    if(IsNumber(str))
        sprintf(ret, "%s", str);
    else if(*str == '#')
        sprintf(ret, "Ad_%s", str + 1); // Memory access
    else if(*str == '$') {
        sprintf(ret, "I%c_%s", bit_int, str + 1);
    } else {
        sprintf(ret, "U%c_%s", bit_int, str);
    }

    char trans[1024];
    Transliterate(trans, ret);
    strcpy(ret, trans);

    return ret;
}

static const char *MapSym(const NameArray &name, int how = ASINT);
static const char *MapSym(const NameArray &name, int how)
{
    return MapSym(name.c_str(), how);
}

//-----------------------------------------------------------------------------
// Generate a declaration for an integer var; easy, a static.
//-----------------------------------------------------------------------------
static void DeclareInt(FILE *f, FILE *fh, const char *str, int sov)
{
    if(IsNumber(str))
        return;
    if(*str == 'A') {
        if(IsNumber(&str[3])) {
            fprintf(f, "#define %s SFR_ADDR(%s) // Memory access\n", str, &str[3]);
        } else {
            ADDR_T addr;
            char   name[MAX_NAME_LEN];
            sprintf_s(name, "#%s", &str[3]);
            MemForVariable(name, &addr);
            fprintf(f, "#define %s SFR_ADDR(0x%X) // Memory access\n", str, addr);
        }
    } else if(sov == 1) {
        fprintf(f, "_STATIC_ SBYTE %s = 0;\n", str);
        fprintf(fh, "#ifdef EXTERN_EVERYTHING\n  extern SBYTE %s;\n#endif\n", str);
    } else if(sov == 2) {
        ///// Added by JG to get SPI # / I2C #
        if(compiler_variant == MNU_COMPILE_HI_TECH_C) {
            char devname[MAX_NAME_LEN]; // spi name = "SPI"
            int  devpins[4];
            strcpy(devname, str);
            if(strcmp(devname, "Ui_SPI") == 0) {
                SPI_Used = PinsForSpiVariable(&devname[3], 3, devpins); // to force pin assignment on SPI and get port + pins
                if(SPI_Used == 0)
                    THROW_COMPILER_EXCEPTION(_("SPI Internal error"));

                SPI_MOSI = devpins[0];
                SPI_MISO = devpins[1];
                SPI_SCK = devpins[2];
                SPI_SS = devpins[3];
            }
            if(strcmp(devname, "Ui_I2C") == 0) {
                I2C_Used = PinsForI2cVariable(&devname[3], 3, devpins); // to force pin assignment on I2Cn
                if(I2C_Used == 0)
                    THROW_COMPILER_EXCEPTION(_("I2C Internal error"));
            }
        } else if(compiler_variant == MNU_COMPILE_ARMGCC) {
            char devname[MAX_NAME_LEN];
            int  devpins[4]; // unused here
            strcpy(devname, str);
            devname[6] = '0'; // to simplify comparison
            if(strcmp(devname, "Ui_SPI0") == 0) {
                int u = atoi(str + 6); // SPI #
                if((SPI_Used != -1) && (SPI_Used != u))
                    THROW_COMPILER_EXCEPTION(_("Only one SPI can be used"));
                else
                    SPI_Used = u;

                strcpy(devname, str);
                PinsForSpiVariable(&devname[3], 4, devpins); // to force pin assignment on SPIn
            }
            if(strcmp(devname, "Ui_I2C0") == 0) {
                int u = atoi(str + 6); // I2C #
                if((I2C_Used != -1) && (I2C_Used != u))
                    THROW_COMPILER_EXCEPTION(_("Only one I2C can be used"));
                else
                    I2C_Used = u;

                strcpy(devname, str);
                PinsForI2cVariable(&devname[3], 4, devpins); // to force pin assignment on I2Cn
            }
        } else if(compiler_variant == MNU_COMPILE_AVRGCC) {
            char devname[MAX_NAME_LEN]; // spi name = "SPI"
            int  devpins[4];
            strcpy(devname, str);
            if(strstr(devname, "Ui_SPI")) {
                SPI_Used = PinsForSpiVariable(&devname[3], 3, devpins); // to force pin assignment on SPI and get port + pins
                if(SPI_Used == 0)
                    THROW_COMPILER_EXCEPTION(_("SPI Internal error"));

                SPI_MOSI = devpins[0];
                SPI_MISO = devpins[1];
                SPI_SCK = devpins[2];
                SPI_SS = devpins[3];
            }
            if(strcmp(devname, "Ui_I2C") == 0) {
                I2C_Used = PinsForI2cVariable(&devname[3], 3, devpins); // to force pin assignment on I2Cn
                if(I2C_Used == 0)
                    THROW_COMPILER_EXCEPTION(_("I2C Internal error"));
            }
        }
        /////

        fprintf(f, "_STATIC_ SWORD %s = 0;\n", str);
        fprintf(fh, "#ifdef EXTERN_EVERYTHING\n  extern SWORD %s;\n#endif\n", str);
    } else if((sov == 3) || (sov == 4)) {
        fprintf(f, "_STATIC_ SDWORD %s = 0;\n", str);
        fprintf(fh, "#ifdef EXTERN_EVERYTHING\n  extern SDWORD %s;\n#endif\n", str);
    } else {
        fprintf(f, "_STATIC_ SWORD %s = 0;\n", str);
        fprintf(fh, "#ifdef EXTERN_EVERYTHING\n  extern SWORD %s;\n#endif\n", str);
    }
}

//-----------------------------------------------------------------------------
// Generate a declaration for a string.
//-----------------------------------------------------------------------------
static void DeclareStr(FILE *f, FILE *fh, const char *str, int sov)
{
    fprintf(f, "_STATIC_ char %s[%d];\n", str, sov);
    fprintf(f, "\n");
    fprintf(fh, "#ifdef EXTERN_EVERYTHING\n  extern char %s[%d];\n#endif\n", str, sov);
    fprintf(fh, "\n");
}

//-----------------------------------------------------------------------------
// Generate a declaration for a bit var; three cases, input, output, and
// internal relay. An internal relay is just a bool variable, but for an
// input or an output someone else must provide read/write functions.
//-----------------------------------------------------------------------------
static void DeclareBit(FILE *f, FILE *fh, FILE *flh, const char *str, int set1)
{
    // The mapped symbol has the form U_b_{X,Y,R}name, so look at character
    // four to determine if it's an input, output, internal relay.
    int type = GetAssignedType(&str[3], str);

    if(type == IO_TYPE_DIG_INPUT) {
        if(compiler_variant == MNU_COMPILE_ARDUINO) {
            McuIoPinInfo *iop = PinInfoForName(&str[3]);
            const char *  s = ArduinoPinName(iop);
            if(strlen(s)) {
                fprintf(fh, "const int pin_%s = %s; // %s\n", str, s, iop->pinName);
            } else {
                fprintf(fh, "const int pin_%s = -1;\n", str);
                all_arduino_pins_are_mapped = false;
            }

            fprintf(fh, "#ifndef NO_PROTOTYPES\n");
            fprintf(fh, "  // LDmicro provide this macro or function.\n");
            fprintf(fh, "  #ifdef USE_MACRO\n");
            fprintf(fh, "    #define Read_%s() digitalRead(pin_%s)\n", str, str);
            fprintf(fh, "  #else\n");
            fprintf(fh, "    PROTO(BOOLEAN_t Read_%s(void));\n", str);
            fprintf(fh, "  #endif\n");
            fprintf(fh, "#endif\n");
            fprintf(fh, "\n");

            fprintf(f, "#ifndef USE_MACRO\n");
            fprintf(f, "  // LDmicro provide this function.\n");
            fprintf(f, "  BOOLEAN_t Read_%s(void) {\n", str);
            fprintf(f, "    return digitalRead(pin_%s);\n", str);
            fprintf(f, "  }\n");
            fprintf(f, "#endif\n");
            fprintf(f, "\n");
        } else {
            McuIoPinInfo *iop = PinInfoForName(&str[3]);
            if(iop) {
                fprintf(fh, "// LDmicro provide this macro or function.\n");
                fprintf(fh, "#ifdef USE_MACRO\n");
                fprintf(fh, "  #define Read_%s() PIN_BIT_STATE(%c, %d)\n", str, iop->port, iop->bit);
                /*
                if(compiler_variant == MNU_COMPILE_CCS_PIC_C) {
                    fprintf(fh, "  #define Read_%s() input_state(PIN_%c%d)\n", str, iop->port, iop->bit);
                } else if(compiler_variant == MNU_COMPILE_HI_TECH_C) {
                    if(Prog.mcu()->core == PIC18HighEndCore16bit)
                        fprintf(fh, "  #define Read_%s() PORT%cbits.R%c%d\n", str, iop->port, iop->port, iop->bit);
                    else
                        fprintf(fh, "  #define Read_%s() R%c%d\n", str, iop->port, iop->bit);
                } else {
                    fprintf(fh, "  #define Read_%s() (PIN%c & (1<<PIN%c%d))\n", str, iop->port, iop->port, iop->bit);
                }
                */
                fprintf(fh, "#else\n");
                fprintf(fh, "  PROTO(BOOLEAN_t Read_%s(void));\n", str);
                fprintf(fh, "#endif\n");
                fprintf(fh, "\n");

                fprintf(f, "#ifndef USE_MACRO\n");
                fprintf(f, "  // LDmicro provide this function.\n");
                fprintf(f, "  BOOLEAN_t Read_%s(void) {\n", str);
                fprintf(f, "    return PORT_BIT_STATE(%c, %d);\n", iop->port, iop->bit);
                /*
                if(compiler_variant == MNU_COMPILE_CCS_PIC_C) {
                    fprintf(f, "    return input_state(PIN_%c%d);\n", iop->port, iop->bit);
                } else if(compiler_variant == MNU_COMPILE_HI_TECH_C) {
                    if(Prog.mcu()->core == PIC18HighEndCore16bit)
                        fprintf(f, "    return PORT%cbits.R%c%d;\n", iop->port, iop->port, iop->bit);
                    else
                        fprintf(f, "    return R%c%d;\n", iop->port, iop->bit);
                } else if(compiler_variant == MNU_COMPILE_ARMGCC) {
                    fprintf(f, "    return GPIO_ReadInputDataBit(GPIO%c, GPIO_Pin_%d);\n", iop->port, iop->bit);
                } else {
                    fprintf(f, "    return PIN%c & (1<<PIN%c%d);\n", iop->port, iop->port, iop->bit);
                }
                */
                fprintf(f, "  }\n");
                fprintf(f, "#endif\n");
                fprintf(f, "\n");
            } else {
                fprintf(fh, "// You provide this function.\n");
                fprintf(fh, "PROTO(BOOLEAN_t Read_%s(void));\n", str);
                fprintf(fh, "\n");

                fprintf(f, "\n");
                fprintf(f, "// You provide this function.\n");
                fprintf(f, "LDSTUB%d( BOOLEAN_t Read_%s(void) )\n", set1, str);
                fprintf(f, "\n");
            }
        }
    } else if(type == IO_TYPE_DIG_OUTPUT) {
        if(compiler_variant == MNU_COMPILE_ARDUINO) {
            McuIoPinInfo *iop = PinInfoForName(&str[3]);
            const char *  s = ArduinoPinName(iop);
            if(strlen(s)) {
                fprintf(fh, "const int pin_%s = %s; // %s\n", str, s, iop->pinName);
            } else {
                fprintf(fh, "const int pin_%s = -1;\n", str);
                all_arduino_pins_are_mapped = false;
            }

            fprintf(fh, "#ifndef NO_PROTOTYPES\n");
            fprintf(fh, "  // LDmicro provide these macros or functions.\n");
            fprintf(fh, "  #ifdef USE_MACRO\n");
            fprintf(fh, "    #define Read_%s() digitalRead(pin_%s)\n", str, str);
            fprintf(fh, "    #define Write0_%s() digitalWrite(pin_%s, LOW)\n", str, str);
            fprintf(fh, "    #define Write1_%s() digitalWrite(pin_%s, HIGH)\n", str, str);
            fprintf(fh, "    #define Write_%s(b) (b) ? Write1_%s() : Write0_%s()\n", str, str, str);
            fprintf(fh, "  #else\n");
            fprintf(fh, "    PROTO(BOOLEAN_t Read_%s(void));\n", str);
            fprintf(fh, "    PROTO(void Write_%s(BOOLEAN_t b));\n", str);
            fprintf(fh, "    PROTO(void Write1_%s(void));\n", str);
            fprintf(fh, "    PROTO(void Write0_%s(void));\n", str);
            fprintf(fh, "  #endif\n");
            fprintf(fh, "#endif\n");
            fprintf(fh, "\n");

            fprintf(f, "#ifndef USE_MACRO\n");
            fprintf(f, "  // LDmicro provide these functions.\n");
            fprintf(f, "  BOOLEAN_t Read_%s(void) {\n", str);
            fprintf(f, "    return digitalRead(pin_%s);\n", str);
            fprintf(f, "  }\n");
            fprintf(f, "  void Write_%s(BOOLEAN_t b) {\n", str);
            fprintf(f, "    digitalWrite(pin_%s,b);\n", str);
            fprintf(f, "  }\n");
            fprintf(f, "  void Write1_%s(void) {\n", str);
            fprintf(f, "    digitalWrite(pin_%s,HIGH);\n", str);
            fprintf(f, "  }\n");
            fprintf(f, "  void Write0_%s(void) {\n", str);
            fprintf(f, "    digitalWrite(pin_%s,LOW);\n", str);
            fprintf(f, "  }\n");
            fprintf(f, "#endif\n");
            fprintf(f, "\n");
        } else {
            McuIoPinInfo *iop = PinInfoForName(&str[3]);
            if(iop) {
                fprintf(fh, "#ifdef USE_MACRO\n");
                fprintf(fh, "  // LDmicro provide these macro.\n");
                fprintf(fh, "  #define Read_%s() PORT_BIT_STATE(%c, %d)\n", str, iop->port, iop->bit);
                /*
                if(compiler_variant == MNU_COMPILE_CCS_PIC_C) {
                    fprintf(fh, "  #define Read_%s() input_state(PIN_%c%d)\n", str, iop->port, iop->bit);
                } else if(compiler_variant == MNU_COMPILE_HI_TECH_C) {
                    if(Prog.mcu()->core == PIC18HighEndCore16bit)
                        fprintf(fh, "  #define Read_%s() PORT%cbits.R%c%d\n", str, iop->port, iop->port, iop->bit);
                    else
                        fprintf(fh, "  #define Read_%s() R%c%d\n", str, iop->port, iop->bit);
                } else if(compiler_variant == MNU_COMPILE_ARMGCC) {
                    // Do Nothing (no macro used)
                } else {
                    fprintf(fh, "  #define Read_%s() (PORT%c & (1<<PORT%c%d))\n", str, iop->port, iop->port, iop->bit);
                }
                */
                fprintf(fh, "  #define Write0_%s() PORT_BIT_CLEAR(%c, %d)\n", str, iop->port, iop->bit);
                fprintf(fh, "  #define Write1_%s() PORT_BIT_SET(%c, %d)\n", str, iop->port, iop->bit);
                fprintf(fh, "  #define Write_%s(b) if(b){ Write1_%s(); }else{ Write0_%s(); }\n", str, str, str);
                /*
                if(compiler_variant == MNU_COMPILE_CCS_PIC_C) {
                    fprintf(fh, "  #define Write0_%s() output_low(PIN_%c%d)\n", str, iop->port, iop->bit);
                    fprintf(fh, "  #define Write1_%s() output_high(PIN_%c%d)\n", str, iop->port, iop->bit);
                    fprintf(fh, "  #define Write_%s(b) (b) ? Write1_%s() : Write0_%s()\n", str, str, str);
                } else if(compiler_variant == MNU_COMPILE_HI_TECH_C) {
                    if(Prog.mcu()->core == PIC18HighEndCore16bit) {
                        fprintf(fh, "  #define Write0_%s() (PORT%cbits.R%c%d = 0)\n", str, iop->port, iop->port, iop->bit);
                        fprintf(fh, "  #define Write1_%s() (PORT%cbits.R%c%d = 1)\n", str, iop->port, iop->port, iop->bit);
                        fprintf(fh, "  #define Write_%s(b) PORT%cbits.R%c%d = (b) ? 1 : 0\n", str, iop->port, iop->port, iop->bit);
                    } else {
                        fprintf(fh, "  #define Write0_%s() (R%c%d = 0)\n", str, iop->port, iop->bit);
                        fprintf(fh, "  #define Write1_%s() (R%c%d = 1)\n", str, iop->port, iop->bit);
                        fprintf(fh, "  #define Write_%s(b) R%c%d = (b) ? 1 : 0\n", str, iop->port, iop->bit);
                    }
                } else if(compiler_variant == MNU_COMPILE_ARMGCC) {
                    fprintf(f, "\n");
                    // Do Nothing (no macro used)
                } else {
                    fprintf(fh, "  #define Write0_%s() (PORT%c &= ~(1<<PORT%c%d))\n", str, iop->port, iop->port, iop->bit);
                    fprintf(fh, "  #define Write1_%s() (PORT%c |= 1<<PORT%c%d)\n", str, iop->port, iop->port, iop->bit);
                    fprintf(fh, "  #define Write_%s(b) (b) ? Write1_%s() : Write0_%s()\n", str, str, str);
                }
                */
                fprintf(fh, "#else\n");
                fprintf(fh, "  PROTO(BOOLEAN_t Read_%s(void));\n", str);
                fprintf(fh, "  PROTO(void Write_%s(BOOLEAN_t b));\n", str);
                fprintf(fh, "#endif\n");
                fprintf(fh, "\n");

                fprintf(f, "#ifndef USE_MACRO\n");
                fprintf(f, "  // LDmicro provide these functions.\n");
                fprintf(f, "  BOOLEAN_t Read_%s(void) {\n", str);
                /*
                if(compiler_variant == MNU_COMPILE_CCS_PIC_C) {
                    fprintf(f, "    return input_state(PIN_%c%d);\n", iop->port, iop->bit);
                } else if(compiler_variant == MNU_COMPILE_HI_TECH_C) {
                    if(Prog.mcu()->core == PIC18HighEndCore16bit)
                        fprintf(f, "    return PORT%cbits.R%c%d;\n", iop->port, iop->port, iop->bit);
                    else
                        fprintf(f, "    return R%c%d;\n", iop->port, iop->bit);
                } else if(compiler_variant == MNU_COMPILE_ARMGCC) {
                    fprintf(f, "    return GPIO_ReadInputDataBit(GPIO%c, GPIO_Pin_%d);\n", iop->port, iop->bit);
                } else {
                    fprintf(f, "    return PORT%c & (1<<PORT%c%d);\n", iop->port, iop->port, iop->bit);
                }
                */
                fprintf(f, "      return PORT_BIT_STATE(%c, %d);\n", iop->port, iop->bit);
                fprintf(f, "  }\n");
                fprintf(f, "  void Write1_%s(void) {\n", str);
                fprintf(f, "      PORT_BIT_SET(%c, %d);\n", iop->port, iop->bit);
                fprintf(f, "  }\n");
                fprintf(f, "  void Write0_%s(void) {\n", str);
                fprintf(f, "      PORT_BIT_CLEAR(%c, %d);\n", iop->port, iop->bit);
                fprintf(f, "  }\n");
                fprintf(f, "  void Write_%s(BOOLEAN_t b) {\n", str);
                fprintf(f, "    if(b)\n");
                fprintf(f, "      Write1_%s();\n", str);
                fprintf(f, "    else\n");
                fprintf(f, "      Write0_%s();\n", str);
                fprintf(f, "  }\n");
                fprintf(f, "#endif\n");
                fprintf(f, "\n");
            } else {
                fprintf(fh, "// You provide these functions.\n");
                fprintf(fh, "PROTO(BOOLEAN_t Read_%s(void));\n", str);
                fprintf(fh, "PROTO(void Write_%s(BOOLEAN_t b));\n", str);
                fprintf(fh, "\n");

                fprintf(f, "/* You provide these functions. */\n");
                fprintf(f, "LDSTUB%d( BOOLEAN_t Read_%s(void) )\n", set1, str);
                fprintf(f, "LDSTUB( void Write_%s(BOOLEAN_t v) )\n", str);
                fprintf(f, "\n");
            }
        }
    } else if(type == IO_TYPE_PWM_OUTPUT) {
        if(compiler_variant == MNU_COMPILE_ARDUINO) {
            McuIoPinInfo *iop = PinInfoForName(&str[3]);
            const char *  s = ArduinoPinName(iop);
            if(strlen(s)) {
                fprintf(fh, "const int pin_%s = %s; // %s // Check that it's a PWM pin!\n", str, s, iop->pinName);
            } else {
                fprintf(fh, "const int pin_%s = -1; // Check that it's a PWM pin!\n", str);
                all_arduino_pins_are_mapped = false;
            }

            fprintf(fh, "#ifndef NO_PROTOTYPES\n");
            fprintf(fh, "  // LDmicro provide this macro or function.\n");
            fprintf(fh, "  #ifdef USE_MACRO\n");
            fprintf(fh, "    #define Write_%s(x) analogWrite(pin_%s, x)\n", str, str);
            fprintf(fh, "  #else\n");
            fprintf(fh, "    void Write_%s(SWORD x);\n", str);
            fprintf(fh, "  #endif\n");
            fprintf(fh, "#endif\n");
            fprintf(fh, "\n");

            fprintf(f, "#ifndef USE_MACRO\n");
            fprintf(f, "  // LDmicro provide this function.\n");
            fprintf(f, "  void Write_%s(SWORD x) {\n", str);
            fprintf(f, "    analogWrite(pin_%s, x);\n", str);
            fprintf(f, "  }\n");
            fprintf(f, "#endif\n");
            fprintf(f, "\n");
        } else if(compiler_variant == MNU_COMPILE_CCS_PIC_C) {
            fprintf(fh, "#ifndef NO_PROTOTYPES\n");
            fprintf(fh, "  // LDmicro provide this macro or function.\n");
            fprintf(fh, "  #ifdef USE_MACRO\n");
            fprintf(fh, "    #define Write_%s(x) pwm_set_duty_percent(x); pwm_on();\n", str);
            fprintf(fh, "  #else\n");
            fprintf(fh, "    void Write_%s(SWORD x);\n", str);
            fprintf(fh, "  #endif\n");
            fprintf(fh, "#endif\n");
            fprintf(fh, "\n");

            fprintf(f, "#ifndef USE_MACRO\n");
            fprintf(f, "  // LDmicro provide this function.\n");
            fprintf(f, "  void Write_%s(SWORD x) {\n", str);
            fprintf(f, "    pwm_set_duty_percent(x);\n");
            fprintf(f, "    pwm_on();\n");
            fprintf(f, "  }\n");
            fprintf(f, "#endif\n");
            fprintf(f, "\n");

        } else if(compiler_variant == MNU_COMPILE_HI_TECH_C) {
            McuIoPinInfo *iop = PinInfoForName(&str[3]); // infos on pin used by this PWM
            if(!iop)
                THROW_COMPILER_EXCEPTION(_("PWM pin error")); // error in pin layout

            McuPwmPinInfo *pop = PwmPinInfo(iop->pin);
            if(pop) {
                int pwm = 0;
                pwm = pop->timer;  // PWM# (0x01, 0x02) from timer number
                PWM_Used[pwm] = 1; // marked as used
                PWM_Resol[pwm] = 10;

                // define one function for each pwm used
                fprintf(f, "\n");
                fprintf(f, "/* You provide this function. */\n");
                fprintf(f, "void setPwmFrequency%X(SDWORD freq, SWORD percent, SWORD resol) {\n", pwm);
                fprintf(f, "  static SDWORD oldfreq= 0;\n");
                fprintf(f, "  if (freq != oldfreq)\n");
                fprintf(f, "    PWM_Init(0x%2.2X, %d, freq, resol);\n", pwm, Prog.mcuClock);
                fprintf(f, "  PWM_Set(0x%2.2X, percent, resol);\n", pwm);
                fprintf(f, "  oldfreq= freq;\n");
                fprintf(f, "}\n\n");
            } else {
                THROW_COMPILER_EXCEPTION(_("PWM pin error")); // error in PWM pin layout
            }
        } else if(compiler_variant == MNU_COMPILE_ARMGCC) {
            //  McuPwmPinInfo *ioq = PwmMaxInfoForName(&str[3], Prog.cycleTimer);       //// donne Pin # + timer # + resol

            McuIoPinInfo *iop = PinInfoForName(&str[3]); // infos on pin used by this PWM
            if(!iop)
                THROW_COMPILER_EXCEPTION(_("PWM pin error")); // error in pin layout

            const char *s = iop->pinName; // full pin name supposed to be in format (PWMn.c) n= PWM#, c=Channel#
            if(strlen(s)) {
                int         pwm = 0;
                const char *pos = strstr(s, "PWM");
                if(pos) {
                    pwm = 16 * atoi(pos + 3); // 16*PWM# from pin description

                    pos = strchr(pos, '.');
                    if(pos) {
                        pwm += atoi(pos + 1); // 16*PWM# + Channel#   (4.1 -> 0x41)
                        PWM_Used[pwm] = 1;    // marked as used
                    } else
                        THROW_COMPILER_EXCEPTION(_("PWM pin error")); // error in pin layout
                } else
                    THROW_COMPILER_EXCEPTION(_("PWM pin error")); // error in pin layout

                // define one variable and function for each pwm used
                fprintf(f, "\n");
                fprintf(f, "/* You provide this function. */\n");
                fprintf(f, "LibPWM_TIM_t TIM%d_Chan%d;\n", pwm / 16, pwm % 16);
                fprintf(f, "\n");
                fprintf(f, "void setPwmFrequency%X(SDWORD freq, SWORD percent) {\n", pwm);
                fprintf(f, "  static SDWORD oldfreq= 0;\n");
                fprintf(f, "  if (freq != oldfreq)\n");
                fprintf(f, "    LibPWM_InitTimer(TIM%d, &TIM%d_Chan%d, (double) freq);\n", pwm / 16, pwm / 16, pwm % 16);
                fprintf(f, "  LibPWM_SetChannelPercent(&TIM%d_Chan%d, LibPWM_Channel_%d, percent);\n", pwm / 16, pwm % 16, pwm % 16);
                fprintf(f, "  oldfreq= freq;\n");
                fprintf(f, "}\n\n");

            } else {
                THROW_COMPILER_EXCEPTION(_("PWM Internal error")); // error in PWM pin layout
            }
        } else if(compiler_variant == MNU_COMPILE_AVRGCC) {
            McuIoPinInfo *iop = PinInfoForName(&str[3]); // infos on pin used by this PWM
            if(!iop)
                THROW_COMPILER_EXCEPTION(_("PWM pin error")); // error in pin layout

            const char *s = iop->pinName; // full pin name supposed to be in format (OCn) n= PWM#
            if(strlen(s)) {
                int         pwm = 0;
                const char *pos = strstr(s, "OC");
                if(pos) {
                    pwm = strtol(pos + 2, nullptr, 16); // PWM# (0x00, 0x1A, 0x1B, 0x02...) from pin description
                    PWM_Used[pwm] = 1;                  // marked as used
                } else
                    THROW_COMPILER_EXCEPTION(_("PWM pin error")); // error in pin layout

                // define one function for each pwm used
                fprintf(f, "\n");
                fprintf(f, "/* You provide this function. */\n");
                fprintf(f, "void setPwmFrequency%X(SDWORD freq, SWORD percent, SWORD resol, SWORD maxcs) {\n", pwm);
                fprintf(f, "  static SDWORD oldfreq= 0;\n");
                fprintf(f, "  if (freq != oldfreq)\n");
                fprintf(f, "    PWM_Init(0x%2.2X, %d, freq, resol, maxcs);\n", pwm, Prog.mcuClock);
                fprintf(f, "  PWM_Set(0x%2.2X, percent, resol);\n", pwm);
                fprintf(f, "  oldfreq= freq;\n");
                fprintf(f, "}\n\n");
            } else {
                THROW_COMPILER_EXCEPTION(_("PWM Internal error")); // error in PWM pin layout
            }
            /////
        } else {
            fprintf(f, "/* You provide this function. */\n");
            fprintf(f, "SWORD Write_%s(void) {\n", str);
            fprintf(f, "  return 0;\n");
            fprintf(f, "}\n");
        }
    } else if(type == IO_TYPE_READ_ADC) {
        if(compiler_variant == MNU_COMPILE_ARDUINO) {
            McuIoPinInfo *iop = PinInfoForName(&str[3]);
            const char *  s = ArduinoPinName(iop);
            if(strlen(s)) {
                fprintf(fh, "const int pin_%s = %s; // %s // Check that it's a ADC pin!\n", str, s, iop->pinName);
            } else {
                fprintf(fh, "const int pin_%s = -1; // Check that it's a ADC pin!\n", str);
                all_arduino_pins_are_mapped = false;
            }

            fprintf(fh, "#ifndef NO_PROTOTYPES\n");
            fprintf(fh, "  // LDmicro provide this macro or function.\n");
            fprintf(fh, "  #ifdef USE_MACRO\n");
            fprintf(fh, "    #define Read_%s() analogRead(pin_%s)\n", str, str);
            fprintf(fh, "  #else\n");
            fprintf(fh, "    SWORD Read_%s(void);\n", str);
            fprintf(fh, "  #endif\n");
            fprintf(fh, "#endif\n");
            fprintf(fh, "\n");

            fprintf(f, "#ifndef USE_MACRO\n");
            fprintf(f, "  // LDmicro provide this function.\n");
            fprintf(f, "  SWORD Read_%s(void) {\n", str);
            fprintf(f, "    return analogRead(pin_%s);\n", str);
            fprintf(f, "  }\n");
            fprintf(f, "#endif\n");
            fprintf(f, "\n");
        } else if(compiler_variant == MNU_COMPILE_CCS_PIC_C) {
            fprintf(fh, "#ifndef NO_PROTOTYPES\n");
            fprintf(fh, "  // LDmicro provide this function.\n");
            fprintf(fh, "  SWORD Read_%s(void);\n", str);
            fprintf(fh, "#endif\n");
            fprintf(fh, "\n");

            fprintf(f, "// LDmicro provide this function.\n");
            fprintf(f, "SWORD Read_%s(void) {\n", str);
            fprintf(f, "  set_adc_channel(%d);\n", MuxForAdcVariable(&str[3]));
            fprintf(f, "  return read_adc();\n");
            fprintf(f, "}\n");
            fprintf(f, "\n");
        }

        else if(compiler_variant == MNU_COMPILE_HI_TECH_C) {
            MuxForAdcVariable(&str[3]); // to check pin assignment

            McuIoPinInfo *iop = PinInfoForName(&str[3]); // infos on pin used by this ADC
            if(!iop)
                THROW_COMPILER_EXCEPTION(_("ADC pin error")); // error in pin layout

            int s = iop->bit; // RA0 = AN0, ... ,RA3 = AN3
            /// Bug corrected by JG3
            if(s == 5)
                s = 4; // RA5 = AN4
            if(iop->port == 'E')
                s += 5; // RE0 = AN5, ... ,RE2 = AN7
                        ///
            if((s >= 0) && (s <= 7)) {
                ADC_Used[s] = 1;
                ADC_Chan[s] = 0;

                // One function for each ADC in use
                fprintf(f, "\n");
                fprintf(f, "/* You provide this function. */\n");
                fprintf(f, "SWORD Read_%s(int refs) {\n", str);
                fprintf(f, "  SWORD v= 0;\n");
                fprintf(f, "  v = ADC_Read(%d, refs);\n", s); // acquisition ADC
                fprintf(f, "  return v;\n");
                fprintf(f, "}\n");
            } else {
                THROW_COMPILER_EXCEPTION(_("ADC pin error")); // error in ADC pin layout
            }
        } else if(compiler_variant == MNU_COMPILE_ARMGCC) {
            MuxForAdcVariable(&str[3]); // to check pin assignment

            McuIoPinInfo *iop = PinInfoForName(&str[3]); // infos on pin used by this ADC
            if(!iop)
                THROW_COMPILER_EXCEPTION(_("ADC pin error")); // error in pin layout

            const char *s = iop->pinName; // full pin name supposed to be in format (ADCn.c) n= ADC#, c= Channel#
            if(strlen(s)) {
                int         adc = 0;
                int         chan = 0;
                const char *pos = strstr(s, "ADC");
                if(pos) {
                    adc = atoi(pos + 3); // ADC # from pin description
                    chan = MuxForAdcVariable(&str[3]); // channel # from McuAdcPinInfo (could also be retrieved from s)
                    ADC_Used[16 * adc + chan] = 1;     // 16*ADC# + Channel#
                    ADC_Chan[16 * adc + chan] = chan;
                } else {
                    THROW_COMPILER_EXCEPTION(_("ADC pin error")); // error in pin layout
                }

                // One function for each ADC in use
                fprintf(f, "\n");
                fprintf(f, "/* You provide this function. */\n");
                fprintf(f, "SWORD Read_%s(void) {\n", str);
                fprintf(f, "  SWORD v= 0;\n");
                fprintf(f, "  v = LibADC_Read(ADC%d, ADC_Channel_%d);\n", adc, chan); // acquisition ADC n Channel c
                fprintf(f, "  return v;\n");
                fprintf(f, "}\n");
            } else {
                THROW_COMPILER_EXCEPTION(_("ADC pin error")); // error in ADC pin layout
            }
        } else if(compiler_variant == MNU_COMPILE_AVRGCC) {
            MuxForAdcVariable(&str[3]); // to check pin assignment

            McuIoPinInfo *iop = PinInfoForName(&str[3]); // infos on pin used by this ADC
            if(!iop)
                THROW_COMPILER_EXCEPTION(_("ADC pin error")); // error in pin layout

            const char *s = iop->pinName; // full pin name supposed to be in format (ADCn) n= ADC#
            if(strlen(s)) {
                int         adc = 0;
                const char *pos = strstr(s, "ADC");
                if(pos) {
                    adc = atoi(pos + 3); // ADC # from pin description
                    ADC_Used[adc] = 1;
                    ADC_Chan[adc] = 0; // no channel for AVRs
                } else {
                    THROW_COMPILER_EXCEPTION(_("ADC pin error")); // error in pin layout
                }

                // One function for each ADC in use
                fprintf(f, "\n");
                fprintf(f, "/* You provide this function. */\n");
                fprintf(f, "SWORD Read_%s(void) {\n", str);
                fprintf(f, "  SWORD v= 0;\n");
                fprintf(f, "  v = ADC_Read(%d);\n", adc); // acquisition ADC
                fprintf(f, "  return v;\n");
                fprintf(f, "}\n");
            } else {
                THROW_COMPILER_EXCEPTION(_("ADC pin error")); // error in ADC pin layout
            }
            /////
        } else {
            fprintf(f, "/* You provide this function. */\n");
            fprintf(f, "SWORD Read_%s(void) {\n", str);
            fprintf(f, "  return 0;\n");
            fprintf(f, "}\n");
        }
    } else {
        fprintf(f, "_STATIC_ BOOLEAN_t %s = 0;\n", str);
        fprintf(fh, "#ifdef EXTERN_EVERYTHING\n  extern BOOLEAN_t %s;\n#endif\n", str);
        fprintf(fh, "#define Read_%s() %s\n", str, str);
        fprintf(fh, "#define Write_%s(x) (%s = x)\n", str, str);
        fprintf(fh, "#define Write0_%s() (%s = 0)\n", str, str);
        fprintf(fh, "#define Write1_%s() (%s = 1)\n", str, str);
        fprintf(fh, "\n");
    }

    if(compiler_variant == MNU_COMPILE_ARMGCC) {
        if(!TIMER_Used) { // to write this declarations once only
            fprintf(f,
                    "\n"
                    "// PLC Cycle timing variable.\n"
                    "volatile unsigned char Next_Plc_Cycle = 0;\n"
                    "\n");

            TIMER_Used = 1;
        }
    }
}

static void DeclareBit(FILE *f, FILE *fh, FILE *flh, const char *str)
{
    DeclareBit(f, fh, flh, str, 0);
}

//-----------------------------------------------------------------------------
// Generate declarations for all the 16-bit/single bit variables in the ladder
// program.
//-----------------------------------------------------------------------------
static void GenerateDeclarations(FILE *f, FILE *fh, FILE *flh)
{
    all_arduino_pins_are_mapped = true;

    for(uint32_t i = 0; i < IntCode.size(); i++) {
        ADDR_T      addr, addr2;
        int         bit, bit2;
        const char *bitVar1 = nullptr, *bitVar2 = nullptr;
        const char *intVar1 = nullptr, *intVar2 = nullptr, *intVar3 = nullptr, *intVar4 = nullptr;
        //const char *adcVar1 = nullptr;
        const char *strVar1 = nullptr;
        int         sov1 = 0, sov2 = 0, sov3 = 0, sov4 = 0;

        int bitVar1set1 = 0;

        addr = 0;
        bit = 0;
        addr2 = 0;
        bit2 = 0;

        IntOp *a = &IntCode[i];

        switch(IntCode[i].op) {
            case INT_SET_BIT:
            case INT_CLEAR_BIT:
                isPinAssigned(a->name1);
                bitVar1 = IntCode[i].name1.c_str();
                break;

            case INT_IF_BIT_EQU_BIT:
            case INT_IF_BIT_NEQ_BIT:
            case INT_COPY_NOT_BIT_TO_BIT:
            case INT_COPY_BIT_TO_BIT:
                isPinAssigned(a->name1);
                isPinAssigned(a->name2);
                bitVar1 = IntCode[i].name1.c_str();
                bitVar2 = IntCode[i].name2.c_str();
                break;

            case INT_SET_VARIABLE_TO_LITERAL:
            case INT_SET_SEED_RANDOM:
            case INT_SET_VARIABLE_RANDOM:
                intVar1 = IntCode[i].name1.c_str();
                break;

            case INT_SET_BIN2BCD:
            case INT_SET_BCD2BIN:
            case INT_SET_OPPOSITE:
            case INT_COPY_VAR_BIT_TO_VAR_BIT:
            case INT_SET_VARIABLE_NOT:
            case INT_SET_SWAP:
            case INT_SET_VARIABLE_NEG:
                intVar1 = IntCode[i].name1.c_str();
                intVar2 = IntCode[i].name2.c_str();
                break;

            case INT_SET_VARIABLE_INDEXED:
                //intVar1 = IntCode[i].name1.c_str();
                //intVar2 = IntCode[i].name2.c_str();
                break;

            case INT_SET_VARIABLE_TO_VARIABLE:
                intVar1 = IntCode[i].name1.c_str();
                intVar2 = IntCode[i].name2.c_str();
                intVar3 = IntCode[i].name3.c_str();
                break;

            case INT_SET_VARIABLE_ROL:
            case INT_SET_VARIABLE_ROR:
            case INT_SET_VARIABLE_SHL:
            case INT_SET_VARIABLE_SHR:
            case INT_SET_VARIABLE_AND:
            case INT_SET_VARIABLE_OR:
            case INT_SET_VARIABLE_XOR:
            case INT_SET_VARIABLE_SR0:
            case INT_SET_VARIABLE_MOD:
            case INT_SET_VARIABLE_DIVIDE:
            case INT_SET_VARIABLE_MULTIPLY:
            case INT_SET_VARIABLE_SUBTRACT:
            case INT_SET_VARIABLE_ADD:
                intVar1 = IntCode[i].name1.c_str();
                intVar2 = IntCode[i].name2.c_str();
                intVar3 = IntCode[i].name3.c_str();
                break;

            case INT_DECREMENT_VARIABLE:
            case INT_INCREMENT_VARIABLE:
                intVar1 = IntCode[i].name1.c_str();
                break;

            case INT_PWM_OFF:
                bitVar1 = IntCode[i].name1.c_str();
                break;

            case INT_SET_PWM:
                intVar1 = IntCode[i].name1.c_str();
                intVar2 = IntCode[i].name2.c_str();
                bitVar1 = IntCode[i].name3.c_str();
                break;

            case INT_READ_ADC:
                intVar1 = IntCode[i].name1.c_str();
                bitVar1 = IntCode[i].name1.c_str();
                break;

            case INT_SPI_WRITE:
                intVar1 = IntCode[i].name1.c_str(); // create var name1= spi name
                //  intVar2 = IntCode[i].name2.c_str();     // don't create var name2= spi send var
                break;

            case INT_SPI:
                intVar1 = IntCode[i].name1.c_str();     // create var name1= spi name
                if(IntCode[i].name2.length())
                    if(!IsNumber(IntCode[i].name2))
                        intVar2 = IntCode[i].name2.c_str(); // create var name2= spi send var if not a number
                if(IntCode[i].name3.length())
                    intVar3 = IntCode[i].name3.c_str(); // create var name3= spi recv var
                break;

            case INT_I2C_READ:
                intVar1 = IntCode[i].name1.c_str(); // create var name1= i2c name
                intVar2 = IntCode[i].name2.c_str(); // create var name2= i2c recv var
                intVar3 = IntCode[i].name3.c_str(); // create var name3= i2c address if not a number
                intVar4 = IntCode[i].name4.c_str(); // create var name4= i2c register if not a number
                break;

            case INT_I2C_WRITE:
                intVar1 = IntCode[i].name1.c_str(); // create var name1= i2c name
                intVar2 = IntCode[i].name2.c_str(); // create var name2= i2c send var if not a number
                intVar3 = IntCode[i].name3.c_str(); // create var name3= i2c address if not a number
                intVar4 = IntCode[i].name4.c_str(); // create var name4= i2c register if not a number
                break;

            case INT_UART_RECV1:
            case INT_UART_SEND1:
                if(GetVariableType(IntCode[i].name1) == IO_TYPE_STRING)
                    strVar1 = IntCode[i].name1.c_str();
                else
                    intVar1 = IntCode[i].name1.c_str();
                if(IntCode[i].name2.length())
                    intVar2 = IntCode[i].name2.c_str();
                break;

            case INT_UART_WR:
                if(!IsString(IntCode[i].name1)) {
                    strVar1 = IntCode[i].name1.c_str();
                }
                break;

            case INT_UART_RECV_AVAIL:
            case INT_UART_SEND_READY:
            case INT_UART_SEND_BUSY:
                bitVar1 = IntCode[i].name1.c_str();
                break;

            case INT_IF_BIT_SET:
            case INT_IF_BIT_CLEAR:
                isPinAssigned(a->name1);
                bitVar1 = IntCode[i].name1.c_str();
                bitVar1set1 = IntCode[i].literal1;
                break;

            case INT_VARIABLE_SET_BIT:
            case INT_VARIABLE_CLEAR_BIT:
            case INT_IF_BIT_SET_IN_VAR:
            case INT_IF_BIT_CLEAR_IN_VAR:
                intVar1 = IntCode[i].name1.c_str();
                intVar2 = IntCode[i].name2.c_str();
                break;
#ifdef NEW_CMP
            case INT_IF_EQU:
            case INT_IF_NEQ:
            case INT_IF_LES:
            case INT_IF_GRT:
            case INT_IF_LEQ:
            case INT_IF_GEQ:
                intVar1 = IntCode[i].name1.c_str();
                intVar2 = IntCode[i].name2.c_str();
                break;
#else
            case INT_IF_VARIABLE_LES_LITERAL:
                intVar1 = IntCode[i].name1;
                break;

            case INT_IF_VARIABLE_EQUALS_VARIABLE:
            case INT_IF_VARIABLE_GRT_VARIABLE:
                intVar1 = IntCode[i].name1;
                intVar2 = IntCode[i].name2;
                break;
#endif

            case INT_END_IF:
            case INT_ELSE:
            case INT_COMMENT:
            case INT_DELAY:
            case INT_LOCK:
            case INT_CLRWDT:
            case INT_SIMULATE_NODE_STATE:
                break;

            case INT_EEPROM_BUSY:
            //case INT_EEPROM_BUSY_CHECK:
                bitVar1 = IntCode[i].name1.c_str();
                break;

            case INT_EEPROM_READ:
                intVar1 = IntCode[i].name1.c_str();
                break;

            case INT_EEPROM_WRITE_BYTE:
                intVar1 = IntCode[i].name1.c_str();
                intVar2 = IntCode[i].name2.c_str();
                break;

            case INT_STRING:
                strVar1 = IntCode[i].name1.c_str();
                if(IntCode[i].name3.length())
                    intVar3 = IntCode[i].name3.c_str();
                break;

            case INT_SLEEP:
            case INT_AllocKnownAddr:
            case INT_AllocFwdAddr:
            case INT_FwdAddrIsNow:
            case INT_GOTO:
            case INT_GOSUB:
            case INT_RETURN:
                break;

#ifdef TABLE_IN_FLASH
            case INT_STRING_INIT:
            case INT_FLASH_INIT:
                break;

            case INT_RAM_READ:
            case INT_FLASH_READ:
                intVar1 = IntCode[i].name1.c_str();
                intVar3 = IntCode[i].name3.c_str();
                break;
#endif

            default:
                THROW_COMPILER_EXCEPTION_FMT("INT_%d", a->op);
        }
        bitVar1 = MapSym(bitVar1, ASBIT);
        bitVar2 = MapSym(bitVar2, ASBIT);

        if(intVar1)
            sov1 = SizeOfVar(intVar1);
        if(intVar2)
            sov2 = SizeOfVar(intVar2);
        if(intVar3)
            sov3 = SizeOfVar(intVar3);
        if(intVar4)
            sov4 = SizeOfVar(intVar4);

        intVar1 = MapSym(intVar1, ASINT);
        intVar2 = MapSym(intVar2, ASINT);
        intVar3 = MapSym(intVar3, ASINT);
        intVar4 = MapSym(intVar4, ASINT);

        if(bitVar1 && !SeenVariable(bitVar1))
            DeclareBit(f, fh, flh, bitVar1, bitVar1set1);
        if(bitVar2 && !SeenVariable(bitVar2))
            DeclareBit(f, fh, flh, bitVar2);

        if(intVar1 && !SeenVariable(intVar1))
            DeclareInt(f, fh, intVar1, sov1);
        if(intVar2 && !SeenVariable(intVar2))
            DeclareInt(f, fh, intVar2, sov2);
        if(intVar3 && !SeenVariable(intVar3))
            DeclareInt(f, fh, intVar3, sov3);
        if(intVar4 && !SeenVariable(intVar4))
            DeclareInt(f, fh, intVar3, sov4);

        if(strVar1)
            sov1 = SizeOfVar(strVar1);
        strVar1 = MapSym(strVar1, ASSTR);
        if(strVar1 && !SeenVariable(strVar1))
            DeclareStr(f, fh, strVar1, sov1);
    }
    if(Prog.cycleDuty) {
        const char *bitVar1 = nullptr;
        bitVar1 = MapSym("YPlcCycleDuty", ASBIT);
        if(bitVar1 && !SeenVariable(bitVar1))
            DeclareBit(f, fh, flh, bitVar1);
    }
}

//-----------------------------------------------------------------------------
// printf-like comment function
//-----------------------------------------------------------------------------
static void _Comment(FILE *f, const char *str, ...)
{
    va_list v;
    char    buf[MAX_NAME_LEN];
    va_start(v, str);
    vsnprintf(buf, MAX_NAME_LEN, str, v);
    fprintf(f, "//%s\n", buf);
    va_end(v);
}
#define Comment(...) _Comment(f, __VA_ARGS__)

//-----------------------------------------------------------------------------
static int  indent = 1;
static void doIndent(FILE *f, int i)
{
    if((IntCode[i].op != INT_SIMULATE_NODE_STATE) //
    && (IntCode[i].op != INT_FwdAddrIsNow)     //
//  && (IntCode[i].op != INT_AllocKnownAddr)   //
//  && (IntCode[i].op != INT_AllocFwdAddr)     //
    ) {
        for(int j = 0; j < indent; j++)
            fprintf(f, "    ");
    }
}

//-----------------------------------------------------------------------------
// Actually generate the C source for the program.
//-----------------------------------------------------------------------------
static void GenerateAnsiC(FILE *f, int begin, int end)
{
    int lock_label = 0;
    indent = 1;
    for(int i = begin; i <= end; i++) {
        char        opc;
        const char *ops;

        if(IntCode[i].op == INT_END_IF)
            indent--;
        if(IntCode[i].op == INT_ELSE)
            indent--;

        doIndent(f, i);

        switch(IntCode[i].op) {
            case INT_SET_BIT:
                /*
                if((IntCode[i].name1[0] != '$') && (IntCode[i].name1[0] != 'R'))
                    fprintf(f, "Write1_%s();\n", MapSym(IntCode[i].name1, ASBIT));
                else
                    fprintf(f, "Write_%s(1);\n", MapSym(IntCode[i].name1, ASBIT));
                */
                fprintf(f, "Write1_%s();\n", MapSym(IntCode[i].name1, ASBIT));
                break;

            case INT_CLEAR_BIT:
                /*
                if((IntCode[i].name1[0] != '$') && (IntCode[i].name1[0] != 'R'))
                    fprintf(f, "Write0_%s();\n", MapSym(IntCode[i].name1, ASBIT));
                else
                    fprintf(f, "Write_%s(0);\n", MapSym(IntCode[i].name1, ASBIT));
                */
                fprintf(f, "Write0_%s();\n", MapSym(IntCode[i].name1, ASBIT));
                break;

            case INT_IF_BIT_EQU_BIT:
                fprintf(f, "if(Read_%s() == Read_%s()) {\n", MapSym(IntCode[i].name1, ASBIT), MapSym(IntCode[i].name2, ASBIT));
                indent++;
                break;

            case INT_IF_BIT_NEQ_BIT:
                fprintf(f, "if(Read_%s() != Read_%s()) {\n", MapSym(IntCode[i].name1, ASBIT), MapSym(IntCode[i].name2, ASBIT));
                indent++;
                break;

            case INT_COPY_NOT_BIT_TO_BIT:
                fprintf(f, "Write_%s(!Read_%s());\n", MapSym(IntCode[i].name1, ASBIT), MapSym(IntCode[i].name2, ASBIT));
                break;

            case INT_COPY_BIT_TO_BIT:
                fprintf(f, "Write_%s(Read_%s());\n", MapSym(IntCode[i].name1, ASBIT), MapSym(IntCode[i].name2, ASBIT));
                break;

            case INT_SET_VARIABLE_TO_LITERAL:
                if(IntCode[i].name1[0] == '#') { // TODO: in many other places :(
                    if(IsNumber(&IntCode[i].name1[1])) {
                        fprintf(f, "//pokeb(%s, %d); // Variants 1 and 2\n", IntCode[i].name1.c_str() + 1, IntCode[i].literal1);
                    } else {
                        ADDR_T addr;
                        char   name[MAX_NAME_LEN];
                        sprintf_s(name, "#%s", &IntCode[i].name1[1]);
                        MemForVariable(name, &addr);
                        fprintf(f, "//pokeb(0x%X, %d); // %s // Variants 1 and 2\n", addr, IntCode[i].literal1, IntCode[i].name1.c_str() + 1);
                    }
                    doIndent(f, i);
                }
                fprintf(f, "%s = %d;\n", MapSym(IntCode[i].name1, ASINT), IntCode[i].literal1);
                break;

            case INT_COPY_VAR_BIT_TO_VAR_BIT:
                fprintf(f, "if (%s & (1<<%d)) {\n", MapSym(IntCode[i].name2, ASINT), IntCode[i].literal2);
                indent++;
                doIndent(f, i);
                fprintf(f, "%s |=  (1<<%d); } else {\n", MapSym(IntCode[i].name1, ASINT), IntCode[i].literal1);
                doIndent(f, i);
                fprintf(f, "%s &= ~(1<<%d); }\n", MapSym(IntCode[i].name1, ASINT), IntCode[i].literal1);
                indent--;
                break;

            case INT_SET_VARIABLE_INDEXED:
                fprintf(f, "%s = %s[%s];\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASSTR), MapSym(IntCode[i].name3, ASINT));
                break;

            case INT_SET_VARIABLE_TO_VARIABLE: {
                int sov = SizeOfVar(IntCode[i].name1);
                if(GetVariableType(IntCode[i].name2) == IO_TYPE_STRING)
                    fprintf(f, "%s = %s[%s];\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASSTR), MapSym(IntCode[i].name3, ASINT));
                else if(IntCode[i].name3.length() && IntCode[i].literal1)
                    if(sov == 1)
                        fprintf(f, "%s = BYTE_AT(%s, %s+%d);\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASSTR), MapSym(IntCode[i].name3, ASINT), IntCode[i].literal1);
                    else if(sov == 2)
                        fprintf(f, "%s = WORD_AT(%s, %s+%d);\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASSTR), MapSym(IntCode[i].name3, ASINT), IntCode[i].literal1);
                    else
                        oops();
                else if(IntCode[i].name3.length())
                    if(sov == 1)
                        fprintf(f, "%s = BYTE_AT(%s, %s);\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASSTR), MapSym(IntCode[i].name3, ASINT));
                    else if(sov == 2)
                        fprintf(f, "%s = WORD_AT(%s, %s);\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASSTR), MapSym(IntCode[i].name3, ASINT));
                    else
                        oops();
                else if(IntCode[i].literal1)
                    if(sov == 1)
                        fprintf(f, "%s = BYTE_AT(%s, %d);\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASSTR), IntCode[i].literal1);
                    else if(sov == 2)
                        fprintf(f, "%s = WORD_AT(%s, %d);\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASSTR), IntCode[i].literal1);
                    else
                        oops();
                else
                    fprintf(f, "%s = %s;\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT));
                break;
            }

            case INT_SET_VARIABLE_NEG:
                fprintf(f, "%s = - %s;\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT));
                break;

            case INT_SET_BIN2BCD:
                fprintf(f, "%s = bin2bcd(%s);\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT));
                break;

            case INT_SET_BCD2BIN:
                fprintf(f, "%s = bcd2bin(%s);\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT));
                break;

            case INT_SET_SWAP:
                fprintf(f, "%s = swap(%s);\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT));
                break;

            case INT_SET_OPPOSITE:
                fprintf(f, "%s = opposite(%s);\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT));
                break;

            case INT_SET_VARIABLE_NOT:
                fprintf(f, "%s = ~ %s;\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT));
                break;

            case INT_SET_VARIABLE_RANDOM:
                fprintf(f, "%s = rand();\n", MapSym(IntCode[i].name1, ASINT));
                break;

            case INT_SET_SEED_RANDOM:
                fprintf(f, "srand(%s);\n", MapSym(IntCode[i].name1, ASINT));
                break;

            case INT_SET_VARIABLE_SHL:
                opc = '<';
                goto arith_shift;
            case INT_SET_VARIABLE_SHR:
                opc = '>';
                goto arith_shift;
            case INT_SET_VARIABLE_SR0:
                opc = '>';
                goto arith_shift;
            arith_shift:
                fprintf(f, "%s = %s %c%c %s;\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT), opc, opc, MapSym(IntCode[i].name3, ASINT));
                break;

            case INT_SET_VARIABLE_ROL:
                ops = "rol";
                goto cicle_shift;
            case INT_SET_VARIABLE_ROR:
                ops = "ror";
                goto cicle_shift;
            cicle_shift:
                fprintf(f, "%s = %s%d(%s, %s);\n", MapSym(IntCode[i].name1, ASINT), ops, SizeOfVar(IntCode[i].name2), MapSym(IntCode[i].name2, ASINT), MapSym(IntCode[i].name3, ASINT));
                break;

            case INT_SET_VARIABLE_AND:
                opc = '&';
                goto arith;
            case INT_SET_VARIABLE_OR:
                opc = '|';
                goto arith;
            case INT_SET_VARIABLE_XOR:
                opc = '^';
                goto arith;
            case INT_SET_VARIABLE_ADD:
                opc = '+';
                goto arith;
            case INT_SET_VARIABLE_SUBTRACT:
                opc = '-';
                goto arith;
            case INT_SET_VARIABLE_MULTIPLY:
                opc = '*';
                goto arith;
            case INT_SET_VARIABLE_DIVIDE:
                opc = '/';
                goto arith;
            case INT_SET_VARIABLE_MOD:
                opc = '%';
                goto arith;
            arith:
                if(IntCode[i].name1 == IntCode[i].name2)
                    fprintf(f, "%s %c= %s;\n", MapSym(IntCode[i].name1, ASINT), opc, MapSym(IntCode[i].name3, ASINT));
                else
                    fprintf(f, "%s = %s %c %s;\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT), opc, MapSym(IntCode[i].name3, ASINT));
                break;

            case INT_INCREMENT_VARIABLE:
                fprintf(f, "%s++;\n", MapSym(IntCode[i].name1, ASINT));
                break;

            case INT_DECREMENT_VARIABLE:
                fprintf(f, "%s--;\n", MapSym(IntCode[i].name1, ASINT));
                break;

            case INT_IF_BIT_SET:
                fprintf(f, "if(Read_%s()) {\n", MapSym(IntCode[i].name1, ASBIT));
                indent++;
                break;

            case INT_IF_BIT_CLEAR:
                fprintf(f, "if(!Read_%s()) {\n", MapSym(IntCode[i].name1, ASBIT));
                indent++;
                break;

            case INT_IF_BIT_SET_IN_VAR:
                fprintf(f, "if(%s & (1<<%s)) {\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT));
                indent++;
                break;

            case INT_IF_BIT_CLEAR_IN_VAR:
                fprintf(f, "if((%s & (1<<%s)) == 0) {\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT));
                indent++;
                break;

            case INT_VARIABLE_SET_BIT:
                fprintf(f, "%s |= (1 << %s);\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT));
                break;

            case INT_VARIABLE_CLEAR_BIT:
                fprintf(f, "%s &= ~(1 << %s);\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT));
                break;

#ifdef NEW_CMP
            case INT_IF_EQU:
                fprintf(f, "if(%s == %s) {\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT));
                indent++;
                break;

            case INT_IF_NEQ:
                fprintf(f, "if(%s != %s) {\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT));
                indent++;
                break;

            case INT_IF_LES:
                fprintf(f, "if(%s < %s) {\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT));
                indent++;
                break;

            case INT_IF_GRT:
                fprintf(f, "if(%s > %s) {\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT));
                indent++;
                break;

            case INT_IF_LEQ:
                fprintf(f, "if(%s <= %s) {\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT));
                indent++;
                break;

            case INT_IF_GEQ:
                fprintf(f, "if(%s >= %s) {\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT));
                indent++;
                break;
#else
            case INT_IF_VARIABLE_LES_LITERAL:
                fprintf(f, "if(%s < %d) {\n", MapSym(IntCode[i].name1, ASINT), IntCode[i].literal1);
                indent++;
                break;

            case INT_IF_VARIABLE_EQUALS_VARIABLE:
                fprintf(f, "if(%s == %s) {\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT));
                indent++;
                break;

            case INT_IF_VARIABLE_GRT_VARIABLE:
                fprintf(f, "if(%s > %s) {\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT));
                indent++;
                break;
#endif

            case INT_END_IF:
                fprintf(f, "}\n");
                break;

            case INT_ELSE:
                fprintf(f, "} else {\n");
                indent++;
                break;

            case INT_SIMULATE_NODE_STATE:
                // simulation-only
                // fprintf(f, "\n");
                break;

            case INT_COMMENT:
                if(IntCode[i].name1.length()) {
                    fprintf(f, "// %s\n", IntCode[i].name1.c_str());
                } else {
                    fprintf(f, "\n");
                }
                break;

            case INT_CLRWDT:
                fprintf(f, "// CLRWDT\n");
                break;

            case INT_LOCK:
                lock_label++;
                fprintf(f, "lock_label%d: goto lock_label%d;\n", lock_label, lock_label);
                break;

            case INT_DELAY:
                /*
                fprintf(f, "#ifdef CCS_PIC_C\n");
                doIndent(f, i);
                fprintf(f, "  delay_us(%s);\n", MapSym(IntCode[i].name1, ASINT));
                doIndent(f, i);
                fprintf(f, "#elif defined(HI_TECH_C)\n");
                doIndent(f, i);
                fprintf(f, "  __delay_us(%s);\n", MapSym(IntCode[i].name1, ASINT));
                doIndent(f, i);
                fprintf(f, "#elif defined(__CODEVISIONAVR__)\n");
                doIndent(f, i);
                fprintf(f, "  delay_us(%s);\n", MapSym(IntCode[i].name1, ASINT));
                doIndent(f, i);
                fprintf(f, "#elif defined(__GNUC__)\n");
                doIndent(f, i);
                fprintf(f, "  _delay_us(%s);\n", MapSym(IntCode[i].name1, ASINT));
                doIndent(f, i);
                fprintf(f, "#else\n");
                doIndent(f, i);
                fprintf(f, "  delayMicroseconds(%s);\n", MapSym(IntCode[i].name1, ASINT));
                doIndent(f, i);
                fprintf(f, "#endif\n");

                if(compiler_variant == MNU_COMPILE_ARDUINO)
                    fprintf(f, "delayMicroseconds(%s);\n", MapSym(IntCode[i].name1, ASINT));
                else
                */
                fprintf(f, "delay_us(%s);\n", MapSym(IntCode[i].name1, ASINT));
                break;

            case INT_SPI:

                if(mcu_ISA == ISA_ARM) {
                    int u = atoi(MapSym(IntCode[i].name1, ASINT) + 6); // name1= "Ui_SPIn"
                    if((SPI_Used != -1) && (SPI_Used != u))
                        THROW_COMPILER_EXCEPTION(_("Only one SPI can be used"));
                    else
                        SPI_Used = u;
                    // send and / or receive 1 byte
                    fprintf(f, "%s = SPI_SendRecv(%s, %s);\n", MapSym(IntCode[i].name3, ASINT), IntCode[i].name1.c_str(), MapSym(IntCode[i].name2, ASINT));
                } else if((mcu_ISA == ISA_AVR) || (mcu_ISA == ISA_PIC16) || (mcu_ISA == ISA_PIC18)) {
                    // send and / or receive 1 byte
                    fprintf(f, "%s = SPI_SendRecv(%s);\n", MapSym(IntCode[i].name3, ASINT), MapSym(IntCode[i].name2, ASINT));
                } else
                    /////
                    fprintf(f, "SPI(%s, %s);\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT));
                break;


            case INT_SPI_WRITE:
                if(mcu_ISA == ISA_ARM) {
                    int u = atoi(MapSym(IntCode[i].name1, ASINT) + 6); // name1= "Ui_SPIn"
                    if((SPI_Used != -1) && (SPI_Used != u))
                        THROW_COMPILER_EXCEPTION(_("Only one SPI can be used"));
                    else
                        SPI_Used = u;
                    // send a literal string without reception care
                    fprintf(f, "SPI_Write(\"%s\");\n", MapSym(IntCode[i].name2, ASINT) + 3); // remove "Ui_" prefix

                    double maxw = ((double)Prog.cycleTime) / ((double)1000000);
                    maxw = (maxw * ((double)Prog.spiRate)) / ((double)20);
                    if(strlen(MapSym(IntCode[i].name2, ASINT) + 3) > maxw)
                        THROW_COMPILER_EXCEPTION(_(" SPI: Sending too many chars in one cycle may cause timing issues"));
                } else if((mcu_ISA == ISA_AVR) || (mcu_ISA == ISA_PIC16) || (mcu_ISA == ISA_PIC18)) {
                    // int u = atoi(MapSym(IntCode[i].name1, ASINT)+6);         // name1= "Ui_SPI"
                    // send a literal string without reception care
                    fprintf(f, "SPI_Write((char *) \"%s\");\n", MapSym(IntCode[i].name2, ASINT) + 3); // remove "Ui_" prefix

                    double maxw = ((double)Prog.cycleTime) / ((double)1000000);
                    maxw = (maxw * ((double)Prog.spiRate)) / ((double)20);
                    if(strlen(MapSym(IntCode[i].name2, ASINT) + 3) > maxw)
                        THROW_COMPILER_EXCEPTION(_(" SPI: Sending too many chars in one cycle may cause timing issues"));
                } else
                    fprintf(f, "SPI_WRITE(%s, %s);\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT));
                break;

            case INT_I2C_READ:
                if(mcu_ISA == ISA_ARM) {
                    int u = atoi(MapSym(IntCode[i].name1, ASINT) + 6); // name1= "Ui_I2Cn"
                    if((I2C_Used != -1) && (I2C_Used != u))
                        THROW_COMPILER_EXCEPTION(_("Only one I2C can be used"));
                    else
                        I2C_Used = u;

                    // read one byte from I2Caddr:reg in recv variable
                    fprintf(f, "%s= I2C_Recv(%s, %s);\n", MapSym(IntCode[i].name2, ASINT), IntCode[i].name3.c_str(), IntCode[i].name4.c_str());
                } else if((mcu_ISA == ISA_AVR) || (mcu_ISA == ISA_PIC16) || (mcu_ISA == ISA_PIC18)) {
                    // int u = atoi(MapSym(IntCode[i].name1, ASINT)+6);         // name1= "Ui_I2C"

                    // read one byte from I2Caddr:reg in recv variable
                    fprintf(f, "%s= I2C_Recv(%s, %s);\n", MapSym(IntCode[i].name2, ASINT), IntCode[i].name3.c_str(), IntCode[i].name4.c_str());
                } else
                    fprintf(f, "%s= I2C_RECV(%s, %s, %s);\n", MapSym(IntCode[i].name2, ASINT), IntCode[i].name1.c_str(), IntCode[i].name3.c_str(), IntCode[i].name4.c_str());
                break;

            case INT_I2C_WRITE:
                if(mcu_ISA == ISA_ARM) {
                    int u = atoi(MapSym(IntCode[i].name1, ASINT) + 6); // name1= "Ui_I2Cn"
                    if((I2C_Used != -1) && (I2C_Used != u))
                        THROW_COMPILER_EXCEPTION(_("Only one I2C can be used"));
                    else
                        I2C_Used = u;
                    // write one byte from send variable or value
                    fprintf(f, "I2C_Send(%s, %s, %s);\n", IntCode[i].name3.c_str(), IntCode[i].name4.c_str(), MapSym(IntCode[i].name2, ASINT));
                } else if((mcu_ISA == ISA_AVR) || (mcu_ISA == ISA_PIC16) || (mcu_ISA == ISA_PIC18)) {
                    // int u = atoi(MapSym(IntCode[i].name1, ASINT)+6);         // name1= "Ui_I2C"

                    // write one byte from send variable or value
                    fprintf(f, "I2C_Send(%s, %s, %s);\n", IntCode[i].name3.c_str(), IntCode[i].name4.c_str(), MapSym(IntCode[i].name2, ASINT));
                } else
                    fprintf(f, "%s = I2C_SEND(%s, %s, %s);\n", IntCode[i].name1.c_str(), IntCode[i].name3.c_str(), IntCode[i].name4.c_str(), MapSym(IntCode[i].name2, ASINT));
                break;
                /////

            case INT_UART_WR:
                if(IsString(IntCode[i].name1)) {
                    fprintf(f, "UART_Write(%s);\n", IntCode[i].name1.c_str());
                } else {
                    fprintf(f, "UART_Write(%s);\n", MapSym(IntCode[i].name1, ASSTR));
                }
                break;

            case INT_UART_RECV1:
                if(IntCode[i].name2.length())
                    fprintf(f, "BYTE_AT(%s, %s+%d) = UART_Receive();\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT), IntCode[i].literal1);
                else
                    fprintf(f, "BYTE_AT(%s, %d) = UART_Receive();\n", MapSym(IntCode[i].name1, ASINT), IntCode[i].literal1);
                break;

            case INT_UART_SEND1:
                if(IntCode[i].name2.length())
                    fprintf(f, "UART_Transmit(BYTE_AT(%s, %s+%d));\n", MapSym(IntCode[i].name1, GetVariableType(IntCode[i].name1) == IO_TYPE_STRING ? ASSTR : ASINT), MapSym(IntCode[i].name2, ASINT), IntCode[i].literal1);
                else if(IntCode[i].literal1)
                    fprintf(f, "UART_Transmit(BYTE_AT(%s, %d));\n", MapSym(IntCode[i].name1, GetVariableType(IntCode[i].name1) == IO_TYPE_STRING ? ASSTR : ASINT), IntCode[i].literal1);
                else
                    fprintf(f, "UART_Transmit(%s);\n", MapSym(IntCode[i].name1, GetVariableType(IntCode[i].name1) == IO_TYPE_STRING ? ASSTR : ASINT));
                break;

            case INT_UART_RECV_AVAIL:
                fprintf(f, "%s = UART_Receive_Avail();\n", MapSym(IntCode[i].name1, ASBIT));
                break;

            case INT_UART_SEND_READY:
                fprintf(f, "%s = UART_Transmit_Ready();\n", MapSym(IntCode[i].name1, ASBIT));
                break;

            case INT_UART_SEND_BUSY:
                fprintf(f, "%s = UART_Transmit_Busy();\n", MapSym(IntCode[i].name1, ASBIT));
                break;

            case INT_EEPROM_BUSY:
            //case INT_EEPROM_BUSY_CHECK:
                if(compiler_variant == MNU_COMPILE_ARDUINO) {
                    fprintf(f, "Write0_%s(); // dummy // 0 = EEPROM is ready\n", MapSym(IntCode[i].name1, ASBIT));
                } else {
                    fprintf(f, "%s = EEPROM_busy(); // 0 = EEPROM is ready\n", MapSym(IntCode[i].name1, ASBIT));
                }
                break;

            case INT_EEPROM_READ: {
                int sov = SizeOfVar(IntCode[i].name1);
                fprintf(f, "%s = EEPROM_read(%d);\n", MapSym(IntCode[i].name1, ASINT), IntCode[i].literal1);
                if(sov >= 2) {
                    doIndent(f, i);
                    fprintf(f, "%s |= (uint16_t)EEPROM_read(%d) << 8;\n", MapSym(IntCode[i].name1, ASINT), IntCode[i].literal1 + 1);
                }
                if(sov >= 3) {
                    doIndent(f, i);
                    fprintf(f, "%s |= (uint32_t)EEPROM_read(%d) << 16;\n", MapSym(IntCode[i].name1, ASINT), IntCode[i].literal1 + 2);
                }
                if(sov >= 4) {
                    doIndent(f, i);
                    fprintf(f, "%s |= (uint32_t)EEPROM_read(%d) << 24;\n", MapSym(IntCode[i].name1, ASINT), IntCode[i].literal1 + 3);
                }
                break;
            }
            /*
            case INT_EEPROM_WRITE: {
                int sov = SizeOfVar(IntCode[i].name1);
                fprintf(f, "EEPROM_write(%d, %s & 0xFF);\n", IntCode[i].literal1, MapSym(IntCode[i].name1, ASINT));
                if(sov >= 2) {
                    doIndent(f, i);
                    fprintf(f, "EEPROM_write(%d, (%s >> 8) & 0xFF);\n", IntCode[i].literal1 + 1, MapSym(IntCode[i].name1, ASINT));
                }
                if(sov >= 3) {
                    doIndent(f, i);
                    fprintf(f, "EEPROM_write(%d, (%s >> 16) & 0xFF);\n", IntCode[i].literal1 + 2, MapSym(IntCode[i].name1, ASINT));
                }
                if(sov >= 4) {
                    doIndent(f, i);
                    fprintf(f, "EEPROM_write(%d, (%s >> 24) & 0xFF);\n", IntCode[i].literal1 + 3, MapSym(IntCode[i].name1, ASINT));
                }
                break;
            }
            */
            case INT_EEPROM_WRITE_BYTE: {
                fprintf(f, "EEPROM_write(%s, %s & 0xFF);\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT));
                break;
            }
            case INT_READ_ADC:

                if(compiler_variant == MNU_COMPILE_HI_TECH_C) {
                    McuIoPinInfo *iop = PinInfoForName(IntCode[i].name1.c_str()); // infos on pin used by this ADC
                    if(!iop)
                        THROW_COMPILER_EXCEPTION(_("ADC pin error")); // error in pin layout

                    const int s = iop->bit; // RA0 = AN0, ... RA7 = AN7
                    if((s >= 0) && (s <= 7)) {
                        ADC_Used[s] = 1;                   // marked as used
                        ADC_Chan[s] = IntCode[i].literal1; // Chan used to store ADC Refs

                        fprintf(f, "%s = Read_%s(0x%1.1X);\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name1, ASBIT), ADC_Chan[s]);
                    } else {
                        THROW_COMPILER_EXCEPTION(_("ADC Internal error")); // error in PWM pin layout
                    }
                } else
                    /////
                    fprintf(f, "%s = Read_%s();\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name1, ASBIT));
                break;

            case INT_PWM_OFF:

                if(compiler_variant == MNU_COMPILE_HI_TECH_C) {
                    McuIoPinInfo *iop = PinInfoForName(IntCode[i].name1.c_str()); // infos on pin used by this PWM
                    if(!iop)
                        THROW_COMPILER_EXCEPTION(_("PWM pin error")); // error in pin layout

                    McuPwmPinInfo *pop = PwmPinInfo(iop->pin);
                    if(pop) {
                        int pwm = 0;
                        pwm = pop->timer;  // PWM# (0x01, 0x02) from timer number
                        PWM_Used[pwm] = 1; // marked as used
                        PWM_Resol[pwm] = 10;

                        fprintf(f, "  PWM_Stop(%d);\n", pwm); // stop this PWM
                    } else {
                        THROW_COMPILER_EXCEPTION(_("PWM pin error")); // error in PWM pin layout
                    }
                } else if(compiler_variant == MNU_COMPILE_ARMGCC) {
                    //  McuPwmPinInfo *ioq = PwmMaxInfoForName(IntCode[i].name1.c_str(), Prog.cycleTimer);      //// donne Pin # + timer # resol

                    McuIoPinInfo *iop = PinInfoForName(IntCode[i].name1.c_str()); // infos on pin used by this PWM
                    if(!iop)
                        THROW_COMPILER_EXCEPTION("PWM pin error"); // error in pin layout

                    const char *s = iop->pinName; // full pin name supposed to be in format (PWMn.c) n= PWM#, c=Channel#
                    if(strlen(s)) {
                        int         pwm = 0;
                        const char *pos = strstr(s, "PWM");
                        if(pos) {
                            pwm = 16 * atoi(pos + 3); // 16*PWM# from pin description

                            pos = strchr(pos, '.');
                            if(pos) {
                                pwm += atoi(pos + 1); // 16*PWM# + Channel#   (4.1 -> 0x41)
                                PWM_Used[pwm] = 1;    // marked as used
                            } else
                                THROW_COMPILER_EXCEPTION(_("PWM pin error")); // error in pin layout
                        } else
                            THROW_COMPILER_EXCEPTION(_("PWM pin error")); // error in pin layout

                        fprintf(f, "  setPwmFrequency%X(%d, %s);\n", pwm, 0, "0"); // set freq= 0 => stop PWM
                    } else {
                        THROW_COMPILER_EXCEPTION(_("PWM Internal error")); // error in PWM pin layout
                    }
                } else if(compiler_variant == MNU_COMPILE_AVRGCC) {
                    McuIoPinInfo *iop = PinInfoForName(IntCode[i].name1.c_str()); // infos on pin used by this PWM
                    if(!iop)
                        THROW_COMPILER_EXCEPTION(_("PWM pin error")); // error in pin layout

                    const char *s = iop->pinName; // full pin name supposed to be in format (OCn) n= PWM#
                    if(strlen(s)) {
                        int         pwm = 0;
                        const char *pos = strstr(s, "OC");
                        if(pos) {
                            pwm = strtol(pos + 2, nullptr, 16);
                            ;                  // PWM # (0x00, 0x1A, 0x1B, 0x02...) from pin description
                            PWM_Used[pwm] = 1; // marked as used

                        } else
                            THROW_COMPILER_EXCEPTION(_("PWM pin error")); // error in pin layout

                        fprintf(f, "  setPwmFrequency%X(%d, %s, %d, %d);\n", pwm, 0, "0", 0, 0); // set resol= 0 => stop PWM
                    } else {
                        THROW_COMPILER_EXCEPTION(_("PWM Internal error")); // error in PWM pin layout
                    }
                } else
                    fprintf(f, "Write0_%s();\n", MapSym(IntCode[i].name1, ASBIT));
                break;

            case INT_SET_PWM:
                //Op(INT_SET_PWM, l->d.setPwm.duty_cycle, l->d.setPwm.targetFreq, l->d.setPwm.name, l->d.setPwm.resolution);

                if(compiler_variant == MNU_COMPILE_HI_TECH_C) {
                    McuIoPinInfo *iop = PinInfoForName(IntCode[i].name3.c_str()); // infos on pin used by this PWM
                    if(!iop)
                        THROW_COMPILER_EXCEPTION(_("PWM pin error")); // error in pin layout

                    McuPwmPinInfo *pop = PwmPinInfo(iop->pin);
                    if(pop) {
                        int pwm = 0;
                        pwm = pop->timer;  // PWM# (0x01, 0x02) from timer number
                        PWM_Used[pwm] = 1; // marked as used
                        PWM_Resol[pwm] = 10;

                        PWM_Freq[pwm] = hobatoi(IntCode[i].name2.c_str());
                        fprintf(f, "  setPwmFrequency%X(%ld, %s, %d);\n", pwm, PWM_Freq[pwm], MapSym(IntCode[i].name1, ASINT), PWM_Resol[pwm]);
                    } else {
                        THROW_COMPILER_EXCEPTION(_("PWM pin error")); // error in PWM pin layout
                    }
                } else if(compiler_variant == MNU_COMPILE_ARMGCC) {
                    //  McuPwmPinInfo *ioq = PwmMaxInfoForName(IntCode[i].name3.c_str(), Prog.cycleTimer);      //// donne Pin # + timer # resol

                    McuIoPinInfo *iop = PinInfoForName(IntCode[i].name3.c_str()); // infos on pin used by this PWM
                    if(!iop)
                        THROW_COMPILER_EXCEPTION("PWM pin error"); // error in pin layout

                    const char *s = iop->pinName; // full pin name supposed to be in format (PWMn.c) n= PWM#, c=Channel#
                    if(strlen(s)) {
                        int         pwm = 0;
                        const char *pos = strstr(s, "PWM");
                        if(pos) {
                            pwm = 16 * atoi(pos + 3); // 16*PWM# from pin description

                            pos = strchr(pos, '.');
                            if(pos) {
                                pwm += atoi(pos + 1); // 16*PWM# + Channel#   (4.1 -> 0x41)
                                PWM_Used[pwm] = 1;    // marked as used
                            } else
                                THROW_COMPILER_EXCEPTION(_("PWM pin error")); // error in pin layout
                        } else
                            THROW_COMPILER_EXCEPTION(_("PWM pin error")); // error in pin layout

                        PWM_Freq[pwm] = hobatoi(IntCode[i].name2.c_str());
                        fprintf(f, "  setPwmFrequency%X(%ld, %s);\n", pwm, PWM_Freq[pwm], MapSym(IntCode[i].name1, ASINT));
                    } else {
                        THROW_COMPILER_EXCEPTION(_("PWM Internal error")); // error in PWM pin layout
                    }
                } else if(compiler_variant == MNU_COMPILE_AVRGCC) {
                    McuIoPinInfo *iop = PinInfoForName(IntCode[i].name3.c_str()); // infos on pin used by this PWM
                    if(!iop)
                        THROW_COMPILER_EXCEPTION(_("PWM pin error")); // error in pin layout

                    const char *s = iop->pinName; // full pin name supposed to be in format (OCn) n= PWM#
                    if(strlen(s)) {
                        int         pwm = 0;
                        const char *pos = strstr(s, "OC");
                        if(pos) {
                            pwm = strtol(pos + 2, nullptr, 16);
                            ;                  // PWM # (0x00, 0x1A, 0x1B, 0x02...) from pin description
                            PWM_Used[pwm] = 1; // marked as used

                            McuPwmPinInfo *ioq = PwmMaxInfoForName(IntCode[i].name3.c_str(), Prog.cycleTimer); // max available resolution

                            PWM_MaxCs[pwm] = ioq->maxCS;      // number of possible prediv
                            PWM_Resol[pwm] = ioq->resolution; // 8, 10 bits...
                        } else
                            THROW_COMPILER_EXCEPTION(_("PWM pin error")); // error in pin layout

                        PWM_Freq[pwm] = hobatoi(IntCode[i].name2.c_str());
                        fprintf(f, "  setPwmFrequency%X(%ld, %s, %d, %d);\n", pwm, PWM_Freq[pwm], MapSym(IntCode[i].name1, ASINT), PWM_Resol[pwm], PWM_MaxCs[pwm]);
                    } else {
                        THROW_COMPILER_EXCEPTION(_("PWM Internal error")); // error in PWM pin layout
                    }
                } else
                /////
                {
                    fprintf(f, "#ifdef USE_PWM_FREQUENCY\n");
                    doIndent(f, i);
                    fprintf(f, "  setPwmFrequency(pin_%s, 64);", MapSym(IntCode[i].name3, ASBIT));
                    if(IsNumber(IntCode[i].name2))
                        fprintf(f, " // %ld Hz\n", hobatoi(IntCode[i].name2.c_str()));
                    else
                        fprintf(f, " // %s Hz\n", MapSym(IntCode[i].name2, ASINT));
                    doIndent(f, i);
                    fprintf(f, "#endif\n");
                    doIndent(f, i);
                    fprintf(f, "Write_%s(%s);\n", MapSym(IntCode[i].name3, ASBIT), MapSym(IntCode[i].name1, ASINT));
                }
                break;

            case INT_AllocFwdAddr:
                //fprintf(f, "#warning INT_%d\n", IntCode[i].op);
                break;

            case INT_AllocKnownAddr:
                fprintf(f, "Label%s:;\n", IntCode[i].name1.c_str());
                /*
                if(IntCode[i].name1)
                    fprintf(f, "//KnownAddr Rung%d %s %s\n", IntCode[i].literal1+1, IntCode[i].name2, IntCode[i].name1);
                else
                    fprintf(f, "//KnownAddr Rung%d\n", IntCode[i].literal1+1);
                */
                if(IntCode[i].name2 == "SUBPROG") {
                    int skip = FindOpNameLast(INT_RETURN, IntCode[i].name1);
                    if(skip <= i)
                        THROW_COMPILER_EXCEPTION_FMT(_("Invalid SUBPROG '%s'"), IntCode[i].name1);
                    i = skip;
                }
                break;
            case INT_FwdAddrIsNow:
                //fprintf(f, "Label%s:;\n", IntCode[i].name1.c_str());
                break;
            case INT_GOTO:
                fprintf(f, "goto Label%s;\n", IntCode[i].name1.c_str());
                break;
            case INT_GOSUB:
                fprintf(f, "Call_SUBPROG_%s();\n", IntCode[i].name1.c_str());
                break;
            case INT_RETURN:
                fprintf(f, "return;\n");
                break;
            case INT_SLEEP:

                if((compiler_variant == MNU_COMPILE_ARMGCC) || (compiler_variant == MNU_COMPILE_AVRGCC)) {
                    THROW_COMPILER_EXCEPTION(_("SLEEP not available in this mode"));
                } else
                /////
                {
                    fprintf(f, "cli();\n");
                    doIndent(f, i);
                    fprintf(f, "sleep_enable();\n");
                    doIndent(f, i);
                    fprintf(f, "sei();\n");
                    doIndent(f, i);
                    fprintf(f, "sleep_cpu();\n");
                    doIndent(f, i);
                    fprintf(f, "sleep_disable();\n");
                    doIndent(f, i);
                    fprintf(f, "sei();\n");
                }
                break;

            case INT_STRING:
                if(IntCode[i].name3.length())
#ifdef USE_STRING_LITERAL
                    fprintf(f, "sprintf_s(%s, %s, %s);\n", MapSym(IntCode[i].name1, ASSTR), MapSym(IntCode[i].name4.c_str(), ASSTR), MapSym(IntCode[i].name3));
#else
                    fprintf(f, "sprintf_s(%s, \"%s\", %s);\n", MapSym(IntCode[i].name1, ASSTR), IntCode[i].name2.c_str(), MapSym(IntCode[i].name3));
#endif
                else
#ifdef USE_STRING_LITERAL
                    fprintf(f, "strcpy(%s, %s);\n", MapSym(IntCode[i].name1, ASSTR), MapSym(IntCode[i].name4.c_str(), ASSTR));
#else
                    fprintf(f, "strcpy(%s, \"%s\");\n", MapSym(IntCode[i].name1, ASSTR), IntCode[i].name2.c_str());
#endif
                break;

#ifdef TABLE_IN_FLASH
            case INT_STRING_INIT:
            case INT_FLASH_INIT:
                break;

            case INT_FLASH_READ: {
                if(IsNumber(IntCode[i].name3)) {
                    fprintf(f, "%s = %d; // %s[%s]\n", MapSym(IntCode[i].name1), IntCode[i].data[CheckMakeNumber(IntCode[i].name3)], MapSym(IntCode[i].name2), IntCode[i].name3.c_str());
                } else {
                    fprintf(f, "#ifdef __GNUC__\n");
                    doIndent(f, i);
                    fprintf(f, "%s = pgm_read_word(&%s[%s]);\n", MapSym(IntCode[i].name1), MapSym(IntCode[i].name2), MapSym(IntCode[i].name3));
                    doIndent(f, i);
                    fprintf(f, "#else\n");
                    doIndent(f, i);
                    fprintf(f, "%s = %s[%s];\n", MapSym(IntCode[i].name1), MapSym(IntCode[i].name2), MapSym(IntCode[i].name3));
                    doIndent(f, i);
                    fprintf(f, "#endif\n");
                }
                break;
            }
            case INT_RAM_READ: {
                if(IsNumber(IntCode[i].name3)) {
                    fprintf(f, "%s = %s[%d]\n", MapSym(IntCode[i].name1), MapSym(IntCode[i].name2, ASSTR), CheckMakeNumber(IntCode[i].name3));
                } else {
                    fprintf(f, "%s = %s[%s];\n", MapSym(IntCode[i].name1), MapSym(IntCode[i].name2, ASSTR), MapSym(IntCode[i].name3));
                }
                break;
            }
#endif
            default:
                THROW_COMPILER_EXCEPTION_FMT("INT_%d", IntCode[i].op);
        }
    }
}

//-----------------------------------------------------------------------------
static void GenerateSUBPROG(FILE *f)
{
    for(uint32_t i = 0; i < IntCode.size(); i++) {
        switch(IntCode[i].op) {
            case INT_GOSUB: {
                fprintf(f, "\n");
                fprintf(f, "void Call_SUBPROG_%s() { // LabelRung%d\n", IntCode[i].name1.c_str(), (int)(IntCode[i].literal1 + 1));
                int indentSave = indent;
                indent = 1;
                GenerateAnsiC(f, FindOpName(INT_AllocKnownAddr, IntCode[i].name1, "SUBPROG") + 1, FindOpNameLast(INT_RETURN, IntCode[i].name1));
                indent = indentSave;
                fprintf(f, "}\n");
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Actually generate the C source for the datas.
//-----------------------------------------------------------------------------
static void GenerateAnsiC_flash_eeprom(FILE *f)
{
#ifdef TABLE_IN_FLASH
    for(uint32_t i = 0; i < IntCode.size(); i++) {
        switch(IntCode[i].op) {
            case INT_STRING_INIT: {
#ifdef USE_STRING_LITERAL
                fprintf(f, "const char %s[] = \"%s\"; // [%d]\n", MapSym(IntCode[i].name1.c_str(), ASSTR), IntCode[i].name2.c_str(), IntCode[i].name2.length());
#endif
                break;
            }
            case INT_FLASH_INIT: {
                int         sovElement = IntCode[i].literal2;
                const char *sovs = "invalid SOV value";
                /*
                CodeVision AVR
                // Pointer to a char string placed in FLASH
                flash char *ptr_to_flash1="This string is placed in FLASH";
                char flash *ptr_to_flash2="This string is also placed in FLASH";

                // Pointer to a char string placed in EEPROM
                eeprom char *ptr_to_eeprom1="This string is placed in EEPROM";
                char eeprom *ptr_to_eeprom2="This string is also placed in EEPROM";
                */
                if(sovElement == 1) {
                    sovs = "flash unsigned char";
                } else if(sovElement == 2) {
                    sovs = "flash unsigned int";
                } else if(sovElement == 3) {
                    sovs = "flash unsigned24bit";
                } else if(sovElement == 4) {
                    sovs = "flash unsigned long int";
                } else {
                    THROW_COMPILER_EXCEPTION_FMT("sovElement=%d", sovElement);
                }
                if(sovElement == 1) {
                    fprintf(f, "// {");
                    for(auto j = 0; j < (IntCode[i].literal1 - 1); j++) {
                        fprintf(f, "'%s', ", ChrToFrmtStr(IntCode[i].data[j], FRMT_SPACE));
                    }
                    fprintf(f, "'%s'};\n", ChrToFrmtStr(IntCode[i].data[IntCode[i].literal1 - 1], FRMT_SPACE));
                }
                //
                fprintf(f, "// {");
                for(int j = 0; j < (IntCode[i].literal1 - 1); j++) {
                    fprintf(f, "0x%X, ", IntCode[i].data[j]);
                }
                fprintf(f, "0x%X};\n", IntCode[i].data[IntCode[i].literal1 - 1]);
                //
                fprintf(f, "#ifdef __CODEVISIONAVR__\n");
                fprintf(f, "%s %s[%d] = {", sovs, MapSym(IntCode[i].name1), IntCode[i].literal1);
                for(int j = 0; j < (IntCode[i].literal1 - 1); j++) {
                    fprintf(f, "%d, ", IntCode[i].data[j]);
                }
                fprintf(f, "%d};\n", IntCode[i].data[IntCode[i].literal1 - 1]);
                fprintf(f, "#endif\n");
                /*
                winavr avr gcc

                const char FlashString[] PROGMEM = "This is a string ";
                */
                if(sovElement == 1) {
                    sovs = "unsigned char";
                } else if(sovElement == 2) {
                    sovs = "unsigned int";
                } else if(sovElement == 3) {
                    sovs = "unsigned24bit";
                } else if(sovElement == 4) {
                    sovs = "unsigned long int";
                } else {
                    THROW_COMPILER_EXCEPTION_FMT("sovElement=%d", sovElement);
                }

                if(mcu_ISA == ISA_ARM) ///// Added by JG to store strings in RAM, not in flash
                {
                    fprintf(f, "#define PROGMEM\n");
                    fprintf(f, "#define pgm_read_word(x) *(x)\n");
                }

                ///// Added by JG for HI-TECH C
                fprintf(f, "#ifdef __PICC__\n");
                fprintf(f, "const %s %s[%d] = {", sovs, MapSym(IntCode[i].name1), IntCode[i].literal1);
                for(int j = 0; j < (IntCode[i].literal1 - 1); j++) {
                    fprintf(f, "%d, ", IntCode[i].data[j]);
                }
                fprintf(f, "%d};\n", IntCode[i].data[IntCode[i].literal1 - 1]);
                fprintf(f, "#endif\n");
                /////

                fprintf(f, "#ifdef __GNUC__\n");
                fprintf(f, "const %s %s[%d] PROGMEM = {", sovs, MapSym(IntCode[i].name1), IntCode[i].literal1);
                for(int j = 0; j < (IntCode[i].literal1 - 1); j++) {
                    fprintf(f, "%d, ", IntCode[i].data[j]);
                }
                fprintf(f, "%d};\n", IntCode[i].data[IntCode[i].literal1 - 1]);
                fprintf(f, "#endif\n");
                break;
            }
#ifdef NEW_FEATURE
            case INT_EEPROM_INIT: {
                fprintf(f, "epprom datas;\n");
                break;
            }
#endif
            default: {
            }
        }
    }
#endif
}

bool CompileAnsiC(const char *outFile, int MNU)
{
    if(Prog.mcu())
        mcu_ISA = Prog.mcu()->whichIsa;
    if(MNU > 0)
        compiler_variant = MNU;
    else
        THROW_COMPILER_EXCEPTION_FMT(_("Invalid MENU:%i"), MNU);

    variables.clear();

    for(int i = 0; i < MAX_ADC_C; i++) {
        ADC_Used[i] = 0;
        ADC_Chan[i] = 0;
    }

    for(int i = 0; i < MAX_PWM_C; i++) {
        PWM_Used[i] = -1;
        PWM_Freq[i] = 0;
        PWM_Resol[i] = 0;
        PWM_MaxCs[i] = 0;
    }
    countpwm = 0;

    if((Prog.mcu()) && (Prog.mcu()->whichIsa == ISA_ARM)) // ARM uses Timer 3
        Prog.cycleTimer = 3;

    char CurrentLdName[MAX_PATH];
    GetFileName(CurrentLdName, outFile);

    char Current_Ld_Name[MAX_PATH];
    strcpy(Current_Ld_Name, CurrentLdName);
    for(size_t i = 0; i<strlen(Current_Ld_Name); i++)
        if(Current_Ld_Name[i] == ' ')
            Current_Ld_Name[i] = '_';

    char desth[MAX_PATH];
    SetExt(desth, outFile, ".h");

    char ladderhName[MAX_PATH];
    char compilePath[MAX_PATH];
    char arduinoDest[MAX_PATH];
    strcpy(compilePath, outFile);
    ExtractFileDir(compilePath);

    TIMER_Used = 0;
    SPI_Used = -1;
    I2C_Used = -1;

    sprintf_s(ladderhName, "%s\\ladder.h", compilePath);

    FileTracker flh(ladderhName, "w");
    if(!flh) {
        THROW_COMPILER_EXCEPTION_FMT(_("Couldn't open file '%s'"), ladderhName);
        //return false;
    }
    fprintf(flh,
            "/* This is example for ladder.h file!\n"
            "   This is auto-generated C header from LDmicro.\n"
            "   Rename this file as ladder.h or copy content(or part) of this file\n"
            "   to existing ladder.h. Remove this comment from ladder.h. */\n"
            "\n"
            "#ifndef __LADDER_H__\n"
            "#define __LADDER_H__\n"
            "\n");

    char deviceName[MAX_PATH];
    strcpy(deviceName, Prog.mcu() ? Prog.mcu()->deviceName : "undefined_mcu");
    if(Prog.mcu()) {
        if(compiler_variant == MNU_COMPILE_CCS_PIC_C) {
            fprintf(flh, "#include <%s.h>\n", Prog.mcu()->mcuH);
            fprintf(flh, "#device %s\n", deviceName);
            fprintf(flh, "#FUSES 1=0x%04X\n", (WORD)Prog.configurationWord & 0xFFFF);
            if(Prog.configurationWord & 0xFFFF0000) {
                fprintf(flh, "#FUSES 2=0x%04X\n", (WORD)(Prog.configurationWord >> 16) & 0xFFFF);
            }
            if(DelayUsed() || UartFunctionUsed()) {
                fprintf(flh, "#USE DELAY(CLOCK=%d)\n", Prog.mcuClock);
            }
            if(UartFunctionUsed()) {
                fprintf(flh, "#USE RS232(BAUD=%d, BITS=8, PARITY=N, STOP=1, ERRORS, UART1) // ENABLE=pin\n", Prog.baudRate);
            }
            if(AdcFunctionUsed()) {
                fprintf(flh, "#device ADC=10\n");
            }
            if(PwmFunctionUsed()) {
                fprintf(flh, "//TODO #USE PWM // http://www.ccsinfo.com/newsdesk_info.php?newsdesk_id=182 \n");
            }
            if(Prog.mcu()) {
                for(int i = 0; i < MAX_IO_PORTS; i++) {
                    if(IS_MCU_REG(i))
                        fprintf(flh, "#USE FAST_IO(%c)\n", 'A' + i);
                }
            }
        }
        fprintf(flh,
                "\n"
                "#define LDTARGET_%s\n\n"
                "#ifndef F_CPU\n"
                "  #define F_CPU %dUL\n"
                "#endif\n"
                "#define _XTAL_FREQ %dUL\n\n",
                _strlwr(deviceName),
                Prog.mcuClock,
                Prog.mcuClock);

        if((mcu_ISA == ISA_PIC16) || (mcu_ISA == ISA_PIC18)) {
            fprintf(flh, "#define CFG_WORD 0x%X\n\n", (WORD)Prog.configurationWord & 0xFFFF);
        }
    }
    fprintf(flh,
            "/* Uncomment EXTERN_EVERYTHING if you want all symbols in %s.c extern. */\n"
            "//#define EXTERN_EVERYTHING\n"
            "#ifdef EXTERN_EVERYTHING\n"
            "  #define _STATIC_\n"
            "#else\n"
            "  #define _STATIC_ static\n"
            "#endif\n"
            "\n"
            "/* Uncomment NO_PROTOTYPES if you want own prototypes for functions. */\n"
            "//#define NO_PROTOTYPES\n"
            "\n"
            "/* Define NO_PROTOTYPES in ladder.h if you don't want LDmicro to provide prototypes for\n"
            "   all the I/O functions (Read_Ux_xxx, Write_Ux_xxx) that you must provide.\n"
            "   If you define this then you must provide your own prototypes for these\n"
            "   functions in %s.h, or provide definitions (e.g. as inlines or macros)\n"
            "   for them in %s.h. */\n"
            "#ifdef NO_PROTOTYPES\n"
            "  #define PROTO(x)\n"
            "#else\n"
            "  #define PROTO(x) x\n"
            "#endif\n"
            "\n",
            CurrentLdName,
            CurrentLdName,
            CurrentLdName);

    fprintf(flh,
            "/* Uncomment DO_LDSTUBS if you want to use the empty stub-functions\n"
            "   from %s.c instead the prototypes for functions.\n"
            "   Use DO_LDSTUBS to just check the compilation of the generated files. */\n"
            "//#define DO_LDSTUBS\n"
            "\n"
            "#ifdef DO_LDSTUBS\n"
            "  #define LDSTUB(x)   x { ; }\n"
            "  #define LDSTUB0(x)  x { return 0; }\n"
            "  #define LDSTUB1(x)  x { return 1; }\n"
            "#else\n"
            "  #define LDSTUB(x)   PROTO(extern x;)\n"
            "  #define LDSTUB0(x)  PROTO(extern x;)\n"
            "  #define LDSTUB1(x)  PROTO(extern x;)\n"
            "#endif\n"
            "\n"
            "/* Uncomment USE_WDT when you need to. */\n"
            "//#define USE_WDT\n"
            "\n"
            "/* Comment out USE_MACRO in next line, if you want to use functions instead of macros. */\n"
            "#define USE_MACRO\n"
            "\n",
            CurrentLdName);

    if(compiler_variant == MNU_COMPILE_ARDUINO) {
        fprintf(flh,
                "#include \"Arduino.h\"\n"
                "\n");
        fprintf(flh,
                "#ifdef __GNUC__\n"
                "  //mem.h vvv\n"
                "  //CodeVisionAVR V2.0 C Compiler\n"
                "  //(C) 1998-2007 Pavel Haiduc, HP InfoTech S.R.L.\n"
                "  //\n"
                "  //  Memory access macros\n"
                "\n"
                "  #ifndef _MEM_INCLUDED_\n"
                "  #define _MEM_INCLUDED_\n"
                "\n"
                "  #define pokeb(addr,data) (*((volatile unsigned char *)(addr)) = (data))\n"
                "  #define pokew(addr,data) (*((volatile unsigned int *)(addr)) = (data))\n"
                "  #define peekb(addr) (*((volatile unsigned char *)(addr)))\n"
                "  #define peekw(addr) (*((volatile unsigned int *)(addr)))\n"
                "\n"
                "  #endif\n"
                "  //mem.h ^^^\n"
                "#endif\n");
    }

    if(compiler_variant == MNU_COMPILE_HI_TECH_C) {

        if(StringFunctionUsed()) {
            fprintf(flh,
                    "#include <stdio.h>\n"
                    "#include <string.h>\n");
        }
        if(SpiFunctionUsed() && I2cFunctionUsed()) {
            THROW_COMPILER_EXCEPTION(_("SPI & I2C can't be used together on PICs"));
        }
    } else if(compiler_variant == MNU_COMPILE_ARMGCC) {
        fprintf(flh,
                "\n"
                "#include <stdio.h>\n");
        if(StringFunctionUsed() || SpiFunctionUsed()) {
            fprintf(flh, "#include <string.h>\n");
        }
        if((Prog.mcu()) && (Prog.mcu()->core == CortexF1))
            fprintf(flh,
                    "#include \"stm32f10x.h\"\n"
                    "#include \"stm32f10x_gpio.h\"\n"
                    "#include \"stm32f10x_rcc.h\"\n"
                    "#include \"stm32f10x_tim.h\"\n");
        else // CortexF4
            fprintf(flh,
                    "#include \"stm32f4xx.h\"\n"
                    "#include \"stm32f4xx_gpio.h\"\n"
                    "#include \"stm32f4xx_rcc.h\"\n"
                    "#include \"stm32f4xx_tim.h\"\n");
        fprintf(flh,
                "\n"
                "#include \"Lib_gpio.h\"\n"
                "#include \"Lib_timer.h\"\n"
                "#include \"Lib_usr.h\"\n");

        if(AdcFunctionUsed()) {
            if((Prog.mcu()) && (Prog.mcu()->core == CortexF1))
                fprintf(flh,
                        "#include \"stm32f10x_adc.h\"\n"
                        "#include \"Lib_adc.h\"\n");
            else
                fprintf(flh,
                        "#include \"stm32f4xx_adc.h\"\n"
                        "#include \"Lib_adc.h\"\n");
        }
        if(PwmFunctionUsed()) {
            fprintf(flh, "#include \"Lib_pwm.h\"\n");
        }
        if(UartFunctionUsed()) {
            if((Prog.mcu()) && (Prog.mcu()->core == CortexF1))
                fprintf(flh,
                        "#include \"stm32f10x_usart.h\"\n"
                        "#include \"Lib_uart.h\"\n");
            else
                fprintf(flh,
                        "#include \"stm32f4xx_usart.h\"\n"
                        "#include \"Lib_uart.h\"\n");
        }
        if(SpiFunctionUsed()) {
            if((Prog.mcu()) && (Prog.mcu()->core == CortexF1))
                fprintf(flh,
                        "#include \"stm32f10x_spi.h\"\n"
                        "#include \"Lib_spi.h\"\n");
            else
                fprintf(flh,
                        "#include \"stm32f4xx_spi.h\"\n"
                        "#include \"Lib_spi.h\"\n");
        }
        if(I2cFunctionUsed()) {
            if((Prog.mcu()) && (Prog.mcu()->core == CortexF1))
                fprintf(flh,
                        "#include \"stm32f10x_i2c.h\"\n"
                        "#include \"Lib_i2c.h\"\n");
            else
                fprintf(flh,
                        "#include \"stm32f4xx_i2c.h\"\n"
                        "#include \"Lib_i2c.h\"\n");
        }
    } else if(compiler_variant == MNU_COMPILE_AVRGCC) {
        fprintf(flh,
                "\n"
                "#include <stdio.h>\n"
                "#include <avr\\io.h>\n"
                "#include <avr\\interrupt.h>\n"
                "//#include <avr\\%s.h>\n"
                "\n",
                Prog.mcu()->mcuH2);
    }

    fprintf(flh,
            "/*\n"
            "  Type                  Size(bits)\n"
            "  BOOLEAN_t    unsigned       1 or 8, the smallest possible\n"
            "  SBYTE     signed integer      8\n"
            "  SWORD     signed integer     16\n"
            "  SDWORD    signed integer     32\n"
            "*/\n");
    if(compiler_variant == MNU_COMPILE_ARDUINO) {
        fprintf(flh,
                "typedef boolean    BOOLEAN_t;\n"
                "typedef char           SBYTE;\n"
                "typedef int            SWORD;\n"
                "typedef long int      SDWORD;\n"
                "\n");
    } else if(compiler_variant == MNU_COMPILE_ARMGCC) {
        fprintf(flh,
                "typedef uint8_t    BOOLEAN_t;\n"
                "typedef int8_t     SBYTE;\n"
                "typedef int16_t    SWORD;\n"
                "typedef int32_t    SDWORD;\n"
                "\n");
    } else if((mcu_ISA == ISA_PIC16) || (mcu_ISA == ISA_PIC18)) {
        if(compiler_variant == MNU_COMPILE_CCS_PIC_C) {
            fprintf(flh,
                "#define uint8_t  unsigned int8\n"
                "#define uint16_t unsigned int16\n"
                "#define uint32_t unsigned int32\n"
                "\n"
                "typedef int1         BOOLEAN_t;\n"
                "typedef signed  int8     SBYTE;\n"
                "typedef signed int16     SWORD;\n"
                "typedef signed int32    SDWORD;\n"
                "\n");
        } else {
            fprintf(flh,
                "typedef unsigned char BOOLEAN_t;\n"
                "typedef signed char       SBYTE;\n"
                "typedef signed short int  SWORD;\n"
                "typedef signed long int  SDWORD;\n"
                "\n");
        }
    } else if(mcu_ISA == ISA_ARM) {
        fprintf(flh,
                "typedef unsigned char BOOLEAN_t;\n"
                "typedef signed char       SBYTE;\n"
                "typedef signed short int  SWORD;\n"
                "typedef signed long int  SDWORD;\n"
                "\n");
    } else if(mcu_ISA == ISA_AVR) {
        fprintf(flh,
                "typedef unsigned char BOOLEAN_t;\n"
                "typedef signed char       SBYTE;\n"
                "typedef signed short int  SWORD;\n"
                "typedef signed long int  SDWORD;\n"
                "\n");
    } else {
        fprintf(flh,
                "typedef unsigned char BOOLEAN_t;\n"
                "typedef signed char       SBYTE;\n"
                "typedef signed short int  SWORD;\n"
                "typedef signed long int  SDWORD;\n"
                "\n");
    }

    fprintf(flh, "#define SFR_ADDR(addr) (*((volatile unsigned char *)(addr)))\n");
    fprintf(flh, "//#define BYTE_AT(var, index) (*(((unsigned char *)(&var)) + (index)))\n");
    fprintf(flh, "#define BYTE_AT(var, index) (((SBYTE *)(&var))[index])\n");
    fprintf(flh, "#define WORD_AT(var, index) (((SWORD *)(&var))[index])\n\n");

    fprintf(flh,"#include \"UsrLib.h\"\n");
    if(AdcFunctionUsed())
        fprintf(flh, "#include \"AdcLib.h\"\n");
    if(PwmFunctionUsed())
        fprintf(flh, "#include \"PwmLib.h\"\n");
    if(UartFunctionUsed())
        fprintf(flh, "#include \"UartLib.h\"\n");
    if(SpiFunctionUsed())
        fprintf(flh, "#include \"SpiLib.h\"\n");
    if(I2cFunctionUsed())
        fprintf(flh, "#include \"I2cLib.h\"\n");
    if(EepromFunctionUsed())
        fprintf(flh, "#include \"romlib.h\"\n");

    FileTracker fh(desth, "w");
    if(!fh) {
        THROW_COMPILER_EXCEPTION_FMT(_("Couldn't open file '%s'"), desth);
    }
    fprintf(fh,
            "/* This is auto-generated C header from LDmicro. Do not edit this file!\n"
            "   Go back to the LDmicro ladder diagram source for changes in the ladder logic,\n"
            "   and make any C additions in additional .c or .h files linked against this one. */\n"
            "#ifndef __%s_H__\n"
            "#define __%s_H__\n"
            "\n"
            "#include \"ladder.h\"\n"
            "\n",
            Current_Ld_Name,
            Current_Ld_Name);

    if(compiler_variant == MNU_COMPILE_ARDUINO) {
        fprintf(fh,
                "// PLC cycle interval, set this according to LDmicro settings. (micro seconds)\n"
                "#define PLC_INTERVAL %lld // us\n"
                "\n",
                Prog.cycleTime);
        /*
        fprintf(fh,
                "#ifdef USE_WDT\n"
                "  #include <avr\\wdt.h>\n"
                "#endif\n");
        if(SleepFunctionUsed()) {
            fprintf(fh, "#include <avr\\sleep.h>\n");
        }
        */
        if(PwmFunctionUsed()) {
            fprintf(fh, "/* Uncomment USE_PWM_FREQUENCY and and set manually the proper divisor for setPwmFrequency().\n");
            fprintf(fh, "   Base frequencies and available divisors on the pins, see in the file PwmFrequency.h */\n");
            fprintf(fh, "//#define USE_PWM_FREQUENCY\n");
            fprintf(fh, "#ifdef USE_PWM_FREQUENCY\n");
            fprintf(fh, "  #include \"PwmFrequency.h\"\n");
            fprintf(fh, "#endif\n");
            fprintf(fh, "\n");
        }
        /*
        if(EepromFunctionUsed())
            fprintf(fh, "#include <EEPROM.h>\n");
        */
    } else if(mcu_ISA == ISA_PIC16) {
        //
    } else if(mcu_ISA == ISA_PIC18) {
        //
    } else if(mcu_ISA == ISA_AVR) {
        fprintf(flh,
                "#include <stdio.h>\n"
                "\n"
                "#ifdef __CODEVISIONAVR__\n"
                "    //typedef char PROGMEM prog_char\n"
                "    //#include <mem.h>\n"
                "    #include <%s.h>\n"
                "#elif defined(__GNUC__)\n"
                "    //#warning __GNUC__\n"
                "    #include <avr\\pgmspace.h>\n"
                "    #define __PROG_TYPES_COMPAT__ //arduino\n"
                "    #ifdef USE_WDT\n"
                "      #include <avr\\wdt.h>\n"
                "    #endif\n"
                "#endif\n",
                Prog.mcu()->mcuH);
    } else {
    }
    /*
    if(DelayUsed()) {
        if(compiler_variant == MNU_COMPILE_CODEVISIONAVR) {
            fprintf(fh, "#include <delay.h>\n");
        } else if(mcu_ISA == ISA_AVR) {
            fprintf(fh, "#include <util/delay.h>\n");
        }
    }
    */

    if(compiler_variant == MNU_COMPILE_ARDUINO) {
        fprintf(fh,
                "extern void loopPlc(void);  // Call loopPlc() function in loop() of your arduino project\n"
                "extern void setupPlc(void); //  or initialize PLC cycle timer in this function\n"
                "extern void PlcCycle(void); //  and call PlcCycle() function once per PLC cycle timer.\n"
                "\n");
    } else {
        fprintf(fh,
                "//extern void loopPlc(void);  // Call loopPlc() function in main() of your project\n"
                "extern void setupPlc(void); //  or initialize PLC cycle timer\n"
                "extern void PlcCycle(void); //  and call PlcCycle() function once per PLC cycle timer.\n"
                "\n");
    }

    if(SpiFunctionUsed()) {
        if(mcu_ISA == ISA_ARM)
            fprintf(fh,
                    "SWORD SPI_SendRecv(SPI_TypeDef *SPIx, SWORD send);\n"
                    "void SPI_Write(char *str);\n"
                    "\n");
        else
            fprintf(fh,
                    "SWORD SPI_SendRecv(SWORD send);\n"
                    "void SPI_Write(char *str);\n"
                    "\n");
    }
    if(I2cFunctionUsed()) {
        fprintf(fh,
                "SWORD I2C_Recv(SBYTE address, SBYTE registr);\n"
                "void I2C_Send(SBYTE address, SBYTE registr, SWORD send);\n"
                "\n");
    }
    /*
    if(EepromFunctionUsed()) {
        if(compiler_variant == MNU_COMPILE_ARDUINO) {
            fprintf(fh,
                    "#define eeprom_busy_wait() do {} while(!eeprom_is_ready())\n"
                    "\n"
                    "#ifdef USE_MACRO\n"
                    "  #define EEPROM_write( addr, x ) EEPROM.write(addr, x)\n"
                    "  #define EEPROM_read( addr ) EEPROM.read(addr)\n"
                    "#else\n"
                    "  void EEPROM_write(int addr, unsigned char data);\n"
                    "  unsigned char EEPROM_read(int addr);\n"
                    "#endif\n"
                    "\n");
        } else
            fprintf(flh,"#include \"romlib.h\"\n");
    }
    */

    FileTracker f;
    if(compiler_variant == MNU_COMPILE_ARDUINO) {
        SetExt(arduinoDest, outFile, ".cpp");
        f.open(arduinoDest, "w");
    } else {
        f.open(outFile, "w");
    }

    if(!f) {
        THROW_COMPILER_EXCEPTION_FMT(_("Couldn't open file '%s'"), outFile);
    }

    if((compiler_variant == MNU_COMPILE_ARMGCC)       //
       || (compiler_variant == MNU_COMPILE_HI_TECH_C) //
       || (compiler_variant == MNU_COMPILE_AVRGCC)) {
        fprintf(f,
                "/* This is auto-generated C code from LDmicro. Do not edit this file! Go\n"
                "   back to the LDmicro ladder diagram source for changes in the ladder logic.\n\n"
                "   Yous must provide additional librairies for I/O read/write operations\n"
                "   and for used peripherals (Timer, ADC, PWM, UART, SPI...) in 'lib' subdirectory\n"
                "   of the folder containing the ladder .ld file, so that it should be compiled\n"
                "   and liked with this file when running flashMcu.bat.\n"
                "   Generated .elf and .hex files will be created in 'bin' subdirectory.*/\n"
                "\n");
    } else {
        fprintf(f,
                "/* This is auto-generated C code from LDmicro. Do not edit this file! Go\n"
                "   back to the LDmicro ladder diagram source for changes in the ladder logic, and make\n"
                "   any C additions either in ladder.h or in additional .c or .h files linked\n"
                "   against this one. */\n"
                "\n"
                "/* You must provide ladder.h; there you must provide:\n"
                "   a typedef for SWORD and BOOLEAN_t, signed 16 bit and boolean types\n"
                "   (probably typedef signed short SWORD; typedef unsigned char bool;)\n"
                "\n"
                "   You must also provide implementations of all the I/O read/write\n"
                "   either as inlines in the header file or in another source file. (The\n"
                "   I/O functions are all declared extern.)\n"
                "\n"
                "   See the generated source code (below) for function names. */\n"
                "\n");
    }
    fprintf(f,
            "#include \"ladder.h\"\n"
            "#include \"%s.h\" // Copy this line into main project file where is function main().\n"
            "\n"
            "/* Ux_xxx symbols correspond to user-defined names. There is such a symbol\n"
            "   for every internal relay, variable, timer, and so on in the ladder\n"
            "   program. Ix_xxx symbols are internally generated. */\n"
            "/* Ix_xxx\n"
            "   Ux_xxx\n"
            "    ^\n"
            "    b means bool type\n"
            "    i means int type */\n",
            CurrentLdName);
    fprintf(f,
            "/* Define EXTERN_EVERYTHING in ladder.h if you want all symbols extern.\n"
            "   This could be useful to implement `magic variables,' so that for\n"
            "   example when you write to the ladder variable duty_cycle, your PLC\n"
            "   runtime can look at the C variable U_duty_cycle and use that to set\n"
            "   the PWM duty cycle on the micro. That way you can add support for\n"
            "   peripherals that LDmicro doesn't know about. */\n"
            "\n");

    if(compiler_variant == MNU_COMPILE_HI_TECH_C) {
        // TODO: change the type of the configurationWord to string to allow access to all the configuration words
        if(mcu_ISA == ISA_PIC16) {
            fprintf(f,
                    "#include <htc.h>\n"
                    "__CONFIG(0x%X);\n",
                    (WORD)Prog.configurationWord & 0xFFFF);
            if(Prog.configurationWord & 0xFFFF0000) {
                fprintf(f, "__CONFIG(0x%X);\n", (WORD)(Prog.configurationWord >> 16) & 0xFFFF);
            }
        } else if(mcu_ISA == ISA_PIC18) { // Beware that __PROG_CONFIG may not work (HITECH-C 18 release notes)
            fprintf(f,
                    "#include <htc.h>\n\n"
                    "//#define _XTAL_FREQ %d\n"
                    "__PROG_CONFIG(1, 0x%4.4X);\n"
                    "__CONFIG(2, WDTDIS & PWRTEN & BORDIS);\n"                 // No Wtdog, No Brown out, Pwrup timer
                    "__CONFIG(3, MCLREN & PBDIGITAL);\n"                       // Mclr, PortB digital
                    "__CONFIG(4, DEBUGDIS & XINSTDIS & LVPDIS & STVREN);\n\n", // No Debug, No Extended set, No LVP, Stack overflow
                    Prog.mcuClock,
                    (WORD)Prog.configurationWord & 0xFFFF); // Config words 5, 6, 7 are left to default
        }
    } else if(compiler_variant == MNU_COMPILE_ARDUINO) {
        fprintf(flh,
                "// You provide analog reference type for ARDUINO in ladder.h here.\n"
                "const int analogReference_type = DEFAULT;\n"
                "\n");
    }
    // now generate declarations for all variables
    GenerateDeclarations(f, fh, flh);
    GenerateAnsiC_flash_eeprom(f);
    GenerateSUBPROG(f);

    if(compiler_variant == MNU_COMPILE_ARDUINO) {
    } else if(mcu_ISA == ISA_AVR) {
        // moved to LIBRARIES_FOR\Avr\SpiLib.c

        if(SpiFunctionUsed()) {
            if(compiler_variant == MNU_COMPILE_AVRGCC) {
                fprintf(f,
                        "\n"
                        "SWORD SPI_SendRecv(SWORD send) {\n"
                        "  return SPI_Send(send);\n"
                        "}\n\n");
                fprintf(f,
                        "void SPI_Write(char *str) {\n"
                        "  int i= 0;\n"
                        "  while(str[i] != 0) {\n"
                        "    SPI_Send(str[i]);\n"
                        "    i++;\n"
                        "   }\n"
                        "}\n\n");
            }
        }
        if(I2cFunctionUsed()) {
            if(compiler_variant == MNU_COMPILE_AVRGCC) {
                fprintf(f,
                        "\n"
                        "SWORD I2C_Recv(SBYTE address, SBYTE registr) {\n"
                        "  SWORD recv= 0;\n"
                        "  recv= I2C_MasterGetReg(address, registr);\n"
                        "  return recv;\n"
                        "}\n\n" /*, I2C_Used*/);
                fprintf(f,
                        "void I2C_Send(SBYTE address, SBYTE registr, SWORD send) {\n"
                        "  I2C_MasterSetReg(address, registr, send);\n"
                        "}\n\n" /*, I2C_Used*/);
            }
        }
    } else if((mcu_ISA == ISA_PIC16) || (mcu_ISA == ISA_PIC18)) {
        if(AdcFunctionUsed()) {
            if(compiler_variant == MNU_COMPILE_CCS_PIC_C) {
                fprintf(f, "void ADC_Init(void) {\n");
                fprintf(f,
                        "  setup_adc_ports(ALL_ANALOG);\n"
                        "  setup_adc(ADC_CLOCK_INTERNAL | ADC_CLOCK_DIV_32);\n");
                fprintf(f,
                        "}\n"
                        "\n");
            } else if(compiler_variant != MNU_COMPILE_HI_TECH_C) {
                fprintf(f, "void ADC_Init(void) {\n");
                fprintf(f,
                        "}\n"
                        "\n");
            }
        }
        if(PwmFunctionUsed()) {
            if(compiler_variant != MNU_COMPILE_HI_TECH_C) {
                fprintf(f, "void PWM_Init(void) {\n");
                fprintf(f,
                        "}\n"
                        "\n");
            }
        }
        if(UartFunctionUsed()) {
            if(compiler_variant == MNU_COMPILE_CCS_PIC_C) {
                // use UartLib.h
            } else if(compiler_variant == MNU_COMPILE_HI_TECH_C) {
                // use UartLib.h
            } else {
                fprintf(f,
                        "void UART_Init(void) {\n"
                        "  // UART baud rate setup\n");
                if(compiler_variant != MNU_COMPILE_ANSIC) {
                    int32_t divisor, brgh;
                    CalcPicUartBaudRate(Prog.mcuClock, Prog.baudRate, &divisor, &brgh);
                    if(brgh)
                        fprintf(f, "  BRGH = 1;\n");
                    else
                        fprintf(f, "  BRGH = 0;\n");
                    fprintf(f,
                            "  SPBRG = %d;\n"
                            "  TXEN = 1; CREN = 1; SPEN = 1;\n",
                            divisor);
                }
                fprintf(f,
                        "}\n"
                        "\n");
            }
            if(compiler_variant == MNU_COMPILE_CCS_PIC_C) {
                // moved to LIBRARIES_FOR\CCS_PIC_C\UartLib.c
            } else {
                // moved to LIBRARIES_FOR\Pic16\UartLib.c
            }
        }
        if(SpiFunctionUsed()) {
            if(compiler_variant == MNU_COMPILE_HI_TECH_C) {
                fprintf(f,
                        "\n"
                        "SWORD SPI_SendRecv(SWORD send) {\n"
                        "  return SPI_Send(send);\n"
                        "}\n\n");
                fprintf(f,
                        "void SPI_Write(char *str) {\n"
                        "  int i= 0;\n"
                        "  while(str[i] != 0) {\n"
                        "    SPI_Send(str[i]);\n"
                        "    i++;\n"
                        "   }\n"
                        "}\n\n");
            }
        }
        if(I2cFunctionUsed()) {
            if(compiler_variant == MNU_COMPILE_HI_TECH_C) {
                fprintf(f,
                        "\n"
                        "SWORD I2C_Recv(SBYTE address, SBYTE registr) {\n"
                        "  SWORD recv= 0;\n"
                        "  recv= I2C_MasterGetReg(address, registr);\n"
                        "  return recv;\n"
                        "}\n\n" /*, I2C_Used*/);
                fprintf(f,
                        "void I2C_Send(SBYTE address, SBYTE registr, SWORD send) {\n"
                        "  I2C_MasterSetReg(address, registr, send);\n"
                        "}\n\n" /*, I2C_Used*/);
            }
        }
    } else if(mcu_ISA == ISA_ARM) {
        if(UartFunctionUsed()) {
            if(compiler_variant == MNU_COMPILE_ARMGCC) {
                char pinName[MAX_NAME_LEN] = "";
                GetPinName(Prog.mcu()->uartNeeds.rxPin, pinName); // search for RXn / TXn pins in MCU definnition
                if(strlen(pinName)) {
                    const char *pos = strstr(pinName, "RX");
                    if(pos)
                        UART_Used = atoi(pos + 2); // UART #
                    else {
                        fprintf(f, "    // Bad UART pin configuration in MCU table : using USART0\n");
                        UART_Used = 0; // abnormal
                    }
                }
                // TODO: move UART_xxx function to UartLib.h
                fprintf(f,
                        "\n"
                        "void UART_Transmit(unsigned char data) {\n"
                        "  LibUart_Putc(USART%d, data);\n"
                        "}\n\n",
                        UART_Used);
                fprintf(f,
                        "unsigned char UART_Receive(void) {\n"
                        "  uint8_t c;\n"
                        "  c= LibUart_Getc(USART%d);\n"
                        "  return c;\n"
                        "}\n\n",
                        UART_Used);
                fprintf(f,
                        "BOOLEAN_t UART_Transmit_Ready(void) {\n"
                        "  if (LibUart_Transmit_Ready(USART%d)) return 1;\n"
                        "  else return 0;\n"
                        "}\n\n",
                        UART_Used);
                fprintf(f,
                        "BOOLEAN_t UART_Transmit_Busy(void) {\n"
                        "  if (UART_Transmit_Ready()) return 0;\n"
                        "  else return 1;\n"
                        "}\n\n");
                fprintf(f,
                        "BOOLEAN_t UART_Receive_Avail(void) {\n"
                        "  if (LibUart_Received_Data(USART%d)) return 1;\n"
                        "  else return 0;\n"
                        "}\n\n",
                        UART_Used);
            }
        }
        if(SpiFunctionUsed()) {
            if(compiler_variant == MNU_COMPILE_ARMGCC) {
                fprintf(f,
                        "\n"
                        "SWORD SPI_SendRecv(SPI_TypeDef *SPIx, SWORD send) {\n"
                        "  return LibSPI_Send(SPIx, send);\n"
                        "}\n\n");
                fprintf(f,
                        "void SPI_Write(char *str) {\n"
                        "  LibSPI_WriteMulti(SPI%d, str, strlen(str));\n"
                        "}\n\n",
                        SPI_Used);
            }
        }
        if(I2cFunctionUsed()) {
            if(compiler_variant == MNU_COMPILE_ARMGCC) {
                fprintf(f,
                        "\n"
                        "SWORD I2C_Recv(SBYTE address, SBYTE registr) {\n"
                        "  SWORD recv= 0;\n"
                        "  recv= LibI2C_GetReg(I2C%d, address, registr);\n"
                        "  return recv;\n"
                        "}\n\n",
                        I2C_Used);
                fprintf(f,
                        "void I2C_Send(SBYTE address, SBYTE registr, SWORD send) {\n"
                        "  LibI2C_SetReg(I2C%d, address, registr, send);\n"
                        "}\n\n",
                        I2C_Used);
            }
        }
    }

    //---------------------------------------------------------------------------
    fprintf(f,
            "\n"
            "/* Call this function once per PLC cycle. You are responsible for calling\n"
            "   it at the interval that you specified in the LDmicro MCU configuration when you\n"
            "   generated this code. */\n"
            "void PlcCycle(void) {\n");
    GenerateAnsiC(f, 0, IntCode.size() - 1);
    fprintf(f, "}\n");
    //---------------------------------------------------------------------------
    if(compiler_variant == MNU_COMPILE_ARDUINO) {
        fprintf(f,
                "\n"
                "// PLC Cycle timing function.\n"
                "boolean IsPlcInterval() {\n"
                "    static unsigned long last_run;\n"
                "    unsigned long micros_now = micros();\n"
                "    if (micros_now - last_run >= PLC_INTERVAL) {\n"
                "        last_run = micros_now;\n"
                "        return true;\n"
                "    }\n"
                "    return false;\n"
                "}\n"
                "\n"
                "// Call loopPlc() function in loop() of your arduino project once.\n"
                "void loopPlc() {\n"
                "    if (IsPlcInterval()) {\n"
                "        #ifdef USE_WDT\n"
                "          WDT_Restart();\n"
                "        #endif\n");
        if(Prog.cycleDuty) {
            fprintf(f, "        Write1_Ub_YPlcCycleDuty();\n");
        }
        fprintf(f, "        PlcCycle();\n\n");
        if(Prog.cycleDuty) {
            fprintf(f, "        Write0_Ub_YPlcCycleDuty();\n");
        }
        fprintf(f,
                "    }\n"
                "}\n"
                "\n"
                "// Call setupPlc() function in setup() function of your arduino project once.\n"
                "void setupPlc(void) {\n"
                "   #ifdef USE_WDT\n"
                "     WDT_Init();\n"
                "   #endif\n"
                "// Initialize PLC cycle timer here if you need.\n"
                "// ...\n"
                "\n");
        if(UartFunctionUsed()) {
            Comment("Set up UART");
            fprintf(f,
                    "   Serial.begin(%d);\n"
                    "   while(!Serial) {\n"
                    "       ; // We expect to connect the serial port. It is only necessary for the Leonardo.\n"
                    "   }\n",
                    Prog.baudRate);
        }
        Comment(" Set up analog reference type");
        fprintf(f, "    analogReference(analogReference_type);\n\n");

        Comment(" Set up I/O pins");
        for(int i = 0; i < Prog.io.count; i++) {
            if((Prog.io.assignment[i].type == IO_TYPE_INT_INPUT) || (Prog.io.assignment[i].type == IO_TYPE_DIG_INPUT))
                fprintf(f, "    pinMode(pin_%s, INPUT_PULLUP);\n", MapSym(Prog.io.assignment[i].name, ASBIT));
            else if(Prog.io.assignment[i].type == IO_TYPE_PWM_OUTPUT)
                fprintf(f, "    pinMode(pin_%s, OUTPUT);\n", MapSym(Prog.io.assignment[i].name, ASBIT));
            else if(Prog.io.assignment[i].type == IO_TYPE_DIG_OUTPUT)
                fprintf(f, "    pinMode(pin_%s, OUTPUT);\n", MapSym(Prog.io.assignment[i].name, ASBIT));
        }
        if(SleepFunctionUsed()) {
            fprintf(f,
                    "\n"
                    "    set_sleep_mode(SLEEP_MODE_PWR_DOWN);\n");
        }

        fprintf(f, "}\n");
    } else {
        //---------------------------------------------------------------------------------------
        if(Prog.cycleTime > 0) {
            if(compiler_variant == MNU_COMPILE_ARMGCC) {
                // PlcDelay() function
                fprintf(f,
                        "\n"
                        "// PLC Cycle timing function.\n"
                        "void PlcDelay() {\n"
                        "    while(!Next_Plc_Cycle)\n"
                        "        ;\n"
                        "    Next_Plc_Cycle = 0;\n"
                        "}\n");

                // Timer 3 interrupt routine for PLC Cycles
                fprintf(f,
                        "\n"
                        "void TIM3_Handler() {\n"
                        "    if(TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) {\n"
                        "        Next_Plc_Cycle = 1;\n"
                        "        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);     // acknowledge interrupt\n"
                        "    }\n"
                        "}\n");
            } else if(mcu_ISA == ISA_PIC16) {
                CalcPicPlcCycle(Prog.cycleTime, PicProgLdLen);
                if(plcTmr.softDivisor > 1) {
                    fprintf(f,
                            "\n"
                            "static volatile unsigned char softDivisor = %ld;\n",
                            plcTmr.softDivisor);
                    if(Prog.cycleTimer == 0) {
                        fprintf(f,
                                "\n"
                                "void interrupt ISR()\n"
                                "{\n"
                                "    if(T0IF) {\n"
                                "        TMR0 += %d;\n" // reprogram TMR0
                                "        T0IF = 0;\n"
                                "        if(softDivisor)\n"
                                "           softDivisor--;\n"
                                "    }\n"
                                "}\n",
                                256 - plcTmr.tmr + 1);
                    }
                }
            } else if(mcu_ISA == ISA_PIC18) {
                CalcPicPlcCycle(Prog.cycleTime, PicProgLdLen);
            } else if(mcu_ISA == ISA_AVR) {
                CalcAvrPlcCycle(Prog.cycleTime, AvrProgLdLen);
                if(plcTmr.softDivisor > 1) {
                    fprintf(f,
                            "\n"
                            "static volatile unsigned char softDivisor = %d;\n",
                            plcTmr.softDivisor);
                    if(Prog.cycleTimer == 0) {
                        fprintf(f,
                                "\n"
                                "ISR(TIMER0_COMPA_vect)\n"
                                "{\n"
                                "    if(softDivisor)\n"
                                "        softDivisor--;\n"
                                "}\n");
                    }
                }
            }
        }
        //---------------------------------------------------------------------------------------
        fprintf(f,
                "\n"
                "void setupPlc(void) {\n");

        if((mcu_ISA == ISA_PIC16) || (mcu_ISA == ISA_PIC18))
            fprintf(f,
                    "    // Set ports as digital instead of analogical (default).\n"
                    "    setPortDigitalIO();\n");

        fprintf(f, "    // Set up I/O pins direction, and drive the outputs low to start.\n");

        ///// Modified by JG BYTE[] -> WORD[]
        WORD isInput[MAX_IO_PORTS], isAnsel[MAX_IO_PORTS], isOutput[MAX_IO_PORTS];
        WORD mask = 0;

        if(Prog.mcu()) {
            BuildDirectionRegisters(isInput, isAnsel, isOutput);
            for(int i = 0; i < MAX_IO_PORTS; i++) {
                if(IS_MCU_REG(i)) {
                    if(compiler_variant == MNU_COMPILE_CCS_PIC_C) {
                        // drive the outputs low to start
                        fprintf(f, "    output_%c(0x%02X);\n", 'a' + i, 0x00);
                        // Set up I/O pins direction
                        fprintf(f, "    set_tris_%c(0x%02X);\n", 'a' + i, ~isOutput[i] & 0xff);
                    } else if(compiler_variant == MNU_COMPILE_HI_TECH_C) {
                        // drive the outputs low to start
                        fprintf(f, "    PORT%c = 0x%02X;\n", 'A' + i, 0x00);
                        // Set up I/O pins direction
                        fprintf(f, "    TRIS%c = 0x%02X;\n", 'A' + i, ~isOutput[i] & 0xff);
                    } else if(compiler_variant == MNU_COMPILE_ARMGCC) {
                        // initialisation des ports utilis�s en sortie et en entree (avec pull-up)
                        if(isOutput[i]) {
                            if((Prog.mcu()) && (Prog.mcu()->core == CortexF1))
                                fprintf(f, "    LibGPIO_Conf(GPIO%c, 0x%4.4X, GPIO_Mode_Out_PP, GPIO_Speed_2MHz);\n", 'A' + i, isOutput[i]);
                            else // CortexF4
                                fprintf(f, "    LibGPIO_Conf(GPIO%c, 0x%4.4X, GPIO_Mode_OUT, GPIO_OType_PP, GPIO_Speed_2MHz);\n", 'A' + i, isOutput[i]);
                            // mise � z�ro des sorties
                            fprintf(f, "    GPIO_Write(GPIO%c, 0);\n", 'A' + i);
                        }
                        if(isInput[i]) {                                            // Pull-ups according to Config Bits
                            mask = (isInput[i] & ~Prog.pullUpRegs[i]) ^ isInput[i]; // Pull-ups to enable
                            if(mask) {
                                if((Prog.mcu()) && (Prog.mcu()->core == CortexF1))
                                    fprintf(f, "    LibGPIO_Conf(GPIO%c, 0x%4.4X, GPIO_Mode_IPU, GPIO_Speed_2MHz);\n", 'A' + i, mask);
                                else // CortexF4
                                    fprintf(f, "    LibGPIO_Conf(GPIO%c, 0x%4.4X, GPIO_Mode_IN, GPIO_PuPd_UP, GPIO_Speed_2MHz);\n", 'A' + i, mask);
                            }
                            mask = (isInput[i] & Prog.pullUpRegs[i]) ^ isInput[i]; // Pull-ups to disable
                            if(mask) {
                                if((Prog.mcu()) && (Prog.mcu()->core == CortexF1))
                                    fprintf(f, "    LibGPIO_Conf(GPIO%c, 0x%4.4X, GPIO_Mode_IN_FLOATING, GPIO_Speed_2MHz);\n", 'A' + i, mask);
                                else // CortexF4
                                    fprintf(f, "    LibGPIO_Conf(GPIO%c, 0x%4.4X, GPIO_Mode_IN, GPIO_PuPd_NOPULL, GPIO_Speed_2MHz);\n", 'A' + i, mask);
                            }
                        }
                    } else {
                        //fprintf(f,"      pokeb(0x%X, 0x%X); // PORT%c\n",Prog.mcu()->dirRegs[i], isOutput[i], Prog.mcu()->pinInfo[i].port);
                        //fprintf(f,"    pokeb(0x%X, 0x%X);\n",Prog.mcu()->dirRegs[i], isOutput[i]);
                        fprintf(f, "    DDR%c = 0x%02X;\n", 'A' + i, isOutput[i]);
                        // turn on the pull-ups, and drive the outputs low to start
                        //fprintf(f,"    pokeb(0x%X, 0x%X);\n",Prog.mcu()->outputRegs[i], isInput[i]);
                        fprintf(f, "    PORT%c = 0x%02X;\n", 'A' + i, isInput[i] & Prog.pullUpRegs[i]);
                    }
                }
            }
        }

        if((compiler_variant == MNU_COMPILE_CCS_PIC_C) //
        || (compiler_variant == MNU_COMPILE_HI_TECH_C)) {
            if(Prog.pullUpRegs[1] & 0xFF) { /// if value is not 0 all pull-ups are enabled on Port B
                fprintf(f,
                    "\n"
                    "    // Turn on the pull-ups on Port B.\n"
                    "    PORTB_PULL_UPS_ON();\n"
                    "\n");
            } else { /// else no pull-up at all on Port B (Modified by JG3)
                fprintf(f,
                    "\n"
                    "    // Turn off the pull-ups on Port B.\n"
                    "    PORTB_PULL_UPS_OFF();\n"
                    "\n");
            }
        }
        /*
        if((compiler_variant == MNU_COMPILE_ARMGCC)) {
            // no watchdog
        } else {
            fprintf(f,
                "    #ifdef USE_WDT\n"
                "      // Watchdog on\n"
                "      #ifdef __CODEVISIONAVR__\n"
                "        #ifndef WDTCR\n"
                "            #define WDTCR WDTCSR\n"
                "        #endif\n"
                "        #asm(\"wdr\")\n"
                "        WDTCR |= (1<<WDE) | (1<<WDP2) | (1<<WDP1) | (1<<WDP0);\n"
                "      #elif defined(__GNUC__)\n"
                "        wdt_reset();\n"
                "        wdt_enable(WDTO_2S);\n"
                "      #elif defined(CCS_PIC_C)\n"
                "        setup_wdt(WDT_2304MS);\n"
                "      #elif defined(HI_TECH_C)\n"
                "        //WDTCON=1;\n"
                "      #else\n"
                "        // Watchdog Init is required. // You must provide this.\n"
                "      #endif\n"
                "    #endif\n"
                "\n");
        }
        */
        fprintf(f,
            "    #ifdef USE_WDT\n"
            "      // Watchdog on\n"
            "      WDT_Restart();\n"
            "      WDT_Init();\n"
            "    #endif\n"
            "\n");
        //---------------------------------------------------------------------
        if(Prog.cycleTime > 0) {
            fprintf(f, "    // Initialize PLC cycle timer here.\n"
                       "    // Configure Timer %d\n",
                       Prog.cycleTimer);

            if(compiler_variant == MNU_COMPILE_CCS_PIC_C) {
                fprintf(f,
                        "    #if getenv(\"TIMER%d\") == 0\n"
                        "        #error Don't exist TIMER%d\n"
                        "    #endif\n",
                        Prog.cycleTimer,
                        Prog.cycleTimer);

                if(Prog.cycleTimer == 0) {
                    fprintf(f, "    setup_timer_0(T0_INTERNAL | T0_DIV_%ld);\n", plcTmr.prescaler);
                } else {
                    fprintf(f, "    setup_timer_1(T1_INTERNAL | T1_DIV_BY_%ld);\n", plcTmr.prescaler);
                }
            } else if(compiler_variant == MNU_COMPILE_ARMGCC) {
                // compute timer frequency according to desired Cycle time
                // real Cycle time can be adjusted by modifying declared frequency F
                double fperiod = (double)(Prog.mcuClock);
                if((Prog.mcu()) && (Prog.mcu()->core == CortexF1))
                    fperiod /= 1000000;
                else
                    fperiod /= 1000000;
                fperiod = (fperiod * Prog.cycleTime) / 1000;
                unsigned long period = (unsigned long)fperiod;
                if(period == 0)
                    period = 1;
                if(period > 65535)
                    period = 65535; // securities
                fprintf(f,
                        "    // Init Timer 3 and activate interrupts.\n"
                        "    LibTimer_Init(TIM3, 1000, %lu);\n" // f= (F/4)/[(prediv)*(period)]
                        "    LibTimer_Interrupts(TIM3, ENABLE);\n"
                        "    NVIC_SetPriority(TIM3_IRQn, 0);\n",
                        period);
                if(DelayUsed()) {
                    fprintf(f,
                            "\n"
                            "    // Init the SysTick Timer and activate interrupts for the delay_xs functions.\n"
                            "    // SysTick_Config(SystemCoreClock / 1000000); // 72\n"
                            "    // NVIC_SetPriority(SysTick_IRQn, 0xF);\n");
                }
            } else if(compiler_variant == MNU_COMPILE_HI_TECH_C) {
                if(Prog.cycleTimer == 0) {
                    if(mcu_ISA == ISA_PIC16) {
                        fprintf(f,
                                "    TMR0 = %d;\n"
                                "    PSA = %d;\n"
                                "    T0CS = 0;\n"
                                "    OPTION_REGbits.PS = %d;\n",
                                256 - plcTmr.tmr + 1,
                                plcTmr.prescaler == 1 ? 1 : 0,
                                plcTmr.PS);
                        if(plcTmr.softDivisor > 1) {
                            fprintf(f,
                                    "    GIE = 1;\n"
                                    "    T0IE = 1;\n");
                        }
                    } else { // PIC18
                        fprintf(f,
                                "    TMR0H = 0;\n"
                                "    TMR0L = %ld;\n"
                                "    PSA = %d;\n"
                                "    T0CON = 0x%X;\n\n",
                                256 - plcTmr.tmr + 1,
                                plcTmr.prescaler == 1 ? 1 : 0,
                                0xC0 | plcTmr.PS);
                        /*
                        fprintf(f,
                                "    TMR0H = %d;\n"
                                "    TMR0L = %d;\n"
                                "    T0CON = 0x%X;\n\n",
                                (plcTmr.tmr >> 8) & 0xFF,
                                plcTmr.tmr & 0xFF,
                                0x80 | plcTmr.PS);
                        */
                    }
                } else { // Timer 1
                    fprintf(f,
                            "    CCPR1H = 0x%X;\n"
                            "    CCPR1L = 0x%X;\n"
                            "    TMR1H = 0;\n"
                            "    TMR1L = 0;\n"
                            "    CCP1CON = 0x0B; // compare mode, reset TMR1 on trigger\n"
                            "    T1CON = 0x%2X;\n\n",
                            (plcTmr.tmr >> 8) & 0xFF,
                            plcTmr.tmr & 0xFF,
                            0x80 | plcTmr.PS);

                    if(McuAs(" PIC16F1512 ") //
                    || McuAs(" PIC16F1513 ") //
                    || McuAs(" PIC16F1516 ") //
                    || McuAs(" PIC16F1517 ") //
                    || McuAs(" PIC16F1518 ") //
                    || McuAs(" PIC16F1519 ") //
                    || McuAs(" PIC16F1526 ") //
                    || McuAs(" PIC16F1527 ") //
                    || McuAs(" PIC16F1933 ") //
                    || McuAs(" PIC16F1947 ")) {
                        fprintf(f, "    TMR1GE = 1;\n");
                    }
                }
            } else if(mcu_ISA == ISA_AVR) {
                if(Prog.cycleTimer == 0) {
                    int counter = plcTmr.tmr - 1; // -1 DONE 1000Hz
                    // the counter is less than the divisor at 1
                    if(counter < 0)
                        counter = 0;
                    if(counter > 0xff)
                        counter = 0xff;
                    fprintf(f,
                            "    TCCR0A = 1 << WGM01;\n" // WGM01=1, WGM00=0 // CTC mode
                            "    TCCR0B = %d;\n"         //  WGM02=0 // CTC mode
                            "    OCR0A = %d;\n",
                            plcTmr.cs & 0xff,
                            counter & 0xff);
                    if(plcTmr.softDivisor > 1) {
                        fprintf(f,
                                "    TIMSK0 = 1<<OCIE0A;\n"
                                "    sei();\n");
                    }
                } else {
                    int counter = plcTmr.tmr - 1;
                    fprintf(f,
                            "    TCCR1A = 0x00; // WGM11=0, WGM10=0\n"
                            "    TCCR1B = (1<<WGM12) | %d; // WGM13=0, WGM12=1\n"
                            "    // `the high byte must be written before the low byte\n"
                            "    OCR1AH = (%d >> 8) & 0xff;\n"
                            "    OCR1AL = %d  & 0xff;\n",
                            plcTmr.cs,
                            counter,
                            counter);
                }
            } else {
                fprintf(f, "    //  You must init PLC timer.\n");
            }
        }
        //---------------------------------------------------------------------
        if(UartFunctionUsed()) {
            if(compiler_variant == MNU_COMPILE_ARMGCC) {
                char pinName[MAX_NAME_LEN] = "";
                GetPinName(Prog.mcu()->uartNeeds.rxPin, pinName); // search for RXn / TXn pins in MCU definnition
                if(strlen(pinName)) {
                    const char *pos = strstr(pinName, "RX");
                    if(pos)
                        UART_Used = atoi(pos + 2); // UART #
                    else {
                        fprintf(f, "    // Bad UART pin configuration in MCU table : using USART0\n");
                        UART_Used = 0; // abnormal
                    }
                }
                // Only 1 UART can be used in a same ladder
                fprintf(f, "    UART_Init(%d, %d);\n", UART_Used, Prog.baudRate);
            } else if(compiler_variant == MNU_COMPILE_HI_TECH_C) {
                if(mcu_ISA == ISA_PIC16) {
                    int32_t divisor, brgh;
                    CalcPicUartBaudRate(Prog.mcuClock, Prog.baudRate, &divisor, &brgh);
                    fprintf(f, "    UART_Init(%d, %d);\n", divisor, brgh);
                } else { // PIC18
                    fprintf(f, "    UART_Init(%d);\n", Prog.baudRate);
                }
            } else if(compiler_variant == MNU_COMPILE_AVRGCC) {
                int    divisor = 0;
                double actual = 0;
                double percentErr = 0;
                calcAvrUsart(&divisor, &actual, &percentErr);
                testAvrUsart(divisor, actual, percentErr);
                fprintf(f, "    UART_Init(%d);\n", divisor);
            } else {
                fprintf(f, "    UART_Init(); // baudRate:%d\n", Prog.baudRate);
            }
        }
        if(AdcFunctionUsed()) {
            if(compiler_variant == MNU_COMPILE_HI_TECH_C) {
                int usedadc = 0;

                for(int a = 0; a < MAX_ADC_C; a++) {
                    if(ADC_Used[a] == 1)
                        usedadc++;
                }
                if(usedadc == 0)
                    THROW_COMPILER_EXCEPTION(_("ADC Internal error")); // error in ADC pin layout

                // Max resolution 10 bits used, prediv= 2^1
                fprintf(f, "\n    ADC_Init();\n");

            } else if(compiler_variant == MNU_COMPILE_ARMGCC) {
                int usedadc = 0;

                for(int a = 0; a < MAX_ADC_C; a++) {
                    if(ADC_Used[a] == 1) {
                        // Max resolution 12 bits used
                        if((Prog.mcu()) && (Prog.mcu()->core == CortexF1))
                            fprintf(f, "    LibADC_Init(ADC%d, ADC_Channel_%d);\n", a / 16, ADC_Chan[a]);
                        else
                            fprintf(f, "    LibADC_Init(ADC%d, ADC_Channel_%d, ADC_Resolution_12b);\n", a / 16, ADC_Chan[a]);
                        usedadc++;
                    }
                }
                if(usedadc == 0) {
                    THROW_COMPILER_EXCEPTION(_("ADC Internal error")); // error in ADC pin layout
                }
            } else if(compiler_variant == MNU_COMPILE_AVRGCC) {
                int usedadc = 0;

                for(int a = 0; a < MAX_ADC_C; a++) {
                    if(ADC_Used[a] == 1)
                        usedadc++;
                }
                if(usedadc == 0)
                    THROW_COMPILER_EXCEPTION(_("ADC Internal error")); // error in ADC pin layout

                // Max resolution 10 bits used, prediv= 2^1
                fprintf(f, "\n    ADC_Init(1, 10);\n");

            } else
                fprintf(f, "    ADC_Init();\n");
        }
        if(PwmFunctionUsed()) {

            if(compiler_variant == MNU_COMPILE_HI_TECH_C) {
                int usedpwm = 0;

                fprintf(f, "\n");
                for(int p = 0; p < MAX_PWM_C; p++) {
                    if(PWM_Used[p] == 1) {
                        // PWM 1 uses Timer 1 registers => use Timer 0
                        if((p == 1) && (Prog.cycleTimer == 1))
                            THROW_COMPILER_EXCEPTION(_("Select Timer 0 in menu 'Settings -> MCU parameters'!"));

                        fprintf(f, "    PWM_Init(0x%2.2X, %d, %ld, %d);\n", p, Prog.mcuClock, PWM_Freq[p], PWM_Resol[p]);
                        usedpwm++;
                    }
                }
                if(usedpwm == 0) {
                    THROW_COMPILER_EXCEPTION(_("PWM Internal error")); // error in PWM config
                }

            } else if(compiler_variant == MNU_COMPILE_ARMGCC) {
                int usedpwm = 0;

                fprintf(f, "\n");
                for(int p = 0; p < MAX_PWM_C; p++) {
                    if(PWM_Used[p] == 1) {
                        fprintf(f, "    LibPWM_InitTimer(TIM%d, &TIM%d_Chan%d, %ld);\n", p / 16, p / 16, p % 16, PWM_Freq[p]);
                        if((Prog.mcu()) && (Prog.mcu()->core == CortexF1)) ///// Modified by JG2
                            fprintf(f, "    LibPWM_InitChannel(&TIM%d_Chan%d, LibPWM_Channel_%d);\n", p / 16, p % 16, p % 16);
                        else
                            fprintf(f, "    LibPWM_InitChannel(&TIM%d_Chan%d, LibPWM_Channel_%d, LibPWM_PinsPack_2);\n", p / 16, p % 16, p % 16);
                        usedpwm++;
                    }
                }
                if(usedpwm == 0) {
                    THROW_COMPILER_EXCEPTION(_("PWM Internal error")); // error in PWM config
                }
            } else if(compiler_variant == MNU_COMPILE_AVRGCC) {
                int usedpwm = 0;

                fprintf(f, "\n");
                for(int p = 0; p < MAX_PWM_C; p++) {
                    if(PWM_Used[p] == 1) {
                        fprintf(f, "    PWM_Init(0x%2.2X, %d, %ld, %d, %d);\n", p, Prog.mcuClock, PWM_Freq[p], PWM_Resol[p], PWM_MaxCs[p]);
                        usedpwm++;
                    }
                }
                if(usedpwm == 0) {
                    THROW_COMPILER_EXCEPTION(_("PWM Internal error")); // error in PWM config
                }
            } else
                fprintf(f, "    PWM_Init();\n");
        }

        if(SpiFunctionUsed()) {
            if(compiler_variant == MNU_COMPILE_ARMGCC) {
                // Only 1 SPI can be used in a same ladder
                fprintf(f, "    #define SPI_RatePrescaler LibSPI_GetPrescaler(SPI%d, %d)\n", SPI_Used, Prog.spiRate);
                fprintf(f, "    LibSPI_Init(SPI%d, LibSPI_DataSize_%db, SPI_RatePrescaler);\n", SPI_Used, 8);
            } else if(compiler_variant == MNU_COMPILE_AVRGCC) {
                // Only 1 SPI can be used in a same ladder
                fprintf(flh,
                        "#define SPIPORT\tPORT%c\n"
                        "#define SPIDDR \tDDR%c\n"
                        "#define MOSI\t%d\n"
                        "#define MISO\t%d\n"
                        "#define SCK \t%d\n"
                        "#define SS  \t%d\n"
                        "\n",
                        SPI_Used,
                        SPI_Used,
                        SPI_MOSI,
                        SPI_MISO,
                        SPI_SCK,
                        SPI_SS);

                fprintf(f, "\n");
                fprintf(f, "    SPI_Init(SPI_GetPrescaler(%d, %d));\n", Prog.mcuClock, Prog.spiRate);
            } else if(compiler_variant == MNU_COMPILE_HI_TECH_C) {
                // Only 1 SPI can be used in a same ladder
                fprintf(f, "\n");
                fprintf(f, "    SPI_Init(SPI_GetPrescaler(%d, %d));\n", Prog.mcuClock, Prog.spiRate);
            }
        }

        if(I2cFunctionUsed()) {
            if(compiler_variant == MNU_COMPILE_ARMGCC) {
                // Only 1 I2C can be used in a same ladder
                fprintf(f, "    LibI2C_MasterInit(I2C%d, %d);\n", I2C_Used, Prog.i2cRate);
            } else if((compiler_variant == MNU_COMPILE_AVRGCC) || (compiler_variant == MNU_COMPILE_HI_TECH_C)) {
                // Only 1 I2C can be used in a same ladder
                fprintf(f, "\n");
                fprintf(f, "    I2C_Init(%d, %d);\n", Prog.mcuClock, Prog.i2cRate);
            }
        }

        fprintf(f, "}\n");

        fprintf(f,
                "\n"
                "void mainPlc(void) { // Call mainPlc() function in main() of your project.\n"
                "    // Put your setup code here, to run once, only if you no longer generate C code from LDmicro again.\n");
        if(compiler_variant == MNU_COMPILE_ARMGCC) {
            fprintf(f, "    SystemInit();  // initialize system clock at 100 MHz (F4) or 72 Mhz (F1) // SYSCLK_VALUE=%d, HSE_VALUE=%d\n", Prog.mcuClock, Prog.oscClock);
        }

        fprintf(f,
                "    setupPlc();\n"
                "    while(1) {\n");
        //------------------------------------------------------------------------------------
        fprintf(f, "        // Test PLC cycle Timer %d interval here.\n", Prog.cycleTimer);
        if(compiler_variant == MNU_COMPILE_CCS_PIC_C) {
            fprintf(f,
                    "        while(get_timer%d() < (%ld-1));\n"
                    "        set_timer%d(0); // Try it when the PLC cycle time is more than 1 ms.\n"
                    "      //set_timer%d(1); // Try it when the PLC cycle time is less than 1 ms.\n",
                    Prog.cycleTimer,
                    plcTmr.tmr,
                    Prog.cycleTimer,
                    Prog.cycleTimer);
            fprintf(f, "      //set_timer%d(get_timer%d() - (%ld-1)); // Try it.\n", Prog.cycleTimer, Prog.cycleTimer, plcTmr.tmr);
        } else if(compiler_variant == MNU_COMPILE_HI_TECH_C) {
            if(plcTmr.softDivisor > 1) {
                fprintf(f, "        softDivisorLabel:\n");
            }
            if(Prog.cycleTimer == 0) {
                if(plcTmr.softDivisor == 1) {
                    fprintf(f,
                            "        while(T0IF == 0);\n"
                            "        TMR0 += %d;\n" // reprogram TMR0
                            "        T0IF = 0;\n",
                            256 - plcTmr.tmr + 1);
                }
            } else {
                fprintf(f,
                        "        while(CCP1IF == 0);\n"
                        "        CCP1IF = 0;\n");
                if(plcTmr.softDivisor > 1) {
                    fprintf(f, "        softDivisor--;\n");
                }
            }
            if(plcTmr.softDivisor > 1) {
                fprintf(f,
                        "        if(softDivisor)\n"
                        "            goto softDivisorLabel;\n"
                        "        softDivisor = %d;\n",
                        plcTmr.softDivisor);
            }
        } else if(mcu_ISA == ISA_AVR) {
            if(plcTmr.softDivisor > 1) {
                fprintf(f, "        softDivisorLabel:\n");
            }
            if(Prog.cycleTimer == 0) {
                if(plcTmr.softDivisor == 1) {
                    fprintf(f,
                            "        while((TIFR0 & (1<<OCF0A)) == 0);\n"
                            "        TIFR0 |= 1<<OCF0A;\n" // To clean a bit in the register TIFR0 need write 1 in the corresponding bit!
                    );
                }
            } else {
                fprintf(f,
                        "        while((TIFR1 & (1<<OCF1A)) == 0);\n"
                        "        TIFR1 |= 1<<OCF1A;\n"); // OCF1A can be cleared by writing a logic one to its bit location
            }
            if(plcTmr.softDivisor > 1) {
                fprintf(f,
                        "        if(softDivisor)\n"
                        "            goto softDivisorLabel;\n"
                        "        softDivisor = %d;\n",
                        plcTmr.softDivisor);
            }
        } else if(compiler_variant == MNU_COMPILE_ARMGCC) {
            fprintf(f, "        PlcDelay();\n");
        } else {
            fprintf(f, "        //  You must check PLC timer interval.\n");
        }
        fprintf(f, "\n");
        //-----------------------------------------------------------------------------------------------
        if(Prog.cycleDuty) {
            fprintf(f, "        Write1_Ub_YPlcCycleDuty();\n\n");
        }
        fprintf(f,
                "        #ifdef USE_WDT\n"
                "          WDT_Restart(); // Watchdog reset\n"
                "        #endif\n"
                "\n"
                "        PlcCycle();\n"
                "        // You can place your code here to run repeatedly, only if you no longer generate C code from LDmicro again.\n"
                "        // ...\n");
        if(Prog.cycleDuty) {
            fprintf(f, "\n        Write0_Ub_YPlcCycleDuty();\n");
        }
        fprintf(f,
                "    }\n"
                "}\n"
                "\n");


        if(compiler_variant == MNU_COMPILE_CODEVISIONAVR)
            fprintf(f,
                "// You can use main() as is.\n"
                "void main(void) {\n"
                "    // Put your setup code here, to run once, only if you no longer generate C code from LDmicro again.\n"
                "    mainPlc();\n"
                "    return;\n"
                "}\n");
        else
            fprintf(f,
                "// You can use main() as is.\n"
                "int main(void) {\n"
                "    // Put your setup code here, to run once, only if you no longer generate C code from LDmicro again.\n"
                "    mainPlc();\n"
                "    return 0;\n"
                "}\n");
    }

    fprintf(fh, "#endif\n");

    if(compiler_variant == MNU_COMPILE_ARDUINO) {
        if(all_arduino_pins_are_mapped)
            fprintf(fh, "//");
        fprintf(fh, " You can comment or delete this line after provide the I/O pin mapping for ARDUINO board in %s.\n", ladderhName);
    }
    fprintf(flh, "\n");
    fprintf(flh, "#endif\n");

    if(compiler_variant == MNU_COMPILE_ARDUINO) {
        SetExt(ladderhName, outFile, ".ino");
        FileTracker fino(ladderhName, "w");
        if(!fino) {
            THROW_COMPILER_EXCEPTION_FMT(_("Couldn't open file '%s'"), ladderhName);
            //return;
        }
        fprintf(fino,
                "/* This is example for %s.ino file!\n"
                "   This is auto-generated ARDUINO C code from LDmicro.\n"
                "   Rename this file as %s.ino or copy content(or part) of this file\n"
                "   to existing %s.ino. Remove this comment from %s.ino */\n"
                "\n"
                "#include \"%s.h\"\n"
                "\n"
                "void setup() {\n"
                "  // Put your setup code here, to run once, only if you no longer generate C code from LDmicro again.\n"
                "  setupPlc();\n"
                "}\n"
                "\n"
                "void loop() {\n"
                "  loopPlc();\n"
                "  // You can place your code here to run repeatedly, only if you no longer generate C code from LDmicro again.\n"
                "}\n"
                "\n",
                CurrentLdName,
                CurrentLdName,
                CurrentLdName,
                CurrentLdName,
                CurrentLdName);
    }

    return true;
}
