LDmicro0.2
MICRO=Atmel AVR ATmega328 28-PDIP
CYCLE=10000 us at Timer1, YPlcCycleDuty:0, ConfigurationWord(s):0x0
CRYSTAL=10000000 Hz
BAUD=9600 Hz, RATE=100000 Hz, SPEED=0 Hz
COMPILED=D:\Programmation\Ladder\Programmes\Tests\AvrGcc\AvrStudio\Src\test-spi.c

VAR LIST
    1 bytes Ccount 	Now not used !!!
    2 bytes Cnew 	Now not used !!!
    2 bytes Cstate 	Now not used !!!
    2 bytes Hello 	Now not used !!!
    2 bytes I2C 	Now not used !!!
    2 bytes I2C1 	Now not used !!!
    2 bytes SPI 	Now not used !!!
    2 bytes SPI1 	Now not used !!!
    2 bytes SPI2 	Now not used !!!
    1 bytes Tdelay 	Now not used !!!
    2 bytes Tnew 	Now not used !!!
    1 bytes Toff 	Now not used !!!
    1 bytes Ton 	Now not used !!!
    1 bytes Tonce 	Now not used !!!
    2 bytes Tosc_off 	Now not used !!!
    2 bytes Tosc_on 	Now not used !!!
    2 bytes char 	Now not used !!!
  129 bytes char7seg 	Now not used !!!
    1 bytes char7seg[0] 	Now not used !!!
    2 bytes code 	Now not used !!!
    2 bytes count 	Now not used !!!
    2 bytes decade 	Now not used !!!
    2 bytes dest 	Now not used !!!
    2 bytes i 	Now not used !!!
    2 bytes inch 	Now not used !!!
    2 bytes index 	Now not used !!!
    7 bytes name 	Now not used !!!
    1 bytes name[0] 	Now not used !!!
    2 bytes outch 	Now not used !!!
    2 bytes recv
    2 bytes send
    2 bytes src 	Now not used !!!
    2 bytes tab 	Now not used !!!
    2 bytes units 	Now not used !!!
    2 bytes var 	Now not used !!!
    2 bytes x06 	Now not used !!!
    2 bytes x3F 	Now not used !!!
END

IO LIST
    SPI_MOSI at 17 0 0
    SPI_MISO at 18 0 0
    SPI_SCK at 19 0 0
    SPI__SS at 16 0 0
END

PROGRAM
RUNG 1
  SPI_WR SPI Hello - Master 0 8 MSB 100000
  MOVE send '1'
END
RUNG 2
  SPI SPI send recv Master 0b11 8 MSB_FIRST 100000
  MOVE send '0'
END
RUNG 3
  SPI SPI send recv Master 0 8 MSB 100000
END
