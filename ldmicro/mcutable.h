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
#ifndef __MCUTABLE_H__
#define __MCUTABLE_H__

#include "stdafx.h"

#include "ldconfig.h"
#include "mcutable.hpp"

// clang-format off

//-----------------------------------------------------------------------------
// AT90USB647( AT90USB646 should also work but I do not have hardware to test ).
McuIoPinInfo AvrAT90USB647_64TQFPIoPinInfo[] = {
    { 'B',  0, 10, "PB0 _SS/PCINT0"       },
    { 'B',  1, 11, "PB1 PCINT1/SCLK"      },
    { 'B',  2, 12, "PB2 PDI/PCINT2/MOSI"  },
    { 'B',  3, 13, "PB3 PDO/PCINT3/MISO"  },
    { 'B',  4, 14, "PB4 PCINT4/OC2A"      },
    { 'B',  5, 15, "PB5 PCINT5/OC1A"      },
    { 'B',  6, 16, "PB6 PCINT6/OC1B"      },
    { 'B',  7, 17, "PB7 PCINT7/OC0A/OC1C" },

    { 'E',  0, 33, "PE0 WR"              },
    { 'E',  1, 34, "PE1 RD"              },
    { 'E',  3,  9, "PE3 IUID"            },
    { 'E',  2, 43, "PE2 ALE/HWB"         },
    { 'E',  4, 18, "PE4 INT4/TOSC1"      },
    { 'E',  5, 19, "PE5 INT5/TOSC2"      },
    { 'E',  6,  1, "PE6 INT6/AIN0"       },
    { 'E',  7,  2, "PE7 INT7/AIN1/UVcon" },

    { 'D',  0, 25, "PD0 OC0B/SCL/INT0" },
    { 'D',  1, 26, "PD1 OC2B/SDA/INT1" },
    { 'D',  2, 27, "PD2 RXD1/INT2"     },
    { 'D',  3, 28, "PD3 TXD1/INT3"     },
    { 'D',  4, 29, "PD4 ICP1"          },
    { 'D',  5, 30, "PD5 XCK1"          },
    { 'D',  6, 31, "PD6 T1"            },
    { 'D',  7, 32, "PD7 T0"            },

    { 'C',  0, 35, "PC0 A8"           },
    { 'C',  1, 36, "PC1 A9"           },
    { 'C',  2, 37, "PC2 A10"          },
    { 'C',  3, 38, "PC3 A11/T3"       },
    { 'C',  4, 39, "PC4 A12/OC3C"     },
    { 'C',  5, 40, "PC5 A13/OC3B"     },
    { 'C',  6, 41, "PC6 A14/OC3A"     },
    { 'C',  7, 42, "PC7 A15/IC3/CLKO" },

    { 'A',  7, 44, "PA7 AD7" },
    { 'A',  6, 45, "PA6 AD6" },
    { 'A',  5, 46, "PA5 AD5" },
    { 'A',  4, 47, "PA4 AD4" },
    { 'A',  3, 48, "PA3 AD3" },
    { 'A',  2, 49, "PA2 AD2" },
    { 'A',  1, 50, "PA1 AD1" },
    { 'A',  0, 51, "PA0 AD0" },

    { 'F',  7, 54, "PF7 ADC7/TDI" },
    { 'F',  6, 55, "PF6 ADC6/TDO" },
    { 'F',  5, 56, "PF5 ADC5/TMS" },
    { 'F',  4, 57, "PF4 ADC4/TCK" },
    { 'F',  3, 58, "PF3 ADC3"     },
    { 'F',  2, 59, "PF2 ADC2"     },
    { 'F',  1, 60, "PF1 ADC1"     },
    { 'F',  0, 61, "PF0 ADC0"     }
};

McuAdcPinInfo AvrAT90USB647_64TQFPAdcPinInfo[] = {
    { 61, 0x00 },
    { 60, 0x01 },
    { 59, 0x02 },
    { 58, 0x03 },
    { 57, 0x04 },
    { 56, 0x05 },
    { 55, 0x06 },
    { 54, 0x07 }
};

//-----------------------------------------------------------------------------
// ATmega2560
McuIoPinInfo AvrAtmega2560_100TQFPIoPinInfo[] = {
//   port bit pin  pinName          ArduinoPin  ArduinoName
//                                              Mega/Mega2560
    { 'E',  0,  2, "PE0 RXD0/PCINT8"     , 0, "0; //RX"   },
    { 'E',  1,  3, "PE1 TXD0"            , 0, "1; //TX"   },
    { 'E',  2,  4, "PE2 AIN0/XCK0"       , 0, ""          },
    { 'E',  3,  5, "PE3 AIN1/OC3A"       , 0, "5; //PWM~" },
    { 'E',  4,  6, "PE4 INT4/OC3B"       , 0, "2; //PWM~" },
    { 'E',  5,  7, "PE5 INT5/OC3C"       , 0, "3; //PWM~" },
    { 'E',  6,  8, "PE6 INT6/T3"         , 0, ""          },
    { 'E',  7,  9, "PE7 INT7/CLKO/ICP3"  , 0, ""          },

    { 'H',  0, 12, "PH0 RXD2"            , 0, "17; //RX2" },
    { 'H',  1, 13, "PH1 TXD2"            , 0, "16; //TX2" },
    { 'H',  2, 14, "PH2 XCK2"            , 0, ""          },
    { 'H',  3, 15, "PH3 OC4A"            , 0, "6; //PWM~" },
    { 'H',  4, 16, "PH4 OC4B"            , 0, "7; //PWM~" },
    { 'H',  5, 17, "PH5 OC4C"            , 0, "8; //PWM~" },
    { 'H',  6, 18, "PH6 OC2B"            , 0, "9; //PWM~" },
    { 'H',  7, 27, "PH7 T4"              , 0, ""          },

    { 'B',  0, 19, "PB0 PCINT0/_SS"      , 0, "53"        },
    { 'B',  1, 20, "PB1 PCINT1/SCK"      , 0, "52"        },
    { 'B',  2, 21, "PB2 PCINT2/MOSI"     , 0, "51"        },
    { 'B',  3, 22, "PB3 PCINT3/MISO"     , 0, "50"        },
    { 'B',  4, 23, "PB4 PCINT4/OC2A"     , 0, "10; //PWM~"},
    { 'B',  5, 24, "PB5 PCINT5/OC1A"     , 0, "11; //PWM~"},
    { 'B',  6, 25, "PB6 PCINT6/OC1B"     , 0, "12; //PWM~"},
    { 'B',  7, 26, "PB7 PCINT7/OC1C/OC0A", 0, "13; //PWM~"},

    { 'L',  0, 35, "PL0 ICP4"            , 0, "49"        },
    { 'L',  1, 36, "PL1 ICP5"            , 0, "48"        },
    { 'L',  2, 37, "PL2 T5"              , 0, "47"        },
    { 'L',  3, 38, "PL3 OC5A"            , 0, "46"        },
    { 'L',  4, 39, "PL4 OC5B"            , 0, "45"        },
    { 'L',  5, 40, "PL5 OC5C"            , 0, "44"        },
    { 'L',  6, 41, "PL6"                 , 0, "43"        },
    { 'L',  7, 42, "PL7"                 , 0, "42"        },

    { 'D',  0, 43, "PD0 INT0/SCL"        , 0, "21; //SCL" },
    { 'D',  1, 44, "PD1 INT1/SDA"        , 0, "20; //SDA" },
    { 'D',  2, 45, "PD2 INT2/RXD1"       , 0, "19; //RX1" },
    { 'D',  3, 46, "PD3 INT3/TXD1"       , 0, "18; //TX1" },
    { 'D',  4, 47, "PD4 ICP1"            , 0, ""          },
    { 'D',  5, 48, "PD5 XCK1"            , 0, ""          },
    { 'D',  6, 49, "PD6 T1"              , 0, ""          },
    { 'D',  7, 50, "PD7 T0"              , 0, "38"        },

    { 'G',  0, 51, "PG0 _WR"             , 0, "41"        },
    { 'G',  1, 52, "PG1 _RD"             , 0, "40"        },
    { 'G',  2, 70, "PG2 ALE"             , 0, "39"        },
    { 'G',  3, 28, "PG3 TOSC2"           , 0, ""          },
    { 'G',  4, 29, "PG4 TOSC1"           , 0, ""          },
    { 'G',  5,  1, "PG5 OC0B"            , 0, "4; //PWM~" },

    { 'C',  0, 53, "PC0 A8"              , 0, "37"        },
    { 'C',  1, 54, "PC1 A9"              , 0, "36"        },
    { 'C',  2, 55, "PC2 A10"             , 0, "35"        },
    { 'C',  3, 56, "PC3 A11"             , 0, "34"        },
    { 'C',  4, 57, "PC4 A12"             , 0, "33"        },
    { 'C',  5, 58, "PC5 A13"             , 0, "32"        },
    { 'C',  6, 59, "PC6 A14"             , 0, "31"        },
    { 'C',  7, 60, "PC7 A15"             , 0, "30"        },

    { 'J',  0, 63, "PJ0 PCINT9/RXD3"     , 0, "15; //RX3" },
    { 'J',  1, 64, "PJ1 PCINT10/TXD3"    , 0, "14; //TX3" },
    { 'J',  2, 65, "PJ2 PCINT11/XCK3"    , 0, ""          },
    { 'J',  3, 66, "PJ3 PCINT12"         , 0, ""          },
    { 'J',  4, 67, "PJ4 PCINT13"         , 0, ""          },
    { 'J',  5, 68, "PJ5 PCINT14"         , 0, ""          },
    { 'J',  6, 69, "PJ6 PCINT15"         , 0, ""          },
    { 'J',  7, 79, "PJ7"                 , 0, ""          },

    { 'A',  0, 78, "PA0 AD8"             , 0, "22"        },
    { 'A',  1, 77, "PA1 AD9"             , 0, "23"        },
    { 'A',  2, 76, "PA2 AD10"            , 0, "24"        },
    { 'A',  3, 75, "PA3 AD11"            , 0, "25"        },
    { 'A',  4, 74, "PA4 AD12"            , 0, "26"        },
    { 'A',  5, 73, "PA5 AD13"            , 0, "27"        },
    { 'A',  6, 72, "PA6 AD14"            , 0, "28"        },
    { 'A',  7, 71, "PA7 AD15"            , 0, "29"        },

    { 'K',  0, 89, "PK0 ADC8/PCINT16"    , 0, "A8"        }, // ADC or I/O
    { 'K',  1, 88, "PK1 ADC9/PCINT17"    , 0, "A9"        },
    { 'K',  2, 87, "PK2 ADC10/PCINT18"   , 0, "A10"       },
    { 'K',  3, 86, "PK3 ADC11/PCINT19"   , 0, "A11"       },
    { 'K',  4, 85, "PK4 ADC12/PCINT20"   , 0, "A12"       },
    { 'K',  5, 84, "PK5 ADC13/PCINT21"   , 0, "A13"       },
    { 'K',  6, 83, "PK6 ADC14/PCINT22"   , 0, "A14"       },
    { 'K',  7, 82, "PK7 ADC15/PCINT23"   , 0, "A15"       },

    { 'F',  0, 97, "PF0 ADC0"            , 0, "A0"        }, // ADC or I/O
    { 'F',  1, 96, "PF1 ADC1"            , 0, "A1"        },
    { 'F',  2, 95, "PF2 ADC2"            , 0, "A2"        },
    { 'F',  3, 94, "PF3 ADC3"            , 0, "A3"        },
    { 'F',  4, 93, "PF4 ADC4/TCK"        , 0, "A4"        },
    { 'F',  5, 92, "PF5 ADC5/TMS"        , 0, "A5"        },
    { 'F',  6, 91, "PF6 ADC6/TDO"        , 0, "A6"        },
    { 'F',  7, 90, "PF7 ADC7/TDI"        , 0, "A7"        }
};

McuAdcPinInfo AvrAtmega2560_100TQFPAdcPinInfo[] = {
    { 97, 0x00 },
    { 96, 0x01 },
    { 95, 0x02 },
    { 94, 0x03 },
    { 93, 0x04 },
    { 92, 0x05 },
    { 91, 0x06 },
    { 90, 0x07 },
    { 89, 0x08 },
    { 88, 0x09 },
    { 87, 0x0A },
    { 86, 0x0B },
    { 85, 0x0C },
    { 84, 0x0D },
    { 83, 0x0E },
    { 82, 0x0F }
};

//-----------------------------------------------------------------------------
// ATmega128 or ATmega64

McuIoPinInfo AvrAtmega128_64TQFPIoPinInfo[] = {
    { 'E',  0,  2 , "PE0 RXD0/PDI"  },
    { 'E',  1,  3 , "PE1 TXD0/PDO"  },
    { 'E',  2,  4 , "PE2 XCK0/AIN0" },
    { 'E',  3,  5 , "PE3 OC3A/AIN1" },
    { 'E',  4,  6 , "PE4 OC3B/INT4" },
    { 'E',  5,  7 , "PE5 OC3C/INT5" },
    { 'E',  6,  8 , "PE6 T3/INT6"   },
    { 'E',  7,  9 , "PE7 ICP3/INT7" },

    { 'B',  0, 10 , "PB0 _SS"      },
    { 'B',  1, 11 , "PB1 SCK"      },
    { 'B',  2, 12 , "PB2 MOSI"     },
    { 'B',  3, 13 , "PB3 MISO"     },
    { 'B',  4, 14 , "PB4 OC0 "     },
    { 'B',  5, 15 , "PB5 OC1A"     },
    { 'B',  6, 16 , "PB6 OC1B"     },
    { 'B',  7, 17 , "PB7 OC2/OC1C" },

    { 'G',  0, 33 , "PG0 WR"    },
    { 'G',  1, 34 , "PG1 RD"    },
    { 'G',  2, 43 , "PG2 ALE"   },
    { 'G',  3, 18 , "PG3 TOSC2" },
    { 'G',  4, 19 , "PG4 TOSC1" },

    { 'D',  0, 25 , "PD0 SCL/INT0"  },
    { 'D',  1, 26 , "PD1 SDA/INT1"  },
    { 'D',  2, 27 , "PD2 RXD1/INT2" },
    { 'D',  3, 28 , "PD3 TXD1/INT3" },
    { 'D',  4, 29 , "PD4 ICP1"      },
    { 'D',  5, 30 , "PD5 XCK1"      },
    { 'D',  6, 31 , "PD6 T1"        },
    { 'D',  7, 32 , "PD7 T2"        },


    { 'C',  0, 35 , "PC0 A8"  },
    { 'C',  1, 36 , "PC1 A9"  },
    { 'C',  2, 37 , "PC2 A10" },
    { 'C',  3, 38 , "PC3 A11" },
    { 'C',  4, 39 , "PC4 A12" },
    { 'C',  5, 40 , "PC5 A13" },
    { 'C',  6, 41 , "PC6 A14" },
    { 'C',  7, 42 , "PC7 A15" },

    { 'A',  7, 44 , "PA7 AD7" },
    { 'A',  6, 45 , "PA6 AD6" },
    { 'A',  5, 46 , "PA5 AD5" },
    { 'A',  4, 47 , "PA4 AD4" },
    { 'A',  3, 48 , "PA3 AD3" },
    { 'A',  2, 49 , "PA2 AD2" },
    { 'A',  1, 50 , "PA1 AD1" },
    { 'A',  0, 51 , "PA0 AD0" },

    { 'F',  7, 54 , "PF7 ADC7/TDI" },
    { 'F',  6, 55 , "PF6 ADC6/TDO" },
    { 'F',  5, 56 , "PF5 ADC5/TMS" },
    { 'F',  4, 57 , "PF4 ADC4/TCK" },
    { 'F',  3, 58 , "PF3 ADC3"     },
    { 'F',  2, 59 , "PF2 ADC2"     },
    { 'F',  1, 60 , "PF1 ADC1"     },
    { 'F',  0, 61 , "PF0 ADC0"     }
};

McuAdcPinInfo AvrAtmega128_64TQFPAdcPinInfo[] = {
    { 61, 0x00 },
    { 60, 0x01 },
    { 59, 0x02 },
    { 58, 0x03 },
    { 57, 0x04 },
    { 56, 0x05 },
    { 55, 0x06 },
    { 54, 0x07 }
};


//-----------------------------------------------------------------------------
// ATmega162

McuIoPinInfo AvrAtmega162IoPinInfo[] = {
    { 'B',  0,  1 , "PB0 OC0/T0"          },
    { 'B',  1,  2 , "PB1 OC2/T1"          },
    { 'B',  2,  3 , "PB2 RXD1/AIN0"       },
    { 'B',  3,  4 , "PB3 TXD1/AIN1"       },
    { 'B',  4,  5 , "PB4 _SS/OC3B"        },
    { 'B',  5,  6 , "PB5 MOSI"            },
    { 'B',  6,  7 , "PB6 MISO"            },
    { 'B',  7,  8 , "PB7 SCK"             },
    { 'D',  0, 10 , "PD0 RXD0"            },
    { 'D',  1, 11 , "PD1 TXD0"            },
    { 'D',  2, 12 , "PD2 INT0/XCK1"       },
    { 'D',  3, 13 , "PD3 INT1/ICP3"       },
    { 'D',  4, 14 , "PD4 TOSC1/XCK0/OC3A" },
    { 'D',  5, 15 , "PD5 OC1A/TOSC2"      },
    { 'D',  6, 16 , "PD6 _WR"             },
    { 'D',  7, 17 , "PD7 _RD"             },
    { 'C',  0, 21 , "PC0 A8/PCINT8"       },
    { 'C',  1, 22 , "PC1 A9/PCINT9"       },
    { 'C',  2, 23 , "PC2 A10/PCINT10"     },
    { 'C',  3, 24 , "PC3 A11/PCINT11"     },
    { 'C',  4, 25 , "PC4 A12/TCK/PCINT12" },
    { 'C',  5, 26 , "PC5 A13/TMS/PCINT13" },
    { 'C',  6, 27 , "PC6 A14/TDO/PCINT14" },
    { 'C',  7, 28 , "PC7 A15/TDI/PCINT15" },
    { 'E',  2, 29 , "PE2 OC1B"            },
    { 'E',  1, 30 , "PE1 ALE"             },
    { 'E',  0, 31 , "PE0 ICP1/INT2"       },
    { 'A',  7, 32 , "PA7 AD7/PCINT7"      },
    { 'A',  6, 33 , "PA6 AD6/PCINT6"      },
    { 'A',  5, 34 , "PA5 AD5/PCINT5"      },
    { 'A',  4, 35 , "PA4 AD4/PCINT4"      },
    { 'A',  3, 36 , "PA3 AD3/PCINT3"      },
    { 'A',  2, 37 , "PA2 AD2/PCINT2"      },
    { 'A',  1, 38 , "PA1 AD1/PCINT1"      },
    { 'A',  0, 39 , "PA0 AD0/PCINT0"      }
};


//-----------------------------------------------------------------------------
// ATmega16 or ATmega32

McuIoPinInfo AvrAtmega16or32IoPinInfo[] = {
    { 'B',  0,  1, "PB0 T0/XCK"    },
    { 'B',  1,  2, "PB1 T1"        },
    { 'B',  2,  3, "PB2 INT2/AIN0" },
    { 'B',  3,  4, "PB3 OC0/AIN1"  },
    { 'B',  4,  5, "PB4 _SS"       },
    { 'B',  5,  6, "PB5 MOSI"      },
    { 'B',  6,  7, "PB6 MISO"      },
    { 'B',  7,  8, "PB7 SCK"       },
    { 'D',  0, 14, "PD0 RXD"       },
    { 'D',  1, 15, "PD1 TXD"       },
    { 'D',  2, 16, "PD2 INT0"      },
    { 'D',  3, 17, "PD3 INT1"      },
    { 'D',  4, 18, "PD4 OC1B"      },
    { 'D',  5, 19, "PD5 OC1A"      },
    { 'D',  6, 20, "PD6 ICP1"      },
    { 'D',  7, 21, "PD7 OC2"       },
    { 'C',  0, 22, "PC0 SCL"       },
    { 'C',  1, 23, "PC1 SDA"       },
    { 'C',  2, 24, "PC2 TCK"       },
    { 'C',  3, 25, "PC3 TMS"       },
    { 'C',  4, 26, "PC4 TDO"       },
    { 'C',  5, 27, "PC5 TDI"       },
    { 'C',  6, 28, "PC6 TOSC1"     },
    { 'C',  7, 29, "PC7 TOSC2"     },
    { 'A',  0, 40, "PA0 ADC0"      },
    { 'A',  1, 39, "PA1 ADC1"      },
    { 'A',  2, 38, "PA2 ADC2"      },
    { 'A',  3, 37, "PA3 ADC3"      },
    { 'A',  4, 36, "PA4 ADC4"      },
    { 'A',  5, 35, "PA5 ADC5"      },
    { 'A',  6, 34, "PA6 ADC6"      },
    { 'A',  7, 33, "PA7 ADC7"      }
};

McuAdcPinInfo AvrAtmega16or32AdcPinInfo[] = {
    { 40, 0x00 },
    { 39, 0x01 },
    { 38, 0x02 },
    { 37, 0x03 },
    { 36, 0x04 },
    { 35, 0x05 },
    { 34, 0x06 },
    { 33, 0x07 }
};

//-----------------------------------------------------------------------------
// ATmega16 or ATmega32 in 44-Pin packages
McuIoPinInfo AvrAtmega16or32IoPinInfo44[] = {
    { 'B',  0, 40, "PB0 T0/XCK"    },
    { 'B',  1, 41, "PB1 T1"        },
    { 'B',  2, 42, "PB2 INT2/AIN0" },
    { 'B',  3, 43, "PB3 OC0/AIN1"  },
    { 'B',  4, 44, "PB4 _SS"       },
    { 'B',  5,  1, "PB5 MOSI"      },
    { 'B',  6,  2, "PB6 MISO"      },
    { 'B',  7,  3, "PB7 SCK"       },
    { 'D',  0,  9, "PD0 RXD"       },
    { 'D',  1, 10, "PD1 TXD"       },
    { 'D',  2, 11, "PD2 INT0"      },
    { 'D',  3, 12, "PD3 INT1"      },
    { 'D',  4, 13, "PD4 OC1B"      },
    { 'D',  5, 14, "PD5 OC1A"      },
    { 'D',  6, 15, "PD6 ICP1"      },
    { 'D',  7, 16, "PD7 OC2"       },
    { 'C',  0, 19, "PC0 SCL"       },
    { 'C',  1, 20, "PC1 SDA"       },
    { 'C',  2, 21, "PC2 TCK"       },
    { 'C',  3, 22, "PC3 TMS"       },
    { 'C',  4, 23, "PC4 TDO"       },
    { 'C',  5, 24, "PC5 TDI"       },
    { 'C',  6, 25, "PC6 TOSC1"     },
    { 'C',  7, 26, "PC7 TOSC2"     },
    { 'A',  0, 37, "PA0 ADC0"      },
    { 'A',  1, 36, "PA1 ADC1"      },
    { 'A',  2, 35, "PA2 ADC2"      },
    { 'A',  3, 34, "PA3 ADC3"      },
    { 'A',  4, 33, "PA4 ADC4"      },
    { 'A',  5, 32, "PA5 ADC5"      },
    { 'A',  6, 31, "PA6 ADC6"      },
    { 'A',  7, 30, "PA7 ADC7"      }
};

McuAdcPinInfo AvrAtmega16or32AdcPinInfo44[] = {
    { 37, 0x00 },
    { 36, 0x01 },
    { 35, 0x02 },
    { 34, 0x03 },
    { 33, 0x04 },
    { 32, 0x05 },
    { 31, 0x06 },
    { 30, 0x07 }
};

//-----------------------------------------------------------------------------
// ATmega16U4 or ATmega32U4 in 44-Pin packages
McuIoPinInfo AvrAtmega16U4or32U4IoPinInfo44[] = {
//   port bit pin  pinName                  ArduinoPin  ArduinoName
//                                                   X  Leonardo
    { 'B',  0,  8, "PB0 PCINT0/_SS"             ,  8, "17; //RXLED"      },
    { 'B',  1,  9, "PB1 PCINT1/SCL"             ,  9, ""                 },
    { 'B',  2, 10, "PB2 PCINT2/PDI/MOSI"        , 10, ""                 },
    { 'B',  3, 11, "PB3 PCINT3/PDO/MISO"        , 11, ""                 },
    { 'B',  4, 28, "PB4 PCINT4/ADC11"           , 28, "8"                },
    { 'B',  5, 29, "PB5 PCINT5/OC1A/OC4B/ADC12" , 29, "9; //PWM~"        },
    { 'B',  6, 30, "PB6 PCINT6/OC1B/_OC4B/ADC13", 30, "10; //PWM~"       },
    { 'B',  7, 12, "PB7 PCINT7/OC1C/OC0A/_RTS"  , 12, "11; //PWM~"       },
    { 'D',  0, 18, "PD0 INT0/OC0B/SCL"          , 18, "3; //PWM~"        },
    { 'D',  1, 19, "PD1 INT1/SDA"               , 19, "2"                },
    { 'D',  2, 20, "PD2 INT2/RXD1"              , 20, "0"                },
    { 'D',  3, 21, "PD3 INT3/TXD1"              , 21, "1"                },
    { 'D',  4, 25, "PD4 ICP1/ADC8"              , 25, "4"                },
    { 'D',  5, 22, "PD5 XCK1/_CTS"              , 22, "30; //TXLED"      },
    { 'D',  6, 26, "PD6 T1/_OC4D/ADC9"          , 26, "12"               },
    { 'D',  7, 27, "PD7 T0/OC4D/ADC10"          , 27, "6; //PWM~"        },
    { 'C',  6, 31, "PC6 OC3A/_OC4A"             , 31, "5; //PWM~"        },
    { 'C',  7, 32, "PC7 ICP3/CLK0/OC4A"         , 32, "13; //PWM~ //LED" },
    { 'F',  0, 41, "PF0 ADC0"                   , 41, "A0"               },
    { 'F',  1, 40, "PF1 ADC1"                   , 40, "A1"               },
    { 'F',  4, 39, "PF4 ADC4/TCK"               , 39, "A4"               },
    { 'F',  5, 38, "PF5 ADC5/TMS"               , 38, "A5"               },
    { 'F',  6, 37, "PF6 ADC6/TDO"               , 37, "A6"               },
    { 'F',  7, 36, "PF7 ADC7/TDI"               , 36, "A7"               },
    { 'E',  2, 33, "PE2 _HWB"                   , 33, ""                 },
    { 'E',  6,  1, "PE6 INT6/AIN0"              ,  1, "7"                }
};

