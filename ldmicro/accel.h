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

#include "circuit.h"

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

double Proportional(double k, double n);
double eFv(double m, double v);

//k=const ===================================================================
double tsProp(double k, double s);
double stProp(double k, double t);
double vProp(double k, double t);
double aProp(double k, double t);
double kProp(int nSize);

//v=k*t^2 ===================================================================
double ts2(double k, double s);
double st2(double k, double t);
double v2(double k, double t);
double a2(double k, double t);
double k2(int nSize);

//v=k*sqrt(t) ===============================================================
double tsSqrt2(double k, double s);
double stSqrt2(double k, double t);
double vSqrt2(double k, double t);
double aSqrt2(double k, double t);
double kSqrt2(int nSize);

//v=k*Sqrt3(t) ==============================================================
double tsSqrt3(double k, double s);
double stSqrt3(double k, double t);
double vSqrt3(double k, double t);
double aSqrt3(double k, double t);
double kSqrt3(int nSize);

//v=k*t^2 v=-k*(t-tz)^2======================================================
double tsS(double k, double s);
double stS(double k, double t);
double vS(double k, double t);
double aS(double k, double t);
double ktS(int nSize);
double ksS(int nSize);

//===========================================================================
void makeAccelTable(FILE *f, int max, int P, int nSize, ElemAccel **TT,
         char *name,
         int nN, // (1-для несимметричных кривых разгонов, 2-для cимметричных S-образных кривых разгона)
         int sFt, // (1-sFt, 0-tFs)
         int *n,
         int *Psum,
         int *shrt, // mult = 2 ^ shrt
         int *sovElement,
         fxFunc *fs, double ks,
         fxFunc *ft, double kt,
         fxFunc *fv, double kv,
         fxFunc *fa, double ka,
         fxFunc *eFv, double m);

#endif //__ACCEL_H
