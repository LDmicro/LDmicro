LDmicro0.1
MICRO=Microchip PIC16F877 40-PDIP
CYCLE=10000
CRYSTAL=4000000
BAUD=2400
COMPILED=c:\temp\asd.hex

IO LIST
    Xlimit at 2
    Yled at 22
    Ain at 3
END

PROGRAM
RUNG
    COMMENT Serial/ADC test program for PIC16F877: read an ADC input, convert from integer to string\r\n(three ASCII chars), then send it out over the serial port. Display a message at startup:
END
RUNG
    PARALLEL
        SERIES
            EQU Cinit 2
            MOVE initc 'a'
        END
        SERIES
            EQU Cinit 3
            MOVE initc 'd'
        END
        SERIES
            EQU Cinit 4
            MOVE initc 'c'
        END
        SERIES
            PARALLEL
                EQU Cinit 0
                EQU Cinit 5
            END
            MOVE initc 13
        END
        SERIES
            PARALLEL
                EQU Cinit 1
                EQU Cinit 6
            END
            MOVE initc 10
        END
    END
END
RUNG
    TON Ttod 500000
    CONTACTS Rinitdone 1
    CONTACTS Rtxbusy 1
    UART_SEND initc
    COIL Rtxbusy 0 0 0
END
RUNG
    CONTACTS Rtxbusy 0
    CTU Cinit 7
    COIL Rinitdone 0 0 0
END
RUNG
    COMMENT Clock for the ADC read and a circular counter that sequences it. Just block the\r\noscillator until the init sequence completes.
END
RUNG
    CONTACTS Rinitdone 0
    CONTACTS Rosc 1
    TOF Tct1 50000
    TON Tct2 50000
    COIL Rosc 0 0 0
END
RUNG
    CONTACTS Rosc 0
    CTC Cchar 4
END
RUNG
    OSR
    MOVE Cchar 3
END
RUNG
    COMMENT Read the A/D input, then saturate the input so that we only have to display three digits.
END
RUNG
    PARALLEL
        READ_ADC Ain
        SERIES
            LEQ Ain 999
            MOVE intval Ain
        END
        SERIES
            GEQ Ain 1000
            MOVE intval 999
        END
    END
END
RUNG
    COMMENT Split the integer into the ones, tens, and hundreds digits. This is just cumbersome, but\r\nit does not waste much memory (the multiply and divide are subroutines in the PIC code).
END
RUNG
    PARALLEL
        DIV hundreds intval 100
        MUL hundredsv hundreds 100
        SUB wohundreds intval hundredsv
        DIV tens wohundreds 10
        MUL tensv tens 10
        SUB ones wohundreds tensv
    END
END
RUNG
    COMMENT Now convert the ones, tens, and hundreds digits to ASCII characters by adding\r\nASCII character '0' (since the ASCII digits are contiguous in order).
END
RUNG
    PARALLEL
        ADD hunc hundreds '0'
        ADD tensc tens '0'
        ADD onesc ones '0'
    END
END
RUNG
    COMMENT Then send out the characters. For each line we must send out five characters:\r\nhundreds, tens, ones, ones, carriage return, newline; use a CTC to sequence.
END
RUNG
    PARALLEL
        SERIES
            EQU Cchar 0
            MOVE outch hunc
        END
        SERIES
            EQU Cchar 1
            MOVE outch tensc
        END
        SERIES
            EQU Cchar 2
            MOVE outch onesc
        END
        SERIES
            EQU Cchar 3
            MOVE outch 13
        END
        SERIES
            EQU Cchar 4
            MOVE outch 10
        END
    END
END
RUNG
    COMMENT Time it off Rosc (10 characters/s) which is slow so ignore the busy out.
END
RUNG
    CONTACTS Rosc 0
    OSR
    UART_SEND outch
END
RUNG
    PARALLEL
        CONTACTS Rosc 0
        CONTACTS Xlimit 1
    END
    COIL Yled 0 0 0
END
