LDmicro0.1
MICRO=Atmel AVR ATmega8 28-PDIP
CYCLE=1000
CRYSTAL=16000000
BAUD=2400
COMPILED=D:\LDs\CTU_CTD_CTC\ctu_ctd_ctc.hex

IO LIST
    Ymeandr at 19
    Ynew0 at 18
    Ynew1 at 24
    Ynew2 at 25
    Ynew3 at 23
END

PROGRAM
RUNG
    OSR
    MOVE Cnew2 9
END
RUNG
    CONTACTS Ymeandr 1
    COIL Ymeandr 0 0 0
END
RUNG
    CONTACTS Ynew0 1
    COIL Ynew0 0 0 0
END
RUNG
    CONTACTS Ynew0 0
    CTU Cnew1 5
    PARALLEL
        COIL Ynew1 0 0 0
        RES Cnew1
    END
END
RUNG
    CONTACTS Ynew0 0
    CTD Cnew2 5
    PARALLEL
        COIL Ynew2 0 0 0
        SERIES
            OSF
            MOVE Cnew2 9
        END
    END
END
RUNG
    CONTACTS Ynew0 0
    CTC Cnew3 5
END
RUNG
    EQU Cnew3 5
    PARALLEL
        COIL Ynew3 0 0 0
        RES Cnew3
    END
END
