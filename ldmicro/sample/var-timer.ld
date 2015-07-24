LDmicro0.1
CYCLE=10000
CRYSTAL=4000000
BAUD=2400

IO LIST
    Xin at 0
    Yout at 0
    Apot at 0
END

PROGRAM
RUNG
    COMMENT This is the actual variable-delay TOF timer. When Rdel_input goes high, Rdel_output goes high \r\nt_max cycle times later. When Rdel_input goes low, Rdel_output goes low immediately.
END
RUNG
    CONTACTS Rdel_input 0
    LEQ t_count t_max
    ADD t_count t_count 1
END
RUNG
    CONTACTS Rdel_input 1
    MOVE t_count 0
END
RUNG
    COMMENT The extra branch is to make a delay of 0 cycles work correctly.
END
RUNG
    PARALLEL
        GRT t_count t_max
        SERIES
            CONTACTS Rdel_input 0
            EQU t_max 0
        END
    END
    COIL Rdel_output 0 0 0
END
RUNG
    COMMENT \r\n
END
RUNG
    COMMENT This is just an example of how to use the timer. The voltage attached to Apot (which goes\r\nfrom 0 V to Vdd) sets the delay between when Xin goes high and when Yout goes high.
END
RUNG
    READ_ADC Apot
END
RUNG
    COMMENT This only works with a cycle time of 10 ms. If we want the delay to be between 0 s and 5 s,\r\nthen it should be 0 to 500 cycle times; the ADC goes from 0 to 1023, so (roughly) divide by 2.
END
RUNG
    DIV t_max Apot 2
END
RUNG
    COMMENT Then just hook up the input and the output of the timer to pins. Of course\r\nthat could drive more complicated logic, not just a pin on the processor.
END
RUNG
    CONTACTS Xin 0
    COIL Rdel_input 0 0 0
END
RUNG
    CONTACTS Rdel_output 0
    COIL Yout 0 0 0
END
