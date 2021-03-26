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
// A ladder logic compiler for 8 bit micros: user draws a ladder diagram,
// with an appropriately constrained `schematic editor,' and then we can
// simulated it under Windows or generate PIC/AVR code that performs the
// requested operations. This files contains the program entry point, plus
// most of the UI logic relating to the main window.
// Jonathan Westhues, Oct 2004
//-----------------------------------------------------------------------------
#include "stdafx.h"

#include "ldmicro.h"
#include "freeze.h"
#include "intcode.h"
#include "pcports.h"
#include "accel.h"
#include "display.h"
#include "flash.h"

#include "ldversion.h"
#include <ldlog.hpp>
#include <string.h>

HINSTANCE Instance;
HWND      MainWindow;
HDC       Hdc;

// parameters used to capture the mouse when implementing our totally non-
// general splitter control
static HHOOK MouseHookHandle;
static int   MouseY;

// For the open/save dialog boxes
#define LDMICRO_PATTERN                            \
    "LDmicro Ladder Logic Programs (*.ld)\0*.ld\0" \
    "All files\0*\0\0"

bool ProgramChangedNotSaved = false;

ULONGLONG PrevWriteTime = 0;
ULONGLONG LastWriteTime = 0;

#define HEX_PATTERN "Intel Hex Files (*.hex)\0*.hex\0All files\0*\0\0"
#define C_PATTERN "C Source Files (*.c)\0*.c\0All Files\0*\0\0"
#define INTERPRETED_PATTERN "Interpretable Byte Code Files (*.int)\0*.int\0All Files\0*\0\0"
#define PASCAL_PATTERN "PASCAL Source Files (*.pas)\0*.pas\0All Files\0*\0\0"
#define ARDUINO_C_PATTERN "ARDUINO C Source Files (*.cpp)\0*.cpp\0All Files\0*\0\0"
#define XINT_PATTERN "Extended Byte Code Files (*.xint)\0*.xint\0All Files\0*\0\0"

char ExePath[MAX_PATH];
char CurrentSaveFile[MAX_PATH]; // .ld
char CurrentLdPath[MAX_PATH];
char CurrentCompileFile[MAX_PATH]; //.hex, .asm, ...
char CurrentCompilePath[MAX_PATH];

#define TXT_PATTERN "Text Files (*.txt)\0*.txt\0All files\0*\0\0"

// Everything relating to the PLC's program, I/O configuration, processor
// choice, and so on--basically everything that would be saved in the
// project file.
PlcProgram Prog;
int        compile_MNU = -1;

//-----------------------------------------------------------------------------
// Get a filename with a common dialog box and then save the program to that
// file and then set our default filename to that.
//-----------------------------------------------------------------------------
static bool SaveAsDialog()
{
    OPENFILENAME ofn;

    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hInstance = Instance;
    ofn.lpstrFilter = LDMICRO_PATTERN;
    ofn.lpstrDefExt = "ld";
    ofn.lpstrFile = CurrentSaveFile;
    ofn.nMaxFile = sizeof(CurrentSaveFile);
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

    if(!GetSaveFileName(&ofn))
        return false;

    if(!SaveProjectToFile(CurrentSaveFile, MNU_SAVE)) {
        Error(_("Couldn't write to '%s'."), CurrentSaveFile);
        return false;
    } else {
        ProgramChangedNotSaved = false;
        RefreshControlsToSettings();
        strcpy(CurrentCompileFile, "");
        return true;
    }
}

//---------------------------------------------------------------------------
char *ExtractFileDir(char *dest) // without last backslash
{
    if(strlen(dest)) {
        auto c = strrchr(dest, '\\');
        if(c)
            *c = '\0';
    };
    return dest;
}

char *ExtractFilePath(char *dest) // with last backslash
{
    if(strlen(dest)) {
        auto c = strrchr(dest, '\\');
        if(c)
            c[1] = '\0';
    };
    return dest;
}

//---------------------------------------------------------------------------
const char *ExtractFileName(const char *src) // with .ext
{
    const char *c;
    if(strlen(src)) {
        c = strrchr(src, '\\');
        if(c)
            return &c[1];
    }
    return src;
}

//---------------------------------------------------------------------------
char *GetFileName(char *dest, const char *src) // without .ext
{
    dest[0] = '\0';
    strcpy(dest, ExtractFileName(src));
    if(strlen(dest)) {
        auto c = strrchr(dest, '.');
        if(c)
            c[0] = '\0';
    }
    return dest;
}

//-----------------------------------------------------------------------------
char *SetExt(char *dest, const char *src, const char *ext)
{
    if(dest != src)
        if(strlen(src))
            strcpy(dest, src);
    if(strlen(dest)) {
        auto c = strrchr(dest, '.');
        if(c)
            c[0] = '\0';
    };
    if(!strlen(dest))
        strcat(dest, "new");

    if(strlen(ext))
        if(!strchr(ext, '.'))
            strcat(dest, ".");

    return strcat(dest, ext);
}

//-----------------------------------------------------------------------------
// Get a filename with a common dialog box and then export the program as
// an ASCII art drawing.
//-----------------------------------------------------------------------------
static bool ExportDialog()
{
    char         exportFile[MAX_PATH];
    OPENFILENAME ofn;

    exportFile[0] = '\0';
    SetExt(exportFile, CurrentSaveFile, "txt");

    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hInstance = Instance;
    ofn.lpstrFilter = TXT_PATTERN;
    ofn.lpstrFile = exportFile;
    ofn.lpstrTitle = _("Export As Text");
    ofn.nMaxFile = sizeof(exportFile);
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

    if(!GetSaveFileName(&ofn))
        return false;

    ExportDrawingAsText(exportFile);
    return true;
}

//-----------------------------------------------------------------------------
// If we already have a filename, save the program to that. Otherwise same
// as Save As. Returns true if it worked, else returns false.
//-----------------------------------------------------------------------------
static bool SaveProgram(int code)
{
    if(strlen(CurrentSaveFile)) {
        if(!SaveProjectToFile(CurrentSaveFile, code)) {
            Error(_("Couldn't write to '%s'."), CurrentSaveFile);
            return false;
        } else {
            ProgramChangedNotSaved = false;
            RefreshControlsToSettings();
            return true;
        }
    } else {
        return SaveAsDialog();
    }
}

//-----------------------------------------------------------------------------
bool ExistFile(const char *name)
{
    if(FILE *file = fopen(name, "r")) {
        fclose(file);
        return true;
    }
    return false;
}
/*
bool ExistFile2(const char *name)
{
    return GetFileAttributes(name) != INVALID_FILE_ATTRIBUTES;
}

bool ExistFile3(const char *name)
{
    #if defined(_WIN32) || defined(_WIN64)
    return name && PathFileExists(name);
    #else
    struct stat sb;
    return name && 0 == stat (name, &sb);
    #endif
}

bool exists_test0 (const char *name) {
    ifstream f(name);
    return f.good();
}

bool exists_test2 (const char *name) {
    return ( access( name, F_OK ) != -1 );
}

bool exists_test3 (const char *name) {
  struct stat buffer;
  return (stat (name, &buffer) == 0);
}
*/
//-----------------------------------------------------------------------------
long int fsize(FILE *fp)
{
    long int prev = ftell(fp);
    fseek(fp, 0L, SEEK_END);
    long int sz = ftell(fp);
    fseek(fp, prev, SEEK_SET); //go back to where we were
    return sz;
}

long int fsize(char *filename)
{
    FILE *fp;
    fp = fopen(filename, "rb");
    if(fp == nullptr) {
        return 0;
    }
    fseek(fp, 0L, SEEK_END);
    long int sz = ftell(fp);
    fclose(fp);
    return sz;
}

//-----------------------------------------------------------------------------
void GetErrorMessage(DWORD err, LPTSTR lpszFunction)
{
    // Retrieve the system error message for the error code
    if(err) {
        LPVOID lpMsgBuf;
        LPVOID lpDisplayBuf;

        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            err,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR) &lpMsgBuf,
            0, NULL );

        lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
            (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 256) * sizeof(TCHAR));

        StringCchPrintf((LPTSTR)lpDisplayBuf,
            LocalSize(lpDisplayBuf) / sizeof(TCHAR),
            "%s\n%s %d:\n%s",
            lpszFunction, _("System error code"), err, lpMsgBuf);

        Error((char *)lpDisplayBuf);

        LocalFree(lpMsgBuf);
        LocalFree(lpDisplayBuf);
    }
}

//-----------------------------------------------------------------------------
void IsErr(DWORD err, char *msg)
{
    GetErrorMessage(err, msg);
return;
    const char *s;
    switch(err) {
        // clang-format off
        case 0:                    s = "The system is out of memory or resources"; break;
        case ERROR_BAD_FORMAT:     s = "The .exe file is invalid";                 break;
        case ERROR_FILE_NOT_FOUND: s = "The specified file was not found";         break;
        case ERROR_PATH_NOT_FOUND: s = "The specified path was not found";         break;
        default:                   s = "";                                         break;
            // clang-format on
    }
    if(strlen(s))
        Error("Error: %d - %s in command line:\n\n%s", err, s, msg);
}

char *GetComspec(char *comspec, int size)
{
    TCHAR sysDir[MAX_PATH] = "";
    GetEnvironmentVariable("COMSPEC", comspec, size);
    if((strlen(comspec) == 0) || (!ExistFile(comspec))) {
        GetSystemDirectory(sysDir, MAX_PATH);
        sprintf(comspec, "%s\\cmd.exe", sysDir);
        if(!ExistFile(comspec))
            sprintf(comspec, "%s\\command.com", sysDir);
    }
    return comspec;
}

//-----------------------------------------------------------------------------
int Execute(char *batchfile, char *batchArgs, int nShowCmd)
{
    DWORD err = GetLastError();
    IsErr(err, "Why???");
    SetLastError(ERROR_SUCCESS);

    char cmdLine[1024*3];
#define VAR 2
#if VAR == 1
    sprintf(cmdLine, "%s %s", batchfile, batchArgs);
    err = WinExec(cmdLine, nShowCmd); // If the function succeeds, the return value is greater than 31.
    if(err > 31)
        err = ERROR_SUCCESS;
    else if (err == 0)
        err = ERROR_NOT_ENOUGH_MEMORY;
    return err;
#else
    char comspec[MAX_PATH*2];
    GetComspec(comspec, sizeof(comspec));

  #if VAR == 2
    sprintf(cmdLine, "/C \"%s %s\"", batchfile, batchArgs);

    err = (DWORD) ShellExecute(NULL, NULL, comspec, cmdLine, NULL, nShowCmd);
    if(err > 32)
        err = ERROR_SUCCESS;
    else if (err == 0)
        err = ERROR_NOT_ENOUGH_MEMORY;
    return err;
  #else
    //sprintf(cmdLine, "/k \"\"a a a.bat\" 1 \"2 2\" \" \" \"4 4\" 5 \"", batchfile, batchArgs);
    sprintf(cmdLine, "/C \"\"%s\" %s\"", batchfile, batchArgs);

    TCHAR sysDir[MAX_PATH] = "";
    GetSystemDirectory(sysDir, MAX_PATH);

    STARTUPINFO si = {0};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {0};

    if( !CreateProcess(comspec, // quotes are not needed
        cmdLine,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &pi )
        )
    {
        IsErr(GetLastError(), cmdLine);
        return 0;
    }

    WaitForSingleObject( pi.hProcess, INFINITE );
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
    return 0;
  #endif
#endif
}

//-----------------------------------------------------------------------------
char *GetIsaName(int ISA)
{
    switch(ISA) {
        // clang-format off
        case ISA_AVR          : return (char *)stringer( ISA_AVR          ) + 4;
        case ISA_PIC16        : return (char *)stringer( ISA_PIC16        ) + 4;
        case ISA_PIC18        : return (char *)stringer( ISA_PIC18        ) + 4;
      //case ISA_ANSIC        : return (char *)stringer( ISA_ANSIC        ) + 4;
        case ISA_INTERPRETED  : return (char *)stringer( ISA_INTERPRETED  ) + 4;
        case ISA_XINTERPRETED : return (char *)stringer( ISA_XINTERPRETED ) + 4;
        case ISA_NETZER       : return (char *)stringer( ISA_NETZER       ) + 4;
        case ISA_PC           : return (char *)stringer( ISA_PC           ) + 4;
      //case ISA_ARDUINO      : return (char *)stringer( ISA_ARDUINO      ) + 4;
      //case ISA_CAVR         : return (char *)stringer( ISA_CAVR         ) + 4;
        case ISA_ARM          : return (char *)stringer( ISA_ARM          ) + 4;
        default               : oops(); // return nullptr;
            // clang-format on
    }
}

//-----------------------------------------------------------------------------
const char *GetMnuCompilerName(int MNU)
{
    switch(MNU) {
        // clang-format off
        case MNU_COMPILE_ANSIC         : return (char *)stringer(MNU_COMPILE_ANSIC) + 12;
        case MNU_COMPILE_HI_TECH_C     : return (char *)stringer(MNU_COMPILE_HI_TECH_C) + 12;
        case MNU_COMPILE_CCS_PIC_C     : return (char *)stringer(MNU_COMPILE_CCS_PIC_C) + 12;
        case MNU_COMPILE_GNUC          : return (char *)stringer(MNU_COMPILE_GNUC) + 12;
        case MNU_COMPILE_AVRGCC        : return (char *)stringer(MNU_COMPILE_AVRGCC) + 12;
        case MNU_COMPILE_ARMGCC        : return (char *)stringer(MNU_COMPILE_ARMGCC) + 12;
        case MNU_COMPILE_CODEVISIONAVR : return (char *)stringer(MNU_COMPILE_CODEVISIONAVR) + 12;
        case MNU_COMPILE_IMAGECRAFT    : return (char *)stringer(MNU_COMPILE_IMAGECRAFT) + 12;
        case MNU_COMPILE_IAR           : return (char *)stringer(MNU_COMPILE_IAR) + 12;
        case MNU_COMPILE_ARDUINO       : return (char *)stringer(MNU_COMPILE_ARDUINO) + 12;
        case MNU_COMPILE_PASCAL        : return (char *)stringer(MNU_COMPILE_PASCAL) + 12;
        case MNU_COMPILE_IHEX          : return (char *)stringer(MNU_COMPILE_IHEX) + 12;
        case MNU_COMPILE_ASM           : return (char *)stringer(MNU_COMPILE_ASM) + 12;
        case MNU_COMPILE_INT           : return (char *)stringer(MNU_COMPILE_INT) + 12;
        case MNU_COMPILE_XINT          : return (char *)stringer(MNU_COMPILE_XINT) + 12;
        default                        : return "";
            // clang-format on
    }
}