McuAdcPinInfo AvrAtmega16U4or32U4AdcPinInfo44[] = {
    { 41, 0x00 },
    { 40, 0x01 },
    { 39, 0x04 },
    { 38, 0x05 },
    { 37, 0x06 },
    { 36, 0x07 },
    { 25, 0x20 },
    { 26, 0x21 },
    { 27, 0x22 },
    { 28, 0x23 },
    { 29, 0x24 },
    { 30, 0x25 }
};

//-----------------------------------------------------------------------------
// ATmega8 PDIP-28

McuIoPinInfo AvrAtmega8IoPinInfo28[] = {
//   port bit pin  pinName        ArduinoPin  ArduinoName
//                                         X
    { 'D',  0,  2, "PD0 RXD"          ,  2, "0"  },
    { 'D',  1,  3, "PD1 TXD"          ,  3, "1"  },
    { 'D',  2,  4, "PD2 INT0"         ,  4, "2"  },
    { 'D',  3,  5, "PD3 INT1"         ,  5, "3"  },
    { 'D',  4,  6, "PD4 XCK/T0"       ,  6, "4"  },
    { 'D',  5, 11, "PD5 T1"           , 11, "5"  },
    { 'D',  6, 12, "PD6 AIN0"         , 12, "6"  },
    { 'D',  7, 13, "PD7 AIN1"         , 13, "7"  },
    { 'B',  0, 14, "PB0 ICP1"         , 14, "8"  },
    { 'B',  1, 15, "PB1 OC1A"         , 15, "9"  },
    { 'B',  2, 16, "PB2 _SS/OC1B"     , 16, "10" },
    { 'B',  3, 17, "PB3 MOSI/OC2"     , 17, "11" },
    { 'B',  4, 18, "PB4 MISO"         , 18, "12" },
    { 'B',  5, 19, "PB5 SCK"          , 19, "13" },
    { 'B',  6,  9, "PB6 XTAL1/TOSC1"  ,  9, ""   },
    { 'B',  7, 10, "PB7 XTAL2/TOSC2"  , 10, ""   },
    { 'C',  0, 23, "PC0 ADC0"         , 23, "A0" },
    { 'C',  1, 24, "PC1 ADC1"         , 24, "A1" },
    { 'C',  2, 25, "PC2 ADC2"         , 25, "A2" },
    { 'C',  3, 26, "PC3 ADC3"         , 26, "A3" },
    { 'C',  4, 27, "PC4 ADC4/SDA"     , 27, "A4" },
    { 'C',  5, 28, "PC5 ADC5/SCL"     , 28, "A5" }
//  { 'C',  6,  1, "PC6 _RESET"       ,  1, ""   }
};

//-----------------------------------------------------------------------------
McuIoPinInfo AvrAtmega8IoPinInfo[] = {
    { 'D',  0,  2, "PD0 RXD"         },
    { 'D',  1,  3, "PD1 TXD"         },
    { 'D',  2,  4, "PD2 INT0"        },
    { 'D',  3,  5, "PD3 INT1"        },
    { 'D',  4,  6, "PD4 XCK/T0"      },
    { 'D',  5, 11, "PD5 T1"          },
    { 'D',  6, 12, "PD6 AIN0"        },
    { 'D',  7, 13, "PD7 AIN1"        },
    { 'B',  0, 14, "PB0 ICP1"        },
    { 'B',  1, 15, "PB1 OC1A"        },
    { 'B',  2, 16, "PB2 _SS/OC1B"    },
    { 'B',  3, 17, "PB3 MOSI/OC2"    },
    { 'B',  4, 18, "PB4 MISO"        },
    { 'B',  5, 19, "PB5 SCK"         },
    { 'B',  6,  9, "PB6 XTAL1/TOSC1" },
    { 'B',  7, 10, "PB7 XTAL2/TOSC2" },
    { 'C',  0, 23, "PC0 ADC0"        },
    { 'C',  1, 24, "PC1 ADC1"        },
    { 'C',  2, 25, "PC2 ADC2"        },
    { 'C',  3, 26, "PC3 ADC3"        },
    { 'C',  4, 27, "PC4 ADC4/SDA"    },
    { 'C',  5, 28, "PC5 ADC5/SCL"    }
//  { 'C',  6,  1, "PC6 _RESET"      }
};

//-----------------------------------------------------------------------------
// ATmega328 PDIP-28

McuIoPinInfo AvrAtmega328IoPinInfo[] = {
//   port bit pin  pinName                 ArduinoPin  ArduinoName
//                                                  X
    { 'D',  0,  2, "PD0 PCINT16/RXD"       ,  2, "0; //RX<-"  },
    { 'D',  1,  3, "PD1 PCINT17/TXD"       ,  3, "1; //TX->"  },
    { 'D',  2,  4, "PD2 PCINT18/INT0"      ,  4, "2"          },
    { 'D',  3,  5, "PD3 PCINT19/OC2B/INT1" ,  5, "3; //PWM~"  },
    { 'D',  4,  6, "PD4 PCINT20/XCK/T0"    ,  6, "4"          },
    { 'D',  5, 11, "PD5 PCINT21/OC0B/T1"   , 11, "5; //PWM~"  },
    { 'D',  6, 12, "PD6 PCINT22/OC0A/AIN0" , 12, "6; //PWM~"  },
    { 'D',  7, 13, "PD7 PCINT23/AIN1"      , 13, "7"          },
    { 'B',  0, 14, "PB0 PCINT0/CLKO/ICP1"  , 14, "8"          },
    { 'B',  1, 15, "PB1 OC1A/PCINT1"       , 15, "9; //PWM~"  },
    { 'B',  2, 16, "PB2 _SS/OC1B/PCINT2"   , 16, "10; //PWM~" },
    { 'B',  3, 17, "PB3 MOSI/OC2A/PCINT3"  , 17, "11; //PWM~" },
    { 'B',  4, 18, "PB4 MISO/PCINT4"       , 18, "12"         },
    { 'B',  5, 19, "PB5 SCK/PCINT5"        , 19, "13"         },
//              7   VCC
//              8   GND
    { 'B',  6,  9, "PB6 PCINT6/XTAL1/TOSC1",  9, "crystal"    },
    { 'B',  7, 10, "PB7 PCINT7/XTAL2/TOSC2", 10, "crystal"    },
//             20   AVCC
//             21   AREF
//             22    GND
    { 'C',  0, 23, "PC0 ADC0/PCINT8"       , 23, "A0"         },
    { 'C',  1, 24, "PC1 ADC1/PCINT9"       , 24, "A1"         },
    { 'C',  2, 25, "PC2 ADC2/PCINT10"      , 25, "A2"         },
    { 'C',  3, 26, "PC3 ADC3/PCINT11"      , 26, "A3"         },
    { 'C',  4, 27, "PC4 ADC4/SDA/PCINT12"  , 27, "A4"         },
    { 'C',  5, 28, "PC5 ADC5/SCL/PCINT13"  , 28, "A5"         },
//  { 'C',  6,  1, "PC6 PCINT14/_RESET"    ,  1, "reset"      }
};

McuAdcPinInfo AvrAtmega8AdcPinInfo[] = {
    { 23, 0x00 }, // ADC0
    { 24, 0x01 },
    { 25, 0x02 },
    { 26, 0x03 },
    { 27, 0x04 },
    { 28, 0x05 }  // ADC5
};


//-----------------------------------------------------------------------------
// ATtiny10 6-Pin packages
McuIoPinInfo AvrATtiny10IoPinInfo6[] = {
    { 'B',  0,  1, "PB0 PCINT0/ADC0/OC0A/TPIDATA/AIN0"          }, // {char port;  int bit;  int pin;}
    { 'B',  1,  3, "PB1 PCINT1/ADC1/OC0B/CLKI/TPICLK/ICP0/AIN1" },
    { 'B',  2,  4, "PB2 PCINT2/ADC2/INT0/CLKO/T0"               },
    { 'B',  3,  6, "PB3 PCINT3/ADC3/RESET"                      }
};

McuAdcPinInfo AvrATtiny10AdcPinInfo6[] = {
    {  1, 0x00 }, // ADC0 {int pin;   BYTE muxRegValue;}
    {  3, 0x01 },
    {  4, 0x02 },
    {  6, 0x03 }  // ADC3
};

McuExtIntPinInfo AvrExtIntPinInfo6[] = {
    { 1 }, // PCINT0
    { 3 },
    { 4 },
    { 6 }
};

//-----------------------------------------------------------------------------
// ATtiny85 8-Pin packages
McuIoPinInfo AvrATtiny85IoPinInfo8[] = {
    { 'B',  0,  5, "PB0 PCINT0/MOSI/DI/SDA/AIN0/OC0A/_OC1A/AREF" },
    { 'B',  1,  6, "PB1 PCINT1/MISO/DO/AIN1/OC0B/OC1A"           },
    { 'B',  2,  7, "PB2 PCINT2/ADC1/SCK/USCK/SCL/T0/INT0"        },
    { 'B',  3,  2, "PB3 PCINT3/ADC3/XTAL1/CLKI/_OC1B"            },
    { 'B',  4,  3, "PB4 PCINT4/ADC2/XTAL2/CLKO/OC1B"             },
    { 'B',  5,  1, "PB5 PCINT5/ADC0/_RESET/dW"                   }
};

McuAdcPinInfo AvrATtiny85AdcPinInfo8[] = {
    {  1, 0x00 }, // ADC0 {int pin;   BYTE muxRegValue;}
    {  7, 0x01 },
    {  3, 0x02 },
    {  2, 0x03 }  // ADC3
};

McuExtIntPinInfo AvrExtIntPinInfo8[] = {
    { 5 }, // PCINT0
    { 6 },
    { 7 },
    { 2 },
    { 3 },
    { 1 }
};

McuIoPinInfo AvrATtiny85IoPinInfo20[] = {
//   port bit pin  pinName                                  ArduinoPin  ArduinoName
//                                                                   X  Gemma
    { 'B',  0, 11, "PB0 PCINT0/MOSI/DI/SDA/AIN0/OC0A/_OC1A/AREF", 11, "0 // D0/PWM~/SDA"     },
    { 'B',  1, 12, "PB1 PCINT1/MISO/DO/AIN1/OC0B/OC1A"          , 12, "1 // D1/PWM~/SCL/LED" },
    { 'B',  2, 14, "PB2 PCINT2/ADC1/SCK/USCK/SCL/T0/INT0"       , 14, "2 // D2/A1"           },
    { 'B',  3,  2, "PB3 PCINT3/ADC3/XTAL1/CLKI/_OC1B"           ,  2, ""                     },
    { 'B',  4,  5, "PB4 PCINT4/ADC2/XTAL2/CLKO/OC1B"            ,  5, ""                     },
    { 'B',  5,  1, "PB5 PCINT5/ADC0/_RESET/dW"                  ,  1, "RESET"                }
};

McuAdcPinInfo AvrATtiny85AdcPinInfo20[] = {
    {  1, 0x00 }, // ADC0 {int pin;   BYTE muxRegValue;}
    { 14, 0x01 },
    {  5, 0x02 },
    {  2, 0x03 }  // ADC3
};

//-----------------------------------------------------------------------------
// ATmega8 32-Pin packages TQFP/QFN/MLF

McuIoPinInfo AvrAtmega8IoPinInfo32[] = {
    { 'D',  0, 30, "PD0 RXD"          }, // {char port;  int bit;  int pin;}
    { 'D',  1, 31, "PD1 TXD"          },
    { 'D',  2, 32, "PD2 INT0"         },
    { 'D',  3,  1, "PD3 INT1"         },
    { 'D',  4,  2, "PD4 XCK/T0"       },
    { 'D',  5,  9, "PD5 T1"           },
    { 'D',  6, 10, "PD6 AIN0"         },
    { 'D',  7, 11, "PD7 AIN1"         },
    { 'B',  0, 12, "PB0 ICP1"         },
    { 'B',  1, 13, "PB1 OC1A"         },
    { 'B',  2, 14, "PB2 OC1B/_SS"     },
    { 'B',  3, 15, "PB3 MOSI/OC2"     },
    { 'B',  4, 16, "PB4 MISO"         },
    { 'B',  5, 17, "PB5 SCK"          },
    { 'B',  6,  7, "PB6 XTAL1/TOSC1"  },
    { 'B',  7,  8, "PB7 XTAL2/TOSC2"  },
    { 'C',  0, 23, "PC0 ADC0"         },
    { 'C',  1, 24, "PC1 ADC1"         },
    { 'C',  2, 25, "PC2 ADC2"         },
    { 'C',  3, 26, "PC3 ADC3"         },
    { 'C',  4, 27, "PC4 ADC4/SDA"     },
    { 'C',  5, 28, "PC5 ADC5/SCL"     },
    { 'C',  6, 29, "PC6 RESET"        }
};

McuAdcPinInfo AvrAtmega8AdcPinInfo32[] = {
    { 23, 0x00 }, // ADC0 {int pin;   BYTE muxRegValue;}
    { 24, 0x01 },
    { 25, 0x02 },
    { 26, 0x03 },
    { 27, 0x04 },
    { 28, 0x05 }, // ADC5
    { 19, 0x06 },
    { 22, 0x07 }  // ADC7
};

McuExtIntPinInfo AvrExtIntPinInfo32[] = {
    { 32 }, // PD2/INT0
    {  1 }  // PD3/INT1
};

//-----------------------------------------------------------------------------
// PIC's
McuExtIntPinInfo PicExtIntPinInfo6[] = {
    {    }, //
};

McuExtIntPinInfo PicExtIntPinInfo14[] = {
    { 11 }, // INT
};

McuExtIntPinInfo PicExtIntPinInfo18[] = {
    {  6 }, // RB0/INT
};

McuExtIntPinInfo PicExtIntPinInfo28[] = {
    { 21 }, // RB0/INT
};

McuExtIntPinInfo PicExtIntPinInfo40[] = {
    { 33 }, // RB0/INT
};

McuExtIntPinInfo PicExtIntPinInfo44[] = {
    { 8 }, // RB0/INT
};

McuExtIntPinInfo PicExtIntPinInfo64[] = {
    { 48 }, // RB0/INT
};

//-----------------------------------------------------------------------------
// AVR's I2C Info Tables

///// Added by JG
McuI2cInfo McuI2cInfoATmega8[] = {
//      name,   REG_CTRL, REG_STAT, REG_DATA, REG_RATE,     SCL,    SDA
    { "I2C1",    0,          0,          0,          0,      28,     27 },       // I2C = PC5 + PC4
//  NB: All pins for a same I2C peripheral must be on the same port PX because of PinsForI2cVariable()
};

McuI2cInfo McuI2cInfoATmega16[] = {                                             // For AtMega16, 32, 164, 324, 644, 1284
//      name,   REG_CTRL, REG_STAT, REG_DATA, REG_RATE,     SCL,    SDA
    { "I2C1",    0,          0,          0,          0,      22,     23 },       // I2C = PB8 + PB9
//  NB: All pins for a same I2C peripheral must be on the same port PX because of PinsForI2cVariable()
};

McuI2cInfo McuI2cInfoATmega64[] = {                                             // For AtMega16, 32, 164, 324, 644, 1284
//      name,   REG_CTRL, REG_STAT, REG_DATA, REG_RATE,     SCL,    SDA
    { "I2C1",    0,          0,          0,          0,      25,     26 },       // I2C = PD0 + PD1
//  NB: All pins for a same I2C peripheral must be on the same port PX because of PinsForI2cVariable()
};

McuI2cInfo McuI2cInfoATmega328[] = {                                            // For AtMega48, 88, 168, 328
//      name,   REG_CTRL, REG_STAT, REG_DATA, REG_RATE,     SCL,    SDA
    { "I2C1",    0,          0,          0,          0,      28,     27 },       // I2C = PB8 + PB9
//  NB: All pins for a same I2C peripheral must be on the same port PX because of PinsForI2cVariable()
};

McuI2cInfo McuI2cInfoATmega2560[] = {                                           // For AtMega640, 1280, 2560
//      name,   REG_CTRL, REG_STAT, REG_DATA, REG_RATE,     SCL,    SDA
    { "I2C1",    0,          0,          0,          0,      43,     44 },       // I2C = PD0 + PD1
//  NB: All pins for a same I2C peripheral must be on the same port PX because of PinsForI2cVariable()
};
/////

//-----------------------------------------------------------------------------
// ATmega328 32-Pin packages TQFP/QFN/MLF

McuIoPinInfo AvrAtmega328IoPinInfo32[] = {
//   port bit pin  pinName             ArduinoPin  ArduinoName
//                                              X  Nano
    { 'D',  0, 30, "PD0 PCINT16/RXD"       , 30, "0"   }, // {char port;  int bit;  int pin; char name[]}
    { 'D',  1, 31, "PD1 PCINT17/TXD"       , 31, "1"   },
    { 'D',  2, 32, "PD2 PCINT18/INT0"      , 32, "2"   },
    { 'D',  3,  1, "PD3 PCINT19/OC2B/INT1" ,  1, "3"   },
    { 'D',  4,  2, "PD4 PCINT20/XCK/T0"    ,  2, "4"   },
    { 'D',  5,  9, "PD5 PCINT21/OC0B/T1"   ,  9, "5"   },
    { 'D',  6, 10, "PD6 PCINT22/OC0A/AIN0" , 10, "6"   },
    { 'D',  7, 11, "PD7 PCINT23/AIN1"      , 11, "7"   },
    { 'B',  0, 12, "PB0 PCINT0/ICP1/CLKO"  , 12, "8"   },
    { 'B',  1, 13, "PB1 PCINT1/OC1A"       , 13, "9"   },
    { 'B',  2, 14, "PB2 PCINT2/OC1B/_SS"   , 14, "10"  },
    { 'B',  3, 15, "PB3 PCINT3/OC2A/MOSI"  , 15, "11"  },
    { 'B',  4, 16, "PB4 PCINT4/MISO"       , 16, "12"  },
    { 'B',  5, 17, "PB5 PCINT5/SCK"        , 17, "13"  },
    { 'B',  6,  7, "PB6 PCINT6/XTAL1/TOSC1",  7, ""    },
    { 'B',  7,  8, "PB7 PCINT7/XTAL2/TOSC2",  8, ""    },
    { 'C',  0, 23, "PC0 PCINT8/ADC0"       , 23, "A0"  },
    { 'C',  1, 24, "PC1 PCINT9/ADC1"       , 24, "A1"  },
    { 'C',  2, 25, "PC2 PCINT10/ADC2"      , 25, "A2"  },
    { 'C',  3, 26, "PC3 PCINT11/ADC3"      , 26, "A3"  },
    { 'C',  4, 27, "PC4 PCINT12/ADC4/SDA"  , 27, "A4"  },
    { 'C',  5, 28, "PC5 PCINT13/ADC5/SCL"  , 28, "A5"  },
    { 'C',  6, 29, "PC6 PCINT14/RESET"     , 29, "RST" },
    { '\0', 6, 19, "ADC6"                  , 19, "A6"  },
    { '\0', 7, 22, "ADC7"                  , 22, "A7"  }
};

McuAdcPinInfo AvrAtmega328AdcPinInfo32[] = {
    { 23, 0x00 }, // ADC0 {int pin;   BYTE muxRegValue;}
    { 24, 0x01 },
    { 25, 0x02 },
    { 26, 0x03 },
    { 27, 0x04 },
    { 28, 0x05 }, // ADC5
    { 19, 0x06 },
    { 22, 0x07 }  // ADC7
};

//-----------------------------------------------------------------------------
// AVR's SPI Info Tables

///// Added by JG
McuSpiInfo McuSpiInfoATmega8[] = {
//     name     REG_CTRL REG_STAT REG_DATA MISO  MOSI SCK  _SS
//                                         PB6   PB5  PB7  PB4
    { "SPI1",       0x0D,    0x0E,    0x0F,  18,   17,  19,  16 },
};

McuSpiInfo McuSpiInfoATmega16[] = {
//     name     REG_CTRL REG_STAT REG_DATA MISO  MOSI SCK  _SS
//                                         PB6   PB5  PB7  PB4
    { "SPI1",       0x0D,    0x0E,    0x0F,  7,    6,   8,   5 },        // For AtMega16, 32, 162, 164, 324, 644, 1284
};

McuSpiInfo McuSpiInfoATmega64[] = {
//     name     REG_CTRL REG_STAT REG_DATA MISO  MOSI SCK  _SS
//                                         PB6   PB5  PB7  PB4
    { "SPI1",       0x0D,    0x0E,    0x4E,  13,   12,  11,  10 },       // For AtMega64, 128
};


McuSpiInfo McuSpiInfoATmega328[] = {                                    // For AtMega48, 88, 168, 328
//     name     REG_CTRL REG_STAT REG_DATA MISO  MOSI SCK  _SS
//                                         PB6   PB5  PB7  PB4
    { "SPI1",       0x4C,    0x4D,    0x4E,  18,   17,  19,  16 },
};
/////

McuSpiInfo McuSpiInfoATmega2560[] = {
//     name     REG_CTRL REG_STAT REG_DATA MISO  MOSI SCK  _SS
//                                         PB3   PB2  PB1  PB0
    { "SPI1",       0x4C,    0x4D,    0x4E,  22,   21,  20,  19 },
//  { "SPI_UART0", 0x2C,    0x2D,    0x2E,  22,   21,  20,  19 }, // TODO
//  { "SPI_UART1", 0x2C,    0x2D,    0x2E,  22,   21,  20,  19 }, // TODO
//  { "SPI_UART2", 0x2C,    0x2D,    0x2E,  22,   21,  20,  19 }, // TODO
//  { "SPI_UART3", 0x2C,    0x2D,    0x2E,  22,   21,  20,  19 }, // TODO
};

//-----------------------------------------------------------------------------
// AVR's PWM Info Tables

//McuPwmPinInfo AvrPwmPinInfo6[] = { // ATtiny10
////     ti  reso max REG_   REG_   REG_   bit    bit    mask  REG_   mask
//// pin mer lutn CS OCRnxL OCRnxH TCCRnA COMnx1 COMnx0 WGMa  TCCRnB WGMb
//  {  1, 0, 8,   5, 0   ,  0x52,  0x53,  0,     0,        0, 0   ,  0    }, // OC0A
//  {  3, 0, 8,   5, 0x4A,  0x4B,  0x4F,  7,     6,        1, 0x4E,  0x08 }, // OC0B
//  {   , 1, 8,   5, 0x48,  0x49,  0x4F,  5,     4,        1, 0x4E,  0x08 }, // OC1A
//  {   , 1, 8,   7, 0x43,  0   ,  0x45,  5,     4,     0x48, 0   ,  0    }, // OC1B
//};

McuPwmPinInfo AvrPwmPinInfo8[] = { // ATtiny85
////     ti  reso max REG_   REG_   REG_   bit    bit    mask  REG_   mask
//// pin mer lutn  CS OCRnxL OCRnxH TCCRnA COMnx1 COMnx0 WGMa  TCCRnB WGMb
    {  5, 0, 8,    5, 0   ,  0x52,  0x53,  0,     0,        0, 0   ,  0    , ""}, // OC0A
    {  6, 0, 8,    5, 0x4A,  0x4B,  0x4F,  7,     6,        1, 0x4E,  0x08 , ""}, // OC0B
    {  6, 1, 8,    5, 0x48,  0x49,  0x4F,  5,     4,        1, 0x4E,  0x08 , ""}, // OC1A
    {  4, 1, 8,    7, 0x43,  0   ,  0x45,  5,     4,     0x48, 0   ,  0    , ""}, // OC1B
};

McuPwmPinInfo AvrPwmPinInfo28_[] = { // ATmega8, etc.
////     ti  reso max REG_   REG_   REG_   bit    bit    mask  REG_   mask
//// pin mer lutn  CS OCRnxL OCRnxH TCCRnA COMnx1 COMnx0 WGMa  TCCRnB WGMb
    {  0, 0, 8,    5, 0   ,  0x52,  0x53,  0,     0,        0, 0   ,  0    , ""}, // Timer0

    { 15, 1, 8,    5, 0x4A,  0x4B,  0x4F,  7,     6,        1, 0x4E,  0x08 , ""}, // OC1A // Fast PWM  8-bit
    { 15, 1, 9,    5, 0x4A,  0x4B,  0x4F,  7,     6,        2, 0x4E,  0x08 , ""}, // OC1A // Fast PWM  9-bit
    { 15, 1,10,    5, 0x4A,  0x4B,  0x4F,  7,     6,        3, 0x4E,  0x08 , ""}, // OC1A // Fast PWM 10-bit

    { 16, 1, 8,    5, 0x48,  0x49,  0x4F,  5,     4,        1, 0x4E,  0x08 , ""}, // OC1B // Fast PWM  8-bit
    { 16, 1, 9,    5, 0x48,  0x49,  0x4F,  5,     4,        2, 0x4E,  0x08 , ""}, // OC1B // Fast PWM  9-bit
    { 16, 1,10,    5, 0x48,  0x49,  0x4F,  5,     4,        3, 0x4E,  0x08 , ""}, // OC1B // Fast PWM 10-bit

    { 17, 2, 8,    7, 0x43,  0   ,  0x45,  5,     4,     0x48, 0   ,  0    , ""}, // OC2  // Fast PWM
};

