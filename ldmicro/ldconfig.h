
#ifndef __LD_CONFIG_H__
#define __LD_CONFIG_H__

#define MAX_NAME_LEN                 64 // 128
#define MAX_COMMENT_LEN             512 // 384 // 1024
#define MAX_LOOK_UP_TABLE_LEN        64
#define MAX_SHIFT_REGISTER_STAGES   256
#define MAX_STRING_LEN              256

#define ISA_PIC16           0x01
#define ISA_AVR             0x02
#define ISA_PC_LPT_COM      0x03
#define ISA_HARDWARE        ISA_PC_LPT_COM
#define ISA_INTERPRETED     0x05
#define ISA_NETZER          0x06
#define ISA_PASCAL          0x07
#define ISA_XINTERPRETED    0x0A    // Extended interpeter
#define ISA_ESP8266         0x0B

#define MAX_IO_PORTS        ('P'-'A'+1)
#define MAX_RAM_SECTIONS    8
#define MAX_ROM_SECTIONS    1

#endif //__LD_CONFIG_H__

