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

#include "ldmicro.h"
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