McuPwmPinInfo AvrPwmPinInfo28[] = { // ATmega328, etc.
////     ti  reso max REG_   REG_   REG_   bit    bit    mask  REG_   mask
//// pin mer lutn  CS OCRnxL OCRnxH TCCRnA COMnx1 COMnx0 WGMa  TCCRnB WGMb
    { 12, 0, 8,    5, 0x47,  0   ,  0x44,  7,     6,        3, 0x45,  0    ,"OC0A" }, // Fast PWM  8-bit
    { 11, 0, 8,    5, 0x48,  0   ,  0x44,  5,     4,        3, 0x45,  0    ,"OC0B" }, // Fast PWM  8-bit

    { 15, 1, 8,    5, 0x88,  0x89,  0x80,  7,     6,        1, 0x81,  0x08 ,"OC1A" }, // Fast PWM  8-bit
    { 15, 1, 9,    5, 0x88,  0x89,  0x80,  7,     6,        2, 0x81,  0x08 ,"OC1A" }, // Fast PWM  9-bit
    { 15, 1,10,    5, 0x88,  0x89,  0x80,  7,     6,        3, 0x81,  0x08 ,"OC1A" }, // Fast PWM 10-bit

    { 16, 1, 8,    5, 0x8A,  0x8B,  0x80,  5,     4,        1, 0x81,  0x08 ,"OC1B" }, // Fast PWM  8-bit
    { 16, 1, 9,    5, 0x8A,  0x8B,  0x80,  5,     4,        2, 0x81,  0x08 ,"OC1B" }, // Fast PWM  9-bit
    { 16, 1,10,    5, 0x8A,  0x8B,  0x80,  5,     4,        3, 0x81,  0x08 ,"OC1B" }, // Fast PWM 10-bit

    { 17, 2, 8,    7, 0xB3,  0   ,  0xB0,  7,     6,        3, 0xB1,  0    ,"OC2A" }, // Fast PWM  8-bit
    {  5, 2, 8,    7, 0xB4,  0   ,  0xB0,  5,     4,        3, 0xB1,  0    ,"OC2B" }, // Fast PWM  8-bit
};

McuPwmPinInfo AvrPwmPinInfo32_[] = {
////     ti  reso max REG_   REG_   REG_   bit    bit    mask  REG_   mask
//// pin mer lutn  CS OCRnxL OCRnxH TCCRnA COMnx1 COMnx0 WGMa  TCCRnB WGMb
//  { 13, 1, 8,    5, 0x4A,  0x4B,  0x4F,  7,     6,        1, 0x4E,  0x00 , ""}, // OC1A // Phase Correct PWM
//  { 14, 1, 8,    5, 0x48,  0x49,  0x4F,  5,     4,        1, 0x4E,  0x00 , ""}, // OC1B // Phase Correct PWM
//  { 15, 2, 8,    7, 0x43,  0   ,  0x45,  5,     4,     0x40, 0   ,  0    , ""}, // OC2  // Phase Correct PWM
    {  0, 0, 8,    5, 0   ,  0x52,  0x53,  0,     0,        0, 0   ,  0    , ""}, // Timer0
    { 13, 1, 8,    5, 0x4A,  0x4B,  0x4F,  7,     6,        1, 0x4E,  0x08 , ""}, // OC1A // Fast PWM 8-bit
    { 14, 1, 8,    5, 0x48,  0x49,  0x4F,  5,     4,        1, 0x4E,  0x08 , ""}, // OC1B // Fast PWM 8-bit
    { 15, 2, 8,    7, 0x43,  0   ,  0x45,  5,     4,     0x48, 0   ,  0    , ""}, // OC2  // Fast PWM
};

McuPwmPinInfo AvrAtmega2560PwmPinInfo[] = {
////     ti  reso max REG_    REG_   REG_   bit    bit    mask  REG_   mask
//// pin mer lutn  CS OCRnxL  OCRnxH TCCRnA COMnx1 COMnx0 WGMa  TCCRnB WGMb
//  { 26, 0, 8,     5, 0x47,  0,     0x44,  7,     6,        1, 0x45,  0x08 , ""}, // OC0A
    {  1, 0, 8,     5, 0x48,  0,     0x44,  5,     4,        2, 0x45,  0x08 , ""}, // OC0B

    { 24, 1, 8,     5, 0x88,  0x89,  0x80,  7,     6,        1, 0x81,  0x08 , ""}, // OC1A
    { 24, 1, 9,     5, 0x88,  0x89,  0x80,  7,     6,        2, 0x81,  0x08 , ""}, // OC1A
    { 24, 1,10,     5, 0x88,  0x89,  0x80,  7,     6,        3, 0x81,  0x08 , ""}, // OC1A

    { 25, 1, 8,     5, 0x8A,  0x8B,  0x80,  5,     4,        1, 0x81,  0x08 , ""}, // OC1B
    { 25, 1, 9,     5, 0x8A,  0x8B,  0x80,  5,     4,        2, 0x81,  0x08 , ""}, // OC1B
    { 25, 1,10,     5, 0x8A,  0x8B,  0x80,  5,     4,        3, 0x81,  0x08 , ""}, // OC1B

    { 26, 1, 8,     5, 0x8C,  0x8D,  0x80,  3,     2,        1, 0x81,  0x08 , ""}, // OC1C
    { 26, 1, 9,     5, 0x8C,  0x8D,  0x80,  3,     2,        2, 0x81,  0x08 , ""}, // OC1C
    { 26, 1,10,     5, 0x8C,  0x8D,  0x80,  3,     2,        3, 0x81,  0x08 , ""}, // OC1C

    { 23, 2, 8,     5, 0xB3,  0,     0xB0,  7,     6,        1, 0xB1,  0x08 , ""}, // OC2A
    { 18, 2, 8,     5, 0xB4,  0,     0xB0,  5,     4,        2, 0xB1,  0x08 , ""}, // OC2B

    {  5, 3, 8,     5, 0x98,  0x99,  0x90,  7,     6,        1, 0x91,  0x08 , ""}, // OC3A
    {  5, 3, 9,     5, 0x98,  0x99,  0x90,  7,     6,        2, 0x91,  0x08 , ""}, // OC3A
    {  5, 3,10,     5, 0x98,  0x99,  0x90,  7,     6,        3, 0x91,  0x08 , ""}, // OC3A

    {  6, 3, 8,     5, 0x9A,  0x9B,  0x90,  5,     4,        1, 0x91,  0x08 , ""}, // OC3B
    {  6, 3, 9,     5, 0x9A,  0x9B,  0x90,  5,     4,        2, 0x91,  0x08 , ""}, // OC3B
    {  6, 3,10,     5, 0x9A,  0x9B,  0x90,  5,     4,        3, 0x91,  0x08 , ""}, // OC3B

    {  7, 3, 8,     5, 0x9C,  0x9D,  0x90,  3,     2,        1, 0x91,  0x08 , ""}, // OC3C
    {  7, 3, 9,     5, 0x9C,  0x9D,  0x90,  3,     2,        2, 0x91,  0x08 , ""}, // OC3C
    {  7, 3,10,     5, 0x9C,  0x9D,  0x90,  3,     2,        3, 0x91,  0x08 , ""}, // OC3C

    { 15, 4, 8,     5, 0xA8,  0xA9,  0xA0,  7,     6,        1, 0xA1,  0x08 , ""}, // OC4A
    { 15, 4, 9,     5, 0xA8,  0xA9,  0xA0,  7,     6,        2, 0xA1,  0x08 , ""}, // OC4A
    { 15, 4,10,     5, 0xA8,  0xA9,  0xA0,  7,     6,        3, 0xA1,  0x08 , ""}, // OC4A

    { 16, 4, 8,     5, 0xAA,  0xAB,  0xA0,  5,     4,        1, 0xA1,  0x08 , ""}, // OC4B
    { 16, 4, 9,     5, 0xAA,  0xAB,  0xA0,  5,     4,        2, 0xA1,  0x08 , ""}, // OC4B
    { 16, 4,10,     5, 0xAA,  0xAB,  0xA0,  5,     4,        3, 0xA1,  0x08 , ""}, // OC4B

    { 17, 4, 8,     5, 0xAC,  0xAD,  0xA0,  3,     2,        1, 0xA1,  0x08 , ""}, // OC4C
    { 17, 4, 9,     5, 0xAC,  0xAD,  0xA0,  3,     2,        2, 0xA1,  0x08 , ""}, // OC4C
    { 17, 4,10,     5, 0xAC,  0xAD,  0xA0,  3,     2,        3, 0xA1,  0x08 , ""}, // OC4C

//  { 38, 5, 8,     5, 0x128, 0x129, 0x120, 7,     6,        1, 0x121, 0x08 , ""}, // OC5A ???
//  { 39, 5, 8,     5, 0x12A, 0x12B, 0x120, 5,     4,        1, 0x121, 0x08 , ""}, // OC5B
//  { 40, 5, 8,     5, 0x12C, 0x12D, 0x120, 3,     2,        1, 0x121, 0x08 , ""}, // OC5C
};

McuPwmPinInfo AvrAtmega16_32PwmPinInfo40[] = {
////     ti  reso max REG_    REG_   REG_   bit    bit    mask  REG_   mask
//// pin mer lutn  CS OCRnxL  OCRnxH TCCRnA COMnx1 COMnx0 WGMa  TCCRnB WGMb
    {  4, 0, 8,     5, 0x5C,  0,     0x53,  5,     4,     0x48, 0   ,  0    , ""}, // OC0

    { 21, 2, 8,     7, 0x43,  0   ,  0x45,  5,     4,     0x48, 0   ,  0    , ""}, // OC2  // Fast PWM
    //
    { 19, 1, 8,     5, 0x4A,  0x4B,  0x4F,  7,     6,        1, 0x4E,  0x08 , ""}, // OC1A // Fast PWM  8-bit
    { 19, 1, 9,     5, 0x4A,  0x4B,  0x4F,  7,     6,        2, 0x4E,  0x08 , ""}, // OC1A // Fast PWM  9-bit
    { 19, 1,10,     5, 0x4A,  0x4B,  0x4F,  7,     6,        3, 0x4E,  0x08 , ""}, // OC1A // Fast PWM 10-bit

    { 18, 1, 8,     5, 0x48,  0x49,  0x4F,  5,     4,        1, 0x4E,  0x08 , ""}, // OC1B // Fast PWM  8-bit
    { 18, 1, 9,     5, 0x48,  0x49,  0x4F,  5,     4,        2, 0x4E,  0x08 , ""}, // OC1B // Fast PWM  9-bit
    { 18, 1,10,     5, 0x48,  0x49,  0x4F,  5,     4,        3, 0x4E,  0x08 , ""}, // OC1B // Fast PWM 10-bit
};

McuPwmPinInfo AvrPwmPinInfo64_[] = {
////     ti  reso max REG_    REG_   REG_   bit    bit    mask  REG_   mask
//// pin mer lutn  CS OCRnxL  OCRnxH TCCRnA COMnx1 COMnx0 WGMa  TCCRnB WGMb
    { 14, 0, 8,     7, 0x51,  0,     0x53,  5,     4,     0x48, 0,     0, "" },    // OC0

    { 17, 2, 8,     5, 0x43,  0,     0x45,  5,     4,     0x48, 0,     0, "" },    // OC2  // Fast PWM
    //
    { 15, 1, 8,     5, 0x4A,  0x4B,  0x4F,  7,     6,        1, 0x4E,  0x08 , ""}, // OC1A // Fast PWM  8-bit
    { 15, 1, 9,     5, 0x4A,  0x4B,  0x4F,  7,     6,        2, 0x4E,  0x08 , ""}, // OC1A // Fast PWM  9-bit
    { 15, 1,10,     5, 0x4A,  0x4B,  0x4F,  7,     6,        3, 0x4E,  0x08 , ""}, // OC1A // Fast PWM 10-bit

    { 16, 1, 8,     5, 0x48,  0x49,  0x4F,  5,     4,        1, 0x4E,  0x08 , ""}, // OC1B // Fast PWM  8-bit
    { 16, 1, 9,     5, 0x48,  0x49,  0x4F,  5,     4,        2, 0x4E,  0x08 , ""}, // OC1B // Fast PWM  9-bit
    { 16, 1,10,     5, 0x48,  0x49,  0x4F,  5,     4,        3, 0x4E,  0x08 , ""}, // OC1B // Fast PWM 10-bit
    // There is problem with OC1C/OC2 in Proteus ?
    { 17, 1, 8,     5, 0x78,  0x79,  0x4F,  3,     2,        1, 0x4E,  0x08 , ""}, // OC1C // Fast PWM  8-bit
    { 17, 1, 9,     5, 0x78,  0x79,  0x4F,  3,     2,        2, 0x4E,  0x08 , ""}, // OC1C // Fast PWM  9-bit
    { 17, 1,10,     5, 0x78,  0x79,  0x4F,  3,     2,        3, 0x4E,  0x08 , ""}, // OC1C // Fast PWM 10-bit
    //
    {  5, 3, 8,     5, 0x86,  0x78,  0x8B,  7,     6,        1, 0x8A,  0x08, "" }, // OC3A // Fast PWM  8-bit
    {  5, 3, 9,     5, 0x86,  0x78,  0x8B,  7,     6,        2, 0x8A,  0x08, "" }, // OC3A // Fast PWM  9-bit
    {  5, 3,10,     5, 0x86,  0x78,  0x8B,  7,     6,        3, 0x8A,  0x08, "" }, // OC3A // Fast PWM 10-bit

    {  6, 3, 8,     5, 0x84,  0x85,  0x8B,  5,     4,        1, 0x8A,  0x08, "" }, // OC3B // Fast PWM  8-bit
    {  6, 3, 9,     5, 0x84,  0x85,  0x8B,  5,     4,        2, 0x8A,  0x08, "" }, // OC3B // Fast PWM  9-bit
    {  6, 3,10,     5, 0x84,  0x85,  0x8B,  5,     4,        3, 0x8A,  0x08, "" }, // OC3B // Fast PWM 10-bit

    {  7, 3, 8,     5, 0x82,  0x83,  0x8B,  3,     2,        1, 0x8A,  0x08, "" }, // OC3C // Fast PWM  8-bit
    {  7, 3, 9,     5, 0x82,  0x83,  0x8B,  3,     2,        2, 0x8A,  0x08, "" }, // OC3C // Fast PWM  9-bit
    {  7, 3,10,     5, 0x82,  0x83,  0x8B,  3,     2,        3, 0x8A,  0x08, "" }, // OC3C // Fast PWM 10-bit
};

McuPwmPinInfo AvrPwmPinInfo40_[] = {
////     ti  reso max REG_    REG_   REG_   bit    bit    mask  REG_   mask
//// pin mer lutn  CS OCRnxL  OCRnxH TCCRnA COMnx1 COMnx0 WGMa  TCCRnB WGMb
    {  4, 0, 8,     5, 0x47,  0,     0x44,  7,     6,        3, 0x45,     0 , ""}, // OC0A // Fast PWM
    {  5, 0, 8,     5, 0x48,  0,     0x44,  5,     4,        3, 0x45,     0 , ""}, // OC0B // Fast PWM

    { 19, 1, 8,     5, 0x88,  0x89,  0x80,  7,     6,        1, 0x81,  0x08 , ""}, // OC1A // Fast PWM  8-bit
    { 19, 1, 9,     5, 0x88,  0x89,  0x80,  7,     6,        2, 0x81,  0x08 , ""}, // OC1A // Fast PWM  9-bit
    { 19, 1,10,     5, 0x88,  0x89,  0x80,  7,     6,        3, 0x81,  0x08 , ""}, // OC1A // Fast PWM 10-bit

    { 18, 1, 8,     5, 0x8A,  0x8B,  0x80,  5,     4,        1, 0x81,  0x08 , ""}, // OC1B // Fast PWM  8-bit
    { 18, 1, 9,     5, 0x8A,  0x8B,  0x80,  5,     4,        2, 0x81,  0x08 , ""}, // OC1B // Fast PWM  9-bit
    { 18, 1,10,     5, 0x8A,  0x8B,  0x80,  5,     4,        3, 0x81,  0x08 , ""}, // OC1B // Fast PWM 10-bit

    {  7, 1, 8,     5, 0x98,  0x99,  0x90,  7,     6,        1, 0x91,  0x08 , ""}, // OC3A // Fast PWM  8-bit
    {  7, 1, 9,     5, 0x98,  0x99,  0x90,  7,     6,        2, 0x91,  0x08 , ""}, // OC3A // Fast PWM  9-bit
    {  7, 1,10,     5, 0x98,  0x99,  0x90,  7,     6,        3, 0x91,  0x08 , ""}, // OC3A // Fast PWM 10-bit

    {  8, 1, 8,     5, 0x9A,  0x9B,  0x90,  5,     4,        1, 0x91,  0x08 , ""}, // OC3B // Fast PWM  8-bit
    {  8, 1, 9,     5, 0x9A,  0x9B,  0x90,  5,     4,        2, 0x91,  0x08 , ""}, // OC3B // Fast PWM  9-bit
    {  8, 1,10,     5, 0x9A,  0x9B,  0x90,  5,     4,        3, 0x91,  0x08 , ""}, // OC3B // Fast PWM 10-bit

    { 21, 2, 8,     7, 0xB3,  0   ,  0xB0,  7,     6,        3, 0xB1,  0x84 , ""}, // OC2A // Fast PWM
    { 20, 2, 8,     7, 0xB4,  0   ,  0xB0,  5,     4,        3, 0xB1,  0x84 , ""}, // OC2B // Fast PWM
};

McuPwmPinInfo AvrPwmPinInfo44[] = {
////     ti  reso max REG_    REG_   REG_   bit    bit    mask  REG_   mask
//// pin mer lutn  CS OCRnxL  OCRnxH TCCRnA COMnx1 COMnx0 WGMa  TCCRnB WGMb
    { 43, 0, 8,     5, 0x47,  0,     0x44,  7,     6,        3, 0x45,  0x00 , ""}, // OC0A // Fast PWM
    { 44, 0, 8,     5, 0x48,  0,     0x44,  5,     4,        3, 0x45,  0x00 , ""}, // OC0B // Fast PWM

    { 14, 1, 8,     5, 0x88,  0x89,  0x80,  7,     6,        1, 0x81,  0x08 , ""}, // OC1A // Fast PWM  8-bit
    { 14, 1, 9,     5, 0x88,  0x89,  0x80,  7,     6,        2, 0x81,  0x08 , ""}, // OC1A // Fast PWM  9-bit
    { 14, 1,10,     5, 0x88,  0x89,  0x80,  7,     6,        3, 0x81,  0x08 , ""}, // OC1A // Fast PWM 10-bit

    { 13, 1, 8,     5, 0x8A,  0x8B,  0x80,  5,     4,        1, 0x81,  0x08 , ""}, // OC1B // Fast PWM  8-bit
    { 13, 1, 9,     5, 0x8A,  0x8B,  0x80,  5,     4,        2, 0x81,  0x08 , ""}, // OC1B // Fast PWM  9-bit
    { 13, 1,10,     5, 0x8A,  0x8B,  0x80,  5,     4,        3, 0x81,  0x08 , ""}, // OC1B // Fast PWM 10-bit

    {  2, 3, 8,     5, 0x98,  0x99,  0x90,  7,     6,        1, 0x91,  0x08 , ""}, // OC3A // Fast PWM  8-bit
    {  2, 3, 9,     5, 0x98,  0x99,  0x90,  7,     6,        2, 0x91,  0x08 , ""}, // OC3A // Fast PWM  9-bit
    {  2, 3,10,     5, 0x98,  0x99,  0x90,  7,     6,        3, 0x91,  0x08 , ""}, // OC3A // Fast PWM 10-bit

    {  3, 3, 8,     5, 0x9A,  0x9B,  0x90,  5,     4,        1, 0x91,  0x08 , ""}, // OC3B // Fast PWM  8-bit
    {  3, 3, 9,     5, 0x9A,  0x9B,  0x90,  5,     4,        2, 0x91,  0x08 , ""}, // OC3B // Fast PWM  9-bit
    {  3, 3,10,     5, 0x9A,  0x9B,  0x90,  5,     4,        3, 0x91,  0x08 , ""}, // OC3B // Fast PWM 10-bit

    { 16, 2, 8,     7, 0xB3,  0   ,  0xB0,  7,     6,        3, 0xB1,  0x00 , ""}, // OC2A // Fast PWM
    { 15, 2, 8,     7, 0xB4,  0   ,  0xB0,  5,     4,        3, 0xB1,  0x00 , ""}, // OC2B // Fast PWM
};

McuPwmPinInfo AvrAtmega162PwmPinInfo40_[] = {
////     ti  reso max REG_    REG_   REG_   bit    bit    mask  REG_   mask
//// pin mer lutn  CS OCRnxL  OCRnxH TCCRnA COMnx1 COMnx0 WGMa  TCCRnB WGMb
    {  2, 2, 8,     7, 0, 0, 0, 0, 0, 0, 0, 0, "" },
};

McuPwmPinInfo AvrPwmPinInfo100_[] = {
    { 23, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "" },
};

McuPwmPinInfo AvrPwmPinInfo32[] = {
////     ti  reso max REG_   REG_   REG_   bit    bit    mask  REG_   mask
//// pin mer lutn  CS OCRnxL OCRnxH TCCRnA COMnx1 COMnx0 WGMa  TCCRnB WGMb
    { 10, 0, 8,    5, 0x47,  0,     0x44,  7,     6,     3,    0x45,  0    , ""}, // OC0A // Fast PWM
    {  9, 0, 8,    5, 0x48,  0,     0x44,  5,     4,     3,    0x45,  0    , ""}, // OC0B

    { 13, 1, 8,    5, 0x88,  0x89,  0x80,  7,     6,     1,    0x81,  0x08 , ""}, // OC1A
    { 13, 1, 9,    5, 0x88,  0x89,  0x80,  7,     6,     2,    0x81,  0x08 , ""}, // OC1A
    { 13, 1,10,    5, 0x88,  0x89,  0x80,  7,     6,     3,    0x81,  0x08 , ""}, // OC1A

    { 14, 1, 8,    5, 0x8A,  0x8B,  0x80,  5,     4,     1,    0x81,  0x08 , ""}, // OC1B
    { 14, 1, 9,    5, 0x8A,  0x8B,  0x80,  5,     4,     2,    0x81,  0x08 , ""}, // OC1B
    { 14, 1,10,    5, 0x8A,  0x8B,  0x80,  5,     4,     3,    0x81,  0x08 , ""}, // OC1B

    { 15, 2, 8,    7, 0xB3,  0,     0xB0,  7,     6,     3,    0xB1,  0    , ""}, // OC2A
    {  1, 2, 8,    7, 0xB4,  0,     0xB0,  5,     4,     3,    0xB1,  0    , ""}, // OC2B
};

McuPwmPinInfo ATmegaXXU4PwmPinInfo[] = {
////     ti  reso max REG_   REG_   REG_   bit    bit    mask  REG_   mask
//// pin mer lutn  CS OCRnxL OCRnxH TCCRnA COMnx1 COMnx0 WGMa  TCCRnB WGMb
    { 12, 0, 8,    5, 0x47,  0,     0x44,  7,     6,     3,    0x45,  0   , "" }, // OC0A // Fast PWM
    { 18, 0, 8,    5, 0x48,  0,     0x44,  5,     4,     3,    0x45,  0   , "" }, // OC0B

    { 29, 1, 8,    5, 0x88,  0x89,  0x80,  7,     6,     1,    0x81,  0x08, "" }, // OC1A
    { 30, 1, 8,    5, 0x8A,  0x8B,  0x80,  5,     4,     1,    0x81,  0x08, "" }, // OC1B
    { 12, 1, 8,    5, 0x8C,  0x8D,  0x80,  3,     2,     1,    0x81,  0x08, "" }, // OC1C

    { 31, 3, 8,    5, 0x98,  0x99,  0x90,  7,     6,     1,    0x91,  0x08, "" }, // OC3A

//  { 32, 4, 8,   15, 0xCF,  0,     0xC0,  7,     6,     3,    0xC3,  0    }, // OC4A
//  { 30, 4, 8,   15, 0xD0,  0,     0xC0,  5,     4,     3,    0xC3,  0    }, // OC4B
//  { 27, 4, 8,   15, 0xD2,  0,     0xC0,  5,     4,     3,    0xC3,  0    }, // OC4D
};

//-----------------------------------------------------------------------------
// PIC's PWM Info Tables
////     ti
//// pin mer
McuPwmPinInfo PicPwmPinInfo8[] = {
    {  5, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ""  },
};

McuPwmPinInfo PicPwmPinInfo18[] = {
    {  9, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ""  },
};

McuPwmPinInfo PicPwmPinInfo28_1[] = {
    { 13, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ""  },
};

McuPwmPinInfo PicPwmPinInfo28_2[] = {
    { 12, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ""  },
    { 13, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ""  },
};

McuPwmPinInfo PicPwmPinInfo40[] = {
    { 16, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ""  },
    { 17, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ""  },
};

McuPwmPinInfo PicPwmPinInfo44[] = {
    { 35, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ""  },
    { 36, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ""  },
};

McuPwmPinInfo PicPwmPinInfo64[] = {
    { 29, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ""  },
};

//-----------------------------------------------------------------------------
// PIC's I2C Info Tables

McuI2cInfo McuI2cInfoPic16F88[] = {                                             // For PIC16F88, 819
//      name,   REG_CTRL, REG_STAT, REG_DATA, REG_RATE,     SCL,    SDA
    { "I2C1",    0,          0,          0,          0,      10,     7 },        // I2C = RB4 + RB1
//  NB: All pins for a same I2C peripheral must be on the same port RBX because of PinsForI2cVariable()
};

McuI2cInfo McuI2cInfoPic16F876[] = {                                            // For PIC16F876, 886
//      name,   REG_CTRL, REG_STAT, REG_DATA, REG_RATE,     SCL,    SDA
    { "I2C1",    0,          0,          0,          0,      14,     15 },       // I2C = RC3 + RC4
//  NB: All pins for a same I2C peripheral must be on the same port RBX because of PinsForI2cVariable()
};

McuI2cInfo McuI2cInfoPic16F877[] = {                                            // For PIC16F877, 887
//      name,   REG_CTRL, REG_STAT, REG_DATA, REG_RATE,     SCL,    SDA
    { "I2C1",    0,          0,          0,          0,      18,     23 },       // I2C = RC3 + RC4
//  NB: All pins for a same I2C peripheral must be on the same port RBX because of PinsForI2cVariable()
};

