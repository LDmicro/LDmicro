LDmicro0.1
MICRO=Atmel AVR ATmega328 32-Pin packages
CYCLE=1000 us at Timer0, YPlcCycleDuty:1
CRYSTAL=16000000 Hz
BAUD=9600 Hz
COMPILED=D:\LDs\SWATCH1\swatch1.hex

IO LIST
    X_StartStopContinue at 2 0 0
    X_StopZero at 1 0 0
    YPlcCycleDuty at 16 0 0
    Yp5 at 17 0 0
END

PROGRAM
RUNG
  COMMENT Sample program: Stopwatch over UART.\r\nCState: 0-stopwatch is stopped, 1-stopwatch is running.
END
RUNG
  COMMENT \r\nInitialization is performed once ('Formatted str over UART' has OSR on input inside itself).
END
RUNG
  FORMATTED_STRING (none) 45 92 102 92 114 92 110 83 116 111 112 119 97 116 99 104 32 111 118 101 114 32 85 65 82 84 46 92 114 92 110 104 104 58 109 109 58 115 115 46 115 49 48 48 92 110
  OSF
  COIL RInited 0 1 0
END
RUNG
  COMMENT Checking the input switches on a board.\r\nT1 and T2 prevent contact bounce of switches. If not, then increase T1, T2 (100ms max value).
END
RUNG
  CONTACTS X_StartStopContinue 1
  TOF T1 25000
  OSF
  COIL RStartStopContinue 0 1 0
END
RUNG
  CONTACTS X_StopZero 1
  TOF T2 25000
  OSF
  COIL RStopZero 0 1 0
END
RUNG
  COMMENT Checking the UART input.\r\n'1' is equivalent to XStartStopContinue. '0' is equivalent to StopZero.
END
RUNG
  UART_RECV char
  PARALLEL
    SERIES
      EQU char '1'
      COIL RStartStopContinue 0 1 0
    END
    SERIES
      EQU char '0'
      COIL RStopZero 0 1 0
    END
  END
END
RUNG
  COMMENT Processing the logical state of stopwatch.\r\nTo start stopwatch press XStartStopContinue on the board or send '1' to bord via UART.
END
RUNG
  COMMENT To stop stopwatch press StopZero or send '0' via UART.\r\nTo continue press XStartStopContinue or send '1' via UART.
END
RUNG
  COMMENT If stopwatch is stopped and StopZero is pressed or '0' detected thrue UART, \r\nthen stopwatch resetting to 0.
END
RUNG
  CONTACTS RStartStopContinue 0
  PARALLEL
    CTC CState 1 0
    COIL RStartStopContinue 0 0 1
  END
END
RUNG
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
RUNG
  COMMENT hh:mm:ss.s100\r\nhours:minutes:seconds.hundredths of second
END
RUNG
  MOVE CState 1
END
RUNG
  EQU CState 1
  RTO T100_Hz 10000
  PARALLEL
    SERIES
      CTC Cs100 99 0
      CTC Css 59 0
      CTC Cmm 59 0
      CTC Chh 23 0
    END
    RES T100_Hz
  END
END
RUNG
  COMMENT Sending the stopwatch via UART, step by step, one char per PLC cycle.\r\nDon't need a high update rate, the human eye perceives 24 FPS. Precise stopwatch data will be displayed in the next cycle of TdisplayRefresh after stopping the stopwatch.
END
RUNG
  CONTACTS RInited 0
  RTO TdisplayRefresh 10000
  PARALLEL
    SERIES
      FORMATTED_STRING Chh 5 92 114 92 50 58
      OSF
      FORMATTED_STRING Cmm 3 92 50 58
      OSF
      FORMATTED_STRING Css 3 92 50 46
      OSF
      FORMATTED_STRING Cs100 2 92 50
      OSF
      COIL RAllDisplayed 0 0 0
    END
    SERIES
      CONTACTS RAllDisplayed 0
      RES TdisplayRefresh
    END
  END
END
RUNG
  LES Cs100 50
  COIL Yp5 0 0 0
END
RUNG
  COMMENT by Ihor Nehrutsa, LDmicro.GitHub@gmail.com, February 2016
END
