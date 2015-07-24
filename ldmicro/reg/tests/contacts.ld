LDmicro0.1
MICRO=Microchip PIC16F876 28-PDIP or 28-SOIC
CYCLE=10000
CRYSTAL=4000000
BAUD=2400
COMPILED=C:\depot\ldmicro\reg\expected\contacts.hex

IO LIST
    Xa at 2
    Xb at 3
    Xc at 4
    Xd at 5
    Xe at 6
    Yf at 11
    Yg at 7
    Yh at 12
END

PROGRAM
RUNG
    COMMENT All types of contacts (NO, NC), and all types of coils (normal, inverted, set,\r\nreset); also test series and parallel combinations.
END
RUNG
    CONTACTS Xa 0
    COIL Yf 0 0 0
END
RUNG
    PARALLEL
        CONTACTS Xb 0
        CONTACTS Xc 0
    END
    COIL Yg 0 1 0
END
RUNG
    CONTACTS Xd 0
    CONTACTS Xe 1
    PARALLEL
        COIL Yg 0 0 1
        COIL Yh 1 0 0
    END
END
