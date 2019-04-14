//-----------------------------------------------------------------------------
// Copyright 2015 Ihor Nehrutsa
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
//-----------------------------------------------------------------------------
#include "stdafx.h"

// clang-format off

//---------------------------------------------------------------------------
/* 7 segments display

     a
    --
 f|    |b
  |  g |
    --
 e|    |c
  |    |
    --
     d  DP

SEGMENT   DP g f e d c b a
BIT        7 6 5 4 3 2 1 0
nibbles   ^       ^       ^
*/
int32_t char7seg[] = { // unsigned char in fact, but we need a SDWORD
//ASCII characters from 0x00 to 0x7F
//     0    1    2    3    4    5    6    7    8    9    A    b    C    d    E    F
    0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F,0x77,0x7C,0x39,0x5E,0x79,0x71,
//     0.   1.   2.   3.   4.   5.   6.   7.   8.   9.   A.   b.   C.   d.   E.   F.
    0xBF,0x86,0xDB,0xCF,0xE6,0xED,0xFD,0x87,0xFF,0xEF,0xF7,0xFC,0xB9,0xDE,0xF9,0xF1,
// space    !    "    #    $    %    &    '    (    )    *    +    ,    -    .    /
    0x00,0xB0,0x22,0x4E,0x6D,0xD2,0xDA,0x20,0x39,0x0F,0x72,0x70,0x0C,0x40,0x80,0x52,
//     0    1    2    3    4    5    6    7    8    9    :    ;    <    =    >    ?
    0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F,0x09,0x0D,0x61,0x41,0x43,0xD3,
//     @    A    b    C    d    E    F    G    h    I    J    K    L    M    n    o
    0x9F,0x77,0x7C,0x39,0x5E,0x79,0x71,0x3D,0x74,0x30,0x1E,0x75,0x38,0x55,0x54,0x5C,
//     P    q    R    S    t    U    v    W    X    Y    Z    [    \    ]    ^    _
    0x73,0x67,0x33,0x6D,0x78,0x3E,0x1C,0x6A,0x76,0x6E,0x5B,0x39,0x64,0x0F,0x23,0x08,
//     `    a    b    c    d    e    F    g    h    i    j    k    l    m    n    o
    0x20,0x5F,0x7C,0x58,0x5E,0x7B,0x71,0x6F,0x74,0x10,0x0E,0x75,0x18,0x55,0x54,0x5C,
//     p    q    r    S    t    U    v    w    X    Y    Z    {    |    }    ~    
    0x73,0x67,0x50,0x6D,0x78,0x3E,0x1C,0x6A,0x76,0x6E,0x5B,0x39,0x30,0x0F,0x01,0x00,
//Special cases characters
// degree
    0x63
};
#define LEN7SEG_ (sizeof(char7seg) / sizeof(char7seg[0]))
//---------------------------------------------------------------------------
/* 9 segments display

     a
    ---j
 f|   / |b
  |  /  |
    ---g
 e|  /  |c
  | /   |
   m---
     d   DP

SEGMENT   0 0 m j DP g f e d c b a
BIT           9 8  7 6 5 4 3 2 1 0
nibbles  ^        ^       ^       ^
*/
//---------------------------------------------------------------------------
/* 14 segments display

     a
  h --- j
 f|\ |i/|b
  | \|/ |
 g1-- --g2
 e| /|\ |c
  |/ |l\|
  m --- k
     d   DP

SEGMENT  0 g2  l  k  i  h  m  j DP g1  f  e  d  c  b  a
BIT        14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
nibbles ^          ^           ^           ^           ^
*/
//---------------------------------------------------------------------------
/* 16 segments display

   a1 a2
  h-- --j
 f|\ |i/|b
  | \|/ |
 g1-- --g2
 e| /|\ |c
  |/ |l\|
  m-- --k
   d1 d2 DP

SEGMENT   0  0  0 a2 d2 g2  l  k  i  h  m  j DP g1  f  e d1  c  b a1
BIT               16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
nibbles  ^          ^           ^           ^           ^           ^
*/
//---------------------------------------------------------------------------

// clang-format on
