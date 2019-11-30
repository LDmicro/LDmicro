//-----------------------------------------------------------------------------
// Copyright 2012-2013 Sven Schlender
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
//-----------------------------------------------------------------------------

#include <stdint.h>

#define CURRENT_FORMAT_VERSION (1u)
#define MAX_PROJECTNAME_LENGTH (16u)

#define PROJECT_ID_IO (0xFFu)

#define START_TAG_BYTE1 (0x00)
#define START_TAG_BYTE2 (0xEF)
#define START_TAG_BYTE3 (0xF4)
#define START_TAG_BYTE4 (0xF0)

#pragma pack(1)

typedef struct MetaFlagsTag {
    uint8_t FormatVersion : 4;
    uint8_t IsCompiled : 1;
    uint8_t Reserved;
} MetaFlags;

typedef struct NetzerMetaInformationTag {
    uint8_t   StartTag[4];
    uint16_t  ImageCRC;
    uint16_t  ImageLength;
    MetaFlags Flags;
    uint8_t   ProjectID; /// Dedicated for given project.
    uint16_t  Opcodes;
    uint32_t  CycleTime;
    uint32_t  TimeStamp;
    uint8_t   ProjectnameLength;
    //  BYTE    Projectname[];
} NetzerMetaInformation_t;

typedef enum {
    MTT_END_OF_HEADER = 0xFF,
    MTT_LOCAL_VARIABLE = 0x00,
    MTT_LOCAL_RELAY = 0x01,
} MetaTagType_t;

typedef struct {
    MetaTagType_t Type;
    uint8_t       Length; // For header close tags (MTT_END_OF_HEADER) this one is omitted.
    //  BYTE TagData[];
} ProcessMetaTag;

#pragma pack()

typedef enum NetzerIntCodesTag {
    OP_END_OF_PROGRAM = 0x00,
    OP_BIT_SET = 0x04,
    OP_BIT_SET_IO = 0x08,
    OP_BIT_CLEAR = 0x0C,
    OP_BIT_CLEAR_IO = 0x10,
    OP_COPY_BITS_SAME_REGISTER = 0x14,
    OP_COPY_BITS = 0x18,
    OP_COPY_BITS_IO = 0x1C,
    OP_COPY_BIT_TO_IO = 0x20,
    OP_COPY_BIT_FROM_IO = 0x24,
    OP_SET_VARIABLE_TO_LITERAL = 0x28,
    OP_SET_VARIABLE_TO_LITERAL_IO = 0x2C,
    OP_SET_VARIABLE_TO_VARIABLE = 0x30,
    OP_SET_VARIABLE_TO_VARIABLE_IO = 0x34,
    OP_SET_VARIABLE_IO_TO_VARIABLE = 0x38,
    OP_SET_VARIABLE_IO_TO_VARIABLE_IO = 0x3C,
    OP_INCREMENT_VARIABLE = 0x40,
    OP_INCREMENT_VARIABLE_IO = 0x44,

    //TODO
    OP_DECREMENT_VARIABLE = 0x45,
    OP_DECREMENT_VARIABLE_IO = 0x45,
    OP_SET_VARIABLE_AND = 0x45,
    OP_SET_VARIABLE_OR = 0x45,
    OP_SET_VARIABLE_XOR = 0x45,
    OP_SET_VARIABLE_NOT = 0x45,
    OP_SET_VARIABLE_NEG = 0x45,
    OP_SET_VARIABLE_RANDOM = 0x45,
    OP_SET_VARIABLE_SHL = 0x45,
    OP_SET_VARIABLE_SHR = 0x45,
    OP_SET_VARIABLE_SR0 = 0x45,
    OP_SET_VARIABLE_ROL = 0x45,
    OP_SET_VARIABLE_ROR = 0x45,

    OP_SET_VARIABLE_ADD = 0x48,
    OP_SET_VARIABLE_SUB = 0x4C,
    OP_SET_VARIABLE_MUL = 0x50,
    OP_SET_VARIABLE_DIV = 0x54,
    OP_SET_VARIABLE_MOD = 0x55,

    OP_IF_BIT_SET = 0x58,
    OP_IF_BIT_SET_IO = 0x5C,
    OP_IF_BIT_CLEARED = 0x60,
    OP_IF_BIT_CLEARED_IO = 0x64,

    OP_IF_VARIABLE_LES_LITERAL = 0x68,
    OP_IF_VARIABLE_IO_LES_LITERAL = 0x6C,
    OP_IF_VARIABLE_GRT_LITERAL = 0x00,
    OP_IF_VARIABLE_IO_GRT_LITERAL = 0x00,
    OP_IF_VARIABLE_EQU_LITERAL = 0x00,
    OP_IF_VARIABLE_NEQ_LITERAL = 0x00,
    OP_IF_VARIABLE_IO_EQU_LITERAL = 0x00,
    OP_IF_VARIABLE_IO_NEQ_LITERAL = 0x00,
    OP_IF_VARIABLE_EQUALS_VARIABLE = 0x70,
    OP_IF_VARIABLE_NEQ_VARIABLE = 0x00,
    OP_IF_VARIABLE_GRT_VARIABLE = 0x74,
    OP_IF_VARIABLE_GEQ_VARIABLE = 0x00,
    OP_IF_VARIABLE_LES_VARIABLE = 0x00,

    OP_ELSE = 0x78,

    OP_WRITE_STRING = 0x7C,
    OP_WRITE_STRING_IO = 0x80
} NetzerIntCodes;

typedef struct OpcodeMetaTag {
    int Opcodes;
    int BytesConsumed;
} OpcodeMeta;
