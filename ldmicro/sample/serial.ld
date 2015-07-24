LDmicro0.1
MICRO=Microchip PIC16F877 40-PDIP
CYCLE=10000
CRYSTAL=4000000
BAUD=2400
COMPILED=c:\temp\ser.hex

IO LIST
    Yled at 22
END

PROGRAM
RUNG
    CONTACTS Rosc 1
    TOF Tct1 250000
    TON Tct2 250000
    COIL Rosc 0 0 0
END
RUNG
    CONTACTS Rfast 1
    TOF Tct3 100000
    TON Tct4 100000
    COIL Rfast 0 0 0
END
RUNG
    CONTACTS Rosc 0
    CTC Cwhich 4
END
RUNG
    PARALLEL
        SERIES
            EQU Cwhich 0
            MOVE ch 'A'
        END
        SERIES
            EQU Cwhich 1
            MOVE ch 'b'
        END
        SERIES
            EQU Cwhich 2
            MOVE ch 'c'
        END
        SERIES
            EQU Cwhich 3
            MOVE ch 13
        END
        SERIES
            EQU Cwhich 4
            MOVE ch 10
        END
    END
END
RUNG
    CONTACTS Rosc 0
    OSF
    UART_SEND ch
END
RUNG
    UART_RECV inch
    COIL Rjustgot 0 1 0
END
RUNG
    CONTACTS Rjustgot 0
    TON Tnew 1000000
    COIL Rjustgot 0 0 1
END
RUNG
    PARALLEL
        SERIES
            CONTACTS Rjustgot 0
            CONTACTS Rfast 0
        END
        SERIES
            CONTACTS Rjustgot 1
            PARALLEL
                SERIES
                    EQU inch 'o'
                    SHORT
                END
                SERIES
                    EQU inch 'b'
                    CONTACTS Rosc 0
                END
                SERIES
                    SHORT
                    OPEN
                END
            END
        END
    END
    COIL Yled 0 0 0
END
