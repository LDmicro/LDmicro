LDmicro0.2
MICRO=Atmel AVR ATmega16 40-PDIP
CYCLE=10000 us at Timer1, YPlcCycleDuty:0, ConfigurationWord(s):0x0
CRYSTAL=8000000 Hz
BAUD=9600 Hz, RATE=100000 Hz, SPEED=100000 Hz
COMPILED=D:\Programmation\Ladder\Programmes\Tests\AvrGcc\AvrStudio\Src\i2c-lcd16x2.c

VAR LIST
    1 bytes Cbool 	Inutilisé actuellement !!!
    1 bytes Ccount 	Inutilisé actuellement !!!
    1 bytes Cdigit 	Inutilisé actuellement !!!
    2 bytes Cnew 	Inutilisé actuellement !!!
    2 bytes Cstate 	Inutilisé actuellement !!!
    2 bytes Hello 	Inutilisé actuellement !!!
    2 bytes I2C 	Inutilisé actuellement !!!
    2 bytes I2C1 	Inutilisé actuellement !!!
    2 bytes I2C2 	Inutilisé actuellement !!!
    2 bytes SPI 	Inutilisé actuellement !!!
    2 bytes SPI1 	Inutilisé actuellement !!!
    2 bytes SPI2 	Inutilisé actuellement !!!
    1 bytes T1 	Inutilisé actuellement !!!
    1 bytes T2 	Inutilisé actuellement !!!
    1 bytes Tdelai1 	Inutilisé actuellement !!!
    1 bytes Tdelai2 	Inutilisé actuellement !!!
    1 bytes Tdelay 	Inutilisé actuellement !!!
    1 bytes Tnew 	Inutilisé actuellement !!!
    1 bytes Toff 	Inutilisé actuellement !!!
    1 bytes Ton 	Inutilisé actuellement !!!
    1 bytes Tonce 	Inutilisé actuellement !!!
    2 bytes Tosc_off 	Inutilisé actuellement !!!
    2 bytes Tosc_on 	Inutilisé actuellement !!!
    2 bytes Ttempo 	Inutilisé actuellement !!!
    2 bytes alpha 	Inutilisé actuellement !!!
    2 bytes bool 	Inutilisé actuellement !!!
    2 bytes char 	Inutilisé actuellement !!!
  129 bytes char7seg 	Inutilisé actuellement !!!
    1 bytes char7seg[0] 	Inutilisé actuellement !!!
    2 bytes code 	Inutilisé actuellement !!!
    2 bytes count 	Inutilisé actuellement !!!
    2 bytes cycle 	Inutilisé actuellement !!!
    2 bytes decade 	Inutilisé actuellement !!!
    2 bytes dest 	Inutilisé actuellement !!!
    2 bytes digit 	Inutilisé actuellement !!!
    2 bytes digit0 	Inutilisé actuellement !!!
    2 bytes digit1 	Inutilisé actuellement !!!
    2 bytes duty_cycle 	Inutilisé actuellement !!!
    2 bytes hou 	Inutilisé actuellement !!!
    2 bytes i
    2 bytes inch 	Inutilisé actuellement !!!
    2 bytes index 	Inutilisé actuellement !!!
    2 bytes min 	Inutilisé actuellement !!!
    7 bytes name
    1 bytes name[0]
    2 bytes outch 	Inutilisé actuellement !!!
    2 bytes recv 	Inutilisé actuellement !!!
    2 bytes sec 	Inutilisé actuellement !!!
    2 bytes send 	Inutilisé actuellement !!!
    2 bytes src 	Inutilisé actuellement !!!
    2 bytes tab
    2 bytes totototo 	Inutilisé actuellement !!!
    2 bytes totototoffg 	Inutilisé actuellement !!!
    2 bytes units 	Inutilisé actuellement !!!
    2 bytes var 	Inutilisé actuellement !!!
    2 bytes x06 	Inutilisé actuellement !!!
    2 bytes x3F 	Inutilisé actuellement !!!
END

IO LIST
    I2C_SCL at 22 0 0
    I2C_SDA at 23 0 0
END

PROGRAM
RUNG 1
  CONTACTS Ronce 1 0
  LOOK_UP_TABLE tab i 7 1 72 101 108 108 111 32 33 name
END
RUNG 2
  CONTACTS Ronce 1 0
  I2C_WR I2C tab - Master 0x20 255 MSB 100000
  ADD i i 1
END
RUNG 3
  GRT i 6
  COIL Ronce 0 1 0 0
END
RUNG 4
  CONTACTS Ronce 0 0
  I2C_WR I2C 0xA0 - Master 0x20 255 MSB 100000
  I2C_WR I2C '0' - Master 0x20 255 MSB 100000
  COIL Rshow 1 0 0 0
END