McuI2cInfo McuI2cInfoPic16F887_44[] = {                                            // For PIC16F877, 887 in 44 TQFP
//      name,   REG_CTRL, REG_STAT, REG_DATA, REG_RATE,     SCL,    SDA
    { "I2C1",    0,          0,          0,          0,      37,     42 },       // I2C = RC3 + RC4
//  NB: All pins for a same I2C peripheral must be on the same port RBX because of PinsForI2cVariable()
};

McuI2cInfo McuI2cInfoPic18F4520[] = {                                            // For PIC18F4520
//      name,   REG_CTRL, REG_STAT, REG_DATA, REG_RATE,     SCL,    SDA
    { "I2C",    0,          0,          0,          0,      18,     23 },       // I2C = RC3 + RC4
//  NB: All pins for a same I2C peripheral must be on the same port RBX because of PinsForI2cVariable()
};

//-----------------------------------------------------------------------------
// PIC's SPI Info Tables

// Pic16F88 or Pic16F819
McuSpiInfo McuSpiInfoPic16F88[] = {
//     name     REG_CTRL REG_STAT REG_DATA MISO  MOSI SCK  _SS
//                                         PB3   PB2  PB1  PB0
    { "SPI1",       0,       0,       0,      7,    8,  10,  11 },
};

// Pic16F876 or Pic16F886
McuSpiInfo McuSpiInfoPic16F876[] = {
//     name     REG_CTRL REG_STAT REG_DATA MISO  MOSI SCK  _SS
//                                         PB3   PB2  PB1  PB0
    { "SPI1",       0,       0,       0,     15,   16,  14,   7 },
};

// Pic16F877 or Pic16F887
McuSpiInfo McuSpiInfoPic16F877[] = {
//     name     REG_CTRL REG_STAT REG_DATA MISO  MOSI SCK  _SS
//                                         PB3   PB2  PB1  PB0
    { "SPI1",       0,       0,       0,     23,   24,  18,   7 },
};
/////

// Pic16F887 in 44-TQFP
McuSpiInfo McuSpiInfoPic16F887_44[] = {
//     name     REG_CTRL REG_STAT REG_DATA MISO  MOSI SCK  _SS
//                                         PB3   PB2  PB1  PB0
    { "SPI1",       0,       0,      0,     42,   43,  37,  24 },
};
/////

// Pic18F4520
McuSpiInfo McuSpiInfoPic18F4520[] = {
//     name     REG_CTRL REG_STAT REG_DATA MISO  MOSI SCK  _SS
//                                         PC4   PC5  PC3  PA5
    { "SPI",       0,       0,       0,     23,   24,  18,   7 },
};

//-----------------------------------------------------------------------------
// ATmega164 or ATmega324 or ATmega644 or ATmega1264

McuIoPinInfo AvrAtmega164IoPinInfo[] = {
    { 'B',  0,  1, "PB0 PCINT8/XCK0/T0"    },
    { 'B',  1,  2, "PB1 PCINT9/CLKO/T1"    },
    { 'B',  2,  3, "PB2 PCINT10/INT2/AIN0" },
    { 'B',  3,  4, "PB3 PCINT11/OC0A/AIN1" },
    { 'B',  4,  5, "PB4 PCINT12/OC0B/_SS"  },
    { 'B',  5,  6, "PB5 PCINT13/ICP3/MOSI" },
    { 'B',  6,  7, "PB6 PCINT14/OC3A/MISO" },
    { 'B',  7,  8, "PB7 PCINT15/OC3B/SCK"  },
    { 'D',  0, 14, "PD0 PCINT24/RXD0/T3*"  },
    { 'D',  1, 15, "PD1 PCINT25/TXD0"      },
    { 'D',  2, 16, "PD2 PCINT26/RXD1/INT0" },
    { 'D',  3, 17, "PD3 PCINT27/TXD1/INT1" },
    { 'D',  4, 18, "PD4 PCINT28/XCK1/OC1B" },
    { 'D',  5, 19, "PD5 PCINT29/OC1A"      },
    { 'D',  6, 20, "PD6 PCINT30/OC2B/ICP"  },
    { 'D',  7, 21, "PD7 OC2A/PCINT31"      },
    { 'C',  0, 22, "PC0 SCL/PCINT16"       },
    { 'C',  1, 23, "PC1 SDA/PCINT17"       },
    { 'C',  2, 24, "PC2 TCK/PCINT18"       },
    { 'C',  3, 25, "PC3 TMS/PCINT19"       },
    { 'C',  4, 26, "PC4 TDO/PCINT20"       },
    { 'C',  5, 27, "PC5 TDI/PCINT21"       },
    { 'C',  6, 28, "PC6 TOSC1/PCINT22"     },
    { 'C',  7, 29, "PC7 TOSC2/PCINT23"     },
    { 'A',  7, 33, "PA7 ADC7/PCINT7"       },
    { 'A',  6, 34, "PA6 ADC6/PCINT6"       },
    { 'A',  5, 35, "PA5 ADC5/PCINT5"       },
    { 'A',  4, 36, "PA4 ADC4/PCINT4"       },
    { 'A',  3, 37, "PA3 ADC3/PCINT3"       },
    { 'A',  2, 38, "PA2 ADC2/PCINT2"       },
    { 'A',  1, 39, "PA1 ADC1/PCINT1"       },
    { 'A',  0, 40, "PA0 ADC0/PCINT0"       }
};

McuAdcPinInfo AvrAtmega164AdcPinInfo[] = {
    { 40, 0x00 },
    { 39, 0x01 },
    { 38, 0x02 },
    { 37, 0x03 },
    { 36, 0x04 },
    { 35, 0x05 },
    { 34, 0x06 },
    { 33, 0x07 },
};

//-----------------------------------------------------------------------------
// ATmega1264 44-pin TQFP/QFN/MLF

McuIoPinInfo AvrAtmegaIo44PinInfo[] = {
    { 'A',  0, 37, "PA0 ADC0/PCINT0"       },
    { 'A',  1, 36, "PA1 ADC1/PCINT1"       },
    { 'A',  2, 35, "PA2 ADC2/PCINT2"       },
    { 'A',  3, 34, "PA3 ADC3/PCINT3"       },
    { 'A',  4, 33, "PA4 ADC4/PCINT4"       },
    { 'A',  5, 32, "PA5 ADC5/PCINT5"       },
    { 'A',  6, 31, "PA6 ADC6/PCINT6"       },
    { 'A',  7, 30, "PA7 ADC7/PCINT7"       },
    { 'B',  0, 40, "PB0 XCK0/T0/PCINT8"    },
    { 'B',  1, 41, "PB1 T1/CLKO/PCINT9"    },
    { 'B',  2, 42, "PB2 AIN0/INT2/PCINT10" },
    { 'B',  3, 43, "PB3 AIN1/OC0A/PCINT11" },
    { 'B',  4, 44, "PB4 _SS/OC0B/PCINT12"  },
    { 'B',  5, 1,  "PB5 PCINT13/ICP3/MOSI" },
    { 'B',  6, 2,  "PB6 PCINT14/OC3A/MISO" },
    { 'B',  7, 3,  "PB7 PCINT15/OC3B/SCK"  },
    { 'D',  0, 9,  "PD0 PCINT24/RXD0/T3"   },
    { 'D',  1, 10, "PD1 PCINT25/TXD0"      },
    { 'D',  2, 11, "PD2 PCINT26/RXD1/INT0" },
    { 'D',  3, 12, "PD3 PCINT27/TXD1/INT1" },
    { 'D',  4, 13, "PD4 PCINT28/XCK1/OC1B" },
    { 'D',  5, 14, "PD5 PCINT29/OC1A"      },
    { 'D',  6, 15, "PD6 PCINT30/OC2B/ICP"  },
    { 'D',  7, 16, "PD7 PCINT31/OC2A"      },
    { 'C',  0, 19, "PC0 PCINT16/SCL"       },
    { 'C',  1, 20, "PC1 PCINT17/SDA"       },
    { 'C',  2, 21, "PC2 PCINT18/TCK"       },
    { 'C',  3, 22, "PC3 PCINT19/TMS"       },
    { 'C',  4, 23, "PC4 TDO/PCINT20)"      },
    { 'C',  5, 24, "PC5 TDI/PCINT21)"      },
    { 'C',  6, 25, "PC6 TOSC1/PCINT22"     },
    { 'C',  7, 26, "PC7 TOSC2/PCINT23"     }
};

McuAdcPinInfo AvrAtmegaAdc44PinInfo[] = {
    { 37, 0x00 },
    { 36, 0x01 },
    { 35, 0x02 },
    { 34, 0x03 },
    { 33, 0x04 },
    { 32, 0x05 },
    { 31, 0x06 },
    { 30, 0x07 },
};

//-----------------------------------------------------------------------------
// A variety of 14-Pin PICs that share the same digital IO assignment.

McuIoPinInfo Pic14PinIoInfo[] = {
//  { ' ', -1, 14, "Vss" },
    { 'A',  0, 13, "RA0 AN0"                },
    { 'A',  1, 12, "RA1 AN1"                },
    { 'A',  2, 11, "RA2 AN2/CCP3/INT"       },
    { 'A',  3,  4, "RA3 _MCLR (Input only)" },
    { 'A',  4,  3, "RA4 AN3/OSC2/CLKOUT"    },
    { 'A',  5,  2, "RA5 CCP2/OSC1/CLKIN"    },
//  { ' ', -1,  1, "Vdd" },
    { 'C',  0, 10, "RC0 AN4"                },
    { 'C',  1,  9, "RC1 AN5/CCP4"           },
    { 'C',  2,  8, "RC2 AN6/P2B"            },
    { 'C',  3,  7, "RC3 AN7/P2A"            },
    { 'C',  4,  6, "RC4 P1B/TX/CK"          },
    { 'C',  5,  5, "RC5 P1A/RX/DT/CCP1"     }
};

////     ti
//// pin mer
McuPwmPinInfo Pic16F1824PwmPinInfo[] = {
    {  5, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "" },
    {  6, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "" },
    {  7, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "" },
    {  8, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "" }
};

//-----------------------------------------------------------------------------
// A variety of 18-Pin PICs that share the same digital IO assignment.

McuIoPinInfo Pic18PinIoInfo[] = {
    { 'A',  2,  1, "RA2 AN2/VREF"           },
    { 'A',  3,  2, "RA3 AN3/CMP1"           },
    { 'A',  4,  3, "RA4 T0CKI/CMP2"         },
    { 'A',  5,  4, "RA5 _MCLR (Input Only)" },
//  { ' ', -1,  5, "Vss"                    },
    { 'B',  0,  6, "RB0 INT"                },
    { 'B',  1,  7, "RB1 RX/DT"              },
    { 'B',  2,  8, "RB2 TX/CK"              },
    { 'B',  3,  9, "RB3 CCP1"               },
    { 'B',  4, 10, "RB4"                    },
    { 'B',  5, 11, "RB5"                    },
    { 'B',  6, 12, "RB6"                    },
    { 'B',  7, 13, "RB7"                    },
//  { ' ', -1, 14, "Vdd"                    },
    { 'A',  6, 15, "RA6 OSC2/CLKOUT"        },
    { 'A',  7, 16, "RA7 OSC1/CLKIN"         },
    { 'A',  0, 17, "RA0 AN0"                },
    { 'A',  1, 18, "RA1 AN1"                }
};

McuAdcPinInfo Pic16F819AdcPinInfo[] = {
    {  1, 0x02 },
    {  2, 0x03 },
    {  3, 0x04 },
    { 17, 0x00 },
    { 18, 0x01 }
};

McuAdcPinInfo Pic16F1827AdcPinInfo[] = {
    { 17,   0x00 },
    { 18,   0x01 },
    {  1,   0x02 },
    {  2,   0x03 },
    {  3,   0x04 },
    { 12,   0x05 },
    { 13,   0x06 },
    { 11,   0x07 },
    { 10,   0x08 },
    {  9,   0x09 },
    {  8,   0x0a },
    {  7,   0x0b }
};

McuAdcPinInfo Pic16F1824AdcPinInfo[] = {
    { 13,   0x00 },
    { 12,   0x01 },
    { 11,   0x02 },
    {  3,   0x03 },
    { 10,   0x04 },
    {  9,   0x05 },
    {  8,   0x06 },
    {  7,   0x07 }
};

//-----------------------------------------------------------------------------
// PIC16F88

McuIoPinInfo Pic16F88PinIoInfo[] = {
    { 'A',  2,  1, "RA2 AN2"                },
    { 'A',  3,  2, "RA3 AN3/CMP1"           },
    { 'A',  4,  3, "RA4 AN4/T0CKI/CMP2"     },
    { 'A',  5,  4, "RA5 _MCLR (Input Only)" }, // without pull-up resistor
//  { ' ', -1,  5, "Vss"                    },
    { 'B',  0,  6, "RB0 INT"                },
    { 'B',  1,  7, "RB1 SDI/SDA"            },
    { 'B',  2,  8, "RB2 SDO/RX/DT"          },
    { 'B',  3,  9, "RB3 CCP1"               },
    { 'B',  4, 10, "RB4 SCK/SCL"            },
    { 'B',  5, 11, "RB5 _SS/TX/CK"          },
    { 'B',  6, 12, "RB6 AN5"                },
    { 'B',  7, 13, "RB7 AN6"                },
//  { ' ', -1, 14, "Vdd"                    },
    { 'A',  6, 15, "RA6 OSC2/CLKOUT"        },
    { 'A',  7, 16, "RA7 OSC1/CLKIN"         },
    { 'A',  0, 17, "RA0 AN0"                },
    { 'A',  1, 18, "RA1 AN1"                }
};

McuAdcPinInfo Pic16F88AdcPinInfo[] = {
    { 17, 0x00 },
    { 18, 0x01 },
    {  1, 0x02 },
    {  2, 0x03 },
    {  3, 0x04 },
    { 12, 0x05 },
    { 13, 0x06 }
};

//-----------------------------------------------------------------------------
// PIC16F1826, PIC16F1827

McuIoPinInfo Pic16f1827IoPinInfo[] = {
    { 'A',  0, 17, "RA0 AN0"                },
    { 'A',  1, 18, "RA1 AN1"                },
    { 'A',  2,  1, "RA2 AN2"                },
    { 'A',  3,  2, "RA3 AN3/CCP3"           },
    { 'A',  4,  3, "RA4 AN4/CCP4/T0CKI"     },
    { 'A',  5,  4, "RA5 _MCLR (Input Only)" },
    { 'A',  6, 15, "RA6 OSC2/CLKOUT"        }, // P1D        P2B
    { 'A',  7, 16, "RA7 OSC1/CLKIN"         }, // P1C        P2A
//  { ' ', -1,  5, "Vss" },
    { 'B',  0,  6, "RB0 INT"                }, // P1A
    { 'B',  1,  7, "RB1 AN11/RX/DT"         },
    { 'B',  2,  8, "RB2 AN10/TX/CK"         },
    { 'B',  3,  9, "RB3 AN9/CCP1/P1A"       }, // P1A def
    { 'B',  4, 10, "RB4 AN8"                },
    { 'B',  5, 11, "RB5 AN7/P1B"            }, // P1B
    { 'B',  6, 12, "RB6 AN5/CCP2/P2A/T1CKI" }, // P1C def    P2A def
    { 'B',  7, 13, "RB7 AN6/P2B"            }, // P1D def    P2B def
//  { ' ', -1, 14, "Vdd" },
};

////     ti
//// pin mer
McuPwmPinInfo Pic16F1827PwmPinInfo[] = {
//  {  9, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "" },
//  { 11, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "" },
    { 12, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "" }, // CCP2
//  { 13, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "" }
};

//-----------------------------------------------------------------------------
// PIC16F877, PIC16F874

McuIoPinInfo Pic16F877IoPinInfo[] = {
    { 'A',  0,  2, "RA0 AN0"        },
    { 'A',  1,  3, "RA1 AN1"        },
    { 'A',  2,  4, "RA2 AN2/VREF-"  },
    { 'A',  3,  5, "RA3 AN3/VREF+"  },
    { 'A',  4,  6, "RA4 T0CKI"      },
    { 'A',  5,  7, "RA5 AN4/_SS"    },
    { 'E',  0,  8, "RE0 _RD/AN5"    },
    { 'E',  1,  9, "RE1 _WR/AN6"    },
    { 'E',  2, 10, "RE2 _CS/AN7"    },
//  {           1, "_MCLR/Vpp"      },
    { 'C',  0, 15, "RC0 T1OSO/T1CKI"},
    { 'C',  1, 16, "RC1 T1OSI/CCP2" },
    { 'C',  2, 17, "RC2 CCP1"       },
    { 'C',  3, 18, "RC3 SCK/SCL"    },
    { 'D',  0, 19, "RD0 PSP0"       },
    { 'D',  1, 20, "RD1 PSP1"       },
    { 'D',  2, 21, "RD2 PSP2"       },
    { 'D',  3, 22, "RD3 PSP3"       },
    { 'C',  4, 23, "RC4 SDI/SDA"    },
    { 'C',  5, 24, "RC5 SDO"        },
    { 'C',  6, 25, "RC6 TX/CK"      },
    { 'C',  7, 26, "RC7 RX/DT"      },
    { 'D',  4, 27, "RD4 PSP4"       },
    { 'D',  5, 28, "RD5 PSP5"       },
    { 'D',  6, 29, "RD6 PSP6"       },
    { 'D',  7, 30, "RD7 PSP7"       },
    { 'B',  0, 33, "RB0 INT"        },
    { 'B',  1, 34, "RB1"            },
    { 'B',  2, 35, "RB2"            },
    { 'B',  3, 36, "RB3"            },
    { 'B',  4, 37, "RB4"            },
    { 'B',  5, 38, "RB5"            },
    { 'B',  6, 39, "RB6"            },
    { 'B',  7, 40, "RB7"            }
};

McuAdcPinInfo Pic16F877AdcPinInfo[] = {
    {  2,   0x00 },
    {  3,   0x01 },
    {  4,   0x02 },
    {  5,   0x03 },
    {  7,   0x04 },
    {  8,   0x05 },
    {  9,   0x06 },
    { 10,   0x07 }
};

//-----------------------------------------------------------------------------
// PIC16F72 28-Pin PDIP, SOIC, SSOP
McuIoPinInfo Pic16F72IoPinInfo[] = {
//  {          1, "MCLR/Vpp"},
    { 'A', 0,  2, "RA0 AN0"        },
    { 'A', 1,  3, "RA1 AN1"        },
    { 'A', 2,  4, "RA2 AN2"        },
    { 'A', 3,  5, "RA3 AN3/VREF"   },
    { 'A', 4,  6, "RA4 T0CKI"      },
    { 'A', 5,  7, "RA5 AN4/_SS"    },
//  {          8, "Vss"
//  {          9, "OSC1/CLKI"
//  {         10, "OSC2/CLKO"
    { 'C', 0, 11, "RC0 T1OSO/T1CKI"},
    { 'C', 1, 12, "RC1 T1OSI"      },
    { 'C', 2, 13, "RC2 CCP1"       },
    { 'C', 3, 14, "RC3 SCK/SCL"    },
    { 'C', 4, 15, "RC4 SDI/SDA"    },
    { 'C', 5, 16, "RC5 SDO"        },
    { 'C', 6, 17, "RC6"            },
    { 'C', 7, 18, "RC7"            },
//  {         19, "Vss"
//  {         20, "Vdd"
    { 'B', 0, 21, "RB0 INT"        },
    { 'B', 1, 22, "RB1"            },
    { 'B', 2, 23, "RB2"            },
    { 'B', 3, 24, "RB3"            },
    { 'B', 4, 25, "RB4"            },
    { 'B', 5, 26, "RB5"            },
    { 'B', 6, 27, "RB6"            },
    { 'B', 7, 28, "RB7"            }
};

McuAdcPinInfo Pic16F72AdcPinInfo[] = {
    {  2, 0x00 },
    {  3, 0x01 },
    {  4, 0x02 },
    {  5, 0x03 },
    {  7, 0x04 },
};

//-----------------------------------------------------------------------------
// PIC16F876, PIC16F873 28-Pin PDIP, SOIC
McuIoPinInfo Pic16F876IoPinInfo[] = {
    { 'A', 0,  2 , "RA0 AN0"         },
    { 'A', 1,  3 , "RA1 AN1"         },
    { 'A', 2,  4 , "RA2 AN2/VREF-"   },
    { 'A', 3,  5 , "RA3 AN3 VREF+"   },
    { 'A', 4,  6 , "RA4 T0CKI"       },
    { 'A', 5,  7 , "RA5 AN4/_SS"     },
    { 'C', 0, 11 , "RC0 T1OSO/T1CKI" },
    { 'C', 1, 12 , "RC1 T1OSI/CCP2"  },
    { 'C', 2, 13 , "RC2 CCP1"        },
    { 'C', 3, 14 , "RC3 SCK/SCL"     },
    { 'C', 4, 15 , "RC4 SDI/SDA"     },
    { 'C', 5, 16 , "RC5 SDO"         },
    { 'C', 6, 17 , "RC6 TX/CK"       },
    { 'C', 7, 18 , "RC7 RX/DT"       },
    { 'B', 0, 21 , "RB0 INT"         },
    { 'B', 1, 22 , "RB1"             },
    { 'B', 2, 23 , "RB2"             },
    { 'B', 3, 24 , "RB3 PGM"         },
    { 'B', 4, 25 , "RB4"             },
    { 'B', 5, 26 , "RB5"             },
    { 'B', 6, 27 , "RB6 PGC"         },
    { 'B', 7, 28 , "RB7 PGD"         }
};

McuAdcPinInfo Pic16F876AdcPinInfo[] = {
    {  2, 0x00 },
    {  3, 0x01 },
    {  4, 0x02 },
    {  5, 0x03 },
    {  7, 0x04 }
};

//-----------------------------------------------------------------------------
// PIC16F884, PIC16F887 44-TQFP

McuIoPinInfo Pic16F887IoPinInfo44TQFP[] = {
    { 'A',  0, 19, "RA0 AN0/ULPWU/C12IN0-"     },
    { 'A',  1, 20, "RA1 AN1/C12IN1-"           },
    { 'A',  2, 21, "RA2 AN2/VREF-/CVREF/C2IN+" },
    { 'A',  3, 22, "RA3 AN3/VREF+/C1IN+"       },
    { 'A',  4, 23, "RA4 T0CKI/C1OUT"           },
    { 'A',  5, 24, "RA5 AN4/_SS/C2OUT"         },
    { 'A',  6, 31, "RA6 OSC2/CLKOUT"           },
    { 'A',  7, 30, "RA7 OSC1/CLKIN"            },
    { 'B',  0,  8, "RB0 AN12/INT"              },
    { 'B',  1,  9, "RB1 AN10/C12IN3-"          },
    { 'B',  2, 10, "RB2 AN8"                   },
    { 'B',  3, 11, "RB3 AN9/C12IN2-"           },
    { 'B',  4, 14, "RB4 AN11"                  },
    { 'B',  5, 15, "RB5 AN13/_T1G"             },
    { 'B',  6, 16, "RB6 ICSPCLK"               },
    { 'B',  7, 17, "RB7 ICSPDAT"               },
    { 'C',  0, 32, "RC0 T1OSO/T1CKI"           },
    { 'C',  1, 35, "RC1 T1OSI/CCP2"            },
    { 'C',  2, 36, "RC2 P1A/CCP1"              },
    { 'C',  3, 37, "RC3 SCK/SCL"               },
    { 'C',  4, 42, "RC4 SDI/SDA"               },
    { 'C',  5, 43, "RC5 SDO"                   },
    { 'C',  6, 44, "RC6 TX/CK"                 },
    { 'C',  7,  1, "RC7 RX/DT"                 },
    { 'D',  0, 38, "RD0"                       },
    { 'D',  1, 39, "RD1"                       },
    { 'D',  2, 40, "RD2"                       },
    { 'D',  3, 41, "RD3"                       },
    { 'D',  4,  2, "RD4"                       },
    { 'D',  5,  3, "RD5 P1B"                   },
    { 'D',  6,  4, "RD6 P1C"                   },
    { 'D',  7,  5, "RD7 P1D"                   },
    { 'E',  0, 25, "RE0 AN5"                   },
    { 'E',  1, 26, "RE1 AN6"                   },
    { 'E',  2, 27, "RE2 AN7"                   },
    { 'E',  3, 18, "RE3 _MCLR (Input Only)"    }
};

McuAdcPinInfo Pic16F887AdcPinInfo44TQFP[] = {
    { 19,   0x00 },
    { 20,   0x01 },
    { 21,   0x02 },
    { 22,   0x03 },
    { 24,   0x04 },
    { 25,   0x05 },
    { 26,   0x06 },
    { 27,   0x07 },
    { 10,   0x08 },
    { 11,   0x09 },
    {  9,   0x0a },
    { 14,   0x0b },
    {  8,   0x0c },
    { 15,   0x0d }
};

//-----------------------------------------------------------------------------
// PIC16F887, PIC16F884

McuIoPinInfo Pic16F887IoPinInfo[] = {
    { 'A',  0,  2, "RA0 AN0/ULPWU/C12IN0-"     },
    { 'A',  1,  3, "RA1 AN1/C12IN1-"           },
    { 'A',  2,  4, "RA2 AN2/VREF-/CVREF/C2IN+" },
    { 'A',  3,  5, "RA3 AN3/VREF+/C1IN+"       },
    { 'A',  4,  6, "RA4 T0CKI/C1OUT"           },
    { 'A',  5,  7, "RA5 AN4/_SS/C2OUT"         },
    { 'E',  0,  8, "RE0 AN5"                   },
    { 'E',  1,  9, "RE1 AN6"                   },
    { 'E',  2, 10, "RE2 AN7"                   },
    { 'E',  3,  1, "RE3 _MCLR (Input Only)"    },
    { 'A',  7, 13, "RA7 OSC1/CLKIN"            },
    { 'A',  6, 14, "RA6 OSC2/CLKOUT"           },
    { 'C',  0, 15, "RC0 T1OSO/T1CKI"           },
    { 'C',  1, 16, "RC1 T1OSI/CCP2"            },
    { 'C',  2, 17, "RC2 P1A/CCP1"              },
    { 'C',  3, 18, "RC3 SCK/SCL"               },
    { 'D',  0, 19, "RD0"                       },
    { 'D',  1, 20, "RD1"                       },
    { 'D',  2, 21, "RD2"                       },
    { 'D',  3, 22, "RD3"                       },
    { 'C',  4, 23, "RC4 SDI/SDA"               },
    { 'C',  5, 24, "RC5 SDO"                   },
    { 'C',  6, 25, "RC6 TX/CK"                 },
    { 'C',  7, 26, "RC7 RX/DT"                 },
    { 'D',  4, 27, "RD4"                       },
    { 'D',  5, 28, "RD5 P1B"                   },
    { 'D',  6, 29, "RD6 P1C"                   },
    { 'D',  7, 30, "RD7 P1D"                   },
    { 'B',  0, 33, "RB0 AN12/INT"              },
    { 'B',  1, 34, "RB1 AN10/C12IN3-"          },
    { 'B',  2, 35, "RB2 AN8"                   },
    { 'B',  3, 36, "RB3 AN9/C12IN2-"           },
    { 'B',  4, 37, "RB4 AN11"                  },
    { 'B',  5, 38, "RB5 AN13/_T1G"             },
    { 'B',  6, 39, "RB6 ICSPCLK"               },
    { 'B',  7, 40, "RB7 ICSPDAT"               }
};