//-----------------------------------------------------------------------------
int GetMnu(char *MNU_name)
{
    // clang-format off
    if(!strlen(MNU_name)) return -1;
    if(strstr("MNU_COMPILE_ANSIC",         MNU_name)) return MNU_COMPILE_ANSIC;
    if(strstr("MNU_COMPILE_HI_TECH_C",     MNU_name)) return MNU_COMPILE_HI_TECH_C;
    if(strstr("MNU_COMPILE_CCS_PIC_C",     MNU_name)) return MNU_COMPILE_CCS_PIC_C;
    if(strstr("MNU_COMPILE_GNUC",          MNU_name)) return MNU_COMPILE_GNUC;
    if(strstr("MNU_COMPILE_AVRGCC",        MNU_name)) return MNU_COMPILE_AVRGCC;
    if(strstr("MNU_COMPILE_CODEVISIONAVR", MNU_name)) return MNU_COMPILE_CODEVISIONAVR;
    if(strstr("MNU_COMPILE_ARDUINO",       MNU_name)) return MNU_COMPILE_ARDUINO;
    if(strstr("MNU_COMPILE_ARMGCC",        MNU_name)) return MNU_COMPILE_ARMGCC;
    if(strstr("MNU_COMPILE_PASCAL",        MNU_name)) return MNU_COMPILE_PASCAL;
    // clang-format on
    return -1;
}

//-----------------------------------------------------------------------------
static void BuildAll(char *name, int ISA)
{
    char s[MAX_PATH];
    char r[MAX_PATH];
    char deviceName[64];

    if(strlen(name) == 0) {
        Warning(_("Save ld before build."));
        return;
    }
    if (!Prog.mcu()) return;
    strcpy(deviceName, Prog.mcu()->deviceName);

    s[0] = '\0';
    SetExt(s, name, "");
    if (compile_MNU == MNU_COMPILE_HI_TECH_C) {
        strcpy(deviceName, deviceName+3);       // remove "Pic" prefix in mcu name
    }

    if (ISA == ISA_AVR) {
        sprintf(r, "%sbuildAvr.bat", ExePath);
        GetFileName(s, CurrentSaveFile);
        // %0= batch_file, %1= project_path, %2= file_name, %3= target_name, %4= compiler_path, %5= prog_tool
        Capture(_("Build Solution"), r, CurrentLdPath, s, _strlwr(deviceName), "",  "");
    }
    else if (ISA == ISA_PIC16) {
        sprintf(r, "%sbuildPic16.bat", ExePath);
        GetFileName(s, CurrentSaveFile);
        // %0= batch_file, %1= project_path, %2= file_name, %3= target_name, %4= compiler_path, %5= prog_tool
        Capture(_("Build Solution"), r, CurrentLdPath, s, _strlwr(deviceName), "",  "");
    }
    else if (ISA == ISA_PIC18) {
        sprintf(r, "%sbuildPic18.bat", ExePath);
        GetFileName(s, CurrentSaveFile);
        // %0= batch_file, %1= project_path, %2= file_name, %3= target_name, %4= compiler_path, %5= prog_tool
        Capture(_("Build Solution"), r, CurrentLdPath, s, _strlwr(deviceName), "",  "");
    }
    else if (ISA == ISA_ARM) {
        sprintf(r, "%sbuildArm.bat", ExePath);
        GetFileName(s, CurrentSaveFile);
        // %0= batch_file, %1= project_path, %2= file_name, %3= target_name, %4= compiler_path, %5= prog_tool
        Capture(_("Build Solution"), r, CurrentLdPath, s, _strlwr(deviceName), "",  "");
    }
}

//-----------------------------------------------------------------------------
static void flashBat(char *name, int ISA)
{
    char s[MAX_PATH];
    char r[MAX_PATH];
    char deviceName[64];

    if(strlen(name) == 0) {
        Warning(_("Save ld before flash."));
        return;
    }
    if(!Prog.mcu())
        return;
    strcpy(deviceName, Prog.mcu()->deviceName);

    s[0] = '\0';
    SetExt(s, name, "");
    if(compile_MNU == MNU_COMPILE_HI_TECH_C) {
        strcpy(deviceName, deviceName + 3); // remove "Pic" prefix in mcu name
    }

    sprintf(r, "%sflashMcu.bat", ExePath);
    Capture(_("Flash MCU"), r, GetIsaName(ISA), s, GetMnuCompilerName(compile_MNU), _strlwr(deviceName), "");
}

//-----------------------------------------------------------------------------
static void readBat(const char *name, int ISA)
{
    char s[MAX_PATH];
    char r[MAX_PATH];
    char deviceName[64];

    if(strlen(name) == 0) {
        name = "read";
    }
    if (!Prog.mcu())
        return;
    strcpy(deviceName, Prog.mcu()->deviceName);

    s[0] = '\0';
    SetExt(s, name, "");
    if (compile_MNU == MNU_COMPILE_HI_TECH_C) {
        strcpy(deviceName, deviceName+3);       // remove "Pic" prefix in mcu name
    }

    sprintf(r, "%sreadMcu.bat", ExePath);
    Capture(_("Read MCU"), r, GetIsaName(ISA), s, GetMnuCompilerName(compile_MNU), _strlwr(deviceName), "");
}

//-----------------------------------------------------------------------------
static void clearBat(int ISA)
{
    char r[MAX_PATH];
    char deviceName[64];

    if (!Prog.mcu()) return;
    strcpy(deviceName, Prog.mcu()->deviceName);

    sprintf(r, "%sclearMcu.bat", ExePath);
    Capture(_("Clear MCU"), r, GetIsaName(ISA), _strlwr(deviceName), "", "", "");
}

//-----------------------------------------------------------------------------
static void notepad(const char *path, const char *name, const char *ext)
{
    char s[MAX_PATH] = "";
    char r[MAX_PATH] = "";

    r[0] = '\0';
    if(path && strlen(path)) {
        strcpy(r, path);
        if(path[strlen(path) - 1] != '\\')
            strcat(r, "\\");
    }
    strcat(r, name);

    s[0] = '\0';
    SetExt(s, r, ext);

    if(!ExistFile(s)) {
        Error(_("File does not exist: '%s'"), s);
        return;
    }
    sprintf(r, "\"%snotepad.bat\" \"%s\"", ExePath, s);
    IsErr(Execute(r, "", SW_SHOWMINIMIZED), r);
}

static void notepad(const char *name, const char *ext)
{
    notepad(nullptr, name, ext);
}

//-----------------------------------------------------------------------------
static void clearBat()
{
    char LdName[MAX_PATH];
    strcpy(LdName, ExtractFileName(CurrentSaveFile));
    SetExt(LdName, LdName, "");

    char CompileName[MAX_PATH];
    strcpy(CompileName, ExtractFileName(CurrentCompileFile));
    SetExt(CompileName, CompileName, "");

    char r[MAX_PATH];

    sprintf(r, "%sclear.bat", ExePath);
    if(!ExistFile(r))
        return;

    sprintf(r, "\"%sclear.bat\" \"%s\" \"%s\" \"%s\" \"%s\"", ExePath, CurrentLdPath, LdName, CurrentCompilePath, CompileName);
    IsErr(Execute(r, "", SW_SHOWMINIMIZED/*SW_SHOWMINNOACTIVE*/), r);
}

//-----------------------------------------------------------------------------
static void postCompile(const char *MNU)
{
    if(!ExistFile(CurrentCompileFile))
        return;

    char LdName[MAX_PATH];
    strcpy(LdName, ExtractFileName(CurrentCompileFile));
    SetExt(LdName, LdName, "");

    if(!fsize(CurrentCompileFile)) {
        char outFile[MAX_PATH];

        remove(CurrentCompileFile);
        if(strstr(CurrentCompileFile, ".hex")) {
            sprintf(outFile, "%s%s%s", CurrentCompilePath, LdName, ".asm");
            remove(outFile);
        }
        if(strstr(CurrentCompileFile, ".c")) {
            sprintf(outFile, "%s%s%s", CurrentCompilePath, LdName, ".h");
            remove(outFile);
            sprintf(outFile, "%s%s", CurrentCompilePath, "ladder.h_");
            remove(outFile);
        }
        if(strstr(CurrentCompileFile, ".cpp")) {
            sprintf(outFile, "%s%s%s", CurrentCompilePath, LdName, ".h");
            remove(outFile);
            sprintf(outFile, "%s%s%s", CurrentCompilePath, LdName, ".ino");
            remove(outFile);
            sprintf(outFile, "%s%s", CurrentCompilePath, "ladder.h");
            remove(outFile);
        }
        return;
    }

    char r[MAX_PATH];

    sprintf(r, "%spostCompile.bat", ExePath);
    if(!ExistFile(r))
        return;

    const char *ISA = "_NULL_";
    if(Prog.mcu())
        ISA = GetIsaName(Prog.mcu()->whichIsa);

    sprintf(r, "\"%spostCompile.bat\" %s %s \"%s\" \"%s\" %s", ExePath, MNU, ISA, CurrentCompilePath, LdName, GetMnuCompilerName(compile_MNU));
    IsErr(Execute(r, "", SW_SHOWMINNOACTIVE), r);
}

