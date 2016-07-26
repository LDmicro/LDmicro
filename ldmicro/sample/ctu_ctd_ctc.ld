LDmicro0.1
CYCLE=10000 us at Timer1, YPlcCycleDuty:0
CRYSTAL=4000000 Hz
BAUD=2400 Hz

PROGRAM
RUNG
  CONTACTS Rnew1 1
  COIL Rnew1 0 0 0
END
RUNG
  PARALLEL
    MOVE var5 5
    MOVE var_5 -5
    MOVE var0 0
    MOVE var10 10
    MOVE var_10 -10
  END
END
RUNG
  CONTACTS Rnew1 0
  PARALLEL
    SERIES
      CTU Cnew1 5 0
      COIL Ynew1 0 0 0
    END
    SERIES
      CTU Cnew1_ var5 0
      COIL Ynew1_ 0 0 0
    END
    SERIES
      CTU Cnew1__ 10 5
      COIL Ynew1__ 0 0 0
    END
    SERIES
      CTU Cnew1___ var10 var5
      COIL Ynew1___ 0 0 0
    END
  END
END
RUNG
  CONTACTS Rnew1 0
  PARALLEL
    SERIES
      CTD Cnew2 -5 0
      COIL Ynew2 0 0 0
    END
    SERIES
      CTD Cnew2_ var_5 0
      COIL Ynew2_ 0 0 0
    END
    SERIES
      CTD Cnew2__ -10 -5
      COIL Ynew2__ 0 0 0
    END
    SERIES
      CTD Cnew2___ var_10 var_5
      COIL Ynew2___ 0 0 0
    END
  END
END
RUNG
  CONTACTS Rnew1 0
  PARALLEL
    SERIES
      CTC Cnew3 5 0
      COIL Ynew3 0 0 0
    END
    SERIES
      CTC Cnew3_ var5 var0
      COIL Ynew3_ 0 0 0
    END
    SERIES
      CTC Cnew3__ 10 5
      COIL Ynew3__ 0 0 0
    END
    SERIES
      CTC Cnew3___ var10 var5
      COIL Ynew3___ 0 0 0
    END
  END
END
RUNG
  CONTACTS Rnew1 0
  PARALLEL
    SERIES
      CTR Cnew4 -5 0
      COIL Ynew4 0 0 0
    END
    SERIES
      CTR Cnew4_ var_5 var0
      COIL Ynew4_ 0 0 0
    END
    SERIES
      CTR Cnew4__ -10 -5
      COIL Ynew4__ 0 0 0
    END
    SERIES
      CTR Cnew4___ var_10 var_5
      COIL Ynew4___ 0 0 0
    END
  END
END
