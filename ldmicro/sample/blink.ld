LDmicro0.1
MICRO=Microchip PIC16F877 40-PDIP
CYCLE=10000
CRYSTAL=4000000
COMPILED=C:\depot\ldmicro\pic.hex

IO LIST
    Xbutton at 21
    Yled at 30
END

PROGRAM
RUNG
    CONTACTS Xbutton 1
    CTC Cwhich 4
END
RUNG
    CONTACTS Xbutton 1
    OSF
    TOF Tdblclk 500000
    COIL Rdblclk 0 0 0
END
RUNG
    CONTACTS Xbutton 1
    CONTACTS Rdblclk 0
    MOVE Cwhich 3
END
RUNG
    CONTACTS Rfast 0
    TOF Tffast 100000
    TON Tnfast 100000
    COIL Rfast 1 0 0
END
RUNG
    CONTACTS Rslow 0
    TON Tnslow 100000
    TOF Tfslow 1000000
    COIL Rslow 1 0 0
END
RUNG
    PARALLEL
        SERIES
            EQU Cwhich 0
            CONTACTS Rfast 0
        END
        SERIES
            EQU Cwhich 1
            CONTACTS Rslow 0
        END
        SERIES
            EQU Cwhich 2
            CONTACTS Rslow 1
        END
        EQU Cwhich 3
        SERIES
            EQU Cwhich 4
            OPEN
        END
    END
    COIL Yled 0 0 0
END