//-----------------------------------------------------------------------------
// Compile the program to a hex file for the target micro. Get the output
// file name if necessary, then call the micro-specific compile routines.
//-----------------------------------------------------------------------------
static void CompileProgram(bool compileAs, int MNU)
{
    if((MNU == MNU_COMPILE) && (compile_MNU > 0))
        MNU = compile_MNU;
    compile_MNU = MNU;

    if(MNU == MNU_COMPILE_GNUC) {
        if((Prog.mcu()) && (Prog.mcu()->whichIsa != ISA_AVR)) {
            int msgboxID = MessageBox(NULL, _("You try to compile to WinAvr C, but MCU core isn't AVR.\nDo you want to continue?"), _("MCU type warning"), MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);
            if(msgboxID != IDYES)
                return;
        }
    }

    if(MNU == MNU_COMPILE_AVRGCC) {
        if((Prog.mcu()) && (Prog.mcu()->whichIsa != ISA_AVR)) {
            int msgboxID = MessageBox(NULL, _("You try to compile to AVR GCC, but MCU core isn't AVR.\nDo you want to continue?"), _("MCU type warning"), MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);
            if(msgboxID != IDYES)
                return;
        }
    }

    if(MNU == MNU_COMPILE_CODEVISIONAVR) {
        if((Prog.mcu()) && (Prog.mcu()->whichIsa != ISA_AVR)) {
            int msgboxID = MessageBox(NULL, _("You try to compile to CodeVision C, but MCU core isn't AVR.\nDo you want to continue?"), _("MCU type warning"), MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);
            if(msgboxID != IDYES)
                return;
        }
    }

    if(MNU == MNU_COMPILE_HI_TECH_C) {
        if((Prog.mcu()) && (Prog.mcu()->whichIsa != ISA_PIC16) && (Prog.mcu()->whichIsa != ISA_PIC18)) {
            int msgboxID = MessageBox(NULL, _("You try to compile to HI-TECH C, but MCU core isn't PIC.\nDo you want to continue?"), _("MCU type warning"), MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);
            if(msgboxID != IDYES)
                return;
        }
    }

    if(MNU == MNU_COMPILE_CCS_PIC_C) {
        if((Prog.mcu()) && (Prog.mcu()->whichIsa != ISA_PIC16) && (Prog.mcu()->whichIsa != ISA_PIC18)) {
            int msgboxID = MessageBox(NULL, _("You try to compile to CSS-PIC C, but MCU core isn't PIC.\nDo you want to continue?"), _("MCU type warning"), MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);
            if(msgboxID != IDYES)
                return;
        }
    }

    if(MNU == MNU_COMPILE_ARMGCC) {
        if((Prog.mcu()) && (Prog.mcu()->whichIsa != ISA_ARM)) {
            int msgboxID = MessageBox(NULL, _("You try to compile to Arm GCC, but MCU core isn't ARM.\nDo you want to continue?"), _("MCU type warning"), MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);
            if(msgboxID != IDYES)
                return;
        }
    }

    if(MNU == MNU_COMPILE_ARDUINO) {
        if((Prog.mcu()) && (Prog.mcu()->whichIsa != ISA_AVR) && (Prog.mcu()->whichIsa != ISA_ESP)) {
            int msgboxID = MessageBox(NULL, _("You try to compile to Arduino sketch, but MCU core isn't AVR.\nDo you want to continue?"), _("MCU type warning"), MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);
            if(msgboxID != IDYES)
                return;
        }

        char onlyName[MAX_PATH];
        strcpy(onlyName, ExtractFileName(CurrentSaveFile));
        SetExt(onlyName, onlyName, "");

        if(strchr(onlyName, ' ')) {
            strcpy(CurrentCompileFile, "");
            ProgramChangedNotSaved = true;
            Error(_("ARDUINO: Space ' ' not allowed in '%s'\nRename file!"), CurrentSaveFile);
            return;
        }
        if(strchr(onlyName, '.')) {
            strcpy(CurrentCompileFile, "");
            ProgramChangedNotSaved = true;
            Error(_("ARDUINO: Dot '.' not allowed in '%s'\nRename file!"), CurrentSaveFile);
            return;
        }
        if(IsNumber(onlyName)) {
            strcpy(CurrentCompileFile, "");
            ProgramChangedNotSaved = true;
            Error(_("ARDUINO: The leading digit '%c' not allowed at the beginning in '%s.ld'\nRename file!"), onlyName[0], onlyName);
            return;
        }

        strcpy(onlyName, ExtractFileName(CurrentCompileFile));
        SetExt(onlyName, onlyName, "");
        if(strchr(onlyName, ' ')) {
            strcpy(CurrentCompileFile, "");
            ProgramChangedNotSaved = true;
        }
        if(strchr(onlyName, '.')) {
            strcpy(CurrentCompileFile, "");
            ProgramChangedNotSaved = true;
        }
        if(IsNumber(onlyName)) {
            strcpy(CurrentCompileFile, "");
            ProgramChangedNotSaved = true;
        }
    }

IsOpenAnable:
    if(!compileAs && strlen(CurrentCompileFile)) {
        if(((MNU == MNU_COMPILE) && strstr(CurrentCompileFile, ".hex")) // && (compile_MNU <= 0)
           || ((MNU == MNU_COMPILE_AS) && strstr(CurrentCompileFile, ".hex")) || ((MNU == MNU_COMPILE_IHEX) && strstr(CurrentCompileFile, ".hex"))
           || ((MNU >= MNU_COMPILE_ANSIC) && strstr(CurrentCompileFile, ".c") && (MNU <= MNU_COMPILE_lastC)) || ((MNU == MNU_COMPILE_ARDUINO) && strstr(CurrentCompileFile, ".cpp"))
           || ((MNU == MNU_COMPILE_PASCAL) && strstr(CurrentCompileFile, ".pas")) || ((MNU == MNU_COMPILE_INT) && strstr(CurrentCompileFile, ".int"))
           || ((MNU == MNU_COMPILE_XINT) && strstr(CurrentCompileFile, ".xint"))) {
            if(FILE *f = fopen(CurrentCompileFile, "w")) {
                fclose(f);
                remove(CurrentCompileFile);
            } else {
                compileAs = true;
                Error(_("Couldn't open file '%s'"), CurrentCompileFile);
            }
        }
    }

    if(compileAs || ((MNU == MNU_COMPILE_AS) && strlen(CurrentCompileFile) == 0) || (strlen(CurrentCompileFile) == 0) || ((MNU == MNU_COMPILE) && !strstr(CurrentCompileFile, ".hex"))
       || ((MNU == MNU_COMPILE_IHEX) && !strstr(CurrentCompileFile, ".hex")) || ((MNU >= MNU_COMPILE_ANSIC) && !strstr(CurrentCompileFile, ".c") && (MNU <= MNU_COMPILE_lastC))
       || ((MNU == MNU_COMPILE_ARDUINO) && !strstr(CurrentCompileFile, ".cpp")) || ((MNU == MNU_COMPILE_PASCAL) && !strstr(CurrentCompileFile, ".pas")) || ((MNU == MNU_COMPILE_INT) && !strstr(CurrentCompileFile, ".int"))
       || ((MNU == MNU_COMPILE_XINT) && !strstr(CurrentCompileFile, ".xint"))) {
        const char * c;
        OPENFILENAME ofn;

        memset(&ofn, 0, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hInstance = Instance;
        ofn.lpstrTitle = _("Compile To");
        if((MNU >= MNU_COMPILE_ANSIC) && (MNU <= MNU_COMPILE_lastC)) {
            ofn.lpstrFilter = C_PATTERN;
            ofn.lpstrDefExt = "c";
            c = "c";
            //compile_MNU = MNU;
            if(MNU == MNU_COMPILE_ANSIC)
                compile_MNU = MNU_COMPILE_ANSIC;
        } else if((MNU == MNU_COMPILE_INT) || (Prog.mcu() && (Prog.mcu()->whichIsa == ISA_INTERPRETED || Prog.mcu()->whichIsa == ISA_NETZER))) {
            ofn.lpstrFilter = INTERPRETED_PATTERN;
            ofn.lpstrDefExt = "int";
            c = "int";
            compile_MNU = MNU_COMPILE_INT;
        } else if((MNU == MNU_COMPILE_XINT) || (Prog.mcu() && Prog.mcu()->whichIsa == ISA_XINTERPRETED)) {
            ofn.lpstrFilter = XINT_PATTERN;
            ofn.lpstrDefExt = "xint";
            c = "xint";
            compile_MNU = MNU_COMPILE_XINT;
        } else if(MNU == MNU_COMPILE_PASCAL) {
            ofn.lpstrFilter = PASCAL_PATTERN;
            ofn.lpstrDefExt = "pas";
            c = "pas";
            compile_MNU = MNU_COMPILE_PASCAL;
        } else if(MNU == MNU_COMPILE_ARDUINO) {
            ofn.lpstrFilter = ARDUINO_C_PATTERN;
            ofn.lpstrDefExt = "cpp";
            c = "cpp";
            compile_MNU = MNU_COMPILE_ARDUINO;
        } else {
            ofn.lpstrFilter = HEX_PATTERN;
            ofn.lpstrDefExt = "hex";
            c = "hex";
            compile_MNU = MNU_COMPILE_IHEX;
        }
        SetExt(CurrentCompileFile, CurrentSaveFile, c);
        ofn.lpstrFile = CurrentCompileFile;
        ofn.nMaxFile = sizeof(CurrentCompileFile);
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

        if(!GetSaveFileName(&ofn))
            return;

        strcpy(CurrentCompilePath, CurrentCompileFile);
        ExtractFileDir(CurrentCompilePath);

        // hex output filename is stored in the .ld file
        ProgramChangedNotSaved = true;
        compileAs = false;
        goto IsOpenAnable;
    }

    if(!GenerateIntermediateCode())
        return;

    if((Prog.mcu() == nullptr) && (MNU != MNU_COMPILE_PASCAL) && (MNU != MNU_COMPILE_ANSIC) && (MNU != MNU_COMPILE_ARDUINO) && (MNU != MNU_COMPILE_INT) && (MNU != MNU_COMPILE_XINT)) {
        Error(_("Must choose a target microcontroller before compiling."));
        return;
    }

    if((UartFunctionUsed() && (Prog.mcu()) && Prog.mcu()->uartNeeds.rxPin == 0) && (MNU != MNU_COMPILE_PASCAL) && (MNU != MNU_COMPILE_ANSIC) && (MNU != MNU_COMPILE_ARDUINO)) {
        Error(_("UART function used but not supported for this micro."));
        return;
    }

    if(SpiFunctionUsed()) {
        if((MNU != MNU_COMPILE_ARMGCC) && (MNU != MNU_COMPILE_AVRGCC) && (MNU != MNU_COMPILE_HI_TECH_C)) {
            Error(_("SPI functions used but not supported for this micro or compile mode."));
            return;
        }

        char deviceName[MAX_PATH] = "";
        if(Prog.mcu())
            strcpy(deviceName, Prog.mcu()->deviceName);

        if((MNU == MNU_COMPILE_HI_TECH_C) && (strcmp(deviceName, "PIC16F628") == 0)) { // no SPI on this PIC
            Error(_("SPI functions used but not supported for this micro or compile mode."));
            return;
        }
    }

    if(I2cFunctionUsed()) {
        if((MNU != MNU_COMPILE_ARMGCC) && (MNU != MNU_COMPILE_AVRGCC) && (MNU != MNU_COMPILE_HI_TECH_C)) {
            Error(_("I2C functions used but not supported for this micro or compile mode."));
            return;
        }

        char deviceName[MAX_PATH] = "";
        if(Prog.mcu())
            strcpy(deviceName, Prog.mcu()->deviceName);

        if((MNU == MNU_COMPILE_HI_TECH_C) && (strcmp(deviceName, "PIC16F628") == 0)) { // no SPI on this PIC
            Error(_("I2C functions used but not supported for this micro or compile mode."));
            return;
        }
    }

    try {
        if((PwmFunctionUsed() && (Prog.mcu()) && (Prog.mcu()->pwmCount == 0)) && (MNU != MNU_COMPILE_PASCAL) && (MNU != MNU_COMPILE_ANSIC) && (MNU != MNU_COMPILE_ARDUINO) && (MNU != MNU_COMPILE_XINT)
           && (Prog.mcu()->whichIsa != ISA_XINTERPRETED)) {
            Error(_("PWM function used but not supported for this micro."));
            return;
        }

        if((MNU >= MNU_COMPILE_ANSIC) && (MNU <= MNU_COMPILE_lastC)) {
            if(CompileAnsiC(CurrentCompileFile, MNU)) {
                CompileSuccesfullAnsiCMessage(CurrentCompileFile);
                postCompile("ANSIC");
            }
        } else if(MNU == MNU_COMPILE_ARDUINO) {
            if(CompileAnsiC(CurrentCompileFile, MNU)) {
                CompileSuccesfullAnsiCMessage(CurrentCompileFile);
                postCompile("ARDUINO");
            }
        } else if(MNU == MNU_COMPILE_PASCAL) {
            CompilePascal(CurrentCompileFile);
            postCompile("PASCAL");
        } else if(MNU == MNU_COMPILE_INT) {
            CompileInterpreted(CurrentCompileFile);
            postCompile("INTERPRETED");
        } else if(MNU == MNU_COMPILE_XINT) {
            CompileXInterpreted(CurrentCompileFile);
            postCompile("XINTERPRETED");
        } else if(Prog.mcu()) {
            switch(Prog.mcu()->whichIsa) {
                case ISA_AVR:
                    CompileAvr(CurrentCompileFile);
                    break;
                case ISA_PIC16:
                    CompilePic16(CurrentCompileFile);
                    break;
                case ISA_PIC18:
                    Info(_("Use menu: 'Compile->Compile HI-TECH C for PIC'")); ///// To Translate
                    break;
                case ISA_INTERPRETED:
                    CompileInterpreted(CurrentCompileFile);
                    break;
                case ISA_XINTERPRETED:
                    CompileXInterpreted(CurrentCompileFile);
                    break;
                case ISA_NETZER:
                    CompileNetzer(CurrentCompileFile);
                    break;
                case ISA_ARM:
                    Info(_("Use menu: 'Compile->Compile ARM-GCC for 32-bit ARM'"));
                    break;
                default:
                    ooops("0x%X", Prog.mcu()->whichIsa);
            }
            postCompile(GetIsaName(Prog.mcu()->whichIsa));
        } else
            oops();

    } catch(const std::exception &e) {
        Error(e.what());
    }

    RefreshControlsToSettings();
}

//-----------------------------------------------------------------------------
// If the program has been modified then give the user the option to save it
// or to cancel the operation they are performing. Return true if they want
// to cancel.
//-----------------------------------------------------------------------------
bool CheckSaveUserCancels()
{
    if(!ProgramChangedNotSaved) {
        // no problem
        return false;
    }

    int r = MessageBox(MainWindow,
                       _("The program has changed since it was last saved.\r\n\r\n"
                         "Do you want to save the changes?"),
                       "LDmicro",
                       MB_YESNOCANCEL | MB_ICONWARNING);
    switch(r) {
        case IDYES:
            if(SaveProgram(MNU_SAVE))
                return false;
            else
                return true;

        case IDNO:
            return false;

        case IDCANCEL:
            return true;

        default:
            oops();
            // return false;
    }
}

//-----------------------------------------------------------------------------
// Load a new program from a file. If it succeeds then set our default filename
// to that, else we end up with an empty file then.
//-----------------------------------------------------------------------------
static void OpenDialog()
{
    OPENFILENAME ofn;

    char tempSaveFile[MAX_PATH] = "";

    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hInstance = Instance;
    ofn.lpstrFilter = LDMICRO_PATTERN;
    ofn.lpstrDefExt = "ld";
    ofn.lpstrFile = tempSaveFile;
    ofn.nMaxFile = sizeof(tempSaveFile);
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    if(!GetOpenFileName(&ofn))
        return;

    if(!LoadProjectFromFile(tempSaveFile)) {
        Error(_("Couldn't open '%s'."), tempSaveFile);
        CurrentSaveFile[0] = '\0';
    } else {
        ProgramChangedNotSaved = false;
        RefreshControlsToSettings();
        strcpy(CurrentSaveFile, tempSaveFile);
        UndoFlush();
        strcpy(CurrentCompileFile, "");
    }

    GenerateIoListDontLoseSelection();
    RefreshScrollbars();
    UpdateMainWindowTitleBar();
}

//-----------------------------------------------------------------------------
// Housekeeping required when the program changes: mark the program as
// changed so that we ask if user wants to save before exiting, and update
// the I/O list.
//-----------------------------------------------------------------------------
void ProgramChanged()
{
    ProgramChangedNotSaved = true;
    GenerateIoListDontLoseSelection();
    RefreshScrollbars();
}
/* moved to ldmicro.h
#define CHANGING_PROGRAM(x) { \
        UndoRemember(); \
        x; \
        ProgramChanged(); \
    }
*/
//-----------------------------------------------------------------------------
// Hook that we install when the user starts dragging the `splitter,' in case
// they drag it out of the narrow area of the drawn splitter bar. Resize
// the listview in response to mouse move, and unhook ourselves when they
// release the mouse button.
//-----------------------------------------------------------------------------
static LRESULT CALLBACK MouseHook(int code, WPARAM wParam, LPARAM lParam)
{
    switch(code) {
        case HC_ACTION: {
            MSLLHOOKSTRUCT *mhs = (MSLLHOOKSTRUCT *)lParam;

            switch(wParam) {
                case WM_MOUSEMOVE: {
                    int dy = MouseY - mhs->pt.y;

                    IoListHeight += dy;
                    if(IoListHeight < 50)
                        IoListHeight = 50;
                    MouseY = mhs->pt.y;
                    MainWindowResized();

                    break;
                }

                case WM_LBUTTONUP:
                    UnhookWindowsHookEx(MouseHookHandle);
                    break;
            }
            break;
        }
    }
    return CallNextHookEx(MouseHookHandle, code, wParam, lParam);
}

//-----------------------------------------------------------------------------
// Handle a selection from the menu bar of the main window.
//-----------------------------------------------------------------------------
static void ProcessMenu(int code)
{
    if(code >= MNU_PROCESSOR_0 && code < static_cast<int>(MNU_PROCESSOR_0 + supportedMcus().size())) {
        strcpy(CurrentCompileFile, "");
        Prog.setMcu(&(supportedMcus()[code - MNU_PROCESSOR_0]));
        LoadWritePcPorts();
        RefreshControlsToSettings();
        ProgramChangedNotSaved = true;
        WhatCanWeDoFromCursorAndTopology();
        return;
    }
    if(code == static_cast<int>(MNU_PROCESSOR_0 + supportedMcus().size())) {
        Prog.setMcu(nullptr);
        strcpy(CurrentCompileFile, "");
        RefreshControlsToSettings();
        ProgramChangedNotSaved = true;
        WhatCanWeDoFromCursorAndTopology();
        return;
    }
    if((code >= MNU_SCHEME_BLACK) && (code < MNU_SCHEME_BLACK + NUM_SUPPORTED_SCHEMES)) {
        scheme = code & 0xff;
        InitForDrawing();
        InvalidateRect(MainWindow, nullptr, false);
        RefreshControlsToSettings();
        return;
    }

    switch(code) {
        case MNU_NEW:
            if(CheckSaveUserCancels())
                break;
            NewProgram();
            strcpy(CurrentSaveFile, "");
            strcpy(CurrentCompileFile, "");
            GenerateIoListDontLoseSelection();
            RefreshScrollbars();
            UpdateMainWindowTitleBar();
            break;

        case MNU_OPEN:
            if(CheckSaveUserCancels())
                break;
            OpenDialog();
            break;

        case MNU_SAVE:
        case MNU_SAVE_01:
        case MNU_SAVE_02:
            SaveProgram(code);
            UpdateMainWindowTitleBar();
            break;

        case MNU_SAVE_AS:
            SaveAsDialog();
            UpdateMainWindowTitleBar();
            break;

        case MNU_EXPORT:
            ExportDialog();
            break;

        case MNU_BUILD_ALL:
            BuildAll(CurrentSaveFile, Prog.mcu() ? Prog.mcu()->whichIsa : 0);
            break;

        case MNU_FLASH_BAT:
            flashBat(CurrentSaveFile, Prog.mcu() ? Prog.mcu()->whichIsa : 0);
            break;

        case MNU_READ_BAT:
            readBat(CurrentSaveFile, Prog.mcu() ? Prog.mcu()->whichIsa : 0);
            break;

        case MNU_CLEAR_BAT:
            clearBat(Prog.mcu() ? Prog.mcu()->whichIsa : 0);
            break;

        case MNU_NOTEPAD_LD:
            if(CheckSaveUserCancels())
                break;
            notepad(CurrentSaveFile, "ld");
            break;

        case MNU_NOTEPAD_PL:
            notepad(CurrentSaveFile, "pl");
            break;

        case MNU_NOTEPAD_TXT:
            notepad(CurrentSaveFile, "txt");
            break;

        case MNU_NOTEPAD_HEX:
            notepad(strlen(CurrentCompileFile) ? CurrentCompileFile : CurrentSaveFile, "hex");
            break;

        case MNU_NOTEPAD_ASM:
            notepad(strlen(CurrentCompileFile) ? CurrentCompileFile : CurrentSaveFile, "asm");
            break;

        case MNU_NOTEPAD_C: {
            notepad(strlen(CurrentCompileFile) ? CurrentCompileFile : CurrentSaveFile, "c");
            notepad(strlen(CurrentCompileFile) ? CurrentCompileFile : CurrentSaveFile, "h");
            char ladderhName[MAX_PATH];
            sprintf(ladderhName, "%s\\ladder.h", strlen(CurrentCompileFile) ? CurrentCompilePath : CurrentSaveFile);
            notepad(ladderhName, "h");
            break;
        }
        case MNU_NOTEPAD_INO: {
            notepad(strlen(CurrentCompileFile) ? CurrentCompileFile : CurrentSaveFile, ".ino");
            notepad(strlen(CurrentCompileFile) ? CurrentCompileFile : CurrentSaveFile, ".cpp");
            notepad(strlen(CurrentCompileFile) ? CurrentCompileFile : CurrentSaveFile, ".h");
            char ladderhName[MAX_PATH];
            sprintf(ladderhName, "%s\\ladder.h", strlen(CurrentCompileFile) ? CurrentCompilePath : CurrentSaveFile);
            notepad(ladderhName, ".h");
            break;
        }
        case MNU_NOTEPAD_PAS:
            notepad(strlen(CurrentCompileFile) ? CurrentCompileFile : CurrentSaveFile, ".pas");
            notepad(strlen(CurrentCompileFile) ? CurrentCompileFile : CurrentSaveFile, "U.pas");
            notepad(strlen(CurrentCompileFile) ? CurrentCompileFile : CurrentSaveFile, "U.inc");
            notepad(strlen(CurrentCompileFile) ? CurrentCompileFile : CurrentSaveFile, "V.pas");
            notepad(strlen(CurrentCompileFile) ? CurrentCompileFile : CurrentSaveFile, "R.pas");
            break;

        case MNU_EXIT:
            if(CheckSaveUserCancels())
                break;
            PostQuitMessage(0);
            break;

        case MNU_TAB:
            if(GetFocus() == MainWindow) {
                SetFocus(IoList);
                BlinkCursor(0, 0, 0, 0);
            } else if(GetFocus() == IoList) {
                SetFocus(MainWindow);
            }
            break;

        case MNU_INSERT_COMMENT:
            CHANGING_PROGRAM(AddComment(_("--add comment here--")));
            break;

        case MNU_INSERT_CONTACTS:
        case MNU_INSERT_CONT_RELAY:
        case MNU_INSERT_CONT_OUTPUT:
            CHANGING_PROGRAM(AddContact(code));
            break;

        case MNU_INSERT_COIL:
        case MNU_INSERT_COIL_RELAY:
            CHANGING_PROGRAM(AddCoil(code));
            break;

        case MNU_INSERT_TIME2COUNT:
            CHANGING_PROGRAM(AddTimer(ELEM_TIME2COUNT));
            break;

        case MNU_INSERT_DELAY:
            CHANGING_PROGRAM(AddTimer(ELEM_DELAY));
            break;

        case MNU_INSERT_TIME2DELAY:
            CHANGING_PROGRAM(AddTimer(ELEM_TIME2DELAY));
            break;

        case MNU_INSERT_TCY:
            CHANGING_PROGRAM(AddTimer(ELEM_TCY));
            break;

        case MNU_INSERT_TON:
            CHANGING_PROGRAM(AddTimer(ELEM_TON));
            break;

        case MNU_INSERT_TOF:
            CHANGING_PROGRAM(AddTimer(ELEM_TOF));
            break;

        case MNU_INSERT_RTO:
            CHANGING_PROGRAM(AddTimer(ELEM_RTO));
            break;

        case MNU_INSERT_RTL:
            CHANGING_PROGRAM(AddTimer(ELEM_RTL));
            break;

        case MNU_INSERT_THI:
            CHANGING_PROGRAM(AddTimer(ELEM_THI));
            break;

        case MNU_INSERT_TLO:
            CHANGING_PROGRAM(AddTimer(ELEM_TLO));
            break;

        case MNU_INSERT_CTU:
            CHANGING_PROGRAM(AddCounter(ELEM_CTU));
            break;

        case MNU_INSERT_CTD:
            CHANGING_PROGRAM(AddCounter(ELEM_CTD));
            break;

        case MNU_INSERT_CTC:
            CHANGING_PROGRAM(AddCounter(ELEM_CTC));
            break;

        case MNU_INSERT_CTR:
            CHANGING_PROGRAM(AddCounter(ELEM_CTR));
            break;

        case MNU_INSERT_RES:
            CHANGING_PROGRAM(AddReset());
            break;

        case MNU_INSERT_OPEN:
            CHANGING_PROGRAM(AddEmpty(ELEM_OPEN));
            break;

        case MNU_INSERT_SHORT:
            CHANGING_PROGRAM(AddEmpty(ELEM_SHORT));
            break;

        case MNU_INSERT_MASTER_RLY:
            CHANGING_PROGRAM(AddMasterRelay());
            break;

        case MNU_INSERT_SLEEP:
            CHANGING_PROGRAM(AddSleep());
            break;

        case MNU_INSERT_CLRWDT:
            CHANGING_PROGRAM(AddClrWdt());
            break;

        case MNU_INSERT_LOCK:
            CHANGING_PROGRAM(AddLock());
            break;

        case MNU_INSERT_LABEL:
            CHANGING_PROGRAM(AddGoto(ELEM_LABEL));
            break;

        case MNU_INSERT_GOTO:
            CHANGING_PROGRAM(AddGoto(ELEM_GOTO));
            break;

        case MNU_INSERT_SUBPROG:
            CHANGING_PROGRAM(AddGoto(ELEM_SUBPROG));
            break;

        case MNU_INSERT_RETURN:
            CHANGING_PROGRAM(AddGoto(ELEM_RETURN));
            break;

        case MNU_INSERT_ENDSUB:
            CHANGING_PROGRAM(AddGoto(ELEM_ENDSUB));
            break;

        case MNU_INSERT_GOSUB:
            CHANGING_PROGRAM(AddGoto(ELEM_GOSUB));
            break;

        case MNU_INSERT_SHIFT_REG:
            CHANGING_PROGRAM(AddShiftRegister());
            break;

        case MNU_INSERT_LUT:
            CHANGING_PROGRAM(AddLookUpTable());
            break;

        case MNU_INSERT_PWL:
            CHANGING_PROGRAM(AddPiecewiseLinear());
            break;

        case MNU_INSERT_FMTD_STRING:
            CHANGING_PROGRAM(AddFormattedString());
            break;
        case MNU_INSERT_FRMT_STR_TO_CHAR:
            CHANGING_PROGRAM(AddFrmtStrToChar());
            break;
        case MNU_INSERT_STRING:
            CHANGING_PROGRAM(AddString());
            break;
            /*
        case ELEM_CPRINTF:
        case ELEM_SPRINTF:
        case ELEM_FPRINTF:
        case ELEM_PRINTF:
        case ELEM_I2C_CPRINTF:
        case ELEM_ISP_CPRINTF:
        case ELEM_UART_CPRINTF:
            CHANGING_PROGRAM(AddPrint(code));
            break;
*/
        case MNU_INSERT_OSR:
            CHANGING_PROGRAM(AddEmpty(ELEM_ONE_SHOT_RISING));
            break;

        case MNU_INSERT_OSF:
            CHANGING_PROGRAM(AddEmpty(ELEM_ONE_SHOT_FALLING));
            break;

        case MNU_INSERT_ODF:
            CHANGING_PROGRAM(AddEmpty(ELEM_ONE_DROP_FALLING));
            break;

        case MNU_INSERT_ODR:
            CHANGING_PROGRAM(AddEmpty(ELEM_ONE_DROP_RISING));
            break;

        case MNU_INSERT_OSC:
            CHANGING_PROGRAM(AddEmpty(ELEM_OSC));
            break;

        case MNU_INSERT_MOV:
            CHANGING_PROGRAM(AddMove());
            break;

        case MNU_INSERT_BIN2BCD:
            CHANGING_PROGRAM(AddBcd(ELEM_BIN2BCD));
            break;

        case MNU_INSERT_BCD2BIN:
            CHANGING_PROGRAM(AddBcd(ELEM_BCD2BIN));
            break;

        case MNU_INSERT_OPPOSITE:
            CHANGING_PROGRAM(AddBcd(ELEM_OPPOSITE));
            break;

        case MNU_INSERT_SWAP:
            CHANGING_PROGRAM(AddBcd(ELEM_SWAP));
            break;

        case MNU_INSERT_BUS:
            CHANGING_PROGRAM(AddBus(ELEM_BUS));
            break;

        case MNU_INSERT_SPI:
            CHANGING_PROGRAM(AddSpi(ELEM_SPI));
            break;

        case MNU_INSERT_SPI_WRITE:
            CHANGING_PROGRAM(AddSpi(ELEM_SPI_WR));
            break;

        case MNU_INSERT_I2C_READ:
            CHANGING_PROGRAM(AddI2c(ELEM_I2C_RD));
            break;

        case MNU_INSERT_I2C_WRITE:
            CHANGING_PROGRAM(AddI2c(ELEM_I2C_WR));
            break;

        case MNU_INSERT_7SEG:
            CHANGING_PROGRAM(AddSegments(ELEM_7SEG));
            break;

        case MNU_INSERT_9SEG:
            CHANGING_PROGRAM(AddSegments(ELEM_9SEG));
            break;

        case MNU_INSERT_14SEG:
            CHANGING_PROGRAM(AddSegments(ELEM_14SEG));
            break;

        case MNU_INSERT_16SEG:
            CHANGING_PROGRAM(AddSegments(ELEM_16SEG));
            break;

        case MNU_INSERT_SET_PWM:
            CHANGING_PROGRAM(AddSetPwm());
            break;

        case MNU_INSERT_NPULSE_OFF:
            CHANGING_PROGRAM(AddEmpty(ELEM_NPULSE_OFF));
            break;

        case MNU_INSERT_READ_ADC:
            CHANGING_PROGRAM(AddReadAdc());
            break;

        case MNU_INSERT_RANDOM:
            CHANGING_PROGRAM(AddRandom());
            break;

        case MNU_INSERT_SEED_RANDOM:
            CHANGING_PROGRAM(AddSeedRandom());
            break;

        case MNU_INSERT_UART_SEND:
            CHANGING_PROGRAM(AddUart(ELEM_UART_SEND));
            break;

        case MNU_INSERT_UART_RECV:
            CHANGING_PROGRAM(AddUart(ELEM_UART_RECV));
            break;
            /*
        case MNU_INSERT_UART_SENDn:
            CHANGING_PROGRAM(AddUart(ELEM_UART_SENDn));
            break;

        case MNU_INSERT_UART_RECVn:
            CHANGING_PROGRAM(AddUart(ELEM_UART_RECVn));
            break;
*/
        case MNU_INSERT_UART_SEND_READY:
            CHANGING_PROGRAM(AddUart(ELEM_UART_SEND_READY));
            break;

        case MNU_INSERT_UART_RECV_AVAIL:
            CHANGING_PROGRAM(AddUart(ELEM_UART_RECV_AVAIL));
            break;

        case MNU_INSERT_PERSIST:
            CHANGING_PROGRAM(AddPersist());
            break;

            {
                int elem;
                case MNU_INSERT_SET_BIT:
                    elem = ELEM_SET_BIT;
                    goto bit_ops;
                case MNU_INSERT_CLEAR_BIT:
                    elem = ELEM_CLEAR_BIT;
                    goto bit_ops;
                case MNU_INSERT_IF_BIT_SET:
                    elem = ELEM_IF_BIT_SET;
                    goto bit_ops;
                case MNU_INSERT_IF_BIT_CLEAR:
                    elem = ELEM_IF_BIT_CLEAR;
                    goto bit_ops;
                bit_ops:
                    CHANGING_PROGRAM(AddBitOps(elem));
                    break;
            }

            {
                int elem;
                case MNU_INSERT_ADD:
                    elem = ELEM_ADD;
                    goto math;
                case MNU_INSERT_SUB:
                    elem = ELEM_SUB;
                    goto math;
                case MNU_INSERT_MUL:
                    elem = ELEM_MUL;
                    goto math;
                case MNU_INSERT_DIV:
                    elem = ELEM_DIV;
                    goto math;
                case MNU_INSERT_MOD:
                    elem = ELEM_MOD;
                    goto math;
                case MNU_INSERT_AND:
                    elem = ELEM_AND;
                    goto math;
                case MNU_INSERT_OR:
                    elem = ELEM_OR;
                    goto math;
                case MNU_INSERT_XOR:
                    elem = ELEM_XOR;
                    goto math;
                case MNU_INSERT_NOT:
                    elem = ELEM_NOT;
                    goto math;
                case MNU_INSERT_NEG:
                    elem = ELEM_NEG;
                    goto math;
                case MNU_INSERT_SHL:
                    elem = ELEM_SHL;
                    goto math;
                case MNU_INSERT_SHR:
                    elem = ELEM_SHR;
                    goto math;
                case MNU_INSERT_SR0:
                    elem = ELEM_SR0;
                    goto math;
                case MNU_INSERT_ROL:
                    elem = ELEM_ROL;
                    goto math;
                case MNU_INSERT_ROR:
                    elem = ELEM_ROR;
                    goto math;
                math:
                    CHANGING_PROGRAM(AddMath(elem));
                    break;
            }

#ifdef USE_SFR
            // Special function register
            {
                int esfr;
                case MNU_INSERT_SFR:
                    esfr = ELEM_RSFR;
                    goto jcmp;
                case MNU_INSERT_SFW:
                    esfr = ELEM_WSFR;
                    goto jcmp;
                case MNU_INSERT_SSFB:
                    esfr = ELEM_SSFR;
                    goto jcmp;
                case MNU_INSERT_csFB:
                    esfr = ELEM_CSFR;
                    goto jcmp;
                case MNU_INSERT_TSFB:
                    esfr = ELEM_TSFR;
                    goto jcmp;
                case MNU_INSERT_T_C_SFB:
                    esfr = ELEM_T_C_SFR;
                    goto jcmp;
                jcmp:
                    CHANGING_PROGRAM(AddSfr(esfr));
                    break;
            }
// Special function register
#endif

            {
                int elem;
                case MNU_INSERT_EQU:
                    elem = ELEM_EQU;
                    goto cmp;
                case MNU_INSERT_NEQ:
                    elem = ELEM_NEQ;
                    goto cmp;
                case MNU_INSERT_GRT:
                    elem = ELEM_GRT;
                    goto cmp;
                case MNU_INSERT_GEQ:
                    elem = ELEM_GEQ;
                    goto cmp;
                case MNU_INSERT_LES:
                    elem = ELEM_LES;
                    goto cmp;
                case MNU_INSERT_LEQ:
                    elem = ELEM_LEQ;
                    goto cmp;
                cmp:
                    CHANGING_PROGRAM(AddCmp(elem));
                    break;
            }

        case MNU_INSERT_STEPPER:
            CHANGING_PROGRAM(AddStepper());
            break;

        case MNU_INSERT_PULSER:
            CHANGING_PROGRAM(AddPulser());
            break;

        case MNU_INSERT_NPULSE:
            CHANGING_PROGRAM(AddNPulse());
            break;

        case MNU_INSERT_QUAD_ENCOD:
            CHANGING_PROGRAM(AddQuadEncod());
            break;

        case MNU_MAKE_NORMAL:
            CHANGING_PROGRAM(MakeNormalSelected());
            break;

        case MNU_NEGATE:
            CHANGING_PROGRAM(NegateSelected());
            break;

        case MNU_MAKE_SET_ONLY:
            CHANGING_PROGRAM(MakeSetOnlySelected());
            break;

        case MNU_MAKE_RESET_ONLY:
            CHANGING_PROGRAM(MakeResetOnlySelected());
            break;

        case MNU_MAKE_TTRIGGER:
            CHANGING_PROGRAM(MakeTtriggerSelected());
            break;

        case MNU_UNDO:
            UndoUndo();
            break;

        case MNU_REDO:
            UndoRedo();
            break;

        case MNU_INSERT_RUNG_BEFORE:
            CHANGING_PROGRAM(InsertRung(false));
            break;

        case MNU_INSERT_RUNG_AFTER:
            CHANGING_PROGRAM(InsertRung(true));
            break;

        case MNU_DELETE_RUNG:
            //CHANGING_PROGRAM(DeleteSelectedRung());
            CHANGING_PROGRAM(CutRung());
            break;

        case MNU_SCROLL_DOWN:
            ScrollDown();
            break;

        case MNU_SCROLL_UP:
            ScrollUp();
            break;

        case MNU_SCROLL_PgDOWN:
            ScrollPgDown();
            break;

        case MNU_SCROLL_PgUP:
            ScrollPgUp();
            break;

        case MNU_ROLL_HOME:
            RollHome();
            break;

        case MNU_ROLL_END:
            RollEnd();
            break;

        case MNU_PUSH_RUNG_UP:
            CHANGING_PROGRAM(PushRungUp());
            break;

        case MNU_PUSH_RUNG_DOWN:
            CHANGING_PROGRAM(PushRungDown());
            break;

        case MNU_COPY_RUNG_DOWN:
            CHANGING_PROGRAM(CopyRungDown());
            break;

        case MNU_SELECT_RUNG: {
            int i = RungContainingSelected();
            if(i >= 0) {
                if(Prog.rungSelected[i] == ' ')
                    Prog.rungSelected[i] = '*';
                else
                    Prog.rungSelected[i] = ' ';
            }
            break;
        }
        case MNU_CUT_RUNG:
            CHANGING_PROGRAM(CutRung());
            break;

        case MNU_COPY_RUNG:
            CHANGING_PROGRAM(CopyRung());
            break;

        case MNU_PASTE_RUNG:
            CHANGING_PROGRAM(PasteRung(0));
            break;

        case MNU_PASTE_INTO_RUNG:
            CHANGING_PROGRAM(PasteRung(1));
            break;

        case MNU_DELETE_ELEMENT:
            CHANGING_PROGRAM(DeleteSelectedFromProgram());
            break;

        case MNU_CUT_ELEMENT:
            CHANGING_PROGRAM(CopyElem());
            CHANGING_PROGRAM(DeleteSelectedFromProgram());
            break;

        case MNU_REPLACE_ELEMENT:
            CHANGING_PROGRAM(ReplaceSelectedElement());
            break;

        case MNU_MCU_SETTINGS:
            CHANGING_PROGRAM(ShowConfDialog());
            break;

        case MNU_PULL_UP_RESISTORS:
            CHANGING_PROGRAM(ShowPullUpDialog());
            break;

        case MNU_SIMULATION_MODE:
            ToggleSimulationMode();
            break;

        case MNU_START_SIMULATION:
            StartSimulation();
            break;

        case MNU_STOP_SIMULATION:
            StopSimulation();
            break;

        case MNU_SINGLE_CYCLE:
            SimulateOneCycle(true);
            break;

        case MNU_COMPILE:
        case MNU_COMPILE_ANSIC:
        case MNU_COMPILE_HI_TECH_C:
        case MNU_COMPILE_CCS_PIC_C:
        case MNU_COMPILE_GNUC:
        case MNU_COMPILE_AVRGCC:
        case MNU_COMPILE_CODEVISIONAVR:
        case MNU_COMPILE_IMAGECRAFT:
        case MNU_COMPILE_IAR:
        case MNU_COMPILE_ARMGCC:
        case MNU_COMPILE_IHEX:
        case MNU_COMPILE_PASCAL:
        case MNU_COMPILE_ARDUINO:
        case MNU_COMPILE_INT:
        case MNU_COMPILE_XINT:
            CompileProgram(false, code);
            break;

        case MNU_PROCESSOR_NEW_PIC12:
            ShellExecute(0, "open", "https://github.com/LDmicro/LDmicro/wiki/HOW-TO:-Soft-start-and-smooth-stop-of-LED-with-software-PWM", nullptr, nullptr, SW_SHOWNORMAL);
            ShellExecute(0, "open", "https://github.com/LDmicro/LDmicro/wiki/NEW:-PIC-8-pins-micros", nullptr, nullptr, SW_SHOWNORMAL);
            break;

        case MNU_PROCESSOR_NEW:
            ShellExecute(0, "open", "https://github.com/LDmicro/LDmicro/wiki/TODO-&-DONE", nullptr, nullptr, SW_SHOWNORMAL);
            ShellExecute(0, "open", "https://github.com/LDmicro/LDmicro/wiki/NEW:--PIC-Enhanced-Mid-Range-Products", nullptr, nullptr, SW_SHOWNORMAL);
            break;

        case MNU_OPEN_SFR:
            ShellExecute(0, "open", "https://github.com/LDmicro/LDmicro/wiki/Replace-the-obsolete-elements", nullptr, nullptr, SW_SHOWNORMAL);
            break;

        case MNU_COMPILE_IHEXDONE:
            Error(
                " "
                "This functionality of LDmicro is in testing and refinement.\n"
                "1. You can send your LD file at the LDmicro.GitHub@gmail.com\n"
                "and get asm output file for PIC's MPLAB or WINASM,\n"
                "or AVR Studio or avrasm2 or avrasm32\n"
                "as shown in the example for asm_demo\n"
                "https://github.com/LDmicro/LDmicro/wiki/HOW-TO:-Integrate-LDmicro-and-AVR-Studio-or-PIC-MPLAB-software.\n"
                "2. You can sponsor development and pay for it. \n"
                "After payment you will get this functionality in a state as is at the time of development \n"
                "and you will be able to generate asm output files for any of your LD files.\n"
                "On the question of payment, please contact LDmicro.GitHub@gmail.com.\n");
            break;

        case MNU_COMPILE_AS:
            CompileProgram(true, code);
            break;

        case MNU_MANUAL:
            ShowHelpDialog(false);
            break;

        case MNU_SELECT_COLOR:
            ShowColorDialog();
            break;

        case MNU_ABOUT:
            ShowHelpDialog(true);
            break;

        case MNU_HOW:
            ShellExecute(0, "open", "https://github.com/LDmicro/LDmicro/wiki/HOW-TO", nullptr, nullptr, SW_SHOWNORMAL);
            break;

        case MNU_FORUM:
            ShellExecute(0, "open", "http://cq.cx/ladder-forum.pl", nullptr, nullptr, SW_SHOWNORMAL);
            break;

        case MNU_CHANGES:
            ShellExecute(0, "open", "https://raw.githubusercontent.com/LDmicro/LDmicro/master/ldmicro/CHANGES.txt", nullptr, nullptr, SW_SHOWNORMAL);
            break;

        case MNU_ISSUE:
            ShellExecute(0, "open", "https://github.com/LDmicro/LDmicro/issues/new", nullptr, nullptr, SW_SHOWNORMAL);
            break;

        case MNU_EMAIL:
            ShellExecute(0, "open", "mailto:LDmicro.GitHub@gmail.com", nullptr, nullptr, SW_SHOWNORMAL);
            break;

        case MNU_EXPLORE_DIR:
            ////ShellExecute(0, "open", CurrentLdPath, nullptr, nullptr, SW_SHOWNORMAL);
            ShellExecute(0, "explore", CurrentLdPath, nullptr, nullptr, SW_SHOWNORMAL);
            //ShellExecute(0, "find", CurrentLdPath, nullptr, nullptr, 0);
            break;

        case MNU_RELEASE:
            ShellExecute(0, "open", "https://github.com/LDmicro/LDmicro/releases", nullptr, nullptr, SW_SHOWNORMAL);
            break;
    }
}

//-----------------------------------------------------------------------------
void ScrollUp()
{
    if(ScrollYOffset > 0)
        ScrollYOffset--;
    RefreshScrollbars();
    InvalidateRect(MainWindow, nullptr, false);

    int gx = 0, gy = 0;
    if(!InSimulationMode && FindSelected(&gx, &gy)) {
        if(gy > ScrollYOffset + ScreenRowsAvailable() - 1) {
            gy = ScrollYOffset + ScreenRowsAvailable() - 1;
            MoveCursorNear(&gx, &gy);
        }
    }
}
//-----------------------------------------------------------------------------
void ScrollDown()
{
    if(ScrollYOffset < ScrollYOffsetMax)
        ScrollYOffset++;
    RefreshScrollbars();
    InvalidateRect(MainWindow, nullptr, false);

    int gx = 0, gy = 0;
    if(!InSimulationMode && FindSelected(&gx, &gy)) {
        if(gy < ScrollYOffset) {
            gy = ScrollYOffset;
            MoveCursorNear(&gx, &gy);
        }
    }
}
//-----------------------------------------------------------------------------
void ScrollPgUp()
{
    int gx = 0, gy = 0;
    FindSelected(&gx, &gy);

    ScrollYOffset = 0;
    RefreshScrollbars();
    InvalidateRect(MainWindow, nullptr, false);

    SelectedGxAfterNextPaint = gx;
    SelectedGyAfterNextPaint = 0;
}
//-----------------------------------------------------------------------------
void ScrollPgDown()
{
    int gx = 0, gy = 0;
    FindSelected(&gx, &gy);

    ScrollYOffset = ScrollYOffsetMax;
    RefreshScrollbars();
    InvalidateRect(MainWindow, nullptr, false);

    SelectedGxAfterNextPaint = gx;
    SelectedGyAfterNextPaint = totalHeightScrollbars - 1;
}
//-----------------------------------------------------------------------------
void RollHome()
{
    int gx = 0, gy = 0;
    if(FindSelected(&gx, &gy)) {
        gy = ScrollYOffset;
        MoveCursorNear(&gx, &gy);
    }
}
//-----------------------------------------------------------------------------
void RollEnd()
{
    int gx = 0, gy = 0;
    if(FindSelected(&gx, &gy)) {
        gy = ScrollYOffset + ScreenRowsAvailable() - 1;
        MoveCursorNear(&gx, &gy);
    }
}
//-----------------------------------------------------------------------------
void TestSelections(UINT msg, int rung1)
{
    int rung2;
    if(SelectedGyAfterNextPaint >= 0)
        rung2 = SelectedGyAfterNextPaint;
    else
        rung2 = RungContainingSelected();

    switch(msg) {
        case WM_LBUTTONDOWN: {
            if(GetAsyncKeyState(VK_SHIFT) & 0x8000) {
                if((rung1 >= 0) && (rung2 >= 0)) {
                    if(!(GetAsyncKeyState(VK_CONTROL) & 0x8000))
                        for(int i = 0; i < Prog.numRungs; i++)
                            if(Prog.rungSelected[i] == '*')
                                Prog.rungSelected[i] = ' ';
                    int d = (rung2 < rung1) ? -1 : +1;
                    for(int i = rung1;; i += d) {
                        Prog.rungSelected[i] = '*';
                        if(i == rung2)
                            break;
                    }
                }
            } else if(GetAsyncKeyState(VK_CONTROL) & 0x8000) {
                if((rung2 >= 0)) {
                    if(Prog.rungSelected[rung2] == ' ')
                        Prog.rungSelected[rung2] = '*';
                    else
                        Prog.rungSelected[rung2] = ' ';
                }
            } else {
                for(int i = 0; i < Prog.numRungs; i++)
                    if(Prog.rungSelected[i] == '*')
                        Prog.rungSelected[i] = ' ';
            }
            break;
        }
        case WM_KEYDOWN: {
            //switch(wParam) {
            //}
            if(GetAsyncKeyState(VK_SHIFT) & 0x8000) {
                if((rung1 >= 0) && (rung2 >= 0)) {
                    int d = (rung2 < rung1) ? -1 : +1;
                    for(int i = rung1;; i += d) {
                        Prog.rungSelected[i] = '*';
                        if(i == rung2)
                            break;
                    }
                }
            } else {
                for(int i = 0; i < Prog.numRungs; i++)
                    if(Prog.rungSelected[i] == '*')
                        Prog.rungSelected[i] = ' ';
            }
            break;
        }
    }
}
//-----------------------------------------------------------------------------
// WndProc for MainWindow.
//-----------------------------------------------------------------------------
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int rung1;

    if(strlen(CurrentSaveFile)) {
        if((LastWriteTime & PrevWriteTime) && (LastWriteTime != PrevWriteTime)) {
            tGetLastWriteTime(CurrentSaveFile, (PFILETIME)&LastWriteTime, 0);
            PrevWriteTime = LastWriteTime;

            char buf[1024];
            sprintf(buf,
                    _("File '%s' modified by another application.\r\n"
                      "Its disk timestamp is newer than the editor one.\n"
                      "Reload from disk?"),
                    CurrentSaveFile);
            int r = MessageBox(MainWindow, buf, "LDmicro", MB_YESNO | MB_ICONWARNING);
            switch(r) {
                case IDYES:
                    if(!LoadProjectFromFile(CurrentSaveFile)) {
                        Error(_("Couldn't reload '%s'."), CurrentSaveFile);
                    } else {
                        ProgramChangedNotSaved = false;
                        RefreshControlsToSettings();

                        GenerateIoListDontLoseSelection();
                        RefreshScrollbars();
                    }
                    break;
                default:
                    break;
            }
        }
    }
    switch(msg) {
        case WM_ERASEBKGND:
            break;

        case WM_SETFOCUS:
            InvalidateRect(MainWindow, nullptr, false);
            break;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            Hdc = BeginPaint(hwnd, &ps);

            // This draws the schematic.
            PaintWindow();

            RECT r;
            // Fill around the scroll bars
            if(NeedHoriz) {
                r.top = IoListTop - ScrollHeight - 2;
                r.bottom = IoListTop - 2;
                FillRect(Hdc, &r, (HBRUSH)GetStockObject(LTGRAY_BRUSH));
            }
            GetClientRect(MainWindow, &r);
            r.left = r.right - ScrollWidth - 2;
            FillRect(Hdc, &r, (HBRUSH)GetStockObject(LTGRAY_BRUSH));

            // Draw the splitter thing to grab to resize the I/O listview.
            GetClientRect(MainWindow, &r);
            r.top = IoListTop - 2;
            r.bottom = IoListTop;
            FillRect(Hdc, &r, (HBRUSH)GetStockObject(LTGRAY_BRUSH));
            r.top = IoListTop - 2;
            r.bottom = IoListTop - 1;
            FillRect(Hdc, &r, (HBRUSH)GetStockObject(WHITE_BRUSH));
            r.top = IoListTop;
            r.bottom = IoListTop + 1;
            FillRect(Hdc, &r, (HBRUSH)GetStockObject(DKGRAY_BRUSH));

            EndPaint(hwnd, &ps);
            return 1;
        }

#define VK_ALT VK_MENU
        case WM_SYSKEYDOWN: {
            switch(wParam) {
                case VK_F3:
                    if(GetAsyncKeyState(VK_ALT) & 0x8000) {
                        notepad(strlen(CurrentCompileFile) ? CurrentCompileFile : CurrentSaveFile, "asm");
                        return 1;
                    }
                    break;

                case VK_F5:
                    if(GetAsyncKeyState(VK_ALT) & 0x8000) {
                        notepad(CurrentSaveFile, "pl");
                        ;
                        return 1;
                    }
                    break;

                case VK_F6:
                    if(GetAsyncKeyState(VK_ALT) & 0x8000) {
                        notepad(strlen(CurrentCompileFile) ? CurrentCompileFile : CurrentSaveFile, "hex");
                        return 1;
                    }
                    break;

                case VK_F7:
                    if(GetAsyncKeyState(VK_ALT) & 0x8000) {
                        notepad("acceleration_deceleration", "txt");
                        ;
                        return 1;
                    }
                    break;

                case VK_BACK:
                    if((GetAsyncKeyState(VK_ALT) & 0x8000) && (GetAsyncKeyState(VK_SHIFT) & 0x8000)) {
                        UndoRedo();
                        return 1;
                    } else if(GetAsyncKeyState(VK_ALT) & 0x8000) {
                        UndoUndo();
                        return 1;
                    }
                    break;

                case VK_DOWN:
                    if(GetAsyncKeyState(VK_ALT) & 0x8000) {
                        CHANGING_PROGRAM(PushRungDown());
                        return 1;
                    }
                    break;

                case VK_UP:
                    if(GetAsyncKeyState(VK_ALT) & 0x8000) {
                        CHANGING_PROGRAM(PushRungUp());
                        return 1;
                    }
                    break;

                case VK_DELETE:
                    if(GetAsyncKeyState(VK_ALT) & 0x8000) {
                        CHANGING_PROGRAM(CopyElem());
                        CHANGING_PROGRAM(DeleteSelectedFromProgram());
                        return 1;
                    }
                    break;

                case VK_INSERT:
                    if(GetAsyncKeyState(VK_ALT) & 0x8000) {
                        CHANGING_PROGRAM(PasteRung(1));
                        return 1;
                    }
                    break;

                case 'L':
                    if(GetAsyncKeyState(VK_ALT) & 0x8000) {
                        CHANGING_PROGRAM(AddCoil(MNU_INSERT_COIL_RELAY))
                        return 1;
                    }
                    break;

                case 'X':
                    if(GetAsyncKeyState(VK_ALT) & 0x8000) {
                        SendMessage(hwnd, WM_CLOSE, 0, 0);
                        return 1;
                    }
                    break;

                default:
                    return DefWindowProc(hwnd, msg, wParam, lParam);
            }
        }

        case WM_KEYDOWN: {
            if(wParam == 'M') {
                if(GetAsyncKeyState(VK_CONTROL) & 0x8000) {
                    ToggleSimulationMode();
                    break;
                }
            } else if(wParam == VK_F7) {
                if(GetAsyncKeyState(VK_CONTROL) & 0x8000)
                    ToggleSimulationMode(true);
                else
                    ToggleSimulationMode();
                break;
            } else if(wParam == VK_TAB) {
                SetFocus(IoList);
                BlinkCursor(0, 0, 0, 0);
                break;
            } else if(wParam == VK_F1) {
                ShowHelpDialog(false);
                break;
            } else if(wParam == VK_F3) {
                if(ExportDialog())
                    notepad(CurrentSaveFile, "txt");
                break;
            } else if(wParam == VK_F4) {
                if(CheckSaveUserCancels())
                    break;
                notepad(CurrentSaveFile, "ld");
                break;
            } else if(wParam == VK_F9) {
                if(GetAsyncKeyState(VK_CONTROL) & 0x8000)
                    readBat(CurrentSaveFile, Prog.mcu() ? Prog.mcu()->whichIsa : 0);
                else
                    flashBat(CurrentSaveFile, Prog.mcu() ? Prog.mcu()->whichIsa : 0);
                break;
            } else if(wParam == VK_F6) {
                BuildAll(CurrentSaveFile, Prog.mcu() ? Prog.mcu()->whichIsa : 0);
                break;
            }

            switch(wParam) {
                case VK_DOWN:
                    if((GetAsyncKeyState(VK_CONTROL) & 0x8000) || InSimulationMode) {
                        ScrollDown();
                    } else {
                        rung1 = RungContainingSelected();
                        MoveCursorKeyboard(wParam);
                        TestSelections(msg, rung1);
                    }
                    break;

                case VK_UP:
                    if((GetAsyncKeyState(VK_CONTROL) & 0x8000) || InSimulationMode) {
                        ScrollUp();
                        //                } else if(GetAsyncKeyState(VK_ALT) & 0x8000) {
                        //                  CHANGING_PROGRAM(PushRungUp());
                    } else {
                        rung1 = RungContainingSelected();
                        MoveCursorKeyboard(wParam);
                        TestSelections(msg, rung1);
                    }
                    break;

                case VK_LEFT:
                    if((GetAsyncKeyState(VK_CONTROL) & 0x8000) || InSimulationMode) {
                        ScrollXOffset -= FONT_WIDTH;
                        if(ScrollXOffset < 0)
                            ScrollXOffset = 0;
                        RefreshScrollbars();
                        InvalidateRect(MainWindow, nullptr, false);
                    } else {
                        MoveCursorKeyboard(wParam);
                    }
                    break;

                case VK_RIGHT:
                    if((GetAsyncKeyState(VK_CONTROL) & 0x8000) || InSimulationMode) {
                        ScrollXOffset += FONT_WIDTH;
                        if(ScrollXOffset >= ScrollXOffsetMax)
                            ScrollXOffset = ScrollXOffsetMax;
                        RefreshScrollbars();
                        InvalidateRect(MainWindow, nullptr, false);
                    } else {
                        MoveCursorKeyboard(wParam);
                    }
                    break;

                case VK_HOME:
                    if(GetAsyncKeyState(VK_CONTROL) & 0x8000) {
                        RollHome();
                    } else {
                        int gx = 0, gy = 0;
                        if(FindSelected(&gx, &gy)) {
                            gx = 0;
                            MoveCursorNear(&gx, &gy);
                            SelectElement(gx, gy, SELECTED_RIGHT);
                        }
                    }
                    break;

                case VK_END:
                    if(GetAsyncKeyState(VK_CONTROL) & 0x8000) {
                        RollEnd();
                    } else {
                        int gx = 0, gy = 0;
                        if(FindSelected(&gx, &gy)) {
                            gx = ColsAvailable;
                            MoveCursorNear(&gx, &gy);
                            SelectElement(gx, gy, SELECTED_LEFT);
                        }
                    }
                    break;

                case VK_PRIOR:
                    rung1 = RungContainingSelected();
                    if(GetAsyncKeyState(VK_CONTROL) & 0x8000) {
                        ScrollPgUp();
                    } else {
                        int gx = 0, gy = 0;
                        FindSelected(&gx, &gy);

                        if(ScrollYOffset - ScreenRowsAvailable() - 1 > 0) {
                            ScrollYOffset -= ScreenRowsAvailable() - 1;
                        } else {
                            ScrollYOffset = 0;
                        }
                        RefreshScrollbars();
                        InvalidateRect(MainWindow, nullptr, false);

                        if(gy - ScreenRowsAvailable() - 1 > 0) {
                            gy -= ScreenRowsAvailable() - 1;
                        } else {
                            gy = 0;
                        }
                        SelectedGxAfterNextPaint = gx;
                        SelectedGyAfterNextPaint = gy;
                    }
                    TestSelections(msg, rung1);
                    break;

                case VK_NEXT:
                    rung1 = RungContainingSelected();
                    if(GetAsyncKeyState(VK_CONTROL) & 0x8000) {
                        ScrollPgDown();
                    } else {
                        int gx = 0, gy = 0;
                        FindSelected(&gx, &gy);

                        if(ScrollYOffset + ScreenRowsAvailable() - 1 < ScrollYOffsetMax) {
                            ScrollYOffset += ScreenRowsAvailable() - 1;
                        } else {
                            ScrollYOffset = ScrollYOffsetMax;
                        }
                        RefreshScrollbars();
                        InvalidateRect(MainWindow, nullptr, false);

                        if(gy + ScreenRowsAvailable() - 1 < totalHeightScrollbars - 1) {
                            gy += ScreenRowsAvailable() - 1;
                        } else {
                            gy = totalHeightScrollbars - 1;
                        }
                        SelectedGxAfterNextPaint = gx;
                        SelectedGyAfterNextPaint = gy;
                    }
                    TestSelections(msg, rung1);
                    break;
            }

            if(InSimulationMode) {
                switch(wParam) {
                    case ' ':
                        SimulateOneCycle(true);
                        break;

                    case VK_F8:
                        if(!RealTimeSimulationRunning)
                            StartSimulation();
                        else
                            StopSimulation();
                        break;

                    case 'R':
                        if(GetAsyncKeyState(VK_CONTROL) & 0x8000)
                            StartSimulation();
                        break;

                    case 'H':
                        if(GetAsyncKeyState(VK_CONTROL) & 0x8000)
                            StopSimulation();
                        break;

                    case VK_RETURN:
                    case VK_ESCAPE:
                        ToggleSimulationMode();
                        break;
                }
                break;
            }

            switch(wParam) {
                case VK_F5:
                    if(GetAsyncKeyState(VK_CONTROL) & 0x8000) {
                        CHANGING_PROGRAM(ShowConfDialog());
                    } else {
                        CompileProgram(false, MNU_COMPILE);
                    }
                    break;

                case VK_SPACE:
                    CHANGING_PROGRAM(ReplaceSelectedElement());
                    break;

                case VK_RETURN:
                    if(GetAsyncKeyState(VK_SHIFT) & 0x8000) {
                        CHANGING_PROGRAM(AddEmpty(ELEM_OPEN));
                    } else if(GetAsyncKeyState(VK_CONTROL) & 0x8000) {
                        CHANGING_PROGRAM(AddEmpty(ELEM_SHORT));
                    } else {
                        CHANGING_PROGRAM(EditSelectedElement());
                    }
                    break;

                case VK_DELETE:
                    if(GetAsyncKeyState(VK_SHIFT) & 0x8000) {
                        //CHANGING_PROGRAM(DeleteSelectedRung());
                        CHANGING_PROGRAM(CutRung());
                    } else {
                        CHANGING_PROGRAM(DeleteSelectedFromProgram());
                    }
                    break;

                case VK_OEM_1:
                    CHANGING_PROGRAM(AddComment(_("--add comment here--")));
                    break;

                case VK_INSERT:
                    if(GetAsyncKeyState(VK_SHIFT) & 0x8000) {
                        CHANGING_PROGRAM(PasteRung(0));
                    } else if(GetAsyncKeyState(VK_CONTROL) & 0x8000) {
                        CHANGING_PROGRAM(CopyRung());
                    } else
                        CHANGING_PROGRAM(CopyElem());
                    break;

                case 'C':
                    if(GetAsyncKeyState(VK_CONTROL) & 0x8000) {
                        CHANGING_PROGRAM(CopyRung());
                    } else if(GetAsyncKeyState(VK_SHIFT) & 0x8000) {
                        CHANGING_PROGRAM(AddContact(MNU_INSERT_CONT_RELAY));
                    } else {
                        CHANGING_PROGRAM(AddContact(MNU_INSERT_CONTACTS));
                    }
                    break;

                // TODO: rather country-specific here
                //case VK_DIVIDE: // use to ELEM_DIV
                case VK_OEM_2:
                    //case '/':
                    CHANGING_PROGRAM(AddEmpty(ELEM_ONE_SHOT_RISING));
                    break;

                case VK_OEM_5:
                    //case '\\':
                    CHANGING_PROGRAM(AddEmpty(ELEM_ONE_SHOT_FALLING));
                    break;

                case 'L':
                    if(GetAsyncKeyState(VK_SHIFT) & 0x8000)
                        CHANGING_PROGRAM(AddContact(MNU_INSERT_CONT_OUTPUT))
                    else
                        CHANGING_PROGRAM(AddCoil(MNU_INSERT_COIL));
                    break;

                case 'R':
                    CHANGING_PROGRAM(MakeResetOnlySelected());
                    break;

                case 'E':
                    if(GetAsyncKeyState(VK_CONTROL) & 0x8000) {
                        ExportDialog();
                    } else {
                        CHANGING_PROGRAM(AddReset());
                    }
                    break;

                case VK_F2: {
                    SaveProgram(MNU_SAVE);
                    UpdateMainWindowTitleBar();
                } break;

                case 'S':
                    if(GetAsyncKeyState(VK_CONTROL) & 0x8000) {
                        SaveProgram(MNU_SAVE);
                        UpdateMainWindowTitleBar();
                    } else {
                        CHANGING_PROGRAM(MakeSetOnlySelected());
                    }
                    break;

                case 'N':
                    if(GetAsyncKeyState(VK_CONTROL) & 0x8000) {
                        if(CheckSaveUserCancels())
                            break;
                        if(!ProgramChangedNotSaved) {
                            int r = MessageBox(MainWindow, _("Start new program?"), "LDmicro", MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION);
                            if(r == IDNO)
                                break;
                        }
                        NewProgram();
                        strcpy(CurrentSaveFile, "");
                        strcpy(CurrentCompileFile, "");
                        GenerateIoListDontLoseSelection();
                        RefreshScrollbars();
                        UpdateMainWindowTitleBar();
                    } else {
                        CHANGING_PROGRAM(NegateSelected());
                    }
                    break;
                /*
                case 'A':
                    CHANGING_PROGRAM(MakeNormalSelected());
                    break;
                */
                case 'T':
                    CHANGING_PROGRAM(AddTimer(ELEM_RTO));
                    break;

                case 'O':
                    if(GetAsyncKeyState(VK_CONTROL) & 0x8000) {
                        if(CheckSaveUserCancels())
                            break;
                        OpenDialog();
                    } else {
                        CHANGING_PROGRAM(AddTimer(ELEM_TON));
                    }
                    break;

                case 'F':
                    CHANGING_PROGRAM(AddTimer(ELEM_TOF));
                    break;

                case 'U':
                    CHANGING_PROGRAM(AddCounter(ELEM_CTU));
                    break;

                case 'I':
                    CHANGING_PROGRAM(AddCounter(ELEM_CTD));
                    break;

                case 'J':
                    CHANGING_PROGRAM(AddCounter(ELEM_CTC));
                    break;

                case 'K':
                    CHANGING_PROGRAM(AddCounter(ELEM_CTR));
                    break;

                case 'M':
                    CHANGING_PROGRAM(AddMove());
                    break;

                case 'P':
                    CHANGING_PROGRAM(AddSetPwm());
                    break;

                case 'A':
                    CHANGING_PROGRAM(AddReadAdc());
                    break;

                case '1':
                    if(GetAsyncKeyState(VK_SHIFT) & 0x8000)
                        CHANGING_PROGRAM(AddCmp(ELEM_NEQ)); // !
                    break;

                case VK_OEM_PLUS:
                    if(GetAsyncKeyState(VK_SHIFT) & 0x8000)
                        CHANGING_PROGRAM(AddMath(ELEM_ADD))
                    else
                        CHANGING_PROGRAM(AddCmp(ELEM_EQU));
                    break;

                case VK_OEM_MINUS:
                    if(GetAsyncKeyState(VK_SHIFT) & 0x8000) {
                    } else {
                        CHANGING_PROGRAM(AddMath(ELEM_SUB));
                    }
                    break;

                case VK_ADD:
                    CHANGING_PROGRAM(AddMath(ELEM_ADD));
                    break;

                case VK_SUBTRACT:
                    CHANGING_PROGRAM(AddMath(ELEM_SUB));
                    break;

                case VK_MULTIPLY:
                    CHANGING_PROGRAM(AddMath(ELEM_MUL));
                    break;

                case '8':
                    if(GetAsyncKeyState(VK_SHIFT) & 0x8000) {
                        CHANGING_PROGRAM(AddMath(ELEM_MUL));
                    }
                    break;

                case 'D':
                    if(GetAsyncKeyState(VK_CONTROL) & 0x8000) {
                        CHANGING_PROGRAM(CopyRungDown());
                    } else {
                        CHANGING_PROGRAM(AddMath(ELEM_DIV));
                    }
                    break;

                case VK_DIVIDE:
                    CHANGING_PROGRAM(AddMath(ELEM_DIV));
                    break;

                case VK_OEM_PERIOD:
                    if(GetAsyncKeyState(VK_SHIFT) & 0x8000) {
                        CHANGING_PROGRAM(AddCmp(ELEM_GRT));
                    } else {
                        CHANGING_PROGRAM(AddCmp(ELEM_GEQ));
                    }
                    break;

                case VK_OEM_COMMA:
                    if(GetAsyncKeyState(VK_SHIFT) & 0x8000) {
                        CHANGING_PROGRAM(AddCmp(ELEM_LES));
                    } else {
                        CHANGING_PROGRAM(AddCmp(ELEM_LEQ));
                    }
                    break;

                case 'V':
                    if(GetAsyncKeyState(VK_CONTROL) & 0x8000) {
                        CHANGING_PROGRAM(PasteRung(0));
                    } else if(GetAsyncKeyState(VK_SHIFT) & 0x8000) {
                        CHANGING_PROGRAM(InsertRung(true));
                    }
                    break;

                case '6':
                    if(GetAsyncKeyState(VK_SHIFT) & 0x8000) {
                        CHANGING_PROGRAM(InsertRung(false));
                    }
                    break;

                case 'Z':
                    if(GetAsyncKeyState(VK_CONTROL) & 0x8000) {
                        UndoUndo();
                    }
                    break;

                case 'Y':
                    if(GetAsyncKeyState(VK_CONTROL) & 0x8000) {
                        UndoRedo();
                    }
                    break;

                case 'X':
                    if(GetAsyncKeyState(VK_CONTROL) & 0x8000) {
                        CHANGING_PROGRAM(CutRung());
                    }
                    break;

                default:
                    break;
            }
            if(wParam != VK_SHIFT && wParam != VK_CONTROL) {
                InvalidateRect(MainWindow, nullptr, false);
            }
            break;
        }

        case WM_LBUTTONDBLCLK: {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            if(InSimulationMode) {
                EditElementMouseDoubleclick(x, y);
            } else {
                CHANGING_PROGRAM(EditElementMouseDoubleclick(x, y));
            }
            InvalidateRect(MainWindow, nullptr, false);
            break;
        }

        case WM_LBUTTONDOWN: {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            if((y > (IoListTop - 9)) && (y < (IoListTop + 3))) {
                POINT pt;
                pt.x = x;
                pt.y = y;
                ClientToScreen(MainWindow, &pt);
                MouseY = pt.y;
                MouseHookHandle = SetWindowsHookEx(WH_MOUSE_LL, (HOOKPROC)MouseHook, Instance, 0);
            }
            if(!InSimulationMode) {
                rung1 = RungContainingSelected();
                MoveCursorMouseClick(x, y);
                TestSelections(msg, rung1);
            }
            SetFocus(MainWindow);
            InvalidateRect(MainWindow, nullptr, false);
            break;
        }
        case WM_MOUSEMOVE: {
            //int x = LOWORD(lParam);
            int y = HIWORD(lParam);

            if((y > (IoListTop - 9)) && (y < (IoListTop + 3))) {
                SetCursor(LoadCursor(nullptr, IDC_SIZENS));
            } else {
                SetCursor(LoadCursor(nullptr, IDC_ARROW));
            }

            break;
        }
        case WM_MOUSEWHEEL: {
            if((GET_WHEEL_DELTA_WPARAM(wParam)) > 0) {
                VscrollProc(SB_LINEUP);
            } else {
                VscrollProc(SB_LINEDOWN);
            }
            break;
        }

        case WM_SIZE:
            MainWindowResized();
            break;

        case WM_NOTIFY: {
            NMHDR *h = (NMHDR *)lParam;
            if(h->hwndFrom == IoList) {
                IoListProc(h);
            }
            return 0;
        }
        case WM_VSCROLL:
            VscrollProc(wParam);
            break;

        case WM_HSCROLL:
            HscrollProc(wParam);
            break;

        case WM_COMMAND:
            ProcessMenu(LOWORD(wParam));
            InvalidateRect(MainWindow, nullptr, false);
            break;

        case WM_CLOSE:
        case WM_DESTROY:
            WipeIntMemory();
            if(CheckSaveUserCancels())
                break;

            PostQuitMessage(0);
            return 1;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 1;
}

