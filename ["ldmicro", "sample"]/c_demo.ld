LDmicro0.1
CYCLE=10000
CRYSTAL=16000000
BAUD=9600

IO LIST
    Xled at 32
    YTEMP_HI at 14
    YUpdate at 12
    Ycycle2 at 27
    ADCvalue at 26
END

PROGRAM
RUNG
    COMMENT Try compile as C for ARDUINO or AVR hex.\r\n1) You can change 'saved' in EEPROM with PERSITS via UART RECV.
END
RUNG
    OSR
    MOVE saved 32
END
RUNG
    PERSIST saved
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
    COMMENT \r\n3) ADC input changes PWM duty on output.
END
RUNG
    READ_ADC ADCvalue
END
RUNG
END
RUNG
    DIV PWMduty ADCvalue 8
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
            CONTACTS Rbusy 0
            FORMATTED_STRING ADCvalue 11 65 68 67 118 97 108 117 101 61 92 52
            COIL Rbusy1 0 0 0
        END
    END
END
RUNG
    CONTACTS Rbusy1 0
    OSF
    FORMATTED_STRING PWMduty 11 32 80 87 77 100 117 116 121 61 92 52
    COIL Rbusy2 0 0 0
END
RUNG
    CONTACTS Rbusy2 0
    OSF
    FORMATTED_STRING saved 13 32 115 97 118 101 100 61 92 52 92 114 92 110
    COIL Rbusy 1 0 0
END
RUNG
    COMMENT \r\nby Ihor Nehrutsa, any questions to LDmicro.GitHub@gmail.com, March 2016
END
