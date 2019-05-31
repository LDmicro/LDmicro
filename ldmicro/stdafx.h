// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#ifndef STDAFX_LDMICRO_H
#define STDAFX_LDMICRO_H

#include "targetver.h"

#include "gsl.hpp"

// #define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

// Windows Header Files:
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <richedit.h>
#include <shellapi.h>
#include <locale.h>

// C RunTime Header Files
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <malloc.h>
#include <memory.h>

//#include <tchar.h>
//#include <cstring>
#include <string>

#include <cstdint>
#include <vector>

//#include <inttypes.h>
#include <ctype.h>
#include <time.h>
#include <limits.h>
#include <math.h>

#include <signal.h>
#include <setjmp.h>
#include <errno.h>

#include <algorithm>
#include <unordered_set>

// TODO: reference additional headers your program requires here
//#include "current_function.hpp"
#include "bits.h"
#include "display.h"

#include "compilerexceptions.hpp"
#include "filetracker.hpp"

#include "lang.h"

// The library that I use to do registry stuff.
#define FREEZE_SUBKEY "LDMicro"
//#include "freeze.h"

//TODO:  some day this macros should be change to std::max / std::min
#ifndef NOMINMAX
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#endif //NOMINMAX

#endif // STDAFX_LDMICRO_H
