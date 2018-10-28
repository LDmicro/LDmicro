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
#ifndef __ACCEL_H
#define __ACCEL_H

#ifndef round
#define round(r)   ((r) < (LONG_MIN-0.5) || (r) > (LONG_MAX+0.5) ?\
    (r):\
    ((r) >= 0.0) ? ((r) + 0.5) : ((r) - 0.5))
#endif

typedef     double  fxFunc(double k, double x);
/*
    SDWORD  nSize,  // размер таблицы точек разгона/торможения
    SDWORD  n,      // количество точек разгона/торможения где dt отличных от 1
    double  dtMax,  // максимальное значение прироста dt
    SDWORD  mult,   // множитель dtMax до 128
    SDWORD  shrt;   // mult = 2 ^ shrt
*/

//typedef  ElemAccel TableAccel[]; //массив структур

//typedef  ElemAccel (*TableAccelPointer)[]; //указатель на массив структур
#endif
