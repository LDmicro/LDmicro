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
#ifndef __DISPLAY_H
#define __DISPLAY_H

extern int32_t char7seg[129];
extern int32_t char9seg[129]; 
extern int32_t char14seg[129];
extern int32_t char16seg[129];

#define LEN7SEG 129
#define LEN9SEG 129
#define LEN14SEG 129
#define LEN16SEG 129

#define DEGREE_CHAR 0xB0
#define DEGREE7 (LEN7SEG - 1)
#define DEGREE9 (LEN9SEG - 1)
#define DEGREE14 (LEN14SEG - 1)
#define DEGREE16 (LEN16SEG - 1)

#endif