//-----------------------------------------------------------------------------
// Create our window class; nothing exciting.
//-----------------------------------------------------------------------------
static ATOM MakeWindowClass()
{
    WNDCLASSEX wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(wc);

    wc.style = CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW | CS_OWNDC | CS_DBLCLKS;
    wc.lpfnWndProc = (WNDPROC)MainWndProc;
    wc.hInstance = Instance;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = "LDmicro";
    wc.lpszMenuName = nullptr;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = (HICON)LoadImage(Instance, MAKEINTRESOURCE(4000), IMAGE_ICON, 32, 32, 0);
    wc.hIconSm = (HICON)LoadImage(Instance, MAKEINTRESOURCE(4000), IMAGE_ICON, 16, 16, 0);

    return RegisterClassEx(&wc);
}

//-----------------------------------------------------------------------------
/*
static LPSTR _getNextCommandLineArgument(LPSTR lpBuffer)
{
    bool argFound = false;
    while(*lpBuffer) {
        if(isspace(*lpBuffer)) {
            argFound = false;

        } else if((*lpBuffer == '-') || (*lpBuffer == '/')) {
            argFound = true;
        } else if(argFound) {
            return lpBuffer;
        }

        lpBuffer++;
    }
    return nullptr;
}

//-----------------------------------------------------------------------------

static LPSTR _getNextPositionalArgument(LPSTR lpBuffer)
{
    bool argFound = false;
    while(*lpBuffer) {
        if(isspace(*lpBuffer)) {
            argFound = true;
        } else if((*lpBuffer == '-') || (*lpBuffer == '/')) {
            argFound = false;
        } else if(argFound) {
            return lpBuffer;
        }
        lpBuffer++;
    }
    return nullptr;
}

//-----------------------------------------------------------------------------

static char *_removeWhitespace(char *pBuffer)
{
    // Check from left:
    char *pStart = pBuffer;
    while(*pBuffer) {
        if(isspace(*pBuffer) || *pBuffer == '"' || *pBuffer == '\'') {
            pStart++;
        } else {
            break;
        }

        pBuffer++;
    }

    if(*pBuffer == 0) {
        // No alphanumeric characters in this string.
        return nullptr;
    }

    // Check from right.
    {
        int   len = strlen(pBuffer);
        char *pEnd = &pBuffer[len - 1];

        while(pEnd > pStart) {
            if(isspace(*pEnd) || *pEnd == '"' || *pEnd == '\'') {
                *pEnd = 0;
            } else {
                break;
            }

            pEnd--;
        }
    }

    if(strlen(pStart) == 0) {
        return nullptr;
    }

    return pStart;
}
*/
//-----------------------------------------------------------------------------
/*
static void _parseArguments(LPSTR lpCmdLine, char ** pSource, char ** pDest)
{
    // Parse for command line arguments.
    LPSTR lpArgs = lpCmdLine;

    while(1)
    {
        lpArgs = _getNextCommandLineArgument(lpArgs);

        if(lpArgs == nullptr)
        {
            break;
        }
        if(*lpArgs == 'c')
        {
            RunningInBatchMode = true;
        }
        if(*lpArgs == 't')
        {
            RunningInTestMode = true;
        }
    }



    // Parse for positional arguments (first is source, second destination):
    *pSource = nullptr;
    *pDest = nullptr;

    lpCmdLine = _getNextPositionalArgument(lpCmdLine);

    if(lpCmdLine)
    {
        *pSource = lpCmdLine;
        lpCmdLine = _getNextPositionalArgument(lpCmdLine);

        if(lpCmdLine)
        {
            lpCmdLine[-1] = 0;  // Close source string.
            *pDest = lpCmdLine;
            *pDest = _removeWhitespace(*pDest);
        }

        *pSource = _removeWhitespace(*pSource);
    }
}
*/
//---------------------------------------------------------------------------
void abortHandler(int signum)
{
    // associate each signal with a signal name string.
    const char *name = nullptr;
    // clang-format off
    switch(signum) {
        case SIGABRT: name = "SIGABRT";  break;
        case SIGSEGV: name = "SIGSEGV";  break;
      //case SIGBUS:  name = "SIGBUS";   break;
        case SIGILL:  name = "SIGILL";   break;
        case SIGFPE:  name = "SIGFPE";   break;
        case SIGINT:  name = "SIGINT";   break;
        case SIGTERM: name = "SIGTERM";  break;
    }
    // clang-format on

    // Notify the user which signal was caught. We use printf, because this is the
    // most basic output function. Once you get a crash, it is possible that more
    // complex output systems like streams and the like may be corrupted. So we
    // make the most basic call possible to the lowest level, most
    // standard print function.
    if(name)
        dbp(_("Caught signal %d (%s)\n"), signum, name);
    else
        dbp(_("Caught signal %d\n"), signum);

    // Dump a stack trace.
    // This is the function we will be implementing next.
    // printStackTrace();

    // If you caught one of the above signals, it is likely you just
    // want to quit your program right now.
    exit(signum);
}

