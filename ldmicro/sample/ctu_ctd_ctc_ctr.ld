LDmicro0.1
MICRO=Atmel AVR ATmega8 28-PDIP
CYCLE=1000 us at Timer0, YPlcCycleDuty:1
CRYSTAL=16000000 Hz
BAUD=2400 Hz
COMPILED=D:\LDs\CTU_CTD_CTC_CTR\ctu_ctd_ctc_ctr.hex

IO LIST
    YPlcCycleDuty at 19 0 0
    Ynew0 at 18 0 0
    Ynew1 at 2 0 0
    Ynew1_ at 3 0 0
    Ynew1__ at 4 0 0
    Ynew1___ at 5 0 0
    Ynew2 at 6 0 0
    Ynew2_ at 11 0 0
    Ynew2__ at 12 0 0
    Ynew2___ at 13 0 0
    Ynew3 at 14 0 0
    Ynew3_ at 15 0 0
    Ynew3__ at 16 0 0
    Ynew3___ at 17 0 0
    Ynew4 at 23 0 0
    Ynew4_ at 24 0 0
    Ynew4__ at 25 0 0
    Ynew4___ at 26 0 0
END

PROGRAM
RUNG
  CONTACTS Ynew0 1
  COIL Ynew0 0 0 0
END
RUNG
  PARALLEL
    MOVE var4 4
    MOVE var_4 -4
    MOVE var5 5
    MOVE var_5 -5
    MOVE var0 0
    MOVE var9 9
    MOVE var_9 -9
  END
END
RUNG
  CONTACTS Ynew0 0
  PARALLEL
    SERIES
      CTU Cnew1 4 0
      PARALLEL
        COIL Ynew1 0 0 0
        RES Cnew1
      END
    END
    SERIES
      CTU Cnew1_ var4 0
      PARALLEL
        COIL Ynew1_ 0 0 0
        RES Cnew1_
      END
    END
    SERIES
      CTU Cnew1__ 9 5
      PARALLEL
        COIL Ynew1__ 0 0 0
        MOVE Cnew1__ 5
      END
    END
    SERIES
      CTU Cnew1___ var9 var5
      PARALLEL
        COIL Ynew1___ 0 0 0
        SERIES
          MOVE Cnew1___ var5
        END
      END
    END
  END
END
RUNG
  CONTACTS Ynew0 0
  PARALLEL
    SERIES
      CTD Cnew2 -4 0
      PARALLEL
        COIL Ynew2 0 0 0
        SERIES
          OSF
          RES Cnew2
        END
      END
    END
    SERIES
      CTD Cnew2_ var_4 0
      PARALLEL
        COIL Ynew2_ 0 0 0
        SERIES
          OSF
          RES Cnew2_
        END
      END
    END
    SERIES
      CTD Cnew2__ -9 -5
      PARALLEL
        COIL Ynew2__ 0 0 0
        SERIES
          OSF
          MOVE Cnew2__ -5
        END
      END
    END
    SERIES
      CTD Cnew2___ var_9 var_5
      PARALLEL
        COIL Ynew2___ 0 0 0
        SERIES
          OSF
          MOVE Cnew2___ var_5
        END
      END
    END
  END
END
RUNG
  CONTACTS Ynew0 0
  PARALLEL
    SERIES
      CTC Cnew3 4 0
      COIL Ynew3 0 0 0
    END
    SERIES
      CTC Cnew3_ var4 var0
      COIL Ynew3_ 0 0 0
    END
    SERIES
      CTC Cnew3__ 9 5
      COIL Ynew3__ 0 0 0
    END
    SERIES
      CTC Cnew3___ var9 var5
      COIL Ynew3___ 0 0 0
    END
  END
END
RUNG
  CONTACTS Ynew0 0
  PARALLEL
    SERIES
      CTR Cnew4 -4 0
      COIL Ynew4 0 0 0
    END
    SERIES
      CTR Cnew4_ var_4 var0
      COIL Ynew4_ 0 0 0
    END
    SERIES
      CTR Cnew4__ -9 -5
      COIL Ynew4__ 0 0 0
    END
    SERIES
      CTR Cnew4___ var_9 var_5
      COIL Ynew4___ 0 0 0
    END
  END
END
