//-----------------------------------------------------------------------------
// A sample interpreter for the .int files generate by LDmicro. These files
// represent a ladder logic program for a simple 'virtual machine.' The
// interpreter must simulate the virtual machine and for proper timing the
// program must be run over and over, with the period specified when it was
// compiled (in Settings -> MCU Parameters).
//
// This method of running the ladder logic code would be useful if you wanted
// to embed a ladder logic interpreter inside another program. LDmicro has
// converted all variables into addresses, for speed of execution. However,
// the .int file includes the mapping between variable names (same names
// that the user specifies, that are visible on the ladder diagram) and
// addresses. You can use this to establish specially-named variables that
// define the interface between your ladder code and the rest of your program.
//
// In this example, I use this mechanism to print the value of the integer
// variable 'a' after every cycle, and to generate a square wave with period
// 2*Tcycle on the input 'Xosc'. That is only for demonstration purposes, of
// course.
//
// In a real application you would need some way to get the information in the
// .int file into your device; this would be very application-dependent. Then
// you would need something like the InterpretOneCycle() routine to actually
// run the code. You can redefine the program and data memory sizes to
// whatever you think is practical; there are no particular constraints.
//
// The disassembler is just for debugging, of course. Note the unintuitive
// names for the condition ops; the INT_IFs are backwards, and the INT_ELSE
// is actually an unconditional jump! This is because I reused the names
// from the intermediate code that LDmicro uses, in which the if/then/else
// constructs have not yet been resolved into (possibly conditional)
// absolute jumps. It makes a lot of sense to me, but probably not so much
// to you; oh well.
//
// Jonathan Westhues, Aug 2005
//-----------------------------------------------------------------------------
#include <stdio.h>
#include <ctype.h>
#include <windows.h>

typedef unsigned char BYTE;     // 8-bit unsigned
typedef unsigned short WORD;    // 16-bit unsigned
typedef signed short SWORD;     // 16-bit signed

#define INTCODE_H_CONSTANTS_ONLY

#include "intcode.h"
#include "xinterpreter.h"

// Some arbitrary limits on the program and data size
#define MAX_OPS                 1024
#define MAX_VARIABLES           256
#define MAX_INTERNAL_RELAYS     256

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

BinOp8bits Program[MAX_OPS];

SWORD Integers[MAX_VARIABLES];
BYTE Bits[MAX_INTERNAL_RELAYS];

char IntegersSymbols[MAX_VARIABLES][40];
char BitsSymbols[MAX_INTERNAL_RELAYS][40];

// This are addresses (indices into Integers[] or Bits[]) used so that your
// C code can get at some of the ladder variables, by remembering the
// mapping between some ladder names and their addresses.
int SpecialAddrForA;
int SpecialAddrForXosc;

