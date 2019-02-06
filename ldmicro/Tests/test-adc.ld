LDmicro0.2
MICRO=Atmel AVR ATmega16 40-PDIP
CYCLE=10000 us at Timer1, YPlcCycleDuty:0, ConfigurationWord(s):0x0
CRYSTAL=10000000 Hz
BAUD=2400 Hz, RATE=0 Hz
COMPILED=D:\Programmation\Ladder\Programmes\Tests\AvrGcc\AvrStudio\Src\test-adc.c

VAR LIST
END

IO LIST
    Yled1 at 18 0 0
    Yled2 at 19 0 0
    Ain at 40 0 0
END

PROGRAM
RUNG 1
  COMMENT Test de l'ADC (en mode 10 bits par defaut)\r\nSelon que la valeur lue est superieure a 500 ou pas on allume une des leds
END
RUNG 2
  PARALLEL
    READ_ADC Ain 0
    SERIES
      LEQ Ain 500
      COIL Yled1 0 0 0 0
    END
    SERIES
      GEQ Ain 501
      COIL Yled2 0 0 0 0
    END
  END
END
