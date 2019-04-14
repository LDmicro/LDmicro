//-----------------------------------------------------------------------------
// A sample interpreter for the .xint files generate by LDmicro. These files
// represent a ladder logic program for a simple 'virtual machine.' The
// interpreter must simulate the virtual machine and for proper timing the
// program must be run over and over, with the period specified when it was
// compiled (in Settings -> MCU Parameters).
//
// Frederic RIBLE 2016 (frible@teaser.fr)
//
// Based on ldinterpret.c by Jonathan Westhues
//
//-----------------------------------------------------------------------------
#include "stdafx.h"

typedef unsigned char  BYTE;  // 8-bit unsigned
typedef unsigned short WORD;  // 16-bit unsigned

#define INTCODE_H_CONSTANTS_ONLY

#include "intcode.h"

// Some arbitrary limits on the program and data size
#define MAX_OPS 1024
#define MAX_VARIABLES 256
#define MAX_INTERNAL_RELAYS 256

// This data structure represents a single instruction for the 'virtual
// machine.' The .op field gives the opcode, and the other fields give
// arguments. I have defined all of these as 16-bit fields for generality,
// but if you want then you can crunch them down to 8-bit fields (and
// limit yourself to 256 of each type of variable, of course). If you
// crunch down .op then nothing bad happens at all. If you crunch down
// .literal then you only have 8-bit literals now (so you can't move
// 300 into 'var'). If you crunch down .name3 then that limits your code size,
// because that is the field used to encode the jump addresses.
//
// A more compact encoding is very possible if space is a problem for
// you. You will probably need some kind of translator regardless, though,
// to put it in whatever format you're going to pack in flash or whatever,
// and also to pick out the name <-> address mappings for those variables
// that you're going to use for your interface out. I will therefore leave
// that up to you.

uint32_t Program[MAX_OPS];

SWORD Integers[MAX_VARIABLES];
BYTE  Bits[MAX_VARIABLES];

#define READ_BIT(addr) Bits[addr]
#define WRITE_BIT(addr, value) Bits[addr] = (value)
#define READ_INT(addr) Integers[addr]
#define WRITE_INT(addr, value) Integers[addr] = (value)

char Symbols[MAX_VARIABLES][40];
int  line_number = 0;

//-----------------------------------------------------------------------------
// What follows are just routines to load the program, which I represent as
// hex bytes, one instruction per line, into memory. You don't need to
// remember the length of the program because the last instruction is a
// special marker (INT_END_OF_PROGRAM).
//
void BadFormat()
{
    fprintf(stderr, "Bad program format at line %d.\n", line_number);
    exit(-1);
}

int HexDigit(int c)
{
    c = tolower(c);
    if(isdigit(c)) {
        return c - '0';
    } else if(c >= 'a' && c <= 'f') {
        return (c - 'a') + 10;
    } else {
        BadFormat();
    }
    return 0;
}