McuAdcPinInfo Pic16F887AdcPinInfo[] = {
    {  2,   0x00 },
    {  3,   0x01 },
    {  4,   0x02 },
    {  5,   0x03 },
    {  7,   0x04 },
    {  8,   0x05 },
    {  9,   0x06 },
    { 10,   0x07 },
    { 33,   0x0c },
    { 34,   0x0a },
    { 35,   0x08 },
    { 36,   0x09 },
    { 37,   0x0b },
    { 38,   0x0d }
};

//-----------------------------------------------------------------------------
// 28-Pin PDIP, SOIC, SSOP
// PIC16F886,  PIC16F883, PIC16F882
// PIC16F1512, PIC16F1513
// PIC16F1516, PIC16F1518

McuIoPinInfo Pic28Pin_SPDIP_SOIC_SSOP[] = {
    { 'E', 3,  1, "RE3 _MCLR (Input Only)"    },
    { 'A', 0,  2, "RA0 AN0/ULPWU/C12IN0-"     },
    { 'A', 1,  3, "RA1 AN1/C12IN1-"           },
    { 'A', 2,  4, "RA2 AN2/VREF-/CVREF/C2IN+" },
    { 'A', 3,  5, "RA3 AN3/VREF+/C1IN+"       },
    { 'A', 4,  6, "RA4 T0CKI/C1OUT"           },
    { 'A', 5,  7, "RA5 AN4/_SS/C2OUT"         },
//  {          8, "Vss"                       },
    { 'A', 7,  9, "RA7 OSC2/CLKOUT"           },
    { 'A', 6, 10, "RA6 OSC1/CLKIN"            },
    { 'C', 0, 11, "RC0 T1OSO/T1CKI"           },
    { 'C', 1, 12, "RC1 T1OSI/CCP2"            },
    { 'C', 2, 13, "RC2 P1A/CCP1"              },
    { 'C', 3, 14, "RC3 SCK/SCL"               },
    { 'C', 4, 15, "RC4 SDI/SDA"               },
    { 'C', 5, 16, "RC5 SDO"                   },
    { 'C', 6, 17, "RC6 TX/CK"                 },
    { 'C', 7, 18, "RC7 RX/DT"                 },
//  {         19, "Vss"                       },
//  {         20, "Vdd"                       },
    { 'B', 0, 21, "RB0 AN12/INT"              },
    { 'B', 1, 22, "RB1 AN10/P1C/C12IN3-"      },
    { 'B', 2, 23, "RB2 AN8/P1B"               },
    { 'B', 3, 24, "RB3 AN9/C12IN2-"           },
    { 'B', 4, 25, "RB4 AN11/P1D"              },
    { 'B', 5, 26, "RB5 AN13/_T1G"             },
    { 'B', 6, 27, "RB6 ICSPCLK"               },
    { 'B', 7, 28, "RB7 ICSPDAT"               }
};

McuAdcPinInfo Pic16F886AdcPinInfo[] = {
    {  2, 0x00 },
    {  3, 0x01 },
    {  4, 0x02 },
    {  5, 0x03 },
    {  7, 0x04 },
    { 21, 0x0c },
    { 22, 0x0a },
    { 23, 0x08 },
    { 24, 0x09 },
    { 25, 0x0b },
    { 26, 0x0d }
};

//-----------------------------------------------------------------------------
// PIC16F1512, PIC16F1513, PIC16F1516, PIC16F1518

McuAdcPinInfo Pic16F1512AdcPinInfo[] = {
//  PINx  ANx
    {  2,   0 },
    {  3,   1 },
    {  4,   2 },
    {  5,   3 },
    {  7,   4 },
    { 23,   8 },
    { 24,   9 },
    { 22,  10 },
    { 25,  11 },
    { 21,  12 },
    { 26,  13 },
    { 13,  14 },
    { 14,  15 },
    { 15,  16 },
    { 16,  17 },
    { 17,  18 },
    { 18,  19 }
};

//-----------------------------------------------------------------------------
// PIC16F1526, PIC16F1527

McuIoPinInfo Pic16F1527IoPinInfo[] = {
    { 'A', 0, 24, "" },
    { 'A', 1, 23, "" },
    { 'A', 2, 22, "" },
    { 'A', 3, 21, "" },
    { 'A', 4, 28, "" },
    { 'A', 5, 27, "" },
    { 'A', 6, 40, "" },
    { 'A', 7, 39, "" },
    { 'B', 0, 48, "" },
    { 'B', 1, 47, "" },
    { 'B', 2, 46, "" },
    { 'B', 3, 45, "" },
    { 'B', 4, 44, "" },
    { 'B', 5, 43, "" },
    { 'B', 6, 42, "" },
    { 'B', 7, 37, "" },
    { 'C', 0, 30, "" },
    { 'C', 1, 29, "" },
    { 'C', 2, 33, "" },
    { 'C', 3, 34, "" },
    { 'C', 4, 35, "" },
    { 'C', 5, 36, "" },
    { 'C', 6, 31, "" },
    { 'C', 7, 32, "" },
    { 'D', 0, 58, "" },
    { 'D', 1, 55, "" },
    { 'D', 2, 54, "" },
    { 'D', 3, 53, "" },
    { 'D', 4, 52, "" },
    { 'D', 5, 51, "" },
    { 'D', 6, 50, "" },
    { 'D', 7, 49, "" },
    { 'E', 0,  2, "" },
    { 'E', 1,  1, "" },
    { 'E', 2, 64, "" },
    { 'E', 3, 63, "" },
    { 'E', 4, 62, "" },
    { 'E', 5, 61, "" },
    { 'E', 6, 60, "" },
    { 'E', 7, 59, "" },
    { 'F', 0, 18, "" },
    { 'F', 1, 17, "" },
    { 'F', 2, 16, "" },
    { 'F', 3, 15, "" },
    { 'F', 4, 14, "" },
    { 'F', 5, 13, "" },
    { 'F', 6, 12, "" },
    { 'F', 7, 11, "" },
    { 'G', 0,  3, "" },
    { 'G', 1,  4, "" },
    { 'G', 2,  5, "" },
    { 'G', 3,  6, "" },
    { 'G', 4,  8, "" }
//  { 'G', 5,  7 }, //_MCLR
};

McuAdcPinInfo Pic16F1527AdcPinInfo[] = {
//  PINx  ANx
    { 24,   0 },
    { 23,   1 },
    { 22,   2 },
    { 21,   3 },
    { 27,   4 },
    { 11,   5 },
    { 17,   6 },
    { 16,   7 },
    { 15,   8 },
    { 14,   9 },
    { 13,  10 },
    { 12,  11 },
    {  8,  12 },
    {  6,  13 },
    {  5,  14 },
    {  4,  15 },
    { 18,  16 },
    { 48,  17 },
    { 47,  18 },
    { 46,  19 },
    { 45,  20 },
    { 44,  21 },
    { 43,  22 },
    { 58,  23 },
    { 55,  24 },
    { 54,  25 },
    { 53,  26 },
    {  2,  27 },
    {  1,  28 },
    { 64,  29 }
};

//-----------------------------------------------------------------------------
// 6-Pin SOT-23
// PIC10F200, PIC10F202, PIC10F204, PIC10F206
// PIC10F220, PIC10F222
McuIoPinInfo Pic6Pin_SOT23[] = {
    { 'P', 0, 1, "GP0 ICSPDAT"            },
    { 'P', 1, 3, "GP1 ICSPCLK"            },
    { 'P', 2, 4, "GP2 T0CKI/FOSC4"        },
    { 'P', 3, 6, "GP3 _MCLR (Input Only)" }
};

//-----------------------------------------------------------------------------
// 8-Pin PDIP, SOIC, DFN-S, DFN
// PIC12F629
// PIC12F675
// PIC12F683
// PIC12F752
McuIoPinInfo Pic8Pin[] = {
    { 'P', 0, 7, "GP0 AN0/CIN+/ICSPDAT"       },
    { 'P', 1, 6, "GP1 AN1/CIN-/VREF/ICSPCLK"  },
    { 'P', 2, 5, "GP2 AN2/T0CKI/INT/COUT"     },
    { 'P', 3, 4, "GP3 _MCLR (Input Only)"     },
    { 'P', 4, 3, "GP4 AN3/_T1G/OSC2/CLKOUT"   },
    { 'P', 5, 2, "GP5 T1CKI/OSC1/CLKIN"       }
};

McuAdcPinInfo Pic8PinAdcPinInfo[] = {
    {  7, 0x00 },
    {  6, 0x01 },
    {  5, 0x02 },
    {  3, 0x03 }
};

//-----------------------------------------------------------------------------
// 8-Pin PDIP, SOIC, DFN, UDFN
// PIC12F1840
// PIC12F1822
McuIoPinInfo Pic8Pin_[] = {
    { 'A', 0, 7, "RA0 AN0/TX/SDO"             },
    { 'A', 1, 6, "RA1 AN1/RX/SCL/SCK"         },
    { 'A', 2, 5, "RA2 AN2/CCP1/SDA/SDI"       },
    { 'A', 3, 4, "RA3 _MCLR/_SS (Input Only)" },
    { 'A', 4, 3, "RA4 AN3"                    },
    { 'A', 5, 2, "RA5 "                       }
};

//-----------------------------------------------------------------------------
// PIC18F4520
//-----------------------------------------------------------------------------
// PIC18F4520

McuIoPinInfo Pic18F4520IoPinInfo[] = {
    { 'A',  0,  2, "RA0 AN0"         },
    { 'A',  1,  3, "RA1 AN1"         },
    { 'A',  2,  4, "RA2 AN2/VREF-"   },
    { 'A',  3,  5, "RA3 AN3/VREF+"   },
    { 'A',  4,  6, "RA4 T0CKI"       },
    { 'A',  5,  7, "RA5 AN4/_SS"     },
    { 'E',  0,  8, "RE0 _RD/AN5"     },
    { 'E',  1,  9, "RE1 _WR/AN6"     },
    { 'E',  2, 10, "RE2 _CS/AN7"     },
//  {           1, "_MCLR/Vpp"       },
    { 'C',  0, 15, "RC0 T1OSO/T13CKI"},
    { 'C',  1, 16, "RC1 T1OSI/CCP2"  },
    { 'C',  2, 17, "RC2 CCP1/P1A"    },
    { 'C',  3, 18, "RC3 SCK/SCL"     },
    { 'D',  0, 19, "RD0 PSP0"        },
    { 'D',  1, 20, "RD1 PSP1"        },
    { 'D',  2, 21, "RD2 PSP2"        },
    { 'D',  3, 22, "RD3 PSP3"        },
    { 'C',  4, 23, "RC4 SDI/SDA"     },
    { 'C',  5, 24, "RC5 SDO"         },
    { 'C',  6, 25, "RC6 TX/CK"       },
    { 'C',  7, 26, "RC7 RX/DT"       },
    { 'D',  4, 27, "RD4 PSP4"        },
    { 'D',  5, 28, "RD5 PSP5/P1B"    },
    { 'D',  6, 29, "RD6 PSP6/P1C"    },
    { 'D',  7, 30, "RD7 PSP7/P1D"    },
    { 'B',  0, 33, "RB0 INT0"        },
    { 'B',  1, 34, "RB1 INT1"        },
    { 'B',  2, 35, "RB2 INT2"        },
    { 'B',  3, 36, "RB3 CCP2"        },
    { 'B',  4, 37, "RB4 KBI0"        },
    { 'B',  5, 38, "RB5 KBI1"        },
    { 'B',  6, 39, "RB6 KBI2"        },
    { 'B',  7, 40, "RB7 KBI3"        }
};

McuAdcPinInfo Pic18F4520AdcPinInfo[] = {
    {  2,   0x00 },
    {  3,   0x01 },
    {  4,   0x02 },
    {  5,   0x03 },
    {  7,   0x04 },
    {  8,   0x05 },
    {  9,   0x06 },
    {  10,  0x07 }
};

//-----------------------------------------------------------------------------
// ESP8266
McuIoPinInfo ESP8266IoPinInfo[] = {
//   port bit pin pinName
//  { ' ',  0,  1, "RST"},
    { 'A',  0,  2, "ADC"         },
    { ' ',  0,  3, "EN"          },
    { 'O', 16,  4, "GPIO16 WAKE" },
    { 'O', 14,  5, "GPIO14 SCL"  },
    { 'O', 12,  6, "GPIO12"      },
    { 'O', 13,  7, "GPIO13"      },
//  { ' ',  0,  8, "VCC"},
    { ' ',  0,  9, "GPIO11 CS0"  },
    { ' ',  0, 10, "GPIO7 MISO"  },
    { ' ',  0, 11, "GPIO9"       },
    { ' ',  0, 12, "GPIO10"      },
    { ' ',  0, 13, "GPIO8 MOSI"  },
    { ' ',  0, 14, "GPIO6 SCLK"  },
//  { ' ',  0, 15, "GND"},
    { 'O', 15, 16, "GPIO15"      },
    { 'O',  2, 17, "GPIO2 SDA"   },
    { 'O',  0, 18, "GPIO0"       },
    { 'O',  4, 19, "GPIO4"       },
    { 'O',  5, 20, "GPIO5"       },
    { 'O',  3, 21, "GPIO3 RXD0"  },
    { 'O',  1, 22, "GPIO1 TXD0"  }
};

McuAdcPinInfo ESP8266AdcPinInfo[] = {
    { 2, 0x00 },
};
/*
McuPwmPinInfo ESP8266PwmPinInfo[] = {
////     ti
//// pin mer
//  {  1, 0 },
};
*/
//-----------------------------------------------------------------------------
// Controllino Maxi
McuIoPinInfo ControllinoMaxiIoPinInfo[] = {
//   port bit pin pinName ArduinoPin
///// the lack of TX and RX defined pins generates a big crash when trying to use UART SEND/RECV (Note by JG)
    { 'E',  4,  6, "D0",  2  , ""},
    { 'E',  5,  7, "D1",  3  , ""},
    { 'G',  5,  1, "D2",  4  , ""},
    { 'E',  3,  5, "D3",  5  , ""},
    { 'H',  3, 15, "D4",  6  , ""},
    { 'H',  4, 16, "D5",  7  , ""},
    { 'H',  5, 17, "D6",  8  , ""},
    { 'H',  6, 18, "D7",  9  , ""},
    { 'B',  4, 23, "D8",  10 , ""},
    { 'B',  5, 24, "D9",  11 , ""},
    { 'B',  6, 25, "D10", 12 , ""},
    { 'B',  7, 26, "D11", 13 , ""},
    { 'A',  0, 78, "R0",  22 , ""},
    { 'A',  1, 77, "R1",  23 , ""},
    { 'A',  2, 76, "R2",  24 , ""},
    { 'A',  3, 75, "R3",  25 , ""},
    { 'A',  4, 74, "R4",  26 , ""},
    { 'A',  5, 73, "R5",  27 , ""},
    { 'A',  6, 72, "R6",  28 , ""},
    { 'C',  6, 59, "R9",  29 , ""},
    { 'A',  7, 71, "R7",  30 , ""},
    { 'C',  7, 60, "R8",  31 , ""},
    { 'F',  0, 97, "A0",  54 , ""},
    { 'F',  1, 96, "A1",  55 , ""},
    { 'F',  2, 95, "A2",  56 , ""},
    { 'F',  3, 94, "A3",  57 , ""},
    { 'F',  4, 93, "A4",  58 , ""},
    { 'F',  5, 92, "A5",  59 , ""},
    { 'F',  6, 91, "A6",  60 , ""},
    { 'F',  7, 90, "A7",  61 , ""},
    { 'K',  0, 89, "A8",  62 , ""},
    { 'K',  1, 88, "A9",  63 , ""}
};

McuAdcPinInfo ControllinoMaxiAdcPinInfo[] = {
    { 97, 0x00 },
    { 96, 0x01 },
    { 95, 0x02 },
    { 94, 0x03 },
    { 93, 0x04 },
    { 92, 0x05 },
    { 91, 0x06 },
    { 90, 0x07 },
    { 89, 0x08 },
    { 88, 0x09 }
};

McuPwmPinInfo ControllinoMaxiPwmPinInfo[] = {
////ATmega2560
////     ti
//// pin mer
    {  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "" },
    { 24, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "" },
    { 25, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "" },
    { 26, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "" },
    { 18, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "" },
    { 23, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "" },
    {  5, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "" },
    {  6, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "" },
    {  7, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "" },
    { 15, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "" },
    { 16, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "" },
    { 17, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "" },
    { 38, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "" },
    { 39, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "" },
    { 40, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "" }
};

///// Added by JG       // A completer
//-----------------------------------------------------------------------------
McuIoPinInfo ArmSTM32F40X_144LQFPIoPinInfo[] = {
//  ARM STM32F40X
    { 'A',  0,  34, "PA0 TX4"               },
    { 'A',  1,  35, "PA1 RX4"               },
    { 'A',  2,  36, "PA2 TX2"               },
    { 'A',  3,  37, "PA3 RX2"               },
    { 'A',  4,  40, "PA4 ADC1.4"            },
    { 'A',  5,  41, "PA5 ADC1.5/SCK1/PWM2.1"},
    { 'A',  6,  42, "PA6 ADC1.6/MISO1"      },
    { 'A',  7,  43, "PA7 MOSI1"             },
    { 'A',  8, 100, "PA8"                   },
    { 'A',  9, 101, "PA9 TX1"               },
    { 'A', 10, 102, "PA10 RX1"              },
    { 'A', 11, 103, "PA11"                  },
    { 'A', 12, 104, "PA12"                  },
    { 'A', 13, 105, "PA013"                 },
    { 'A', 14, 109, "PA14"                  },
    { 'A', 15, 110, "PA15"                  },
    { 'B',  0,  46, "PB0"                   },
    { 'B',  1,  47, "PB1"                   },
    { 'B',  2,  48, "PB2"                   },
    { 'B',  3, 133, "PB3 PWM2.2"            },
    { 'B',  4, 134, "PB4 ADC2.4"            },
    { 'B',  5, 135, "PB5 ADC2.5"            },
    { 'B',  6, 136, "PB6 ADC2.6"            },
    { 'B',  7, 137, "PB7"                   },
    { 'B',  8, 139, "PB8 SCL1"              },
    { 'B',  9, 140, "PB9 SDA1"              },
    { 'B', 10,  69, "PB10 TX3/SCK2/PWM2.3"  },
    { 'B', 11,  70, "PB11 RX3/PWM2.4"       },
    { 'B', 12,  73, "PB12 _SS1"             },
    { 'B', 13,  74, "PB13"                  },
    { 'B', 14,  75, "PB14"                  },
    { 'B', 15,  76, "PB15"                  },
    { 'C',  0,  26, "PC0"                   },
    { 'C',  1,  27, "PC1"                   },
    { 'C',  2,  28, "PC2 MISO2"             },
    { 'C',  3,  29, "PC3 MOSI2"             },
    { 'C',  4,  44, "PC4"                   },
    { 'C',  5,  45, "PC5"                   },
    { 'C',  6,  96, "PC6 TX6"               },
    { 'C',  7,  97, "PC7 RX6"               },
    { 'C',  8,  98, "PC8"                   },
    { 'C',  9,  99, "PC9"                   },
    { 'C', 10, 111, "PC10"                  },
    { 'C', 11, 112, "PC11"                  },
    { 'C', 12, 113, "PC12 TX5"              },
    { 'C', 13,   7, "PC13"                  },
    { 'C', 14,   8, "PC14"                  },
    { 'C', 15,   9, "PC15 _SS3"             },
    { 'D',  0, 114, "PD0"                   },
    { 'D',  1, 115, "PD1"                   },
    { 'D',  2, 116, "PD2 RX5"               },
    { 'D',  3, 117, "PD3"                   },
    { 'D',  4, 118, "PD4"                   },
    { 'D',  5, 119, "PD5"                   },
    { 'D',  6, 122, "PD6"                   },
    { 'D',  7, 123, "PD7"                   },
    { 'D',  8,  77, "PD8"                   },
    { 'D',  9,  78, "PD9"                   },
    { 'D', 10,  79, "PD10"                  },
    { 'D', 11,  80, "PD11"                  },
    { 'D', 12,  81, "PD12 PWM4.1"           },
    { 'D', 13,  82, "PD13 PWM4.2"           },
    { 'D', 14,  85, "PD14 PWM4.3"           },
    { 'D', 15,  86, "PD15 PWM4.4"           },
    { 'E',  0, 141, "PE0"                   },
    { 'E',  1, 142, "PE1"                   },
    { 'E',  2,   1, "PE2"                   },
    { 'E',  3,   2, "PE3"                   },
    { 'E',  4,   3, "PE4"                   },
    { 'E',  5,   4, "PE5"                   },
    { 'E',  6,   5, "PE6"                   },
    { 'E',  7,  58, "PE7"                   },
    { 'E',  8,  59, "PE8"                   },
    { 'E',  9,  60, "PE9 PWM1.1"            },
    { 'E', 10,  63, "PE10 PWM1.2"           },
    { 'E', 11,  64, "PE11"                  },
    { 'E', 12,  65, "PE12"                  },
    { 'E', 13,  66, "PE13 PWM1.3"           },
    { 'E', 14,  67, "PE14 PWM1.4"           },
    { 'E', 15,  68, "PE15"                  },
    { 'F',  0,  10, "PF0 SDA2"              },
    { 'F',  1,  11, "PF1 SCL2"              },
    { 'F',  2,  12, "PF2"                   },
    { 'F',  3,  13, "PF3"                   },
    { 'F',  4,  14, "PF4"                   },
    { 'F',  5,  15, "PF5"                   },
    { 'F',  6,  18, "PF6"                   },
    { 'F',  7,  19, "PF7"                   },
    { 'F',  8,  20, "PF8 ADC3.6"            },
    { 'F',  9,  21, "PF9 ADC3.7"            },
    { 'F', 10,  22, "PF10 ADC3.8"           },
    { 'F', 11,  49, "PF11"                  },
    { 'F', 12,  50, "PF12"                  },
    { 'F', 13,  53, "PF13"                  },
    { 'F', 14,  54, "PF14"                  },
    { 'F', 15,  55, "PF15"                  },
    { 'G',  0,  56, "PG0"                   },
    { 'G',  1,  57, "PG1"                   },
    { 'G',  2,  87, "PG2"                   },
    { 'G',  3,  88, "PG3"                   },
    { 'G',  4,  89, "PG4"                   },
    { 'G',  5,  90, "PG5"                   },
    { 'G',  6,  91, "PG6"                   },
    { 'G',  7,  92, "PG7"                   },
    { 'G',  8,  93, "PG8"                   },
    { 'G',  9, 124, "PG9"                   },
    { 'G', 10, 125, "PG10 _SS2"             },
    { 'G', 11, 126, "PG11"                  },
    { 'G', 12, 128, "PG12"                  },
    { 'G', 13, 128, "PG13"                  },
    { 'G', 14, 129, "PG14"                  },
    { 'G', 15, 132, "PG15"                  }
};

McuAdcPinInfo ArmSTM32F40X_144LQFPAdcPinInfo[] = {
//  ARM STM32F40X
//  {pin, channel}
    {  40, 0x04 },      // ADC1 channel 4 on PA4
    {  41, 0x05 },      // ADC1 channel 5 on PA5
    {  42, 0x06 },      // ADC1 channel 6 on PA6
    { 134, 0x04 },      // ADC2 channel 4 on PA4        // appears as PB4 because of ADC1
    { 135, 0x05 },      // ADC2 channel 5 on PA5        // appears as PB5 because of ADC1
    { 136, 0x06 },      // ADC2 channel 6 on PA4        // appears as PB5 because of ADC1
    {  20, 0x06 },      // ADC3 channel 6 on PF8
    {  21, 0x07 },      // ADC3 channel 7 on PF9
    {  22, 0x08 }       // ADC3 channel 8 on PF10
};

McuPwmPinInfo ArmSTM32F40X_144LQFPPwmPinInfo[] = {
//  ARM STM32F40X
//  {pin, timer, resol}
    {  60, 1, 16 , 0, 0, 0, 0, 0, 0, 0, 0, 0, ""},       // PWM1 on PE9-PE10 + PE13-14
    {  63, 1, 16 , 0, 0, 0, 0, 0, 0, 0, 0, 0, ""},
    {  66, 1, 16 , 0, 0, 0, 0, 0, 0, 0, 0, 0, ""},
    {  67, 1, 16 , 0, 0, 0, 0, 0, 0, 0, 0, 0, ""},
    {  41, 1, 32 , 0, 0, 0, 0, 0, 0, 0, 0, 0, ""},       // PWM2 on PA5-PB3 + PB10-PB11
    { 133, 2, 32 , 0, 0, 0, 0, 0, 0, 0, 0, 0, ""},
    {  69, 2, 32 , 0, 0, 0, 0, 0, 0, 0, 0, 0, ""},
    {  70, 2, 32 , 0, 0, 0, 0, 0, 0, 0, 0, 0, ""},
    {  81, 4, 16 , 0, 0, 0, 0, 0, 0, 0, 0, 0, ""},       // PWM4 on PD12 to PD15
    {  82, 4, 16 , 0, 0, 0, 0, 0, 0, 0, 0, 0, ""},
    {  85, 4, 16 , 0, 0, 0, 0, 0, 0, 0, 0, 0, ""},
    {  86, 4, 16 , 0, 0, 0, 0, 0, 0, 0, 0, 0, ""}
};

