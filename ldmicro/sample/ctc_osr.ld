LDmicro0.1
CYCLE=1000
CRYSTAL=4000000
BAUD=2400

IO LIST
END

PROGRAM
RUNG
    COMMENT After the counter overflow CTC generate OSR at its output.\r\n
END
RUNG
    CONTACTS Rnew 0
    PARALLEL
        TON Tnew 1000000
        SERIES
            OPEN
            SHORT
        END
    END
    COIL Rnew 1 0 0
END
RUNG
    COMMENT Remove Open-Circuit element for faster simulating.
END
RUNG
    CONTACTS Rnew 0
    CTC Css 59
    CTC Cmm 59
    CTC Chh 23
    CTC Cdays 364
END
RUNG
    COMMENT Any questions to LDmicro.GitHub@gmail.com
END
