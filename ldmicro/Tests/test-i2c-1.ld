LDmicro0.2
MICRO=St ARM STM32F40X 144-LQFP
CYCLE=20000 us at Timer3, YPlcCycleDuty:0, ConfigurationWord(s):0x0
CRYSTAL=100000000 Hz
BAUD=9600 Hz, RATE=100000 Hz, SPEED=100000 Hz
COMPILED=D:\Programmation\Ladder\Programmes\Tests\ArmGcc\Src\i2c-expander.c

VAR LIST
    2 bytes cycle
END

IO LIST
    I2C1_SCL at 139 0 0
    I2C1_SDA at 140 0 0
END

PROGRAM
RUNG 1
  ADD cycle cycle 1
END
RUNG 2
  GRT cycle 255
  MOVE cycle 0
END
RUNG 3
  SHORT
  SHORT
  I2C_WR I2C1 cycle - Master 0x20 0 MSB 100000
END
