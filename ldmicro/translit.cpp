//-----------------------------------------------------------------------------
// Copyright 2017 Ihor Nehrutsa
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
//
//-----------------------------------------------------------------------------
#include "stdafx.h"

#include "ldmicro.h"

#define LDLANG_RU // TODO
//-----------------------------------------------------------------------------
#if defined(LDLANG_RU)
// code page 1251
void Transliterate(char *dest, const char *str)
{
    dest[0] = 0;
    for(; (*str); str++) {
        switch(str[0]) {
            case 'à': {
                dest[0] = 'a';
                dest[1] = 0;
                break;
            };
            case 'á': {
                dest[0] = 'b';
                dest[1] = 0;
                break;
            };
            case 'â': {
                dest[0] = 'v';
                dest[1] = 0;
                break;
            };
            case 'ã': {
                dest[0] = 'g';
                dest[1] = 0;
                break;
            };
            case 'ä': {
                dest[0] = 'd';
                dest[1] = 0;
                break;
            };
            case 'å': {
                dest[0] = 'e';
                dest[1] = 0;
                break;
            };
            case '´': {
                dest[0] = 'y';
                dest[1] = 'e';
                dest[2] = 0;
                dest++;
                break;
            };
            case 'æ': {
                dest[0] = 'z';
                dest[1] = 'h';
                dest[2] = 0;
                dest++;
                break;
            };
            case 'ç': {
                dest[0] = 'z';
                dest[1] = 0;
                break;
            };
            case 'è': {
                dest[0] = 'i';
                dest[1] = 0;
                break;
            };
            case 'é': {
                dest[0] = 'y';
                dest[1] = 0;
                break;
            };
            case 'ê': {
                dest[0] = 'k';
                dest[1] = 0;
                break;
            };
            case 'ë': {
                dest[0] = 'l';
                dest[1] = 0;
                break;
            };
            case 'ì': {
                dest[0] = 'm';
                dest[1] = 0;
                break;
            };
            case 'í': {
                dest[0] = 'n';
                dest[1] = 0;
                break;
            };
            case 'î': {
                dest[0] = 'o';
                dest[1] = 0;
                break;
            };
            case 'ï': {
                dest[0] = 'p';
                dest[1] = 0;
                break;
            };
            case 'ğ': {
                dest[0] = 'r';
                dest[1] = 0;
                break;
            };
            case 'ñ': {
                dest[0] = 's';
                dest[1] = 0;
                break;
            };
            case 'ò': {
                dest[0] = 't';
                dest[1] = 0;
                break;
            };
            case 'ó': {
                dest[0] = 'u';
                dest[1] = 0;
                break;
            };
            case 'ô': {
                dest[0] = 'f';
                dest[1] = 0;
                break;
            };
            case 'õ': {
                dest[0] = 'h';
                dest[1] = 0;
                break;
            };
            case 'ö': {
                dest[0] = 'c';
                dest[1] = 0;
                break;
            };
            case '÷': {
                dest[0] = 'c';
                dest[1] = 'h';
                dest[2] = 0;
                dest++;
                break;
            };
            case 'ø': {
                dest[0] = 's';
                dest[1] = 'h';
                dest[2] = 0;
                dest++;
                break;
            };
            case 'ù': {
                dest[0] = 'c';
                dest[1] = 'h';
                dest[2] = 0;
                dest++;
                break;
            };
            case 'ú': {
                dest[0] = '_';
                dest[1] = 0;
                break;
            };
            case 'û': {
                dest[0] = 'y';
                dest[1] = 0;
                break;
            };
            case 'ü': {
                dest[0] = '_';
                dest[1] = 0;
                break;
            };
            case 'ı': {
                dest[0] = 'e';
                dest[1] = 0;
                break;
            };
            case 'ş': {
                dest[0] = 'y';
                dest[1] = 'u';
                dest[2] = 0;
                dest++;
                break;
            };
            case 'ÿ': {
                dest[0] = 'y';
                dest[1] = 'a';
                dest[2] = 0;
                dest++;
                break;
            };
            case 'À': {
                dest[0] = 'A';
                dest[1] = 0;
                break;
            };
            case 'Á': {
                dest[0] = 'B';
                dest[1] = 0;
                break;
            };
            case 'Â': {
                dest[0] = 'V';
                dest[1] = 0;
                break;
            };
            case 'Ã': {
                dest[0] = 'G';
                dest[1] = 0;
                break;
            };
            case 'Ä': {
                dest[0] = 'D';
                dest[1] = 0;
                break;
            };
            case 'Å': {
                dest[0] = 'E';
                dest[1] = 0;
                break;
            };
            case '¥': {
                dest[0] = 'Y';
                dest[1] = 'e';
                dest[2] = 0;
                dest++;
                break;
            };
            case 'Æ': {
                dest[0] = 'Z';
                dest[1] = 'h';
                dest[2] = 0;
                dest++;
                break;
            };
            case 'Ç': {
                dest[0] = 'Z';
                dest[1] = 0;
                break;
            };
            case 'È': {
                dest[0] = 'I';
                dest[1] = 0;
                break;
            };
            case 'É': {
                dest[0] = 'Y';
                dest[1] = 0;
                break;
            };
            case 'Ê': {
                dest[0] = 'K';
                dest[1] = 0;
                break;
            };
            case 'Ë': {
                dest[0] = 'L';
                dest[1] = 0;
                break;
            };
            case 'Ì': {
                dest[0] = 'M';
                dest[1] = 0;
                break;
            };
            case 'Í': {
                dest[0] = 'N';
                dest[1] = 0;
                break;
            };
            case 'Î': {
                dest[0] = 'O';
                dest[1] = 0;
                break;
            };
            case 'Ï': {
                dest[0] = 'P';
                dest[1] = 0;
                break;
            };
            case 'Ğ': {
                dest[0] = 'R';
                dest[1] = 0;
                break;
            };
            case 'Ñ': {
                dest[0] = 'S';
                dest[1] = 0;
                break;
            };
            case 'Ò': {
                dest[0] = 'T';
                dest[1] = 0;
                break;
            };
            case 'Ó': {
                dest[0] = 'U';
                dest[1] = 0;
                break;
            };
            case 'Ô': {
                dest[0] = 'F';
                dest[1] = 0;
                break;
            };
            case 'Õ': {
                dest[0] = 'H';
                dest[1] = 0;
                break;
            };
            case 'Ö': {
                dest[0] = 'C';
                dest[1] = 0;
                break;
            };
            case '×': {
                dest[0] = 'C';
                dest[1] = 'h';
                dest[2] = 0;
                dest++;
                break;
            };
            case 'Ø': {
                dest[0] = 'S';
                dest[1] = 'h';
                dest[2] = 0;
                dest++;
                break;
            };
            case 'Ù': {
                dest[0] = 'C';
                dest[1] = 'h';
                dest[2] = 0;
                dest++;
                break;
            };
            case 'Ú': {
                dest[0] = '_';
                dest[1] = 0;
                break;
            };
            case 'Û': {
                dest[0] = 'Y';
                dest[1] = 0;
                break;
            };
            case 'Ü': {
                dest[0] = '_';
                dest[1] = 0;
                break;
            };
            case 'İ': {
                dest[0] = 'E';
                dest[1] = 0;
                break;
            };
            case 'Ş': {
                dest[0] = 'Y';
                dest[1] = 'u';
                dest[2] = 0;
                dest++;
                break;
            };
            case 'ß': {
                dest[0] = 'Y';
                dest[1] = 'a';
                dest[2] = 0;
                dest++;
                break;
            };
            default: {
                dest[0] = str[0];
                dest[1] = 0;
                break;
            };
        };
        dest++;
    }
}
#else
void Transliterate(char *dest, const char *str)
{
    strcpy(dest, str);
}
#endif

//-----------------------------------------------------------------------------
int TranslitFile(char *dest)
{
    char *tmp;
    char  ntmp[512];
    char  line[512];
    char  trans[1024];

    if((tmp = tmpnam(ntmp)) == nullptr) {
        Error(_("Couldn't create a unique file name for '%s'"), dest);
        return 1;
    }

    if(rename(dest, ntmp)) {
        Error(_("Couldn't rename(%s,%s)"), dest, ntmp);
        return 2;
    }

    FileTracker ftmp(ntmp, "r");
    if(!ftmp) {
        Error(_("Couldn't open file '%s'"), ntmp);
        return 3;
    }

    FileTracker f(dest, "w");
    if(!f) {
        Error(_("Couldn't open file '%s'"), dest);
        return 4;
    }

    for(;;) {
        if(!fgets(line, sizeof(line), ftmp))
            break;
        Transliterate(trans, line);
        fwrite(trans, 1, strlen(trans), f);
    };

    return 0;
}