McuSpiInfo ArmSTM32F40X_144LQFPMcuSpiInfo[] = {
//  name, REG_CTRL, STAT, DATA, MISO, MOSI, SCK,  SS
#if 1
    { "SPI1", 0,       0,    0,   42,   43,  41,  73 }, // SPI1 = PA6 + PA7 + PA5  + PB12 // ??? not corresponds to manual STM32F405xx, STM32F407xx
    { "SPI2", 0,       0,    0,   28,   29,  69, 125 }, // SPI2 = PC2 + PC3 + PB10 + PG10 // ???
    { "SPI3", 0,       0,    0,  134,  135, 133,   9 }  // SPI3 = PB4 + PB5 + PB3  + PC15 // ???
#else
    { "SPI1", 0,       0,    0,   42,   43,  41,  40 }, // SPI1 = PA6 + PA7 + PA5  + PA4
    { "SPI2", 0,       0,    0,   28,   29,  69,  73 }, // SPI2 = PC2 + PC3 + PB10 + PB12
    { "SPI3", 0,       0,    0,  112,  113, 111, 110 }  // SPI3 = PC11+ PC12+ PC10 + PA15
#endif
//  NB: SS is in fact user defined - if need be - (software SS) but a pin must be declared
};

McuI2cInfo ArmSTM32F40X_144LQFPMcuI2cInfo[] = {
//  name, REG_CTRL, REG_STAT, REG_DATA, REG_RATE, SCL, SDA
    {"I2C1", 0, 0, 0, 0, 139, 140 },                // I2C1 = PB8 + PB9
    {"I2C2", 0, 0, 0, 0,  11,  10 },                // I2C2 = PF1 + PF0
//  {"I2C3", 0, 0, 0, 0, 0, 0 }                     // I2C3 = PH7 + PH8
//  NB: All pins for a same I2C peripheral must be on the same port PX because of PinsForI2cVariable()
};

McuIoPinInfo ArmSTM32F10X_48LQFPIoPinInfo[] = {                 // Used by Bluepill ARM Board
//  ARM STM32F10X
    { 'A',   0, 10, "PA0 ADC1.0/PWM2.1"     },
    { 'A',   1, 11, "PA1 ADC1.1/PWM2.2"     },
    { 'A',   2, 12, "PA2 ADC1.2/PWM2.3/TX2" },
    { 'A',   3, 13, "PA3 ADC1.3/PWM2.4/RX2" },
    { 'A',   4, 14, "PA4 ADC2.4/_SS1"       },
    { 'A',   5, 15, "PA5 ADC2.5/SCK1"       },
    { 'A',   6, 16, "PA6 ADC2.6/MISO1"      },
    { 'A',   7, 17, "PA7 ADC2.7/MOSI1"      },
    { 'A',   8, 29, "PA8 PWM1.1"            },
    { 'A',   9, 30, "PA9 PWM1.2/TX1"        },
    { 'A',  10, 31, "PA10 PWM1.3/RX1"       },
    { 'A',  11, 32, "PA11 PWM1.4"           },
    { 'A',  12, 33, "PA12"                  },
    { 'A',  13, 34, "PA13 JTAG"             },
    { 'A',  14, 37, "PA14 JTAG"             },
    { 'A',  15, 38, "PA15 JTAG"             },
    { 'B',   0, 18, "PB0"                   }, // ADC Channel 8 not supported by libs
    { 'B',   1, 19, "PB1"                   }, // ADC Channel 9 not supported by libs
    { 'B',   2, 20, "PB2 BOOT"              },
    { 'B',   3, 39, "PB3 JTAG"              },
    { 'B',   4, 40, "PB4 JTAG"              },
    { 'B',   5, 41, "PB5"                   },
    { 'B',   6, 42, "PB6 PWM4.1/SCL1"       },
    { 'B',   7, 43, "PB7 PWM4.2/SDA1"       },
    { 'B',   8, 45, "PB8 PWM4.3"            },
    { 'B',   9, 46, "PB9 PWM4.4"            },
    { 'B',  10, 21, "PB10 TX3/SCL2"         },
    { 'B',  11, 22, "PB11 RX3/SDA2"         },
    { 'B',  12, 25, "PB12 _SS2"             },
    { 'B',  13, 26, "PB13 SCK2"             },
    { 'B',  14, 27, "PB14 MISO2"            },
    { 'B',  15, 28, "PB15 MOSI2"            }
    // PC13-15 not supported
};

McuAdcPinInfo ArmSTM32F10X_48LQFPAdcPinInfo[] = {
//  ARM STM32F410X
//  {pin, channel}
    { 10, 0x00 },       // ADC1 channel 0 on PA0
    { 11, 0x01 },       // ADC1 channel 1 on PA1
    { 12, 0x02 },       // ADC1 channel 2 on PA2
    { 13, 0x03 },       // ADC1 channel 3 on PA3
    { 14, 0x04 },       // ADC1 channel 4 on PA4
    { 15, 0x05 },       // ADC1 channel 5 on PA5
    { 16, 0x06 },       // ADC1 channel 6 on PA6
    { 17, 0x07 }        // ADC1 channel 7 on PA7
};

McuPwmPinInfo ArmSTM32F10X_48LQFPPwmPinInfo[] = {
//  ARM STM32F10X
//  {pin, timer, resol}
    { 29, 1, 16 , 0, 0, 0, 0, 0, 0, 0, 0, 0, ""},       // PWM1 on PA8 to PA11
    { 30, 1, 16 , 0, 0, 0, 0, 0, 0, 0, 0, 0, ""},
    { 31, 1, 16 , 0, 0, 0, 0, 0, 0, 0, 0, 0, ""},
    { 32, 1, 16 , 0, 0, 0, 0, 0, 0, 0, 0, 0, ""},
    { 10, 2, 16 , 0, 0, 0, 0, 0, 0, 0, 0, 0, ""},       // PWM2 on PA0 to PA3
    { 11, 2, 16 , 0, 0, 0, 0, 0, 0, 0, 0, 0, ""},
    { 12, 2, 16 , 0, 0, 0, 0, 0, 0, 0, 0, 0, ""},
    { 13, 2, 16 , 0, 0, 0, 0, 0, 0, 0, 0, 0, ""},
    { 42, 4, 16 , 0, 0, 0, 0, 0, 0, 0, 0, 0, ""},       // PWM4 on PB6 to PB9
    { 43, 4, 16 , 0, 0, 0, 0, 0, 0, 0, 0, 0, ""},
    { 45, 4, 16 , 0, 0, 0, 0, 0, 0, 0, 0, 0, ""},
    { 46, 4, 16 , 0, 0, 0, 0, 0, 0, 0, 0, 0, ""}
};

McuSpiInfo ArmSTM32F10X_48LQFPMcuSpiInfo[] = {
//  name,  CTRL, STAT, DATA, MISO, MOSI, SCK, SS
    { "SPI1", 0,    0,    0,   16,   17,  15, 14 }, // SPI1 = PA6  + PA7  + PA5  + PA4
    { "SPI2", 0,    0,    0,   27,   28,  26, 25 }, // SPI2 = PB14 + PB15 + PB13 + PB12
//  NB: SS is in fact user defined - if need be - (software SS) but a pin must be declared
};

McuI2cInfo ArmSTM32F10X_48LQFPMcuI2cInfo[] = {
//  name, REG_CTRL, REG_STAT, REG_DATA, REG_RATE, SCL, SDA
    { "I2C1", 0, 0, 0, 0, 42, 43 },                 // I2C1 = PB6 + PB7
    { "I2C2", 0, 0, 0, 0, 21, 22 },                 // I2C2 = PB10 + PB11
//  NB: All pins for a same I2C peripheral must be on the same port PX because of PinsForI2cVariable()
};
/////


//-----------------------------------------------------------------------------
// PC LPT & COM
McuIoPinInfo PcCfg[] = {
// Dynamically loaded by LoadPcPorts() in psports.cpp
//   port; bit; pin; pinName; ArduinoPin; ArduinoName; pinFunctions; pinUsed;  portN; dbPin; ioType
    { 'L',   0,   1,      "",          0,          "",/*          0,       0,*/    1,     2,      2, 0},
};

