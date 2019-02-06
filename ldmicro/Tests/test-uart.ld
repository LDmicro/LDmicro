LDmicro0.2
MICRO=St ARM STM32F40X 144-LQFP
CYCLE=10000 us at Timer1, YPlcCycleDuty:0, ConfigurationWord(s):0x0
CRYSTAL=100000000 Hz
BAUD=9600 Hz, RATE=0 Hz, SPEED=0 Hz

VAR LIST
    1 bytes Tnew
    2 bytes inch
    2 bytes outch
END

IO LIST
END

PROGRAM
RUNG 1
  COMMENT Test d'echo sur l'UART a 9600 bauds\r\nOn attend la reception d'un caractere, on le stocke et on le renvoie
END
RUNG 2
  TON Tnew 1000000 0
  FORMATTED_STRING (none) 7 72 101 108 108 111 32 33
END
RUNG 3
  UART_RECV inch
  COIL Rreceived 0 0 0 0
END
RUNG 4
  COMMENT L'emission reinitialise received pour eviter l'echo multiple d'un meme caractere recu
END
RUNG 5
  MOVE outch inch
END
RUNG 6
  CONTACTS Rreceived 0 0
  UART_SEND outch
END
