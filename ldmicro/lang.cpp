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
#include <string>
#include <vector>
#include <algorithm>
#include "stdafx.h"
#include "ldmicro.h"

typedef struct LangTableTag {
    char *from;
    char *to;
} LangTable;

typedef struct LangTag {
    LangTable   *tab;
    int          n;
} Lang;

std::wstring to_utf16(const char* s);
std::string to_utf8(const wchar_t* w);

std::wstring to_utf16(const char* s)
{
#if defined(_WIN32)
        const int size = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
        std::wstring output;
        output.resize(size - 1);
        if(output.size() != 0)
            MultiByteToWideChar(CP_UTF8, 0, s, -1, &output[0], size - 1);
        return output;
#else
#error "Function not realised for this platform!";
#endif
}

std::string to_utf8(const wchar_t* w)
{
#if defined(_WIN32)
    const int size = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
    std::string output;
    output.resize(size - 1);
    if(output.size() != 0)
        WideCharToMultiByte(CP_UTF8, 0, w, -1, &output[0], size - 1, nullptr, nullptr);
    return output;
#else
    #error "Function not realised for this platform!";
#endif
}

// These are the actual translation tables, so should be included in just
// one place.
#include "obj/lang-tables.h"

const wchar_t *_(const char *in)
{
    Lang *l = nullptr;

#if defined(LDLANG_EN)

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
#elif defined(LDLANG_JA)
    l = &LangJa;
#elif defined(LDLANG_RU)
    l = &LangRu;
#else
#   error "Unrecognized language!"
#endif
    std::wstring output;
    const char* s = nullptr;
    if(l){
        for(int i = 0; i < l->n; i++) {
            if(strcmp(in, l->tab[i].from)==0) {
                s = l->tab[i].to;
            }
        }
    }

    if( !s )
        s = in;

    const int size = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
    output.resize(size - 1);
    if(output.size() != 0)
        MultiByteToWideChar(CP_UTF8, 0, s, -1, &output[0], size - 1);

    if(strcmp(in, "Tcycle=%.6g %ss F=%.6g %sHz F/2=%.6g %sHz Ncycle=%d T=%.6g %ss")==0)
        {
            volatile int i = 0;
            (void)i;
        }

    static std::vector<std::wstring> strings;
    const wchar_t* out;
    auto str = std::find(std::begin(strings), std::end(strings), output);
    if(str == std::end(strings))
        {
            strings.push_back(output);
            out = strings.back().c_str();
        }
    else
        {
            out = str->c_str();
        }

    return out;
}
