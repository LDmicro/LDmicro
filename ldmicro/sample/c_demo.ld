LDmicro0.1
CYCLE=10000
CRYSTAL=16000000
BAUD=9600

IO LIST
    Xled at 32
    YTEMP_HI at 14
    YUpdate at 17
    Ycycle2 at 27
    ADCvalue at 26
END

PROGRAM
RUNG
    COMMENT Try compile as AVR hex or C for ARDUINO.\r\n
END
RUNG
    COMMENT \r\n1) You can change 'saved' in EEPROM with PERSITS via UART RECV.
END
RUNG
    PERSIST saved
END
RUNG
    OSR
    PARALLEL
        EQU saved 0
        EQU saved 0xffff
    END
    MOVE saved '1'
END
RUNG
    UART_RECV saved
END
RUNG
    COMMENT \r\n2) Press input Contact and see output Coil.
END
RUNG
    PARALLEL
        CONTACTS Ycycle2 1
        CONTACTS Xled 0
    END
    COIL Ycycle2 0 0 0
END
RUNG
    COMMENT 3) ADC input changes PWM duty on output.\r\nMemo: PWM sets only once after reset.
END
RUNG
    READ_ADC ADCvalue
END
RUNG
END
RUNG
    DIV PWMduty ADCvalue 16
END
RUNG
    SET_PWM PWMduty 1000
END
RUNG
    COMMENT \r\n4) Set output Coils and print information to UART.
END
RUNG
    CONTACTS RUpdate 1
    TON TUpdateON 500000
    TOF TUpdateOF 500000
    PARALLEL
        COIL RUpdate 0 0 0
        COIL YUpdate 0 0 0
    END
END
RUNG
    CONTACTS RUpdate 0
    OSR
    PARALLEL
        SERIES
            GEQ ADCvalue 700
            COIL YTEMP_HI 0 1 0
        END
        SERIES
            LEQ ADCvalue 650
            COIL YTEMP_HI 0 0 1
        END
        SERIES
            EQU display 0
            FORMATTED_STRING ADCvalue 11 65 68 67 118 97 108 117 101 61 92 52
            PARALLEL
                MOVE display 1
                SERIES
                    OSF
                    MOVE display 2
                END
            END
        END
    END
END
RUNG
    EQU display 2
    FORMATTED_STRING PWMduty 11 32 80 87 77 100 117 116 121 61 92 52
    OSF
    MOVE display 3
END
RUNG
    EQU display 3
    FORMATTED_STRING saved 13 32 115 97 118 101 100 61 92 52 92 114 92 110
    OSF
    MOVE display 0
END
RUNG
    COMMENT by Ihor Nehrutsa, any questions to LDmicro.GitHub@gmail.com, March 2016.\r\nTested at ATmega328p on the Arduino Nano.
END
