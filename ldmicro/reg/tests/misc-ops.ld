LDmicro0.1
MICRO=Microchip PIC16F876 28-PDIP or 28-SOIC
CYCLE=10000
CRYSTAL=4000000
BAUD=2400
COMPILED=C:\depot\ldmicro\reg\expected\misc-ops.hex

IO LIST
    Xon at 2
    Yalways at 3
    Ain at 4
END

PROGRAM
RUNG
    COMMENT Little things: shift register, look-up table, master control relay.\r\n
END
RUNG
    CONTACTS Xon 0
    MASTER_RELAY
END
RUNG
    CONTACTS Rosc 0
    TON Ton 500000
    TOF Tof 500000
    COIL Rosc 1 0 0
END
RUNG
    CONTACTS Rosc 0
    SHIFT_REGISTER reg 7
END
RUNG
    PARALLEL
        READ_ADC Ain
        MOVE reg0 Ain
    END
END
RUNG
    PARALLEL
        SERIES
            CONTACTS Rosc 0
            CTC Ci 4
        END
        LOOK_UP_TABLE duty Ci 5 0 10 70 80 70 10
        SET_PWM duty 10000
    END
END
RUNG
    MASTER_RELAY
END
RUNG
    COIL Yalways 0 0 0
END
