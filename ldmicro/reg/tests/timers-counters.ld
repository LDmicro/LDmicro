LDmicro0.1
MICRO=Atmel AVR ATmega128 64-TQFP
CYCLE=10000
CRYSTAL=4000000
BAUD=2400
COMPILED=C:\depot\ldmicro\reg\expected\timers-counters.ld

IO LIST
    Xdown at 2
    Xres at 3
    Xup at 4
    Ydown at 27
    Ypulse at 46
    Yup at 56
END

PROGRAM
RUNG
    COMMENT Test all the timers (TON, TOF, RTO) and counters (CTC, CTU, CTD), and the\r\nreset (RES) instruction. Also test one-shots.
END
RUNG
    CONTACTS Rosc 0
    TOF Tof 100000
    TON Ton 100000
    COIL Rosc 1 0 0
END
RUNG
    CONTACTS Rosc 0
    CONTACTS Xup 0
    CTU Ccnt 20
    RTO Trto 10000000
    COIL Yup 0 0 0
END
RUNG
    CONTACTS Rosc 0
    CONTACTS Xdown 0
    CTD Ccnt 10
    COIL Ydown 0 0 0
END
RUNG
    CONTACTS Xres 0
    PARALLEL
        OSR
        OSF
    END
    RES Trto
END
RUNG
    CONTACTS Rosc 0
    CTC Ccirc 7
END
RUNG
    EQU Ccirc 3
    OSF
    TOF Tpulse 200000
    COIL Ypulse 0 0 0
END
