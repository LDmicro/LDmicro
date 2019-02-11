LDmicro0.2
MICRO=Atmel AVR ATmega328 28-PDIP
CYCLE=10000 us at Timer1, YPlcCycleDuty:0, ConfigurationWord(s):0x0
CRYSTAL=3686400 Hz
BAUD=2400 Hz, RATE=0 Hz

VAR LIST
    1 bytes Tdelai1
    1 bytes Tdelai2
    2 bytes cycle
END

IO LIST
    Pled at 17 0 0
END

PROGRAM
RUNG 1
  COMMENT Test de la PWM (rapport cyclique variable)\r\nSelon la valeur de cycle on allume plus ou moins une led
END
RUNG 2
  EQU cycle 100
  MOVE cycle 0
END
RUNG 3
  CONTACTS Rrelai 0 0
  TOF Tdelai1 50000 -1
  TON Tdelai2 50000 -1
  COIL Rrelai 1 0 0 0
END
RUNG 4
  CONTACTS Rrelai 0 0
  OSR
  ADD cycle cycle 5
END
RUNG 5
  SET_PWM cycle 1200 Pled 0-100%
END
