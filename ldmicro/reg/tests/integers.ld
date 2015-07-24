LDmicro0.1
MICRO=Atmel AVR ATmega162 40-PDIP
CYCLE=50000
CRYSTAL=10000000
BAUD=2400
COMPILED=C:\depot\ldmicro\reg\expected\integers.hex

IO LIST
    Xa at 1
    Yno at 16
    Yok at 37
END

PROGRAM
RUNG
    COMMENT Test integer variable manipulations: move, arithmetic, comparison. Also test\r\na 50 ms cycle time, and the ATmega162 (odd timer register locations).
END
RUNG
    PARALLEL
        SERIES
            CONTACTS Xa 0
            MOVE a 1323
        END
        SERIES
            CONTACTS Xa 1
            MOVE a -12
        END
    END
END
RUNG
    PARALLEL
        ADD c a 1000
        SUB d a 5
        MUL e a 3
        DIV f e 3
    END
END
RUNG
    PARALLEL
        EQU a 1323
        SHORT
    END
    PARALLEL
        LES c 0
        LEQ d 1318
        SHORT
    END
    SHORT
    PARALLEL
        GRT c 20000
        GEQ e 3969
        OPEN
    END
    COIL Yok 0 0 0
END
RUNG
    NEQ e -36
    GRT a 0
    TON Ton 1000000
    COIL Yno 0 0 0
END
