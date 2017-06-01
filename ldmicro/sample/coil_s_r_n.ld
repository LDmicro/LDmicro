LDmicro0.1
CYCLE=10000 us
CRYSTAL=16000000 Hz
BAUD=9600 Hz
COMPILED=D:\xLogic\Projects\RUBKA\coil_s_r.hex

VAR LIST
END

IO LIST
  Xnew3 at 0
  Xnew2 at 0
  Xnew1 at 0
  Ynew at 0
END

PROGRAM
RUNG
  COMMENT This SAMPLE\\coil_s_r_n.ld file is used to discuss how display the status of the coil in simulation mode.\r\nFind ELEM_COIL code in intcode.cpp for details.
END
RUNG
  CONTACTS Xnew1 0
  PARALLEL
    COIL Rnew 0 0 0
    COIL Rnew 1 0 0
  END
END
RUNG
  CONTACTS Xnew2 0
  COIL Rnew 0 1 0
END
RUNG
  CONTACTS Xnew3 0
  COIL Rnew 0 0 1
END
RUNG
  CONTACTS Rnew 0
  COIL Ynew 0 0 0
END
