//-----------------------------------------------------------------------------
// Copyright 2007 Jonathan Westhues
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
// Multiple language support. For every non-English language, we have a 
// table that maps the English strings to the translated strings. An
// internationalized version of the program will attempt to translate any
// string that is passed, using the appropriate (selected by a #define)
// table. If it fails then it just returns the English.
// Jonathan Westhues, Apr 2007
//-----------------------------------------------------------------------------
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "ldmicro.h"

typedef struct LangTableTag {
    char *from;
    char *to;
} LangTable;

typedef struct LangTag {
    LangTable   *tab;
    int          n;
} Lang;

// These are the actual translation tables, so should be included in just
// one place.
#include "obj/lang-tables.h"

char *_(char *in)
{
    Lang *l;

#if defined(LDLANG_EN)
    return in;
#elif defined(LDLANG_DE)
    l = &LangDe;
#elif defined(LDLANG_FR)
    l = &LangFr;
#elif defined(LDLANG_ES)
    l = &LangEs;
#elif defined(LDLANG_IT)
    l = &LangIt;
#elif defined(LDLANG_TR)
    l = &LangTr;
#elif defined(LDLANG_PT)
    l = &LangPt;
#else
#   error "Unrecognized language!"
#endif

    int i;

    for(i = 0; i < l->n; i++) {
        if(strcmp(in, l->tab[i].from)==0) {
            return l->tab[i].to;
        }
    }

    return in;
}