int LoadProgram(char *fileName)
{
    int pc;
    int i;

    FILE *f = fopen(fileName, "r");
    char  line[80];

    line_number = 0;
    for(i = 0; i < MAX_VARIABLES; i++) {
        sprintf(Symbols[i], "%d", i);
    }

    // This is not suitable for untrusted input.

    if(!f) {
        fprintf(stderr, "couldn't open '%s'\n", fileName);
        exit(-1);
    }

    if(!fgets(line, sizeof(line), f))
        BadFormat();
    line_number++;
    if(strncmp(line, "$$IO", 4) != 0)
        BadFormat();

    while(fgets(line, sizeof(line), f)) {
        int  addr;
        char name[40];
        int  type;
        int  pin;
        int  modbus_slave;
        int  modbus_offset;

        line_number++;
        if(line[0] == '$')
            break;

        if(sscanf(line, "%d %s %d %d %d %d", &addr, name, &type, &pin, &modbus_slave, &modbus_offset) != 6)
            BadFormat();
        if(addr < 0 || addr >= MAX_VARIABLES)
            BadFormat();
        strcpy(Symbols[addr], name);
    }

    if(strncmp(line, "$$LDcode", 8) != 0)
        BadFormat();

    pc = 0;
    while(fgets(line, sizeof(line), f)) {
        char *t;

        line_number++;
        if(line[0] == '$')
            break;
        for(t = line; t[0] >= 32 && t[1] >= 32; t += 2) {
            Program[pc++] = HexDigit(t[1]) | (HexDigit(t[0]) << 4);
        }
    }

    fclose(f);
    return 0;
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Disassemble the program and pretty-print it. This is just for debugging,
// and it is also the only documentation for what each op does. The bit
// variables (internal relays or whatever) live in a separate space from the
// integer variables; I refer to those as bits[addr] and int16s[addr]
// respectively.
//-----------------------------------------------------------------------------
void Disassemble()
{
    char c;
    for(int pc = 0;;) {
        printf("%03x: ", pc);

        switch(Program[pc]) {
            case INT_SET_BIT:
                printf("bits[%s] := 1", Symbols[Program[pc + 1]]);
                pc += 2;
                break;

            case INT_CLEAR_BIT:
                printf("bits[%s] := 0", Symbols[Program[pc + 1]]);
                pc += 2;
                break;

            case INT_COPY_BIT_TO_BIT:
                printf("bits[%s] := bits[%s]", Symbols[Program[pc + 1]], Symbols[Program[pc + 2]]);
                pc += 3;
                break;

            case INT_SET_VARIABLE_TO_LITERAL:
                printf("int16s[%s] := %d", Symbols[Program[pc + 1]], Program[pc + 2] + (Program[pc + 3] << 8));
                pc += 4;
                break;

            case INT_SET_VARIABLE_TO_VARIABLE:
                printf("int16s[%s] := int16s[%s]", Symbols[Program[pc + 1]], Symbols[Program[pc + 2]]);
                pc += 3;
                break;

            case INT_INCREMENT_VARIABLE:
                printf("(int16s[%s])++", Symbols[Program[pc + 1]]);
                pc += 2;
                break;

            case INT_SET_PWM:
                printf("setpwm(%s, %d Hz)", Symbols[Program[pc + 1]], Program[pc + 2] + (Program[pc + 3] << 8));
                pc += 4;
                break;

            case INT_READ_ADC:
                printf("readadc(%s)", Symbols[Program[pc + 1]]);
                pc += 2;
                break;

            case INT_SET_VARIABLE_ADD:
                c = '+';
                goto arith;
            case INT_SET_VARIABLE_SUBTRACT:
                c = '-';
                goto arith;
            case INT_SET_VARIABLE_MULTIPLY:
                c = '*';
                goto arith;
            case INT_SET_VARIABLE_DIVIDE:
                c = '/';
                goto arith;
            case INT_SET_VARIABLE_MOD:
                c = '%';
                goto arith;
            arith:
                printf("int16s[%s] := int16s[%s] %c int16s[%s]",
                       Symbols[Program[pc]],
                       Symbols[Program[pc + 2]],
                       c,
                       Symbols[Program[pc + 3]]);
                pc += 4;
                break;

            case INT_IF_BIT_SET:
                printf("ifnot (bits[%s] set)", Symbols[Program[pc + 1]]);
                pc += 2;
                goto cond;
            case INT_IF_BIT_CLEAR:
                printf("ifnot (bits[%s] clear)", Symbols[Program[pc + 1]]);
                pc += 2;
                goto cond;
            case INT_IF_VARIABLE_LES_LITERAL:
                printf("ifnot (int16s[%s] < %d)", Symbols[Program[pc + 1]], Program[pc + 2] + (Program[pc + 3] << 8));
                pc += 4;
                goto cond;
            case INT_IF_VARIABLE_EQUALS_VARIABLE:
                printf("ifnot (int16s[%s] == int16s[%s])", Symbols[Program[pc + 1]], Symbols[Program[pc + 2]]);
                pc += 3;
                goto cond;
            case INT_IF_VARIABLE_GRT_VARIABLE:
                printf("ifnot (int16s[%s] > int16s[%s])", Symbols[Program[pc + 1]], Symbols[Program[pc + 2]]);
                pc += 3;
                goto cond;
            cond:
                printf(" jump %03x", pc + Program[pc] + 1);
                pc += 1;
                break;

            case INT_ELSE:
                printf("jump %03x", pc + Program[pc + 1] + 2);
                pc += 2;
                break;

            case INT_END_OF_PROGRAM:
                printf("<end of program>\n");
                return;

            default:
                BadFormat();
                break;
        }
        printf("\n");
    }
}

//-----------------------------------------------------------------------------
// This is the actual interpreter. It runs the program, and needs no state
// other than that kept in Bits[] and Integers[]. If you specified a cycle
// time of 10 ms when you compiled the program, then you would have to
// call this function 100 times per second for the timing to be correct.
//
// The execution time of this function depends mostly on the length of the
// program. It will be a little bit data-dependent but not very.
//-----------------------------------------------------------------------------

void InterpretOneCycle()
{
    int pc;
    for(pc = 0;;) {
        switch(Program[pc]) {
            case INT_SET_BIT:
                WRITE_BIT(Program[pc + 1], 1);
                pc += 2;
                break;

            case INT_CLEAR_BIT:
                WRITE_BIT(Program[pc + 1], 0);
                pc += 2;
                break;

            case INT_COPY_BIT_TO_BIT:
                WRITE_BIT(Program[pc + 1], READ_BIT(Program[pc + 2]));
                pc += 3;
                break;

            case INT_SET_VARIABLE_TO_LITERAL:
                WRITE_INT(Program[pc + 1], Program[pc + 2] + (Program[pc + 3] << 8));
                pc += 4;
                break;

            case INT_SET_VARIABLE_TO_VARIABLE:
                WRITE_INT(Program[pc + 1], READ_INT(Program[pc + 2]));
                pc += 3;
                break;

            case INT_INCREMENT_VARIABLE:
                WRITE_INT(Program[pc + 1], READ_INT(Program[pc + 1]) + 1);
                pc += 2;
                break;

            case INT_SET_VARIABLE_ADD:
                WRITE_INT(Program[pc + 1], READ_INT(Program[pc + 2]) + READ_INT(Program[pc + 3]));
                pc += 4;
                break;

            case INT_SET_VARIABLE_SUBTRACT:
                WRITE_INT(Program[pc + 1], READ_INT(Program[pc + 2]) - READ_INT(Program[pc + 3]));
                pc += 4;
                break;

            case INT_SET_VARIABLE_MULTIPLY:
                WRITE_INT(Program[pc + 1], READ_INT(Program[pc + 2]) * READ_INT(Program[pc + 3]));
                pc += 4;
                break;

            case INT_SET_VARIABLE_DIVIDE:
                if(READ_INT(Program[pc + 3]) != 0) {
                    WRITE_INT(Program[pc + 1], READ_INT(Program[pc + 2]) / READ_INT(Program[pc + 3]));
                }
                pc += 4;
                break;

            case INT_SET_VARIABLE_MOD:
                if(READ_INT(Program[pc + 3]) != 0) {
                    WRITE_INT(Program[pc + 1], READ_INT(Program[pc + 2]) % READ_INT(Program[pc + 3]));
                }
                pc += 4;
                break;

            case INT_SET_PWM:
                //WRITE_PWM(Program[pc + 1]);    // PWM frequency is ignored
                pc += 4;
                break;

            case INT_READ_ADC:
                //READ_ADC(Program[pc + 1]);
                pc += 2;
                break;

            case INT_IF_BIT_SET:
                if(!READ_BIT(Program[pc + 1]))
                    pc += Program[pc + 2];
                pc += 3;
                break;

            case INT_IF_BIT_CLEAR:
                if(READ_BIT(Program[pc + 1]))
                    pc += Program[pc + 2];
                pc += 3;
                break;

            case INT_IF_VARIABLE_LES_LITERAL:
                if(!(READ_INT(Program[pc + 1]) < static_cast<int>(Program[pc + 2] + (Program[pc + 3] << 8))))
                    pc += Program[pc + 4];
                pc += 5;
                break;

            case INT_IF_VARIABLE_EQUALS_VARIABLE:
                if(!(READ_INT(Program[pc + 1]) == READ_INT(Program[pc + 2])))
                    pc += Program[pc + 3];
                pc += 4;
                break;

            case INT_IF_VARIABLE_GRT_VARIABLE:
                if(!(READ_INT(Program[pc + 1]) > READ_INT(Program[pc + 2])))
                    pc += Program[pc + 3];
                pc += 4;
                break;

            case INT_ELSE:
                pc += Program[pc + 1];
                pc += 2;
                break;

            case INT_END_OF_PROGRAM:
                return;

            default:
                return;
        }
    }
}

int main(int argc, char **argv)
{
    int i;
    int rc;

    if(argc != 2) {
        fprintf(stderr, "usage: %s xxx.xint\n", argv[0]);
        return -1;
    }

    rc = LoadProgram(argv[1]);
    memset(Integers, 0, sizeof(Integers));
    memset(Bits, 0, sizeof(Bits));

    Disassemble();
    if(rc)
        exit(rc);

    // 1000 cycles times 10 ms gives 10 seconds execution
    for(i = 0; i < 1000; i++) {
        InterpretOneCycle();
        Sleep(10);
    }

    return 0;
}