//===========================================================================
McuIoInfo SupportedMcus_[] = {
    {
        "Atmel AVR ATmega2560 100-TQFP",
        "ATmega2560",
        "m2560def",
        "mega2560",
        "iom2560", // AVRGCC
        'P',
//        A     B     C     D     E     F     G     H      I   J      K      L
        { 0x20, 0x23, 0x26, 0x29, 0x2C, 0x2F, 0x32, 0x100, 0,  0x103, 0x106, 0x109 }, // PINx  input
        { 0x22, 0x25, 0x28, 0x2B, 0x2E, 0x31, 0x34, 0x102, 0,  0x105, 0x108, 0x10B }, // PORTx output
        { 0x21, 0x24, 0x27, 0x2A, 0x2D, 0x30, 0x33, 0x101, 0,  0x104, 0x107, 0x10A }, // DDRx  dir
        128*1024,
        { { 0x200, 8192 } },
        AvrAtmega2560_100TQFPIoPinInfo,
        arraylen(AvrAtmega2560_100TQFPIoPinInfo),
        AvrAtmega2560_100TQFPAdcPinInfo,
        arraylen(AvrAtmega2560_100TQFPAdcPinInfo),
        1023,
        { 2 , 3 },
//        23,
        ISA_AVR,
        EnhancedCore4M,
        100,
        0,
        AvrAtmega2560PwmPinInfo,
        arraylen(AvrAtmega2560PwmPinInfo),
        nullptr,
        0,
        McuSpiInfoATmega2560,
        arraylen(McuSpiInfoATmega2560),
        McuI2cInfoATmega2560,
        arraylen(McuI2cInfoATmega2560),
        {{0,0}}
    },
    {
        "Atmel AVR AT90USB647 64-TQFP",
        "AT90USB647",
        "",
        "",
        "iousb647", // AVRGCC
        'P',
        { 0x20, 0x23, 0x26, 0x29, 0x2C, 0x2F }, // PINx
        { 0x22, 0x25, 0x28, 0x2B, 0x2E, 0x31 }, // PORTx
        { 0x21, 0x24, 0x27, 0x2A, 0x2D, 0x30 }, // DDRx
        64*1024,
        { { 0x100, 4096 } },
        AvrAT90USB647_64TQFPIoPinInfo,
        arraylen(AvrAT90USB647_64TQFPIoPinInfo),
        AvrAT90USB647_64TQFPAdcPinInfo,
        arraylen(AvrAT90USB647_64TQFPAdcPinInfo),
        1023,
        { 27, 28 },
//        17,
        ISA_AVR,
        EnhancedCore128K, //???
        64,
        0,
        AvrPwmPinInfo64_,
        arraylen(AvrPwmPinInfo64_),
        nullptr,
        0,
        nullptr,
        0,
        nullptr,
        0,
        {{0,0}}
    },
    {
        "Atmel AVR ATmega128 64-TQFP",
        "ATmega128",
        "",
        "",
        "iom128", // AVRGCC
        'P',
//        A     B     C     D     E     F     G     H      I   J      K      L
        { 0x39, 0x36, 0x33, 0x30, 0x21, 0x20, 0x63 }, // PINx
        { 0x3b, 0x38, 0x35, 0x32, 0x23, 0x62, 0x65 }, // PORTx
        { 0x3a, 0x37, 0x34, 0x31, 0x22, 0x61, 0x64 }, // DDRx
        64*1024,
        { { 0x100, 4096 } },
        AvrAtmega128_64TQFPIoPinInfo,
        arraylen(AvrAtmega128_64TQFPIoPinInfo),
        AvrAtmega128_64TQFPAdcPinInfo,
        arraylen(AvrAtmega128_64TQFPAdcPinInfo),
        1023,
        { 27, 28 },
//        17,
        ISA_AVR,
        EnhancedCore128K,
        64,
        0,
        AvrPwmPinInfo64_,
        arraylen(AvrPwmPinInfo64_),
        nullptr,
        0,
        McuSpiInfoATmega64,
        arraylen(McuSpiInfoATmega64),
        McuI2cInfoATmega64,
        arraylen(McuI2cInfoATmega64),
        {{0,0}}
    },
    {
        "Atmel AVR ATmega64 64-TQFP",
        "ATmega64",
        "",
        "",
        "iom64", // AVRGCC
        'P',
        { 0x39, 0x36, 0x33, 0x30, 0x21, 0x20, 0x63 }, // PINx
        { 0x3b, 0x38, 0x35, 0x32, 0x23, 0x62, 0x65 }, // PORTx
        { 0x3a, 0x37, 0x34, 0x31, 0x22, 0x61, 0x64 }, // DDRx
        32*1024,
        { { 0x100, 4096 } },
        AvrAtmega128_64TQFPIoPinInfo,
        arraylen(AvrAtmega128_64TQFPIoPinInfo),
        AvrAtmega128_64TQFPAdcPinInfo,
        arraylen(AvrAtmega128_64TQFPAdcPinInfo),
        1023,
        { 27, 28 },
//        17,
        ISA_AVR,
        EnhancedCore128K,
        64,
        0,
        AvrPwmPinInfo64_,
        arraylen(AvrPwmPinInfo64_),
        nullptr,
        0,
        McuSpiInfoATmega64,
        arraylen(McuSpiInfoATmega64),
        McuI2cInfoATmega64,
        arraylen(McuI2cInfoATmega64),
        {{0,0}}
    },
    {
        "Atmel AVR ATmega162 40-PDIP",
        "ATmega162",
        "",
        "",
        "iom162", // AVRGCC
        'P',
        { 0x39, 0x36, 0x33, 0x30, 0x25 }, // PINx
        { 0x3b, 0x38, 0x35, 0x32, 0x27 }, // PORTx
        { 0x3a, 0x37, 0x34, 0x31, 0x26 }, // DDRx
        8*1024,
        { { 0x100, 1024 } },
        AvrAtmega162IoPinInfo,
        arraylen(AvrAtmega162IoPinInfo),
        nullptr,
        0,
        0,
        { 0, 0 },
//        2, // OC2
        ISA_AVR,
        EnhancedCore128K,
        40,
        0,
        AvrAtmega162PwmPinInfo40_,
        arraylen(AvrAtmega162PwmPinInfo40_),
        nullptr,
        0,
        McuSpiInfoATmega16,
        arraylen(McuSpiInfoATmega16),
        nullptr,
        0,
        {{0,0}}
    },
    {
        "Atmel AVR ATmega32U4 44-Pin packages",
        "ATmega32U4",
        "",
        "",
        "iom32u4", // AVRGCC
        'P',
//        A     B     C     D     E     F     G     H      I   J      K      L
        { 0, 0x23, 0x26, 0x29, 0x2C, 0x2F }, // PINx
        { 0, 0x25, 0x28, 0x2B, 0x2E, 0x31 }, // PORTx
        { 0, 0x24, 0x27, 0x2A, 0x2D, 0x30 }, // DDRx
        16*1024,
        { { 0x100, 2560 } },
        AvrAtmega16U4or32U4IoPinInfo44,
        arraylen(AvrAtmega16U4or32U4IoPinInfo44),
        AvrAtmega16U4or32U4AdcPinInfo44,
        arraylen(AvrAtmega16U4or32U4AdcPinInfo44),
        1023,
        { 20, 21 },
//        0, // OC2
        ISA_AVR,
        EnhancedCore128K,
        44,
        0,
        ATmegaXXU4PwmPinInfo,
        arraylen(ATmegaXXU4PwmPinInfo),
        nullptr,
        0,
        nullptr,
        0,
        nullptr,
        0,
        {{0,0}}
    },
    {
        "Atmel AVR ATmega32 44-Pin packages",
        "ATmega32",
        "m32def",
        "mega32",
        "iom32", // AVRGCC
        'P',
        { 0x39, 0x36, 0x33, 0x30 }, // PINx
        { 0x3b, 0x38, 0x35, 0x32 }, // PORTx
        { 0x3a, 0x37, 0x34, 0x31 }, // DDRx
        16*1024,
        { { 0x60, 2048 } },
        AvrAtmega16or32IoPinInfo44,
        arraylen(AvrAtmega16or32IoPinInfo44),
        AvrAtmega16or32AdcPinInfo44,
        arraylen(AvrAtmega16or32AdcPinInfo44),
        1023,
        { 9, 10 },
//        16, // OC2
        ISA_AVR,
        EnhancedCore128K,
        44,
        0,
        AvrPwmPinInfo44,
        arraylen(AvrPwmPinInfo44),
        nullptr,
        0,
        nullptr,
        0,
        nullptr,
        0,
        {{0,0}}
    },
    {
        "Atmel AVR ATmega32 40-PDIP",
        "ATmega32",
        "m32def",
        "mega32",
        "iom32", // AVRGCC
        'P',
        { 0x39, 0x36, 0x33, 0x30 }, // PINx
        { 0x3b, 0x38, 0x35, 0x32 }, // PORTx
        { 0x3a, 0x37, 0x34, 0x31 }, // DDRx
        16*1024,
        { { 0x60, 2048 } },
        AvrAtmega16or32IoPinInfo,
        arraylen(AvrAtmega16or32IoPinInfo),
        AvrAtmega16or32AdcPinInfo,
        arraylen(AvrAtmega16or32AdcPinInfo),
        1023,
        { 14, 15 },
//        21, // OC2
        ISA_AVR,
        EnhancedCore128K,
        40,
        0,
        AvrAtmega16_32PwmPinInfo40,
        arraylen(AvrAtmega16_32PwmPinInfo40),
        nullptr,
        0,
        McuSpiInfoATmega16,
        arraylen(McuSpiInfoATmega16),
        McuI2cInfoATmega16,
        arraylen(McuI2cInfoATmega16),
        {{0,0}}
    },
    {
        "Atmel AVR ATmega16 40-PDIP",
        "ATmega16",
        "",
        "",
        "iom16", // AVRGCC
        'P',
        { 0x39, 0x36, 0x33, 0x30 }, // PINx
        { 0x3b, 0x38, 0x35, 0x32 }, // PORTx
        { 0x3a, 0x37, 0x34, 0x31 }, // DDRx
        8*1024,
        { { 0x60, 1024 } },
        AvrAtmega16or32IoPinInfo,
        arraylen(AvrAtmega16or32IoPinInfo),
        AvrAtmega16or32AdcPinInfo,
        arraylen(AvrAtmega16or32AdcPinInfo),
        1023,
        { 14, 15 },
//        21, // OC2
        ISA_AVR,
        EnhancedCore128K,
        40,
        0,
        AvrAtmega16_32PwmPinInfo40,
        arraylen(AvrAtmega16_32PwmPinInfo40),
        nullptr,
        0,
        McuSpiInfoATmega16,
        arraylen(McuSpiInfoATmega16),
        McuI2cInfoATmega16,
        arraylen(McuI2cInfoATmega16),
        {{0,0}}
    },
    {
        "Atmel AVR ATmega48 28-PDIP",
        "ATmega48",
        "",
        "",
        "iomx8", // AVRGCC
        'P',
        { 0, 0x23, 0x26, 0x29 }, // PINx
        { 0, 0x25, 0x28, 0x2B }, // PORTx
        { 0, 0x24, 0x27, 0x2A }, // DDRx
        2*1024,
        { { 0x100, 512 } },
        AvrAtmega8IoPinInfo,
        arraylen(AvrAtmega8IoPinInfo),
        AvrAtmega8AdcPinInfo,
        arraylen(AvrAtmega8AdcPinInfo),
        1023,
        { 2, 3 },
//        17,
        ISA_AVR,
        EnhancedCore128K,
        28,
        0,
        AvrPwmPinInfo28,
        arraylen(AvrPwmPinInfo28),
        nullptr,
        0,
        McuSpiInfoATmega328,
        arraylen(McuSpiInfoATmega328),
        McuI2cInfoATmega328,
        arraylen(McuI2cInfoATmega328),
        {{0,0}}
    },
    {
        "Atmel AVR ATmega88 28-PDIP",
        "ATmega88",
        "",
        "",
        "iomx8", // AVRGCC
        'P',
        { 0, 0x23, 0x26, 0x29 }, // PINx
        { 0, 0x25, 0x28, 0x2B }, // PORTx
        { 0, 0x24, 0x27, 0x2A }, // DDRx
        4*1024,
        { { 0x100, 1024 } },
        AvrAtmega8IoPinInfo,
        arraylen(AvrAtmega8IoPinInfo),
        AvrAtmega8AdcPinInfo,
        arraylen(AvrAtmega8AdcPinInfo),
        1023,
        { 2, 3 },
//        17,
        ISA_AVR,
        EnhancedCore128K,
        28,
        0,
        AvrPwmPinInfo28,
        arraylen(AvrPwmPinInfo28),
        nullptr,
        0,
        McuSpiInfoATmega328,
        arraylen(McuSpiInfoATmega328),
        McuI2cInfoATmega328,
        arraylen(McuI2cInfoATmega328),
        {{0,0}}
    },
    {
        "Atmel AVR ATmega168 28-PDIP",
        "ATmega168",
        "",
        "",
        "iomx8", // AVRGCC
        'P',
        { 0, 0x23, 0x26, 0x29 }, // PINx
        { 0, 0x25, 0x28, 0x2B }, // PORTx
        { 0, 0x24, 0x27, 0x2A }, // DDRx
        8*1024,
        { { 0x100, 1024 } },
        AvrAtmega8IoPinInfo,
        arraylen(AvrAtmega8IoPinInfo),
        AvrAtmega8AdcPinInfo,
        arraylen(AvrAtmega8AdcPinInfo),
        1023,
        { 2, 3 },
//        17,
        ISA_AVR,
        EnhancedCore128K,
        28,
        0,
        AvrPwmPinInfo28,
        arraylen(AvrPwmPinInfo28),
        nullptr,
        0,
        McuSpiInfoATmega328,
        arraylen(McuSpiInfoATmega328),
        McuI2cInfoATmega328,
        arraylen(McuI2cInfoATmega328),
        {{0,0}}
    },
    {
        "Atmel AVR ATmega328 28-PDIP",
        "ATmega328",
        "m328def",
        "mega328",
        "iom328p", // AVRGCC
        'P',
        { 0, 0x23, 0x26, 0x29 }, // PINx
        { 0, 0x25, 0x28, 0x2B }, // PORTx
        { 0, 0x24, 0x27, 0x2A }, // DDRx
        16*1024,
        { { 0x100, 2048 } },
        AvrAtmega328IoPinInfo,
        arraylen(AvrAtmega328IoPinInfo),
        AvrAtmega8AdcPinInfo,
        arraylen(AvrAtmega8AdcPinInfo),
        1023,
        { 2, 3 },
//        17,
        ISA_AVR,
        EnhancedCore128K,
        28,
        0,
        AvrPwmPinInfo28,
        arraylen(AvrPwmPinInfo28),
        nullptr,
        0,
        McuSpiInfoATmega328,
        arraylen(McuSpiInfoATmega328),
        McuI2cInfoATmega328,
        arraylen(McuI2cInfoATmega328),
        {{0,0}}
    },
    {
        "Atmel AVR ATmega328 32-Pin packages",//char            *mcuName;
        "ATmega328",
        "m328def", // avrasm2.exe
        "mega328",
        "iom328p", // AVRGCC
        'P',                                  //char             portPrefix;
        { 0, 0x23, 0x26, 0x29 }, // PINx   //DWORD            inputRegs[MAX_IO_PORTS]; // A is 0, J is 9
        { 0, 0x25, 0x28, 0x2B }, // PORTx  //DWORD            outputRegs[MAX_IO_PORTS];
        { 0, 0x24, 0x27, 0x2A }, // DDRx   //DWORD            dirRegs[MAX_IO_PORTS];
        16*1024,                              //DWORD            flashWords;
        { { 0x100, 2048 } },                  //{DWORD start; int len;} ram[MAX_RAM_SECTIONS];
        AvrAtmega328IoPinInfo32,              //McuIoPinInfo    *pinInfo;
        arraylen(AvrAtmega328IoPinInfo32),    //int              pinCount;
        AvrAtmega328AdcPinInfo32,             //McuAdcPinInfo   *adcInfo;
        arraylen(AvrAtmega328AdcPinInfo32),   //int              adcCount;
        1023,                                 //int              adcMax;
        { 30, 31 },                           //{int rxPin; int txPin;} uartNeeds;
//        15,                                   //int              pwmNeedsPin;
        ISA_AVR,                             //int              whichIsa;
        EnhancedCore128K,                     //AvrFamily        Family;
        32,
        0,                                    //DWORD            configurationWord;
        AvrPwmPinInfo32,                //McuPwmPinInfo   *pwmInfo;
        arraylen(AvrPwmPinInfo32),      //int              pwmCount;
        AvrExtIntPinInfo32,
        arraylen(AvrExtIntPinInfo32),
        nullptr,
        0,
        nullptr,
        0,
        {{0,0}}
    },                                        //int int0PinA; int int1PinA;} QuadEncodNeeds;
    {
        "Atmel AVR ATmega164 40-PDIP",
        "ATmega164",
        "",
        "",
        "iom164", // AVRGCC
        'P',
        { 0x20, 0x23, 0x26, 0x29 }, // PINx
        { 0x22, 0x25, 0x28, 0x2B }, // PORTx
        { 0x21, 0x24, 0x27, 0x2A }, // DDRx
        8*1024,
        { { 0x100, 1024 } },
        AvrAtmega164IoPinInfo,
        arraylen(AvrAtmega164IoPinInfo),
        AvrAtmega164AdcPinInfo,
        arraylen(AvrAtmega164AdcPinInfo),
        1023,
        { 14, 15 },
//        21,
        ISA_AVR,
        EnhancedCore128K,
        40,
        0,
        AvrPwmPinInfo40_,
        arraylen(AvrPwmPinInfo40_),
        nullptr,
        0,
        McuSpiInfoATmega16,
        arraylen(McuSpiInfoATmega16),
        McuI2cInfoATmega16,
        arraylen(McuI2cInfoATmega16),
        {{0,0}}
    },
    {
        "Atmel AVR ATmega324 40-PDIP",
        "ATmega324",
        "",
        "",
        "iom324", // AVRGCC
        'P',
        { 0x20, 0x23, 0x26, 0x29 }, // PINx
        { 0x22, 0x25, 0x28, 0x2B }, // PORTx
        { 0x21, 0x24, 0x27, 0x2A }, // DDRx
        16*1024,
        { { 0x100, 2048 } },
        AvrAtmega164IoPinInfo,
        arraylen(AvrAtmega164IoPinInfo),
        AvrAtmega164AdcPinInfo,
        arraylen(AvrAtmega164AdcPinInfo),
        1023,
        { 14, 15 },
//        21,
        ISA_AVR,
        EnhancedCore128K,
        40,
        0,
        AvrPwmPinInfo40_,
        arraylen(AvrPwmPinInfo40_),
        nullptr,
        0,
        McuSpiInfoATmega16,
        arraylen(McuSpiInfoATmega16),
        McuI2cInfoATmega16,
        arraylen(McuI2cInfoATmega16),
        {{0,0}}
    },
    {
        "Atmel AVR ATmega644 40-PDIP",
        "ATmega644",
        "",
        "",
        "iom644", // AVRGCC
        'P',
        { 0x20, 0x23, 0x26, 0x29 }, // PINx
        { 0x22, 0x25, 0x28, 0x2B }, // PORTx
        { 0x21, 0x24, 0x27, 0x2A }, // DDRx
        32*1024,
        { { 0x100, 4096 } },
        AvrAtmega164IoPinInfo,
        arraylen(AvrAtmega164IoPinInfo),
        AvrAtmega164AdcPinInfo,
        arraylen(AvrAtmega164AdcPinInfo),
        1023,
        { 14, 15 },
//        21,
        ISA_AVR,
        EnhancedCore128K,
        40,
        0,
        AvrPwmPinInfo40_,
        arraylen(AvrPwmPinInfo40_),
        nullptr,
        0,
        McuSpiInfoATmega16,
        arraylen(McuSpiInfoATmega16),
        McuI2cInfoATmega16,
        arraylen(McuI2cInfoATmega16),
        {{0,0}}
    },
    {
        "Atmel AVR ATmega1284 44-Pin packages",
        "ATmega1284p",
        "",
        "",
        "iom1284", // AVRGCC
        'P',
        { 0x20, 0x23, 0x26, 0x29 }, // PINx
        { 0x22, 0x25, 0x28, 0x2B }, // PORTx
        { 0x21, 0x24, 0x27, 0x2A }, // DDRx
        64*1024,
        { { 0x100, 16384 } },
        AvrAtmegaIo44PinInfo,
        arraylen(AvrAtmegaIo44PinInfo),
        AvrAtmegaAdc44PinInfo,
        arraylen(AvrAtmegaAdc44PinInfo),
        1023,
        { 9, 10 },
//        21,
        ISA_AVR,
        EnhancedCore128K,
        44,
        0,
        AvrPwmPinInfo44,
        arraylen(AvrPwmPinInfo44),
        nullptr,
        0,
        McuSpiInfoATmega16,
        arraylen(McuSpiInfoATmega16),
        McuI2cInfoATmega16,
        arraylen(McuI2cInfoATmega16),
        {{0,0}}
    },
    {
        "Atmel AVR ATmega1284 40-PDIP",
        "ATmega1284p",
        "",
        "",
        "iom1284", // AVRGCC
        'P',
        { 0x20, 0x23, 0x26, 0x29 }, // PINx
        { 0x22, 0x25, 0x28, 0x2B }, // PORTx
        { 0x21, 0x24, 0x27, 0x2A }, // DDRx
        64*1024,
        { { 0x100, 16384 } },
        AvrAtmega164IoPinInfo,
        arraylen(AvrAtmega164IoPinInfo),
        AvrAtmega164AdcPinInfo,
        arraylen(AvrAtmega164AdcPinInfo),
        1023,
        { 14, 15 },
//        21,
        ISA_AVR,
        EnhancedCore128K,
        40,
        0,
        AvrPwmPinInfo40_,
        arraylen(AvrPwmPinInfo40_),
        nullptr,
        0,
        McuSpiInfoATmega16,
        arraylen(McuSpiInfoATmega16),
        McuI2cInfoATmega16,
        arraylen(McuI2cInfoATmega16),
        {{0,0}}
    },
    {
        "Atmel AVR ATtiny85 8-Pin packages",
        "ATtiny85",
        "tn85def",
        "tiny85",
        "iotn85", // AVRGCC
        'P',
        { 0, 0x36 },
        { 0, 0x38 },
        { 0, 0x37 },
        4*1024,
        { { 0x60, 512 } },
        AvrATtiny85IoPinInfo8,              //McuIoPinInfo    *pinInfo;
        arraylen(AvrATtiny85IoPinInfo8),    //int              pinCount;
        AvrATtiny85AdcPinInfo8,             //McuAdcPinInfo   *adcInfo;
        arraylen(AvrATtiny85AdcPinInfo8),   //int              adcCount;
        1023,
        { 0, 0 },
//        0, // OC2
        ISA_AVR,
        EnhancedCore128K,
        0,
        8,
        AvrPwmPinInfo8,
        arraylen(AvrPwmPinInfo8),
        AvrExtIntPinInfo8,
        arraylen(AvrExtIntPinInfo8),
        nullptr,
        0,
        nullptr,
        0,
        {{0,0}}
    },
    {
        "Atmel AVR ATtiny85 20-Pin packages",
        "ATtiny85",
        "tn85def",
        "tiny85",
        "iotn85", // AVRGCC
        'P',
        { 0, 0x36 },
        { 0, 0x38 },
        { 0, 0x37 },
        4*1024,
        { { 0x60, 512 } },
        AvrATtiny85IoPinInfo20,             //McuIoPinInfo    *pinInfo;
        arraylen(AvrATtiny85IoPinInfo20),   //int              pinCount;
        AvrATtiny85AdcPinInfo20,            //McuAdcPinInfo   *adcInfo;
        arraylen(AvrATtiny85AdcPinInfo20),  //int              adcCount;
        1023,
        { 0, 0 },
//        0, // OC2
        ISA_AVR,
        EnhancedCore128K,
        0,
        8,
        AvrPwmPinInfo8,
        arraylen(AvrPwmPinInfo8),
        AvrExtIntPinInfo8,
        arraylen(AvrExtIntPinInfo8),
        nullptr,
        0,
        nullptr,
        0,
        {{0,0}}
    },
    {
        "Atmel AVR ATmega8 32-Pin packages", //char            *mcuName;
        "ATmega8",
        "m8def", // "iom8"
        "mega8",
        "iom8", // AVRGCC
        'P',                                 //char             portPrefix;
        { 0, 0x36, 0x33, 0x30 }, // PINx  //DWORD            inputRegs[MAX_IO_PORTS]; // A is 0, J is 9
        { 0, 0x38, 0x35, 0x32 }, // PORTx //DWORD            outputRegs[MAX_IO_PORTS];
        { 0, 0x37, 0x34, 0x31 }, // DDRx  //DWORD            dirRegs[MAX_IO_PORTS];
        4*1024,                              //DWORD            flashWords;
        { { 0x60, 1024 } },                  //{DWORD start; int len;} ram[MAX_RAM_SECTIONS];
        AvrAtmega8IoPinInfo32,               //McuIoPinInfo    *pinInfo;
        arraylen(AvrAtmega8IoPinInfo32),     //int              pinCount;
        AvrAtmega8AdcPinInfo32,              //McuAdcPinInfo   *adcInfo;
        arraylen(AvrAtmega8AdcPinInfo32),    //int              adcCount;
        1023,                                //int              adcMax;
        { 30, 31 },                          //{int rxPin; int txPin;} uartNeeds;
//        15,// OC2                            //int              pwmNeedsPin;
        ISA_AVR,                             //int              whichIsa;
        EnhancedCore8K,                      //AvrFamily        Family;
        0,                                   //DWORD            configurationWord;
        32,
//      { 32, 1 }, // INT0, INT1             //int int0PinA; int int1PinA;} QuadEncodNeeds;
//      AvrAtmega8ExtIntPinInfo32,
//      arraylen(AvrAtmega8ExtIntPinInfo32),
        AvrPwmPinInfo32_,
        arraylen(AvrPwmPinInfo32_),
        AvrExtIntPinInfo32,
        arraylen(AvrExtIntPinInfo32),
        nullptr,
        0,
        nullptr,
        0,
        {{0,0}}
    },
    {
        "Atmel AVR ATmega8 28-PDIP",
        "ATmega8",
        "m8def",
        "mega8",
        "iom8", // AVRGCC
        'P',
        { 0, 0x36, 0x33, 0x30 }, // PINx     (but there is no xxxA)
        { 0, 0x38, 0x35, 0x32 }, // PORTx
        { 0, 0x37, 0x34, 0x31 }, // DDRx
        4*1024,
        { { 0x60, 1024 } },
        AvrAtmega8IoPinInfo28,
        arraylen(AvrAtmega8IoPinInfo28),
        AvrAtmega8AdcPinInfo,
        arraylen(AvrAtmega8AdcPinInfo),
        1023,
        { 2, 3 },
//        17,
        ISA_AVR,
        EnhancedCore8K,
        28,
        0,
        AvrPwmPinInfo28_,
        arraylen(AvrPwmPinInfo28_),
        nullptr,
        0,
        McuSpiInfoATmega8,
        arraylen(McuSpiInfoATmega8),
        McuI2cInfoATmega8,
        arraylen(McuI2cInfoATmega8),
        {{0,0}}
    },
    {
        "Atmel AVR ATtiny10 6-Pin packages",
        "ATtiny10",
        "tn10def",
        "tiny10",
        "iotn11", // AVRGCC
        'P',
        { 0, 0x00 }, // PINx
        { 0, 0x02 }, // PORTx
        { 0, 0x01 }, // DDRx
        1*1024,
        { { 0x40, 32 } },
        AvrATtiny10IoPinInfo6,               //McuIoPinInfo    *pinInfo;
        arraylen(AvrATtiny10IoPinInfo6),     //int              pinCount;
        AvrATtiny10AdcPinInfo6,              //McuAdcPinInfo   *adcInfo;
        arraylen(AvrATtiny10AdcPinInfo6),    //int              adcCount;
        255,
        { 0, 0 },
//        0, // OC2
        ISA_AVR,
        ReducedCore,
        0,
        6,
        nullptr, //AvrPwmPinInfo6,
        0, //arraylen(AvrPwmPinInfo6),
        AvrExtIntPinInfo6,
        arraylen(AvrExtIntPinInfo6),
        nullptr,
        0,
        nullptr,
        0,
        {{0,0}}
    },
//===========================================================================
    {
        "Microchip PIC16F628 18-PDIP or 18-SOIC",
        "PIC16F628",
        "P16F628",
        "16F628",
        "PIC16F628",
        'R',
        { 0x05, 0x06 }, // PORTx
        { 0x05, 0x06 }, // PORTx
        { 0x85, 0x86 }, // TRISx
        2048,
        { { 0x20, 96 }, { 0xA0, 80 }, { 0x120, 48 } },
        Pic18PinIoInfo,
        arraylen(Pic18PinIoInfo),
        nullptr,
        0,
        0,
        { 7, 8 },
//        0,
        ISA_PIC16,
        MidrangeCore14bit,
        18,
        // code protection off, data code protection off, LVP disabled,
        // BOD reset enabled, RA5/nMCLR is _MCLR, PWRT enabled, WDT disabled,
        0x3f62, // HS oscillator, _MCLR
//      0x3f42, // HS oscillator, RA5 pin function is digital Input
//      0x3F70, // INTOSC oscillator, _MCLR
//      0x3F50, // INTOSC oscillator, RA5 pin function is digital Input
            /*
            (1 << 13) | // Code protection off,
            (1 <<  8) | // Data memory code protection off,
            (0 <<  7) | // LVP disabled, RB4/PGM is digital I/O,
            (1 <<  6) | // BOR reset enabled,
//          (0 <<  5) | // RA5/_MCLR is RA5, RA5 pin function is digital Input,
            (1 <<  5) | // RA5/_MCLR/Vpp pin function is _MCLR
            (0 <<  3) | // PWRT enabled,
            (0 <<  2) | // WDT disabled,
//          (1 <<  2) | // WDT enabled,
            (2 <<  0),  // HS oscillator: High-speed crystal/resonator on RA6/OSC2/CLKOUT and RA7/OSC1/CLKIN
//       (0x10 <<  0),  // INTOSC oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN
            */
        PicPwmPinInfo18,
        arraylen(PicPwmPinInfo18),
        PicExtIntPinInfo18,
        arraylen(PicExtIntPinInfo18),
        nullptr ,
        0,
        nullptr,
        0,
        {{0,0}}
    },
    {
        "Microchip PIC16F88 18-PDIP or 18-SOIC",
        "PIC16F88",
        "P16F88",
        "16F88",
        "PIC16F88",
        'R',
        { 0x05, 0x06 }, // PORTx
        { 0x05, 0x06 }, // PORTx
        { 0x85, 0x86 }, // TRISx
        4096,
        { { 0x20, 96 }, { 0xA0, 80 }, { 0x120, 48 } },
        Pic16F88PinIoInfo,
        arraylen(Pic16F88PinIoInfo),
        Pic16F88AdcPinInfo,
        arraylen(Pic16F88AdcPinInfo),
        1023,
        { 8, 11 },
//        0,
        ISA_PIC16,
        MidrangeCore14bit,
        18,
//          (1 << 17) | // IESO: Internal External Switchover mode enabled
//          (1 << 16) | // FCMEN: Fail-Safe Clock Monitor enabled
            (1 << 13) | // Flash Program Memory Code Protection off
            (0 << 12) | // CCP1 function on RB3 (doesn't matter)
            (1 << 11) | // DEBUG: ICD disabled
            (3 <<  9) | // Flash Program Memory write protection off
            (1 <<  8) | // Data EE Memory Code protection off
            (0 <<  7) | // LVP disabled
            (1 <<  6) | // BOR enabled
            (0 <<  5) | // RA5/nMCLR is RA5
            (0 <<  4) | // FOSC2=0 for osc sel, HS oscillator
            (0 <<  3) | // PWRT enabled
            (0 <<  2) | // WDT disabled
            (2 <<  0),  // HS oscillator
        PicPwmPinInfo18,
        arraylen(PicPwmPinInfo18),
        PicExtIntPinInfo18,
        arraylen(PicExtIntPinInfo18),
        McuSpiInfoPic16F88,
        arraylen(McuSpiInfoPic16F88),
        McuI2cInfoPic16F88,
        arraylen(McuI2cInfoPic16F88),
        {{0,0}}
    },
    {
        "Microchip PIC16F819 18-PDIP or 18-SOIC",
        "PIC16F819",
        "P16F819",
        "16F819",
        "PIC16F819",
        'R',
        { 0x05, 0x06 }, // PORTx
        { 0x05, 0x06 }, // PORTx
        { 0x85, 0x86 }, // TRISx
        2048,
        { { 0x20, 96 } },
        Pic18PinIoInfo,
        arraylen(Pic18PinIoInfo),
        Pic16F819AdcPinInfo,
        arraylen(Pic16F819AdcPinInfo),
        1023,
        { 0, 0 },
//        0,
        ISA_PIC16,
        MidrangeCore14bit,
        18,
            (1 << 13) | // code protect off
            (1 << 12) | // CCP1 on RB2 (doesn't matter, can't use)
            (1 << 11) | // disable debugger
            (3 <<  9) | // flash protection off
            (1 <<  8) | // data protect off
            (0 <<  7) | // LVP disabled
            (1 <<  6) | // BOR enabled
            (0 <<  5) | // nMCLR/RA5 is RA5
            (0 <<  3) | // PWRTE enabled
            (0 <<  2) | // WDT disabled
//          (1 <<  4) | // 100 = INTRC oscillator; port I/O function on both RA6/OSC2/CLKO pin and RA7/OSC1/CLKI pin
            (2 <<  0),  // 010 = HS oscillator
        PicPwmPinInfo18,
        arraylen(PicPwmPinInfo18),
        PicExtIntPinInfo18,
        arraylen(PicExtIntPinInfo18),
        McuSpiInfoPic16F88,
        arraylen(McuSpiInfoPic16F88),
        McuI2cInfoPic16F88,
        arraylen(McuI2cInfoPic16F88),
        {{0,0}}
    },
    {
        "Microchip PIC16F877 40-PDIP",
        "PIC16F877",
        "P16F877",
        "16F877",
        "pic16F877",
        'R',
        { 0x05, 0x06, 0x07, 0x08, 0x09 }, // PORTx
        { 0x05, 0x06, 0x07, 0x08, 0x09 }, // PORTx
        { 0x85, 0x86, 0x87, 0x88, 0x89 }, // TRISx
        8*1024,
        { { 0x20, 96 }, { 0xA0, 80 }, { 0x110, 96 }, { 0x190, 96 } },
        Pic16F877IoPinInfo,
        arraylen(Pic16F877IoPinInfo),
        Pic16F877AdcPinInfo,
        arraylen(Pic16F877AdcPinInfo),
        1023,
        { 26, 25 },
//        16,
        ISA_PIC16,
        MidrangeCore14bit,
        40,
        // code protection off, debug off, flash write off, EE code protection
        // off, LVP disabled, BOD enabled, CP off, PWRT enabled, WDT disabled,
        // HS oscillator
        0x3f72,
        PicPwmPinInfo40,
        arraylen(PicPwmPinInfo40),
        PicExtIntPinInfo40,
        arraylen(PicExtIntPinInfo40),
        McuSpiInfoPic16F877,
        arraylen(McuSpiInfoPic16F877),
        McuI2cInfoPic16F877,
        arraylen(McuI2cInfoPic16F877),
        {{0,0}}
    },
    {
        "Microchip PIC16F876 28-PDIP or 28-SOIC",
        "PIC16F876",
        "P16F876",
        "16F876",
        "PIC16F876",
        'R',
        { 0x05, 0x06, 0x07 }, // PORTx
        { 0x05, 0x06, 0x07 }, // PORTx
        { 0x85, 0x86, 0x87 }, // TRISx
        8*1024,
        { { 0x20, 96 }, { 0xA0, 80 }, { 0x110, 96 }, { 0x190, 96 } },
        Pic16F876IoPinInfo,
        arraylen(Pic16F876IoPinInfo),
        Pic16F876AdcPinInfo,
        arraylen(Pic16F876AdcPinInfo),
        1023,
        { 18, 17 },
//        12,
        ISA_PIC16,
        MidrangeCore14bit,
        28,
        // code protection off, debug off, flash write off, EE code protection
        // off, LVP disabled, BOD enabled, CP off, PWRT enabled, WDT disabled,
        // HS oscillator
        0x3f72,
        PicPwmPinInfo28_2,
        arraylen(PicPwmPinInfo28_2),
        PicExtIntPinInfo28,
        arraylen(PicExtIntPinInfo28),
        McuSpiInfoPic16F876,
        arraylen(McuSpiInfoPic16F876),
        McuI2cInfoPic16F876,
        arraylen(McuI2cInfoPic16F876),
        {{0,0}}
    },
    {
        "Microchip PIC16F887 40-PDIP",
        "PIC16F887",
        "P16F887",
        "16F887",
        "PIC16F887",
        'R',
        { 0x05, 0x06, 0x07, 0x08, 0x09 }, // PORTx
        { 0x05, 0x06, 0x07, 0x08, 0x09 }, // PORTx
        { 0x85, 0x86, 0x87, 0x88, 0x89 }, // TRISx
        8*1024,
        { { 0x20, 96 }, { 0xA0, 80 }, { 0x120, 80 }, { 0x1a0, 80 } },
        Pic16F887IoPinInfo,
        arraylen(Pic16F887IoPinInfo),
        Pic16F887AdcPinInfo,
        arraylen(Pic16F887AdcPinInfo),
        1023,
        { 26, 25 },
//        16,
        ISA_PIC16,
        MidrangeCore14bit,
        40,
            (3 << (9+16)) | // flash write protection off
            (0 << (8+16)) | // BOR at 2.1 V
            (1 << 13) |     // ICD disabled
            (0 << 12) |     // LVP disabled
            (0 << 11) |     // fail-safe clock monitor disabled
            (0 << 10) |     // internal/external switchover disabled
            (3 <<  8) |     // brown-out detect enabled
            (1 <<  7) |     // data code protection disabled
            (1 <<  6) |     // code protection disabled
            (1 <<  5) |     // nMCLR enabled
            (0 <<  4) |     // PWRTE enabled
            (0 <<  3) |     // WDTE disabled
            (2 <<  0),      // HS oscillator
        PicPwmPinInfo40,
        arraylen(PicPwmPinInfo40),
        PicExtIntPinInfo40,
        arraylen(PicExtIntPinInfo40),
        McuSpiInfoPic16F877,
        arraylen(McuSpiInfoPic16F877),
        McuI2cInfoPic16F877,
        arraylen(McuI2cInfoPic16F877),
        {{0,0}}
    },
    {
        "Microchip PIC16F887 44-TQFP",
        "PIC16F887",
        "P16F887",
        "16F887",
        "PIC16F887",
        'R',
        { 0x05, 0x06, 0x07, 0x08, 0x09 }, // PORTx
        { 0x05, 0x06, 0x07, 0x08, 0x09 }, // PORTx
        { 0x85, 0x86, 0x87, 0x88, 0x89 }, // TRISx
        8*1024,
        { { 0x20, 96 }, { 0xA0, 80 }, { 0x120, 80 }, { 0x1a0, 80 } },
        Pic16F887IoPinInfo44TQFP,
        arraylen(Pic16F887IoPinInfo44TQFP),
        Pic16F887AdcPinInfo44TQFP,
        arraylen(Pic16F887AdcPinInfo44TQFP),
        1023,
        { 1, 44 }, // rxPin; txPin;
//        16,
        ISA_PIC16,
        MidrangeCore14bit,
        40,
            (3 << (9+16)) | // flash write protection off
            (0 << (8+16)) | // BOR at 2.1 V
            (1 << 13) |     // ICD disabled
            (0 << 12) |     // LVP disabled
            (0 << 11) |     // fail-safe clock monitor disabled
            (0 << 10) |     // internal/external switchover disabled
            (3 <<  8) |     // brown-out detect enabled
            (1 <<  7) |     // data code protection disabled
            (1 <<  6) |     // code protection disabled
            (1 <<  5) |     // nMCLR enabled
            (0 <<  4) |     // PWRTE enabled
            (0 <<  3) |     // WDTE disabled
            (2 <<  0),      // HS oscillator
        PicPwmPinInfo44,
        arraylen(PicPwmPinInfo44),
        PicExtIntPinInfo44,
        arraylen(PicExtIntPinInfo44),
        McuSpiInfoPic16F887_44,
        arraylen(McuSpiInfoPic16F887_44),
        McuI2cInfoPic16F887_44,
        arraylen(McuI2cInfoPic16F887_44),
        {{0,0}}
    },
    {
        "Microchip PIC16F886 28-PDIP or 28-SOIC",
        "PIC16F886",
        "P16F886",
        "16F886",
        "PIC16F886",
        'R',
        { 0x05, 0x06, 0x07, 0, 0x09 }, // PORTx = A B C E
        { 0x05, 0x06, 0x07, 0, 0x09 }, // PORTx
        { 0x85, 0x86, 0x87, 0, 0x89 }, // TRISx
        8*1024,
        { { 0x20, 96 }, { 0xA0, 80 }, { 0x120, 80 }, { 0x1a0, 80 } },
        Pic28Pin_SPDIP_SOIC_SSOP,
        arraylen(Pic28Pin_SPDIP_SOIC_SSOP),
        Pic16F886AdcPinInfo,
        arraylen(Pic16F886AdcPinInfo),
        1023,
        { 18, 17 },
//        12,
        ISA_PIC16,
        MidrangeCore14bit,
        28,
         (0x38 << (8+16)) | // Unimplemented: Read as 1
            (3 << (9+16)) | // flash write protection off
            (0 << (8+16)) | // BOR at 2.1 V
         (0xff << 16) |     // Unimplemented: Read as 1
            (1 << 13) |     // ICD disabled
            (0 << 12) |     // LVP disabled
            (0 << 11) |     // fail-safe clock monitor disabled
            (0 << 10) |     // internal/external switchover disabled
            (3 <<  8) |     // brown-out detect enabled
            (1 <<  7) |     // data code protection disabled
            (1 <<  6) |     // code protection disabled
            (1 <<  5) |     // nMCLR enabled
            (0 <<  4) |     // PWRTE enabled
            (0 <<  3) |     // WDTE disabled
            (2 <<  0),      // HS oscillator
        PicPwmPinInfo28_2,
        arraylen(PicPwmPinInfo28_2),
        PicExtIntPinInfo28,
        arraylen(PicExtIntPinInfo28),
        McuSpiInfoPic16F876,
        arraylen(McuSpiInfoPic16F876),
        McuI2cInfoPic16F876,
        arraylen(McuI2cInfoPic16F876),
        {{0,0}}
    },
    {
        "Microchip PIC16F72 28-Pin PDIP, SOIC, SSOP",
        "PIC16F72",
        "P16F72",
        "16F72",
        "PIC16F72",
        'R',
        { 0x05, 0x06, 0x07 }, // PORTx = A B C
        { 0x05, 0x06, 0x07 }, // PORTx
        { 0x85, 0x86, 0x87 }, // TRISx
        2*1024,
        { { 0x20, 96 }, { 0xA0, 32 } },
        Pic16F72IoPinInfo,
        arraylen(Pic16F72IoPinInfo),
        Pic16F72AdcPinInfo,
        arraylen(Pic16F72AdcPinInfo),
        255,
        { 0, 0 },
//        13,
        ISA_PIC16,
        MidrangeCore14bit,
        28,
            (3 <<  6) |     // brown-out detect enabled
            (1 <<  4) |     // code protection disabled
            (0 <<  3) |     // PWRTE enabled
            (0 <<  2) |     // WDTE disabled
            (2 <<  0),      // HS oscillator
        PicPwmPinInfo28_1,
        arraylen(PicPwmPinInfo28_1),
        PicExtIntPinInfo28,
        arraylen(PicExtIntPinInfo28),
        nullptr,
        0,
        nullptr,
        0,
        {{0,0}}
    },
    {
        "Microchip PIC12F675 8-pin packages", // or PIC12F629
        "PIC12F675",
        "P12F675",
        "12F675",
        "PIC12F675",
        'G',
//        A  B  C  D  E  F  G  H  I  J  K  L  M  N  O  P
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x05}, // PORTx = GPIO
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x05}, // PORTx
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x85}, // TRISx
        1024,
        { { 0x20, 64 } },
        Pic8Pin,
        arraylen(Pic8Pin),
        Pic8PinAdcPinInfo,
        arraylen(Pic8PinAdcPinInfo),
        1024,
        { },
