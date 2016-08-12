LDmicro0.2
MICRO=Atmel AVR ATmega328 32-Pin packages
CYCLE=1000 us at Timer0, YPlcCycleDuty:0
CRYSTAL=16000000 Hz
BAUD=9600 Hz
COMPILED=D:\LDs\SWATCH2\swatch2.hex

VAR LIST
    1 bytes #PORTB 
    1 bytes #PORTC 
    1 bytes #PORTD 
    1 bytes CState 
    1 bytes Chh 
    1 bytes Cmm 
    1 bytes Cs100 
    1 bytes Css 
    1 bytes T1 
    1 bytes T100_Hz 
    1 bytes T2 
    1 bytes char7seg 	Now not used !!!
    1 bytes dest 
    1 bytes scanB 
    1 bytes scanC 
END

IO LIST
    X_StartStopContinue at 14 0 0
    X_StopZero at 15 0 0
    Y100_Hz at 16 0 0
    Yb0 at 12 0 0
    Yb1 at 13 0 0
    Yc0 at 23 0 0
    Yc1 at 24 0 0
    Yc2 at 25 0 0
    Yc3 at 26 0 0
    Yc4 at 27 0 0
    Yc5 at 28 0 0
    Yd0 at 30 0 0
    Yd1 at 31 0 0
    Yd2 at 32 0 0
    Yd3 at 1 0 0
    Yd4 at 2 0 0
    Yd5 at 9 0 0
    Yd6 at 10 0 0
    Yd7 at 11 0 0
    Yp5 at 17 0 0
END

PROGRAM
RUNG 1
  COMMENT Sample program: Stopwatch at 7 segments.\r\nCState: 0-stopwatch is stopped, 1-stopwatch is running.
END
RUNG 2
  COMMENT \r\nInit pins as outputs.
END
RUNG 3
  OSR
  PARALLEL
    COIL Yd0 0 1 0
    COIL Yd1 0 1 0
    COIL Yd2 0 1 0
    COIL Yd3 0 1 0
    COIL Yd4 0 1 0
    COIL Yd5 0 1 0
    COIL Yd6 0 1 0
    COIL Yd7 0 1 0
  END
END
RUNG 4
  OSR
  PARALLEL
    COIL Yc0 0 1 0
    COIL Yc1 0 1 0
    COIL Yc2 0 1 0
    COIL Yc3 0 1 0
    COIL Yc4 0 1 0
    COIL Yc5 0 1 0
  END
END
RUNG 5
  OSR
  PARALLEL
    COIL Yb0 0 1 0
    COIL Yb1 0 1 0
  END
END
RUNG 6
  COMMENT Checking the input switches on a board.\r\nT1 and T2 prevent contact bounce of switches. If not, then increase T1, T2 (100ms max value).
END
RUNG 7
  CONTACTS X_StartStopContinue 1
  TOF T1 25000
  OSF
  COIL RStartStopContinue 0 1 0
END
RUNG 8
  CONTACTS X_StopZero 1
  TOF T2 25000
  OSF
  COIL RStopZero 0 1 0
END
RUNG 9
  COMMENT Processing the logical state of stopwatch.\r\nTo start stopwatch press XStartStopContinue button on the board.
END
RUNG 10
  COMMENT To stop stopwatch press StopZero button.\r\nTo continue press XStartStopContinue button.
END
RUNG 11
  COMMENT If stopwatch is stopped and StopZero button is pressed, \r\nthen stopwatch resetting to 0.
END
RUNG 12
  CONTACTS RStartStopContinue 0
  PARALLEL
    CTC CState 1 0
    COIL RStartStopContinue 0 0 1
  END
END
RUNG 13
  CONTACTS RStopZero 0
  PARALLEL
    SERIES
      OSR
      EQU CState 0
      PARALLEL
        MOVE Cs100 0
        MOVE Css 0
        MOVE Cmm 0
        MOVE Chh 0
      END
    END
    MOVE CState 0
    COIL RStopZero 0 0 1
  END
END
RUNG 14
  COMMENT hh:mm:ss.s100\r\nhours:minutes:seconds.hundredths of second
END
RUNG 15
  PARALLEL
    SHORT
    EQU CState 1
  END
  RTO T100_Hz 10000
  PARALLEL
    SERIES
      CTC Cs100 99 0
      CTC Css 59 0
      CTC Cmm 59 0
      CTC Chh 23 0
    END
    RES T100_Hz
    SERIES
      OSR
      PARALLEL
        COIL Y100_Hz 0 1 0
        COIL Y100_Hz 0 0 1
      END
    END
  END
END
RUNG 16
  OSR
  MOVE scanC 0xfe
END
RUNG 17
  ROL scanC scanC 1
  SR0 scanB scanC 6
END
RUNG 18
  COMMENT \r\nhh
END
RUNG 19
  EQU scanC 0xfe
  BIN2BCD dest Chh
  SR0 dest dest 4
END
RUNG 20
  EQU scanC 0xfd
  BIN2BCD dest Chh
  AND dest dest 0x0f
END
RUNG 21
  COMMENT \r\nmm
END
RUNG 22
  EQU scanC 0xfb
  BIN2BCD dest Cmm
  SR0 dest dest 4
END
RUNG 23
  EQU scanC 0xf7
  BIN2BCD dest Cmm
  AND dest dest 0x0f
END
RUNG 24
  COMMENT ss\r\nADD for dot after ss.
END
RUNG 25
  EQU scanC 0xef
  BIN2BCD dest Css
  SR0 dest dest 4
END
RUNG 26
  EQU scanC 0xdf
  BIN2BCD dest Css
  AND dest dest 0x0f
  ADD dest dest 0x10
END
RUNG 27
  COMMENT \r\ns100
END
RUNG 28
  EQU scanC 0xbf
  BIN2BCD dest Cs100
  SR0 dest dest 4
END
RUNG 29
  EQU scanC 0x7f
  BIN2BCD dest Cs100
  AND dest dest 0x0f
END
RUNG 30
  COMMENT \r\nSegments and scan output.
END
RUNG 31
  7SEGMENTS dest dest C
  BUS #PORTD dest 0 5 7 2 1 3 4 6
  PARALLEL
    MOVE #PORTC scanC
    MOVE #PORTB scanB
  END
END
RUNG 32
  COMMENT \r\nThe colons blink every second.
END
RUNG 33
  LES Cs100 50
  COIL Yp5 0 0 0
END
RUNG 34
  COMMENT by Ihor Nehrutsa, LDmicro.GitHub@gmail.com, August 2016
END
