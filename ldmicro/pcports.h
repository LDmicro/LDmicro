//-----------------------------------------------------------------------------
// Copyright 2015 Nehrutsa Ihor
//
// This file is part of LDmicro.
//
// LDmicro is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// LDmicro is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with LDmicro.  If not, see <http://www.gnu.org/licenses/>.
//------
//
//-----------------------------------------------------------------------------
#ifndef __PCPORTS_H
#define __PCPORTS_H
//LPT =======================================================================
// �������� ��������� �����
#define LPT_DATA_REG 0    // ������� ������
#define LPT_STATUS_REG 1  // ������� ���������
#define LPT_STATE_REG 1   // ������� ���������
#define LPT_CONTROL_REG 2 // ������� ����������
#define LPT_EPP_ADDRESS 3 // ������� ������ EPP
#define LPT_EPP_DATA 4    // ������� ������ EPP
//         //PIN# DB25
#define PAR_DATA0 0x01
#define PAR_DATA1 0x02
#define PAR_DATA2 0x04
#define PAR_DATA3 0x08
#define PAR_DATA4 0x10
#define PAR_DATA5 0x20
#define PAR_DATA6 0x40
#define PAR_DATA7 0x80

#define PIN_DATA0 2
#define PIN_DATA1 3
#define PIN_DATA2 4
#define PIN_DATA3 5
#define PIN_DATA4 6
#define PIN_DATA5 7
#define PIN_DATA6 8
#define PIN_DATA7 9

#define PAR_STATUS_NOT_ERROR 0x08
#define PAR_STATUS_SLCT 0x10
#define PAR_STATUS_PE 0x20
#define PAR_STATUS_NOT_ACK 0x40
#define PAR_STATUS_NOT_BUSY 0x80
//                    //PIN# DB25
#define PIN_STATUS_NOT_ERROR 15
#define PIN_STATUS_SLCT 13
#define PIN_STATUS_PE 12
#define PIN_STATUS_NOT_ACK 10
#define PIN_STATUS_NOT_BUSY 11

#define shrERROR 3 //not error on device
#define shrSLCT 4  //device is selected (on-line)
#define shrPE 5    //paper empty
#define shrACK 6   //not acknowledge(data transfer was not ok)
#define shrBUSY 7  //operation in progress

#define PAR_CONTROL_STROBE 0x01
#define PAR_CONTROL_AUTOFD 0x02
#define PAR_CONTROL_NOT_INIT 0x04
#define PAR_CONTROL_SLIN 0x08
#define PAR_CONTROL_IRQ_ENB 0x10 //1 enable interrupts, 0 disable
#define PAR_CONTROL_DIR 0x20     //direction: 1   read, 0   write
#define LPT_DATA_DIRECTION PAR_CONTROL_DIR
#define PAR_CONTROL_WR_CONTROL 0xC0 //the 2 highest bits of the control register must be 1
         //PIN 18..25   GROUND
//                      PIN# DB25
#define PIN_CONTROL_STROBE 1
#define PIN_CONTROL_AUTOFD 14
#define PIN_CONTROL_NOT_INIT 16
#define PIN_CONTROL_SLIN 17

#define shrSTROBE 0 //to read or write data
#define shrAUTOFD 1 //to autofeed continuous form paper
#define shrINIT 2   //begin an initialization routine
#define shrSLIN 3   //to select the device

//COM =======================================================================
#define LCR 3 //LCR - Line Control Register
//���� BaseCOM+LCR - ������� ���������� ������  //�� ����
#define TD 0x40 //������������ ������
#define DB25TD 2
#define DB9TD 3
//
#define DB25RD 3
#define DB9RD 2
//
#define MCR 4 //MCR - Modem Control Register
//���� BaseCOM+MCR - ������� ���������� �������  //�� ����
#define DTR 0x01 //���������� ���� � ������
#define DB25DTR 20
#define DB9DTR 4
//
#define RTS 0x02 //������ �� ��������
#define DB25RTS 4
#define DB9RTS 7
//
#define DB25GND 7
#define DB9GND 5
//
#define MSR 6 //MSR - Modem Status Register
//���� BaseCOM+MSR - ������� ��������� ������  //� ����
#define CTS 0x10 //���-��� ������ � ��������
#define DB25CTS 5
#define DB9CTS 8
//
#define DSR 0x20 //�����-��� ������ � ������
#define DB25DSR 6
#define DB9DSR 6
//
#define RI 0x40 //��������� ������
#define DB25RI 22
#define DB9RI 9
//
#define DCD 0x80 //����� ������� �����������
#define DB25DCD 8
#define DB9DCD 1
//
int  MaskToBit(int Mask);
void LptDb25PinToRegMask(int Pin, int *Reg, int *Mask);
void ComDb9PinToRegMask(int Pin, int *Reg, int *Mask);
void ComDb25PinToRegMask(int Pin, int *Reg, int *Mask);
bool SavePcPorts(const char *filename);
bool LoadPcPorts(const char *filename);

#endif
