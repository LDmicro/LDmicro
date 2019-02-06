LDmicro0.1
MICRO=St ARM STM32F40X 144-LQFP
CYCLE=10000 us at Timer3, YPlcCycleDuty:0, ConfigurationWord(s):0x0
CRYSTAL=100000000 Hz
BAUD=2400 Hz, RATE=0 Hz, SPEED=0 Hz
COMPILED=D:\Programmation\Ladder\Programmes\Tests\ArmGcc\Src\test-led2.c

IO LIST
    Xbutton at 141 0 0
    Yled at 77 0 0
END

PROGRAM
RUNG
  COMMENT Led alternativement allumee / clignotante / eteinte selon appuis sur le Bouton
END
RUNG
  CONTACTS Rosc 0 0
  TON Tosc_on 250000 0
  TOF Tosc_off 250000 0
  COIL Rosc 1 0 0 0
END
RUNG
  CONTACTS Xbutton 1 1
  CTC Cstate 2 0 /
END
RUNG
  PARALLEL
    EQU Cstate 1
    SERIES
      EQU Cstate 2
      CONTACTS Rosc 0 0
    END
  END
  COIL Yled 0 0 0 0
END
