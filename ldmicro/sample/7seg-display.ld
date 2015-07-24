LDmicro0.1
CYCLE=5000
CRYSTAL=4000000
BAUD=2400

IO LIST
    Ycom_digit0 at 0
    Ycom_digit1 at 0
    Ycom_digit2 at 0
    Yseg_a at 0
    Yseg_b at 0
    Yseg_c at 0
    Yseg_d at 0
    Yseg_e at 0
    Yseg_f at 0
    Yseg_g at 0
END

PROGRAM
RUNG
    COMMENT Sample program: how to drive a multiplexed 7-segment LED display. This is\r\nfor a 3-digit common-cathode display but it is easy to modify.
END
RUNG
    COMMENT With a 5 ms cycle time, this will oscillate at 100 Hz, which should be\r\nokay (33 Hz refresh rate, 1/3 duty cycle).
END
RUNG
    CONTACTS Rdosc 1
    COIL Rdosc 0 0 0
END
RUNG
    CONTACTS Rdosc 0
    CTC Cdigit 2
END
RUNG
    COMMENT Use the Ycom_digitx lines to drive the base/gate of the NPN transistor/\r\nn-FET used to switch the cathode of digit x.
END
RUNG
    PARALLEL
        SERIES
            EQU Cdigit 0
            PARALLEL
                COIL Ycom_digit0 0 0 0
                MOVE digit digit0
            END
        END
        SERIES
            EQU Cdigit 1
            PARALLEL
                COIL Ycom_digit1 0 0 0
                MOVE digit digit1
            END
        END
        SERIES
            EQU Cdigit 2
            PARALLEL
                COIL Ycom_digit2 0 0 0
                MOVE digit digit2
            END
        END
    END
END
RUNG
    COMMENT You can drive the segment pins of the display directly from the GPIO pins,\r\nYseg_a to Yseg_b.
END
RUNG
    PARALLEL
        SERIES
            NEQ digit 1
            NEQ digit 4
            COIL Yseg_a 0 0 0
        END
        SERIES
            NEQ digit 5
            NEQ digit 6
            COIL Yseg_b 0 0 0
        END
        SERIES
            NEQ digit 2
            COIL Yseg_c 0 0 0
        END
        SERIES
            NEQ digit 1
            NEQ digit 4
            NEQ digit 7
            COIL Yseg_d 0 0 0
        END
        SERIES
            PARALLEL
                EQU digit 0
                EQU digit 2
                EQU digit 6
                EQU digit 8
            END
            COIL Yseg_e 0 0 0
        END
        SERIES
            NEQ digit 1
            NEQ digit 2
            NEQ digit 3
            NEQ digit 7
            COIL Yseg_f 0 0 0
        END
        SERIES
            NEQ digit 0
            NEQ digit 1
            NEQ digit 7
            COIL Yseg_g 0 0 0
        END
    END
END
RUNG
    COMMENT Fill in your program here; just set the output that you want on digit0,\r\ndigit1, and digit2.
END
RUNG
    PARALLEL
        MOVE digit0 1
        MOVE digit1 8
        MOVE digit2 5
    END
END
RUNG
    COMMENT by Jonathan Westhues, June 2005
END
