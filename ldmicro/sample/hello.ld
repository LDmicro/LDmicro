LDmicro0.1
MICRO=Atmel AVR ATmega8 28-PDIP
CYCLE=10000 us at Timer1, YPlcCycleDuty:0
CRYSTAL=16000000 Hz
BAUD=9600 Hz
COMPILED=D:\LDs\HELLO\hello.hex

IO LIST
END

PROGRAM
RUNG
  COMMENT Very simple, just print 'Hello, world' over serial, over and over.
END
RUNG
  CONTACTS Rosc 0
  TON Tosc 1000000
  COIL Rosc 1 0 0
END
RUNG
  COMMENT For no particular reason, keep a counter that increments with each\r\nline that we print.
END
RUNG
  CONTACTS Rosc 0
  OSR
  ADD cnt cnt 1
END
RUNG
  CONTACTS Rosc 0
  FORMATTED_STRING cnt 21 72 101 108 108 111 44 32 119 111 114 108 100 58 32 92 45 53 92 114 92 110
END
