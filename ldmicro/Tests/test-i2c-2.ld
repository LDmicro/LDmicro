LDmicro0.2
MICRO=St ARM STM32F40X 144-LQFP
CYCLE=4000 us at Timer1, YPlcCycleDuty:0, ConfigurationWord(s):0x0
CRYSTAL=95000000 Hz
BAUD=2400 Hz, RATE=0 Hz, SPEED=100000 Hz
COMPILED=D:\Programmation\Ladder\Programmes\Tests\ArmGcc\Src\test-i2c.c

VAR LIST
    1 bytes Ccount
    2 bytes digit
    2 bytes digit0
    2 bytes digit1
END

IO LIST
    Xbutton at 141 0 0
    Yseg_a at 56 0 0
    Yseg_b at 57 0 0
    Yseg_c at 87 0 0
    Yseg_d at 88 0 0
    Yseg_e at 89 0 0
    Yseg_f at 90 0 0
    Yseg_g at 91 0 0
    Yselect at 93 0 0
    I2C1_SCL at 139 0 0
    I2C1_SDA at 140 0 0
END

PROGRAM
RUNG 1
  COMMENT Sample program: how to drive a multiplexed 7-segment LED display. \r\nThis is for a 2-digit common-anode display
END
RUNG 2
  COMMENT With a 4 ms cycle time, it should be okay\r\n
END
RUNG 3
  CONTACTS Rdosc 1 0
  COIL Rdosc 0 0 0 0
END
RUNG 4
  CONTACTS Rdosc 0 0
  CTC Ccount 1 0 /
END
RUNG 5
  COMMENT Use Yselect to drive the base/gate of the transistor used to select display
END
RUNG 6
  EQU Ccount 0
  PARALLEL
    MOVE digit digit0
    COIL Yselect 0 0 0 0
  END
END
RUNG 7
  EQU Ccount 1
  PARALLEL
    MOVE digit digit1
    COIL Yselect 1 0 0 0
  END
END
RUNG 8
  COMMENT You can drive the segment pins of the display directly from the GPIO pins,\r\nYseg_a to Yseg_b.
END
RUNG 9
  PARALLEL
    SERIES
      NEQ digit 1
      NEQ digit 4
      COIL Yseg_a 1 0 0 0
    END
    SERIES
      NEQ digit 5
      NEQ digit 6
      COIL Yseg_b 1 0 0 0
    END
    SERIES
      NEQ digit 2
      COIL Yseg_c 1 0 0 0
    END
    SERIES
      NEQ digit 1
      NEQ digit 4
      NEQ digit 7
      COIL Yseg_d 1 0 0 0
    END
    SERIES
      PARALLEL
        EQU digit 0
        EQU digit 2
        EQU digit 6
        EQU digit 8
      END
      COIL Yseg_e 1 0 0 0
    END
    SERIES
      NEQ digit 1
      NEQ digit 2
      NEQ digit 3
      NEQ digit 7
      COIL Yseg_f 1 0 0 0
    END
    SERIES
      NEQ digit 0
      NEQ digit 1
      NEQ digit 7
      COIL Yseg_g 1 0 0 0
    END
  END
END
RUNG 10
  COMMENT Fill in your program here; just set the output that you want on digit0 and digit1\r\nConnect a DS1307 real time clock to I2C1
END
RUNG 11
  I2C_RD I2C1 - sec Master 0x68 0 MSB 100000
  BCD2BIN sec sec
END
RUNG 12
  DIV digit0 sec 10
END
RUNG 13
  MOD digit1 sec 10
END
RUNG 14
  COMMENT Push button to reset seconds
END
RUNG 15
  CONTACTS Xbutton 1 0
  I2C_WR I2C1 0 - Master 0x68 0 MSB 100000
END