//        0,
        ISA_PIC16,
        MidrangeCore14bit,
        8,
        0x3FC4,
        /*
          ($3f <<  7) |
            (1 <<  6) |     // BOD enabled, Brown-out Detect Enable
            (0 <<  5) |     // _MCLR disabled, GP3/MCLR pin function is digital I/O, MCLR internally tied to VDD
            (0 <<  4) |     // PWRT enabled, Power-up Timer Enable
            (0 <<  3) |     // WDTE disabled, Watchdog Timer disable
            (4 <<  0),      // 100 = INTOSC oscillator: I/O function on GP4/OSC2/CLKOUT pin, I/O function on GP5/OSC1/CLKIN
        */
        nullptr,
        0,
        nullptr,
        0,
        nullptr,
        0,
        nullptr,
        0,
        {{0,0}}
    },
    {
        "Microchip PIC12F683 8-pin packages",
        "PIC12F683",
        "P12F683",
        "12F683",
        "PIC12F683",
        'G',
//        A  B  C  D  E  F  G  H  I  J  K  L  M  N  O  P
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x05}, // PORTx = GPIO
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x05}, // PORTx
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x85}, // TRISx
        2*1024,
        { { 0x20, 96 }, { 0xA0, 32 } },
        Pic8Pin,
        arraylen(Pic8Pin),
        Pic8PinAdcPinInfo,
        arraylen(Pic8PinAdcPinInfo),
        1024,
        { },
//        0,
        ISA_PIC16,
        MidrangeCore14bit,
        8,
        0x3FC4,
        /*
            ($7f <<7) |
            (1 <<  6) |     // BOD enabled
            (0 <<  5) |     // _MCLR disabled
            (0 <<  4) |     // PWRT enabled
            (0 <<  3) |     // WDTE disabled
            (4 <<  0),      // 100 = INTOSC oscillator: I/O function on GP4/OSC2/CLKOUT pin, I/O function on GP5/OSC1/CLKIN
        */
        nullptr,
        0,
        nullptr,
        0,
        nullptr,
        0,
        nullptr,
        0,
        {{0,0}}
    },
    {
        "Microchip PIC12F1840 8-pin packages",
        "PIC12F1840",
        "P12F1840",
        "12F1840",
        "PIC12F1840",
        'R',
//        A
        { 0x0C}, // PORTx = RAx
        { 0x0C}, // PORTx
        { 0x8C}, // TRISx
        4*1024,
        { { 0x20, 80 }, { 0x70, 16 } },
        Pic8Pin_,
        arraylen(Pic8Pin_),
        Pic8PinAdcPinInfo,
        arraylen(Pic8PinAdcPinInfo),
        1024,
        { 6, 7 },
//        0,
        ISA_PIC16,
        EnhancedMidrangeCore14bit,
        8,
        0x1EFF3F84,
        /*
            ($FFFF <<16) |
            (3 <<  9) |     // 11 = BOR enabled
            (1 <<  8) |     // _CPD
            (1 <<  7) |     // _CP
            (0 <<  6) |     // _MCLR disabled
            (0 <<  5) |     // PWRT enabled
            (0 <<  3) |     // WDTE disabled
            (4 <<  0),      // 100 = INTOSC oscillator: I/O function on CLKIN pin
        */
        PicPwmPinInfo8,
        arraylen(PicPwmPinInfo8),
        nullptr,
        0,
        nullptr,
        0,
        nullptr,
        0,
        {{0,0}}
    },
    {
        "Microchip PIC16F1512 28-Pin SPDIP, SOIC, SSOP",
        "PIC16F1512",
        "P16F1512",
        "16F1512",
        "PIC16F1512",
        'R',
        { 0x0C, 0x0D, 0x0E, 0, 0x10 }, // PORTx = A B C E
        { 0x0C, 0x0D, 0x0E, 0, 0x10 }, // PORTx
        { 0x8C, 0x8D, 0x8E, 0, 0x90 }, // TRISx
        2048,
        { { 0x20, 96 } },
        Pic28Pin_SPDIP_SOIC_SSOP,
        arraylen(Pic28Pin_SPDIP_SOIC_SSOP),
        Pic16F1512AdcPinInfo,
        arraylen(Pic16F1512AdcPinInfo),
        1023,
        { 18, 17 },
//        12,
        ISA_PIC16,
        EnhancedMidrangeCore14bit,
        28,
            (0 << (13+16)) | // High-voltage on MCLR must be used for programming
            (1 << (12+16)) | // In-Circuit Debugger disabled, ICSPCLK and ICSPDAT are general purpose I/O pins
            (0 << (11+16)) | // Low-Power BOR is enabled
            (1 << (10+16)) | // Brown-out Reset voltage (VBOR), low trip point selected
            (0 << ( 9+16)) | // Stack Overflow or Underflow will not cause a Reset
            (3 << ( 0+16)) | // flash write protection off
            (0 << 13) |      // fail-safe clock monitor disabled
            (0 << 12) |      // internal/external switchover disabled
            (3 <<  9) |      // brown-out detect enabled
            (1 <<  7) |      // code protection disabled
            (1 <<  6) |      // nMCLR enabled
            (0 <<  5) |      // PWRT enabled
            (0 <<  4) |      // WDT disabled
            (0 <<  3) |      // WDT disabled
            (2 <<  0),       // HS oscillator
        PicPwmPinInfo28_2,
        arraylen(PicPwmPinInfo28_2),
        PicExtIntPinInfo28,
        arraylen(PicExtIntPinInfo28),
        nullptr,
        0,
        nullptr,
        0,
        {{0,0}}
    },
    {
        "Microchip PIC16F1516 28-Pin SPDIP, SOIC, SSOP",
        "PIC16F1516",
        "P16F1516",
        "16F1516",
        "PIC16F1516",
        'R',
        { 0x0C, 0x0D, 0x0E, 0, 0x10 }, // PORTx = A B C E
        { 0x0C, 0x0D, 0x0E, 0, 0x10 }, // PORTx
        { 0x8C, 0x8D, 0x8E, 0, 0x90 }, // TRISx
        2048,
        { { 0x20, 96 } },
        Pic28Pin_SPDIP_SOIC_SSOP,
        arraylen(Pic28Pin_SPDIP_SOIC_SSOP),
        Pic16F1512AdcPinInfo,
        arraylen(Pic16F1512AdcPinInfo),
        1023,
        { 18, 17 },
//        12,
        ISA_PIC16,
        EnhancedMidrangeCore14bit,
        28,
            (0 << (13+16)) | // High-voltage on MCLR must be used for programming
            (1 << (12+16)) | // In-Circuit Debugger disabled, ICSPCLK and ICSPDAT are general purpose I/O pins
            (0 << (11+16)) | // Low-Power BOR is enabled
            (1 << (10+16)) | // Brown-out Reset voltage (VBOR), low trip point selected
            (0 << ( 9+16)) | // Stack Overflow or Underflow will not cause a Reset
            (3 << ( 0+16)) | // flash write protection off
            (0 << 13) |      // fail-safe clock monitor disabled
            (0 << 12) |      // internal/external switchover disabled
            (3 <<  9) |      // brown-out detect enabled
            (1 <<  7) |      // code protection disabled
            (1 <<  6) |      // nMCLR enabled
            (0 <<  5) |      // PWRT enabled
            (0 <<  4) |      // WDT disabled
            (0 <<  3) |      // WDT disabled
            (2 <<  0),       // HS oscillator
        PicPwmPinInfo28_2,
        arraylen(PicPwmPinInfo28_2),
        PicExtIntPinInfo28,
        arraylen(PicExtIntPinInfo28),
        nullptr,
        0,
        nullptr,
        0,
        {{0,0}}
    },
    {
        "Microchip PIC16F1527 64-Pin packages",
        "PIC16F1527",
        "P16F1527",
        "16F1527",
        "PIC16F1527",
        'R',
        { 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x28C, 0x28D }, // PORTx = A B C D E F G
        { 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x28C, 0x28D }, // PORTx
        { 0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x30C, 0x30D }, // TRISx
        2048*8,
        { { 0x20, 96 }, { 0xA0, 80 }, { 0x120, 80 }, { 0x1A0, 80 }, { 0x220, 80 }, { 0x2A0, 80 }, { 0x320, 80 }, { 0x3A0, 80 } },
        Pic16F1527IoPinInfo,
        arraylen(Pic16F1527IoPinInfo),
        Pic16F1527AdcPinInfo,
        arraylen(Pic16F1527AdcPinInfo),
        1023,
        { 32, 31 }, //, 5, 4 },
//        29,
        ISA_PIC16,
        EnhancedMidrangeCore14bit,
        64,
            (0 << (13+16)) | // High-voltage on MCLR must be used for programming
            (1 << (12+16)) | // In-Circuit Debugger disabled, ICSPCLK and ICSPDAT are general purpose I/O pins
            (0 << (11+16)) | // Low-Power BOR is enabled
            (1 << (10+16)) | // Brown-out Reset voltage (VBOR), low trip point selected
            (0 << ( 9+16)) | // Stack Overflow or Underflow will not cause a Reset
            (3 << ( 0+16)) | // flash write protection off
            (0 << 13) |      // fail-safe clock monitor disabled
            (0 << 12) |      // internal/external switchover disabled
            (3 <<  9) |      // brown-out detect enabled
            (1 <<  7) |      // code protection disabled
            (1 <<  6) |      // nMCLR enabled
            (0 <<  5) |      // PWRT enabled
            (0 <<  4) |      // WDT disabled
            (0 <<  3) |      // WDT disabled
            (2 <<  0),       // HS oscillator
        PicPwmPinInfo64,
        arraylen(PicPwmPinInfo64),
        PicExtIntPinInfo64,
        arraylen(PicExtIntPinInfo64),
        nullptr,
        0,
        nullptr,
        0,
        {{0,0}}
    },
    {
        "Microchip PIC16F1824 14-Pin PDIP, SOIC, TSSOP",
        "PIC16F1824",
        "P16F1824",
        "16F1824",
        "PIC16F1824",
        'R',
        { 0x0C, 0, 0x0E }, // PORTx
        { 0x0C, 0, 0x0E }, // PORTx
        { 0x8C, 0, 0x8E }, // TRISx
        4*1024,
        { { 0x20, 96 }, { 0xA0, 80 }, { 0x120, 80 } },
        Pic14PinIoInfo,
        arraylen(Pic14PinIoInfo),
        Pic16F1824AdcPinInfo,
        arraylen(Pic16F1824AdcPinInfo),
        1023,
        {  5, 6 },
//        0, // pwm TODO
        ISA_PIC16,
        EnhancedMidrangeCore14bit,
        14,
       (0 << (13+16)) | // LVP disabled
       (1 << (12+16)) | // DEBUG disable
       (1 << (10+16)) | // BORV: Brown-out Reset voltage (Vbor), low trip point selected
       (1 << ( 9+16)) | // STVREN: Stack Overflow/Underflow Reset Enable bit
       (0 << ( 8+16)) | // PLLEN: 4xPLL disabled
     (0xff << (0+16)) | // WRT: Write protection off
            (1 << 13) | // FCMEN enabled
            (1 << 12) | // IESO enabled
            (1 << 11) | // _CLKOUTEN disabled
            (3 <<  9) | // BOR enabled
            (1 <<  8) | // data protect off
            (1 <<  7) | // code protection off
            (0 <<  6) | // nMCLR as digital input
            (0 <<  5) | // PWRTE enabled
           (00 <<  3) | // WDT disabled
            (4 <<  0),  // 100 = INTRC oscillator 16MHz; port I/O function on both RA6/OSC2/CLKO pin and RA7/OSC1/CLKI pin
          //(2 <<  0),  // 010 = HS oscillator
        Pic16F1824PwmPinInfo,
        arraylen(Pic16F1824PwmPinInfo),
        PicExtIntPinInfo14,
        arraylen(PicExtIntPinInfo14),
        nullptr,
        0,
        nullptr,
        0,
        {{0,0}}
    },
    {
        "Microchip PIC16F1827 18-Pin PDIP, SOIC",
        "PIC16F1827",
        "P16F1827",
        "16F1827",
        "PIC16F1827",
        'R',
        { 0x0C, 0x0D }, // PORTx
        { 0x0C, 0x0D }, // PORTx
        { 0x8C, 0x8D }, // TRISx
        4*1024,
        { { 0x20, 96 }, { 0xA0, 80 }, { 0x120, 80 }, { 0x1A0, 80 }, { 0x220, 48 } },
        Pic16f1827IoPinInfo,
        arraylen(Pic16f1827IoPinInfo),
        Pic16F1827AdcPinInfo,
        arraylen(Pic16F1827AdcPinInfo),
        1023,
        { 7, 8 },
//        0, // pwm TODO
        ISA_PIC16,
        EnhancedMidrangeCore14bit,
        18,
       (0 << (13+16)) | // LVP disabled
       (1 << (12+16)) | // DEBUG disable
       (1 << (10+16)) | // BORV: Brown-out Reset voltage (Vbor), low trip point selected
       (1 << ( 9+16)) | // STVREN: Stack Overflow/Underflow Reset Enable bit
       (0 << ( 8+16)) | // PLLEN: 4xPLL disabled
     (0xff << (0+16)) | // WRT: Write protection off
            (1 << 13) | // FCMEN enabled
            (1 << 12) | // IESO enabled
            (1 << 11) | // _CLKOUTEN disabled
            (3 <<  9) | // BOR enabled
            (1 <<  8) | // data protect off
            (1 <<  7) | // code protection off
            (0 <<  6) | // nMCLR as digital input
            (0 <<  5) | // PWRTE enabled
           (00 <<  3) | // WDT disabled
            (4 <<  0),  // 100 = INTRC oscillator 16MHz; port I/O function on both RA6/OSC2/CLKO pin and RA7/OSC1/CLKI pin
          //(2 <<  0),  // 010 = HS oscillator
        Pic16F1827PwmPinInfo,
        arraylen(Pic16F1827PwmPinInfo),
        PicExtIntPinInfo18,
        arraylen(PicExtIntPinInfo18),
        nullptr,
        0,
        nullptr,
        0,
        {{0,0}}
    },
    {
        "Microchip PIC10F202 6-SOT",
        "PIC10F202",
        "P10F202",
        "10F202",
        "PIC10F202",
        'G',
//        A  B  C  D  E  F  G  H  I  J  K  L  M  N  O  P
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x06}, // PORTx = GPIO
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x06}, // PORTx
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x06}, // TRISx
        512-1, //Location 001FFh contains the internal clock oscillator
               //calibration value. This value should never be overwritten.
        { { 0x08, 24 } },
        Pic6Pin_SOT23,
        arraylen(Pic6Pin_SOT23),
        nullptr,
        0,
        0,
        { },
//        0,
        ISA_PIC16,
        BaselineCore12bit,
        6,
            (0 <<  4) |     // nMCLR disabled
            (1 <<  3) |     // Code protection disabled
            (0 <<  2) |     // WDTE disabled
            (0 <<  1) |     //
            (0 <<  0),      //
        nullptr,
        0,
        nullptr,
        0,
        nullptr,
        0,
        nullptr,
        0,
        {{0,0}}
    },
    {
        "Microchip PIC10F200 6-SOT",
        "PIC10F200",
        "P10F200",
        "10F200",
        "PIC10F200",
        'G',
//        A  B  C  D  E  F  G  H  I  J  K  L  M  N  O  P
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x06 }, // PORTx = GPIO
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x06 }, // PORTx
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x06 }, // TRISx
        256-1, //Location 00FFh contains the internal clock oscillator
               //calibration value. This value should never be overwritten.
        { { 0x10, 16 } },
        Pic6Pin_SOT23,
        arraylen(Pic6Pin_SOT23),
        nullptr,
        0,
        0,
        { },
//        0,
        ISA_PIC16,
        BaselineCore12bit,
        6,
            (0 <<  4) |     // nMCLR disabled
            (1 <<  3) |     // Code protection disabled
            (0 <<  2) |     // WDTE disabled
            (0 <<  1) |     //
            (0 <<  0),      //
        nullptr,
        0,
        nullptr,
        0,
        nullptr,
        0,
        nullptr,
        0,
        {{0,0}}
    },
    {
        "Microchip PIC18F4520 40-PDIP",
        "PIC18F4520",
        "P18F4520",
        "18F4520",
        "pic18F4520",
        'R',
        { 0x05, 0x06, 0x07, 0x08, 0x09 }, // PORTx
        { 0x05, 0x06, 0x07, 0x08, 0x09 }, // PORTx
        { 0x85, 0x86, 0x87, 0x88, 0x89 }, // TRISx
        16*1024,
        { { 0x20, 96 }, { 0xA0, 80 }, { 0x110, 96 }, { 0x190, 96 } },
        Pic18F4520IoPinInfo,
        arraylen(Pic18F4520IoPinInfo),
        Pic18F4520AdcPinInfo,
        arraylen(Pic18F4520AdcPinInfo),
        1023,
        { 26, 25 },
//        16,
        ISA_PIC18,
        PIC18HighEndCore16bit,
        40,
        // code protection off, debug off, flash write off, EE code protection
        // off, LVP disabled, BOD enabled, CP off, PWRT enabled, WDT disabled,
        // HS oscillator
        0xFFFF,                         /// A VOIR
        PicPwmPinInfo40,
        arraylen(PicPwmPinInfo40),
        PicExtIntPinInfo40,
        arraylen(PicExtIntPinInfo40),
        McuSpiInfoPic18F4520,
        arraylen(McuSpiInfoPic18F4520),
        McuI2cInfoPic18F4520,
        arraylen(McuI2cInfoPic18F4520),
        {{0,0}}
    },
//===========================================================================
    {
        "ESP8266",
        "",
        "",
        "",
        "",
        'I',
        { 0x20 }, // PINx
        { 0x22 }, // PORTx
        { 0x21 }, // DDRx
        128 * 1024,
        { { 0x200, 8192 } },
        ESP8266IoPinInfo,
        arraylen(ESP8266IoPinInfo),
        ESP8266AdcPinInfo,
        arraylen(ESP8266AdcPinInfo),
        1023,
        { 21 , 22 },
//        0,
        ISA_ESP,
        ESP8266,
        22,
        0,
        nullptr, //ESP8266PwmPinInfo,
        0, //arraylen(ESP8266PwmPinInfo),
        nullptr,
        0,
        nullptr,
        0,
        nullptr,
        0,
        {{0,0}}
    },
//===========================================================================
    {   ///// Added by JG               // A Completer
        #define BASE_A 0x40020000
        #define BASE_B 0x40020400
        #define BASE_C 0x40020800
        #define BASE_D 0x40020C00
        #define BASE_E 0x40021000
        #define BASE_F 0x40021400
        #define BASE_G 0x40021800
        "St ARM STM32F40X 144-LQFP",                    // Nom
        "STM32F40X",                                    // Liste d'alias
        "",
        "",
        "",
        'P',                                        // Prefixe ports
        { BASE_A+0x14, BASE_B+0x14, BASE_C+0x14, BASE_D+0x14, BASE_E+0x14, BASE_F+0x14, BASE_G+0x18, 0, 0, 0, 0},   // Adresses registres In
        { BASE_A+0x18, BASE_B+0x18, BASE_C+0x18, BASE_D+0x18, BASE_E+0x18, BASE_F+0x18, BASE_G+0x18, 0, 0, 0, 0},   // Adresses registres Out
        { BASE_A, BASE_B, BASE_C, BASE_D, BASE_E, BASE_F, BASE_G, 0, 0, 0, 0},                                      // Adresses registres Dir
        512 * 1024,                                 // 1 MO Flash
        { { 0x0800000, 0x100000 } },                // debut + taille
        ArmSTM32F40X_144LQFPIoPinInfo,              // Pin info
        arraylen(ArmSTM32F40X_144LQFPIoPinInfo),    // Nb broches (declarees)
        ArmSTM32F40X_144LQFPAdcPinInfo,             // Adc info
        9,                                          // Nb Adc
        4095,                                       // Adc valeur Maxi                          // 12 bits
        {97, 96},                                   // UART RX + TX (UART 6)                    // RXn + TXn in I/O definition
//        0,                                          // PWM default Pin
        ISA_ARM,                                    // Type
        CortexF4,                                   // Core
        144,                                        // Nb de broches
        0,                                          // For PIC but may be used for ARM
        ArmSTM32F40X_144LQFPPwmPinInfo,             // Pwm info
        arraylen(ArmSTM32F40X_144LQFPPwmPinInfo),   // Pwm info size
        nullptr,                                    // ExtInt info
        0,                                          // ExtInt info size
        ArmSTM32F40X_144LQFPMcuSpiInfo,             // SPI info
        arraylen(ArmSTM32F40X_144LQFPMcuSpiInfo),   // SPI info size
        ArmSTM32F40X_144LQFPMcuI2cInfo,             // I2C info
        arraylen(ArmSTM32F40X_144LQFPMcuI2cInfo),   // I2C info size
        {{0,0}}
        #undef BASE_A
        #undef BASE_B
        #undef BASE_C
        #undef BASE_D
        #undef BASE_E
        #undef BASE_F
        #undef BASE_G
    },

    {
        #define BASE_A 0x40010800
        #define BASE_B 0x40010C00
        "St ARM STM32F10X 48-LQFP / Bluepill",          // Nom
        "STM32F10X",                                    // Liste d'alias
        "BLUEPILL",
        "",
        "",
        'P',                                        // Prefixe ports
        { BASE_A+0x14, BASE_B+0x14, 0, 0, 0, 0, 0, 0, 0, 0, 0},     // Adresses registres In
        { BASE_A+0x18, BASE_B+0x18, 0, 0, 0, 0, 0, 0, 0, 0, 0},     // Adresses registres Out
        { BASE_A, BASE_B,  0, 0, 0, 0, 0, 0, 0, 0, 0},              // Adresses registres Dir
        64 * 1024,                                  // 128 KO Flash
        { { 0x0800000, 0x20000 } },                 // debut + taille
        ArmSTM32F10X_48LQFPIoPinInfo,               // Pin info
        arraylen(ArmSTM32F10X_48LQFPIoPinInfo),     // Nb broches (declarees)
        ArmSTM32F10X_48LQFPAdcPinInfo,              // Adc info
        8,                                          // Nb Adc
        4095,                                       // Adc valeur Maxi                          // 12 bits
        {22, 21},                                   // UART RX + TX (UART 3)                    // RXn + TXn in I/O definition
//        0,                                          // PWM default Pin
        ISA_ARM,                                    // Type
        CortexF1,                                   // Core
        48,                                         // Nb de broches
        0,                                          // For PIC but may be used for ARM
        ArmSTM32F10X_48LQFPPwmPinInfo,              // Pwm info
        arraylen(ArmSTM32F10X_48LQFPPwmPinInfo),    // Pwm info size
        nullptr,                                    // ExtInt info
        0,                                          // ExtInt info size
        ArmSTM32F10X_48LQFPMcuSpiInfo,              // SPI info
        arraylen(ArmSTM32F10X_48LQFPMcuSpiInfo),    // SPI info size
        ArmSTM32F10X_48LQFPMcuI2cInfo,              // I2C info
        arraylen(ArmSTM32F10X_48LQFPMcuI2cInfo),    // I2C info size
        {{0,0}}
        #undef BASE_A
        #undef BASE_B
    },

//===========================================================================
    {
        "Controllino Maxi / Ext bytecode",
        "ControllinoMaxi",
        "",
        "",
        "",
        'P',
        { 0x20, 0x23, 0x26, 0x29, 0x2C, 0x2F, 0x32, 0x100, 0x103, 0x106, 0x109 }, // PINx
        { 0x22, 0x25, 0x28, 0x2B, 0x2E, 0x32, 0x34, 0x102, 0x105, 0x108, 0x10B }, // PORTx
        { 0x21, 0x24, 0x27, 0x2A, 0x2D, 0x30, 0x33, 0x101, 0x104, 0x107, 0x10A }, // DDRx
        128 * 1024,
        { { 0x200, 8192 } },
        ControllinoMaxiIoPinInfo,
        arraylen(ControllinoMaxiIoPinInfo),
        ControllinoMaxiAdcPinInfo,
        arraylen(ControllinoMaxiAdcPinInfo),
        1023,
        { 2 , 3 },
//        23,
        ISA_XINTERPRETED,
        EnhancedCore4M,
        100,
        0,
        ControllinoMaxiPwmPinInfo,
        arraylen(ControllinoMaxiPwmPinInfo),
        nullptr,
        0,
        nullptr,
        0,
        nullptr,
        0,
        {{0,0}}
    },
    {
        "PC LPT COM",
        "PcLptCom",
        "",
        "",
        "",
        'L', // PC LPT & COM support
        { 0x00 },
        { 0x00 },
        { 0x00 },
        0,
        { { 0x00, int(0xffffff) } },
        PcCfg,
        arraylen(PcCfg),
        nullptr,
        0,
        0,
        { 0, 0 },
//        0,
        ISA_PC,
        PC_LPT_COM,
        0,
        0,
        nullptr,
        0,
        nullptr,
        0,
        nullptr,
        0,
        nullptr,
        0,
        {{0,0}}
    },
};

// clang-format on

std::vector<McuIoInfo> SupportedMcus(SupportedMcus_, SupportedMcus_ + arraylen(SupportedMcus_));

#endif //__MCUTABLE_H__
