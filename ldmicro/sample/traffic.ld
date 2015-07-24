LDmicro0.1
MICRO=Atmel AVR ATmega128 64-TQFP
CYCLE=10000
CRYSTAL=4000000
COMPILED=C:\depot\ldmicro\out.hex

IO LIST
    Xbutton1 at 46
    Xbutton2 at 45
    Ygreen at 49
    Yred at 51
    Ywait at 48
    Yyellow at 50
END

PROGRAM
RUNG
    CONTACTS Rclock 1
    TOF Ton 500000
    TON Toff 500000
    COIL Rclock 0 0 0
END
RUNG
    CONTACTS Rclock 0
    CTC Ccycle 19
END
RUNG
    GEQ Ccycle 0
    LES Ccycle 8
    COIL Yred 0 0 0
END
RUNG
    GEQ Ccycle 8
    LES Ccycle 12
    COIL Yyellow 0 0 0
END
RUNG
    GEQ Ccycle 12
    LES Ccycle 20
    PARALLEL
        COIL Ygreen 0 0 0
        COIL Rwait 0 0 1
    END
END
RUNG
    PARALLEL
        CONTACTS Xbutton1 1
        CONTACTS Xbutton2 1
    END
    PARALLEL
        SERIES
            GEQ Ccycle 0
            LES Ccycle 6
            MOVE cycle 6
        END
        COIL Rwait 0 1 0
    END
END
RUNG
    CONTACTS Rwait 0
    PARALLEL
        SERIES
            GEQ Ccycle 8
            LES Ccycle 12
            CONTACTS Rclock 0
        END
        SERIES
            GEQ Ccycle 0
            LES Ccycle 8
        END
    END
    COIL Ywait 0 0 0
END