//-----------------------------------------------------------------------------
// What follows are just routines to load the program, which I represent as
// hex bytes, one instruction per line, into memory. You don't need to
// remember the length of the program because the last instruction is a
// special marker (INT_END_OF_PROGRAM).
//
void BadFormat(void)
{
    fprintf(stderr, "Bad program format.\n");
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
    FILE *f = fopen(fileName, "r");
    char line[80];

	for (int i = 0; i < MAX_VARIABLES; i++) {
		sprintf(IntegersSymbols[i], "%d", i);
	}

	for (int i = 0; i < MAX_INTERNAL_RELAYS; i++) {
		sprintf(BitsSymbols[i], "%d", i);
	}

    // This is not suitable for untrusted input.

    if(!f) {
        fprintf(stderr, "couldn't open '%s'\n", f);
        exit(-1);
    }

    if(!fgets(line, sizeof(line), f)) BadFormat();
    if(strcmp(line, "$$LDcode\n")!=0) BadFormat();

    for(pc = 0; ; pc++) {
        char *t, i;
        BYTE *b;

        if(!fgets(line, sizeof(line), f)) BadFormat();
        if(line[0] == '$') break;
        if(strlen(line) != sizeof(BinOp8bits)*2 + 1) BadFormat();

        t = line;
        b = (BYTE *)&Program[pc];

        for(i = 0; i < sizeof(BinOp8bits); i++) {
            b[i] = HexDigit(t[1]) | (HexDigit(t[0]) << 4);
            t += 2;
        }
    }

    SpecialAddrForA = -1;
    SpecialAddrForXosc = -1;
	char (*sym)[256][40] = &BitsSymbols;
	int offset = 0;

    while(fgets(line, sizeof(line), f)) {
		static const char *delim = ": \n";
		char *s = strtok(line, delim);
		char *n = strtok(NULL, delim);

		if (strcmp(s, "$$int16s") == 0) { sym = &IntegersSymbols; offset = XINTERNAL_OFFSET;}
		if (strcmp(s, "$$bits") == 0) { sym = &BitsSymbols; offset = XINTERNAL_OFFSET; }
		if (strcmp(s, "$$pin16s") == 0) { sym = &IntegersSymbols; offset = 0;}
		if (strcmp(s, "$$pins") == 0) { sym = &BitsSymbols; offset = 0;}

		if (s[0] != '$' && s[1] != '$' && n) {
			int index = atoi(n) + offset;
			if (index >= 0 && index < MAX_VARIABLES) {
				sprintf((*sym)[index], "'%s'", s);
				printf("New symbol %0d: %s\n", index, s);
			}
		}
        if(strcmp(s, "a")==0 && n) {
            SpecialAddrForA = atoi(n);
        }
        if(strcmp(s, "Xosc")==0 && n) {
            SpecialAddrForXosc = atoi(n);
        }
        if(strcmp(line, "$$cycle")==0 && n) {
            if(atoi(n) != 10*1000) {
                fprintf(stderr, "cycle time was not 10 ms when compiled; "
                    "please fix that.\n");
                return(-1);
            }
        }
    }

    if(SpecialAddrForA < 0 || SpecialAddrForXosc < 0) {
        fprintf(stderr, "special interface variables 'a' or 'Xosc' not "
            "used in prog.\n");
        return(-1);
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
void Disassemble(void)
{
    int pc;
    for(pc = 0; ; pc++) {
        BinOp8bits *p = &Program[pc];
        printf("%03x: ", pc);

        switch(Program[pc].op) {
            case INT_SET_BIT:
                printf("bits[%s] := 1", BitsSymbols[p->name1]);
                break;

            case INT_CLEAR_BIT:
                printf("bits[%s] := 0", BitsSymbols[p->name1]);
                break;

            case INT_COPY_BIT_TO_BIT:
                printf("bits[%s] := bits[%s]", BitsSymbols[p->name1], BitsSymbols[p->name2]);
                break;

            case INT_SET_VARIABLE_TO_LITERAL:
                printf("int16s[%s] := %d (0x%04x)", IntegersSymbols[p->name1],
                    p->literal);
                break;

            case INT_SET_VARIABLE_TO_VARIABLE:
                printf("int16s[%s] := int16s[%s]", IntegersSymbols[p->name1], IntegersSymbols[p->name2]);
                break;

            case INT_INCREMENT_VARIABLE:
                printf("(int16s[%s])++", IntegersSymbols[p->name1]);
                break;

			case INT_SET_PWM:
				printf("setpwm(%s, %d Hz)", IntegersSymbols[p->name1], p->literal);
				break;

			case INT_READ_ADC:
				printf("readadc(%s)", IntegersSymbols[p->name1]);
				break;

            {
                char c;
                case INT_SET_VARIABLE_ADD: c = '+'; goto arith;
                case INT_SET_VARIABLE_SUBTRACT: c = '-'; goto arith;
                case INT_SET_VARIABLE_MULTIPLY: c = '*'; goto arith;
                case INT_SET_VARIABLE_DIVIDE: c = '/'; goto arith;
arith:
                    printf("int16s[%s] := int16s[%s] %c int16s[%s]",
						IntegersSymbols[p->name1], IntegersSymbols[p->name2], c, IntegersSymbols[p->name3]);
                    break;
            }

            case INT_IF_BIT_SET:
                printf("ifnot (bits[%s] set)", BitsSymbols[p->name1]);
                goto cond;
            case INT_IF_BIT_CLEAR:
                printf("ifnot (bits[%s] clear)", BitsSymbols[p->name1]);
                goto cond;
            case INT_IF_VARIABLE_LES_LITERAL:
                printf("ifnot (int16s[%s] < %d)", IntegersSymbols[p->name1], p->literal);
                goto cond;
            case INT_IF_VARIABLE_EQUALS_VARIABLE:
                printf("ifnot (int16s[%s] == int16s[%s])", IntegersSymbols[p->name1],
					IntegersSymbols[p->name2]);
                goto cond;
            case INT_IF_VARIABLE_GRT_VARIABLE:
                printf("ifnot (int16s[%s] > int16s[%s])", IntegersSymbols[p->name1],
					IntegersSymbols[p->name2]);
                goto cond;
cond:
                printf(" jump %03x", pc + p->name3 + 1);
                break;

            case INT_ELSE:
                printf("jump %03x", pc + p->name3 + 1);
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
void InterpretOneCycle(void)
{
    int pc;
    for(pc = 0; ; pc++) {
        BinOp8bits *p = &Program[pc];

        switch(Program[pc].op) {
            case INT_SET_BIT:
                Bits[p->name1] = 1;
                break;

            case INT_CLEAR_BIT:
                Bits[p->name1] = 0;
                break;

            case INT_COPY_BIT_TO_BIT:
                Bits[p->name1] = Bits[p->name2];
                break;

            case INT_SET_VARIABLE_TO_LITERAL:
                Integers[p->name1] = p->literal;
                break;

            case INT_SET_VARIABLE_TO_VARIABLE:
                Integers[p->name1] = Integers[p->name2];
                break;

            case INT_INCREMENT_VARIABLE:
                (Integers[p->name1])++;
                break;

            case INT_SET_VARIABLE_ADD:
                Integers[p->name1] = Integers[p->name2] + Integers[p->name3];
                break;

            case INT_SET_VARIABLE_SUBTRACT:
                Integers[p->name1] = Integers[p->name2] - Integers[p->name3];
                break;

            case INT_SET_VARIABLE_MULTIPLY:
                Integers[p->name1] = Integers[p->name2] * Integers[p->name3];
                break;

            case INT_SET_VARIABLE_DIVIDE:
                if(Integers[p->name3] != 0) {
                    Integers[p->name1] = Integers[p->name2] /
                                                Integers[p->name3];
                }
                break;

            case INT_IF_BIT_SET:
                if(!Bits[p->name1]) pc += p->name3;
                break;

            case INT_IF_BIT_CLEAR:
                if(Bits[p->name1]) pc += p->name3;
                break;

            case INT_IF_VARIABLE_LES_LITERAL:
                if(!(Integers[p->name1] < p->literal)) pc += p->name3;
                break;

            case INT_IF_VARIABLE_EQUALS_VARIABLE:
                if(!(Integers[p->name1] == Integers[p->name2])) pc += p->name3;
                break;

            case INT_IF_VARIABLE_GRT_VARIABLE:
                if(!(Integers[p->name1] > Integers[p->name2])) pc += p->name3;
                break;

            case INT_ELSE:
				pc += p->name3;
                break;

            case INT_END_OF_PROGRAM:
                return;
        }
    }
}


int main(int argc, char **argv)
{
    int i;

    if(argc != 2) {
        fprintf(stderr, "usage: %s xxx.int\n", argv[0]);
        return -1;
    }

    int rc = LoadProgram(argv[1]);
    memset(Integers, 0, sizeof(Integers));
    memset(Bits, 0, sizeof(Bits));

	Disassemble();
	if (rc) exit(rc);

    // 1000 cycles times 10 ms gives 10 seconds execution
    for(i = 0; i < 1000; i++) {
        InterpretOneCycle();

        // Example for reaching in and reading a variable: just print it.
        printf("a = %d              \r", Integers[SpecialAddrForA]);

        // Example for reaching in and writing a variable.
        Bits[SpecialAddrForXosc] = !Bits[SpecialAddrForXosc];

        // XXX, nonportable; replace with whatever timing functions are
        // available on your target.
        Sleep(10);
    }

    return 0;
}