//-----------------------------------------------------------------------------
void KxStackTrace()
{
    signal(SIGABRT, abortHandler);
    signal(SIGSEGV, abortHandler);
    //signal( SIGBUS,  abortHandler );
    signal(SIGILL, abortHandler);
    signal(SIGFPE, abortHandler);
    signal(SIGINT, abortHandler);
    signal(SIGTERM, abortHandler);
}

//-----------------------------------------------------------------------------
void CheckPwmPins()
{
    return;
    /*
    uint32_t j;
    for(uint32_t i = 0; i < supportedMcus().size(); i++) {
        for(j = 0; j < supportedMcus()[i].pwmCount; j++) {
            if(!supportedMcus()[i].pwmNeedsPin && supportedMcus()[i].pwmCount) {
                ooops("1 %s", supportedMcus()[i].mcuName);
            } else if(supportedMcus()[i].pwmNeedsPin)
                if(supportedMcus()[i].pwmNeedsPin == supportedMcus()[i].pwmInfo[j].pin)
                    break;
        }
        if(supportedMcus()[i].pwmCount)
            if(j >= supportedMcus()[i].pwmCount)
                ooops("2 %s", supportedMcus()[i].mcuName);
    }
    */
}

#ifndef LDMICRO_GUI_XX
//-----------------------------------------------------------------------------
// Entry point into the program.
//-----------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow)
{
    (void)hPrevInstance;
    (void)nCmdShow;
    auto logg = ldlog::getLogger("default");
    try {
        logg->add_sink(ldlog::newWindowsDebugStringSink());

        LOG(ldlog::Info, logg, "Run LDmicro ver.: {}.", LDMICRO_VER_STR);

        srand((int)time(0));

        if(LEN7SEG != arraylen(char7seg))
            oops();

        if(LEN9SEG != arraylen(char9seg))
            oops();

        if(LEN14SEG != arraylen(char14seg))
            oops();

        if(LEN16SEG != arraylen(char16seg))
            oops();
        if(arraylen(Schemes) != NUM_SUPPORTED_SCHEMES) {
            Error("arraylen(Schemes)=%d != NUM_SUPPORTED_SCHEMES=%d", arraylen(Schemes), NUM_SUPPORTED_SCHEMES);
            oops();
        }

        CheckPwmPins();

        GetModuleFileName(hInstance, ExePath, MAX_PATH);
        ExtractFilePath(ExePath);

        Instance = hInstance;

        setlocale(LC_ALL, "");
        //RunningInBatchMode = false;
        fillPcPinInfos();

        MakeWindowClass();
        MakeDialogBoxClass();
        HMENU top = MakeMainWindowMenus();

        MainWindow = CreateWindowEx(0, "LDmicro", "", WS_OVERLAPPED | WS_THICKFRAME | WS_CLIPCHILDREN | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU | WS_SIZEBOX, 10, 10, 800, 600, nullptr, top, Instance, nullptr);
        ThawWindowPos(MainWindow);
        IoListHeight = 100;
        ThawDWORD(IoListHeight);

        InitCommonControls();
        InitForDrawing();

        MakeMainWindowControls();
        MainWindowResized();

        NewProgram();
        strcpy(CurrentSaveFile, "");

        // Check if we're running in non-interactive mode; in that case we should
        // load the file, compile, and exit.
        while(isspace(*lpCmdLine)) {
            lpCmdLine++;
        }
        if(memcmp(lpCmdLine, "/c", 2) == 0) {
            RunningInBatchMode = true;

            const char *err = "Bad command line arguments: run 'ldmicro /c src.ld dest.ext'";

            char *source = lpCmdLine + 2;
            while(isspace(*source)) {
                source++;
            }
            if(*source == '\0') {
                Error(err);
                doexit(EXIT_FAILURE);
            }
            char *dest = source;
            while(!isspace(*dest) && *dest) {
                dest++;
            }
            if(*dest == '\0') {
                Error(err);
                doexit(EXIT_FAILURE);
            }
            *dest = '\0';
            dest++;
            while(isspace(*dest)) {
                dest++;
            }
            if(*dest == '\0') {
                Error(err);
                doexit(EXIT_FAILURE);
            }
            char *l, *r;
            if((l = strchr(dest, '.')) != (r = strrchr(dest, '.'))) {
                while(*r) {
                    *l = *r;
                    r++;
                    l++;
                }
                *l = '\0';
            }
            if(!LoadProjectFromFile(source)) {
                Error(_("Couldn't open '%s', running non-interactively."), source);
                doexit(EXIT_FAILURE);
            }
            strcpy(CurrentCompileFile, dest);
            GenerateIoList(-1);
            CompileProgram(false, compile_MNU);
            doexit(EXIT_SUCCESS);
        }
        if(memcmp(lpCmdLine, "/t", 2) == 0) {
            RunningInBatchMode = true;

            char exportFile[MAX_PATH];

            char *source = lpCmdLine + 2;
            while(isspace(*source)) {
                source++;
            }
            if(*source == '\0') {
                const char *err = "Bad command line arguments: run 'ldmicro /t src.ld [dest.txt]'";
                Error(err);
                doexit(EXIT_FAILURE);
            }

            char *dest = source;
            while(!isspace(*dest) && *dest) {
                dest++;
            }
            *dest = '\0';
            if(!LoadProjectFromFile(source)) {
                Error(_("Couldn't open '%s', running non-interactively."), source);
                doexit(EXIT_FAILURE);
            }
            strcpy(CurrentSaveFile, source);
            char *s;
            GetFullPathName(source, sizeof(CurrentSaveFile), CurrentSaveFile, &s);

            dest++;
            while(isspace(*dest)) {
                dest++;
            }
            if(*dest != '\0') {
                strcpy(exportFile, dest);
            } else {
                exportFile[0] = '\0';
                SetExt(exportFile, source, "txt");
            }
            GenerateIoList(-1);
            ExportDrawingAsText(exportFile);
            doexit(EXIT_SUCCESS);
        }

        // We are running interactively, or we would already have exited. We
        // can therefore show the window now, and otherwise set up the GUI.
        ShowWindow(MainWindow, SW_SHOW);
        SetTimer(MainWindow, TIMER_BLINK_CURSOR, 800, BlinkCursor);

        if(strlen(lpCmdLine) > 0) {
            char line[MAX_PATH];
            if(*lpCmdLine == '"') {
                strcpy(line, lpCmdLine + 1);
            } else {
                strcpy(line, lpCmdLine);
            }
            if(strchr(line, '"'))
                *strchr(line, '"') = '\0';

            if(!strchr(line, '.'))
                strcat(line, ".ld");

            char *s;
            GetFullPathName(line, sizeof(CurrentSaveFile), CurrentSaveFile, &s);

            bool res = false;
            try {
                res = LoadProjectFromFile(CurrentSaveFile);
            } catch(const std::exception &e) {
                Error(e.what());
            }
            if(!res) {
                NewProgram();
                Error(_("Couldn't open '%s'."), CurrentSaveFile);
                CurrentSaveFile[0] = '\0';
            }

            UndoFlush();
        }
        GenerateIoListDontLoseSelection();
        RefreshScrollbars();
        UpdateMainWindowTitleBar();

        MSG msg;
        while(GetMessage(&msg, nullptr, 0, 0) > 0) {
            if(msg.hwnd == IoList && msg.message == WM_KEYDOWN) {
                if(msg.wParam == VK_TAB) {
                    SetFocus(MainWindow);
                    continue;
                }
            }
            if(msg.message == WM_KEYDOWN && msg.wParam != VK_UP && msg.wParam != VK_NEXT && msg.wParam != VK_PRIOR && msg.wParam != VK_DOWN && msg.wParam != VK_RETURN && msg.wParam != VK_SHIFT) {
                if(msg.hwnd == IoList) {
                    msg.hwnd = MainWindow;
                    SetFocus(MainWindow);
                }
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        FreezeWindowPos(MainWindow);
        FreezeDWORD(IoListHeight);

        UndoEmpty();
        Prog.reset();

        return 0;
    } catch(std::runtime_error &e) {
        LOG_ERROR(logg, "Runtime error: \"{}\"", e.what());
        Prog.setMcu(nullptr);
        srand((unsigned int)time(nullptr));
        char fname[20];
        sprintf(fname, "tmpfile_%4.4d.ld", rand() % 10000);
        SaveProjectToFile(fname, MNU_SAVE_02);
        return EXIT_FAILURE;
    } catch(...) {

        LOG_ERROR(logg, "{}", "Receive unknown exception");
        ///// Added by JG to save work in case of big bug
        Prog.setMcu(nullptr);
        srand((unsigned int)time(nullptr));
        char fname[20];
        sprintf(fname, "tmpfile_%4.4d.ld", rand() % 10000);
        SaveProjectToFile(fname, MNU_SAVE_02);
        /////

        abortHandler(EXCEPTION_EXECUTE_HANDLER);
    };

    return 0;
}
#endif //LDMICRO_GUI_XX
