LDmicro0.1
CYCLE=10000 us at Timer1, YPlcCycleDuty:0
CRYSTAL=16000000 Hz
BAUD=9600 Hz

IO LIST
    Xswitch at 0 0 0
END

PROGRAM
RUNG
  CONTACTS Xswitch 0 0
  COIL Rswitch 0 0 0 0
END
RUNG
  COMMENT --add comment here--
END
RUNG
  CONTACTS Rswitch 0 0
  MASTER_RELAY
END
RUNG
  ADD countA countA 1
END
RUNG
  MASTER_RELAY
END
RUNG
  COMMENT --add comment here--
END
RUNG
  ADD between between 1
END
RUNG
  COMMENT --add comment here--
END
RUNG
  CONTACTS Rswitch 1 0
  MASTER_RELAY
END
RUNG
  ADD countB countB 1
END
RUNG
  MASTER_RELAY
END
