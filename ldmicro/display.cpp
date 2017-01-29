#include "ldmicro.h"
#include "display.h"
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
