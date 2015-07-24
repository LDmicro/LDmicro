LDmicro0.1
MICRO=Atmel AVR ATmega128 64-TQFP
CYCLE=10000
CRYSTAL=4000000
BAUD=2400
COMPILED=C:\depot\ldmicro\reg\expected\avr-hw.hex

IO LIST
    Xinc at 2
    Aanalog at 61
END

PROGRAM
RUNG
    COMMENT The more hardware-oriented ops (PWM, ADC, UART, EEPROM), test for the\r\nAVRs.
END
RUNG
    PERSIST saved
END
RUNG
    READ_ADC Aanalog
END
RUNG
    PARALLEL
        CONTACTS Xinc 0
        GEQ Aanalog 512
    END
    OSR
    ADD saved saved 1
END
RUNG
    PARALLEL
        SERIES
            LES saved 10
            MOVE duty 20
        END
        SERIES
            GEQ saved 10
            MOVE duty 50
        END
        SET_PWM duty 2000
    END
END
RUNG
    TON Tfmtd 1000000
    OSR
    FORMATTED_STRING saved 15 115 97 118 101 100 32 61 32 92 45 53 92 114 92 110
END
RUNG
    MOVE char 'x'
END
RUNG
    TON Tutx 2000000
    OSR
    UART_SEND char
END
RUNG
    UART_RECV inchar
    EQU inchar 'a'
    MOVE saved 0
END
