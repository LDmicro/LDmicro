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
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <shellapi.h>

#include "ldmicro.h"
#include "freeze.h"
#include "mcutable.h"
#include "git_commit.h"
#include "intcode.h"
#include "locale.h"

HINSTANCE   Instance;
HWND        MainWindow;
HDC         Hdc;

char ExePath[MAX_PATH];
char CurrentLdPath[MAX_PATH];

// parameters used to capture the mouse when implementing our totally non-
// general splitter control
static HHOOK       MouseHookHandle;
static int         MouseY;

// For the open/save dialog boxes
#define LDMICRO_PATTERN "LDmicro Ladder Logic Programs (*.ld)\0*.ld\0" \
                     "All files\0*\0\0"
char CurrentSaveFile[MAX_PATH]; // .ld
BOOL ProgramChangedNotSaved = FALSE;

ULONGLONG PrevWriteTime = 0;
ULONGLONG LastWriteTime = 0;

#define HEX_PATTERN  "Intel Hex Files (*.hex)\0*.hex\0All files\0*\0\0"
#define C_PATTERN "C Source Files (*.c)\0*.c\0All Files\0*\0\0"
#define INTERPRETED_PATTERN \
    "Interpretable Byte Code Files (*.int)\0*.int\0All Files\0*\0\0"
#define PASCAL_PATTERN "PASCAL Source Files (*.pas)\0*.pas\0All Files\0*\0\0"
#define ARDUINO_C_PATTERN "ARDUINO C Source Files (*.cpp)\0*.cpp\0All Files\0*\0\0"
#define XINT_PATTERN \
    "Extended Byte Code Files (*.xint)\0*.xint\0All Files\0*\0\0"
char CurrentCompileFile[MAX_PATH]; //.hex, .asm, ...
char CurrentCompilePath[MAX_PATH];

#define TXT_PATTERN  "Text Files (*.txt)\0*.txt\0All files\0*\0\0"

// Everything relating to the PLC's program, I/O configuration, processor
// choice, and so on--basically everything that would be saved in the
// project file.
PlcProgram Prog;

//-----------------------------------------------------------------------------
// Get a filename with a common dialog box and then save the program to that
// file and then set our default filename to that.
//-----------------------------------------------------------------------------
static BOOL SaveAsDialog(void)
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
        return FALSE;

    if(!SaveProjectToFile(CurrentSaveFile, MNU_SAVE)) {
        Error(_("Couldn't write to '%s'."), CurrentSaveFile);
        return FALSE;
    } else {
        ProgramChangedNotSaved = FALSE;
        RefreshControlsToSettings();
        return TRUE;
    }
}

//---------------------------------------------------------------------------
char *ExtractFileDir(char *dest) // without last backslash
{
    char *c;
    if(strlen(dest)) {
        c = strrchr(dest,'\\');
        if(c)
            *c = '\0';
    };
    return dest;
}

char *ExtractFilePath(char *dest) // with last backslash
{
    char *c;
    if(strlen(dest)) {
        c = strrchr(dest,'\\');
        if(c)
            c[1] = '\0';
    };
    return dest;
}

//---------------------------------------------------------------------------
char *ExtractFileName(char *src) // with .ext
{
    char *c;
    if(strlen(src)) {
        c = strrchr(src,'\\');
        if(c)
            return &c[1];
    }
    return src;
}

//---------------------------------------------------------------------------
char *GetFileName(char *dest, char *src) // without .ext
{
    dest[0] = '\0';
    char *c;
    strcpy(dest, ExtractFileName(src));
    if(strlen(dest)) {
        c = strrchr(dest,'.');
        if(c)
            c[0] = '\0';
    }
    return dest;
}

//-----------------------------------------------------------------------------
char *SetExt(char *dest, const char *src, const char *ext)
{
    char *c;
    if(dest != src)
      if(strlen(src))
        strcpy(dest, src);
    if(strlen(dest)) {
        c = strrchr(dest,'.');
        if(c)
            c[0] = '\0';
    };
    if(!strlen(dest))
        strcat(dest, "new");

    if(strlen(ext))
        if(!strchr(ext,'.'))
            strcat(dest, ".");

    return strcat(dest, ext);
}

//-----------------------------------------------------------------------------
// Get a filename with a common dialog box and then export the program as
// an ASCII art drawing.
//-----------------------------------------------------------------------------
static void ExportDialog(void)
{
    char exportFile[MAX_PATH];
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
        return;

    ExportDrawingAsText(exportFile);
}

//-----------------------------------------------------------------------------
// If we already have a filename, save the program to that. Otherwise same
// as Save As. Returns TRUE if it worked, else returns FALSE.
//-----------------------------------------------------------------------------
static BOOL SaveProgram(int code)
{
    if(strlen(CurrentSaveFile)) {
        if(!SaveProjectToFile(CurrentSaveFile, code)) {
            Error(_("Couldn't write to '%s'."), CurrentSaveFile);
            return FALSE;
        } else {
            ProgramChangedNotSaved = FALSE;
            RefreshControlsToSettings();
            return TRUE;
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
        return TRUE;
    }
    return FALSE;
}
//-----------------------------------------------------------------------------
long int fsize(FILE *fp)
{
    long int prev=ftell(fp);
    fseek(fp, 0L, SEEK_END);
    long int sz=ftell(fp);
    fseek(fp,prev,SEEK_SET); //go back to where we were
    return sz;
}

long int fsize(char *filename)
{
    FILE *fp;
    fp=fopen(filename,"rb");
    if(fp==NULL) {
        return 0;
    }
    fseek(fp, 0L, SEEK_END);
    long int sz=ftell(fp);
    fclose(fp);
    return sz;
}

//-----------------------------------------------------------------------------
static void isErr(int Err, char *r)
{
  char *s;
  switch(Err){
    case 0:s="The system is out of memory or resources"; break;
    case ERROR_BAD_FORMAT:s="The .exe file is invalid"; break;
    case ERROR_FILE_NOT_FOUND:s="The specified file was not found"; break;
    case ERROR_PATH_NOT_FOUND:s="The specified path was not found"; break;
    default:s=""; break;
  }
  if(strlen(s))
      Error("Error: %d - %s in command line:\n\n%s",Err, s, r);
}

//-----------------------------------------------------------------------------
static int Execute(char *r)
{
   return WinExec(r, SW_SHOWNORMAL/* | SW_SHOWMINIMIZED*/);
}

//-----------------------------------------------------------------------------
char *GetIsaName(int ISA)
{
    switch(ISA) {
        case ISA_AVR         : return (char *)stringer( ISA_AVR         ) + 4;
        case ISA_PIC16       : return (char *)stringer( ISA_PIC16       ) + 4;
        case ISA_ANSIC       : return (char *)stringer( ISA_ANSIC       ) + 4;
        case ISA_INTERPRETED : return (char *)stringer( ISA_INTERPRETED ) + 4;
        case ISA_XINTERPRETED: return (char *)stringer( ISA_XINTERPRETED) + 4;
        case ISA_NETZER      : return (char *)stringer( ISA_NETZER      ) + 4;
        case ISA_PASCAL      : return (char *)stringer( ISA_PASCAL      ) + 4;
        case ISA_ARDUINO     : return (char *)stringer( ISA_ARDUINO     ) + 4;
        case ISA_CAVR        : return (char *)stringer( ISA_CAVR        ) + 4;
        default              : oops(); return NULL;
    }
}

//-----------------------------------------------------------------------------
static void flashBat(char *name, int ISA)
{
    char s[MAX_PATH];
    char r[MAX_PATH];

    if(strlen(name) == 0) {
        Error(_(" Save ld before flash."));
        return;
    }

    s[0] = '\0';
    SetExt(s, name, "");
    sprintf(r,"\"%sflashMcu.bat\" %s \"%s\"",ExePath,GetIsaName(ISA),s);

    isErr(Execute(r), r);
}

//-----------------------------------------------------------------------------
static void readBat(char *name, int ISA)
{
    char s[MAX_PATH];
    char r[MAX_PATH];

    if(strlen(name) == 0) {
        name = "read";
    }

    s[0] = '\0';
    SetExt(s, name, "");
    sprintf(r,"\"%sreadMcu.bat\" %s \"%s\"",ExePath,GetIsaName(ISA),s);

    isErr(Execute(r), r);
}

//-----------------------------------------------------------------------------
static void notepad(char *name, char *ext)
{
    char s[MAX_PATH]="";
    char r[MAX_PATH];

    s[0] = '\0';
    SetExt(s, name, ext);
    sprintf(r,"\"%snotepad.bat\" \"%s\"",ExePath,s);

    isErr(Execute(r), r);
}

//-----------------------------------------------------------------------------
static void postCompile(int ISA)
{
    if(!ExistFile(CurrentCompileFile))
        return;

    if(!fsize(CurrentCompileFile)) {
        remove(CurrentCompileFile);

        if(strstr(CurrentCompileFile,".hex")) {
            char outFile[MAX_PATH];
            SetExt(outFile, CurrentCompileFile, ".asm");
            remove(outFile);
        }
        if(strstr(CurrentCompileFile,".c")) {
            char outFile[MAX_PATH];
            SetExt(outFile, CurrentCompileFile, ".h");
            remove(outFile);
          //remove("ladder.h_");
        }
        return;
    }

    char r[MAX_PATH];
    char onlyName[MAX_PATH];

    sprintf(r,"%spostCompile.bat", ExePath);
    if(!ExistFile(r))
        return;

    strcpy(onlyName, ExtractFileName(CurrentSaveFile));
    SetExt(onlyName, onlyName, "");

    sprintf(r,"\"%spostCompile.bat\" %s \"%s\" \"%s\"", ExePath, GetIsaName(ISA), CurrentLdPath, onlyName);
    isErr(Execute(r), r);
}

//-----------------------------------------------------------------------------
// Compile the program to a hex file for the target micro. Get the output
// file name if necessary, then call the micro-specific compile routines.
//-----------------------------------------------------------------------------
static void CompileProgram(BOOL compileAs, int compile_MNU)
{
    if(compile_MNU == MNU_COMPILE){
        if(strstr(CurrentCompileFile,".cpp"))
            compile_MNU = MNU_COMPILE_ARDUINO;
        else if(strstr(CurrentCompileFile,".ino"))
            compile_MNU = MNU_COMPILE_ARDUINO;
        else if(strstr(CurrentCompileFile,".c"))
            compile_MNU = MNU_COMPILE_ANSIC;
        else if(strstr(CurrentCompileFile,".pas"))
            compile_MNU = MNU_COMPILE_PASCAL;
        else if (strstr(CurrentCompileFile, ".xint"))
            compile_MNU = MNU_COMPILE_XINT;
        else
            compile_MNU = MNU_COMPILE_IHEX;
    }

    IsOpenAnable:
    if(!compileAs && strlen(CurrentCompileFile)) {
      if( (compile_MNU == MNU_COMPILE      )  && strstr(CurrentCompileFile,".hex")
      ||  (compile_MNU == MNU_COMPILE_IHEX )  && strstr(CurrentCompileFile,".hex")
      ||  (compile_MNU == MNU_COMPILE_ANSIC)  && strstr(CurrentCompileFile,".c"  )
      ||  (compile_MNU == MNU_COMPILE_ARDUINO)&& strstr(CurrentCompileFile,".cpp")
      ||  (compile_MNU == MNU_COMPILE_PASCAL) && strstr(CurrentCompileFile,".pas")
      ||  (compile_MNU == MNU_COMPILE_XINT)   && strstr(CurrentCompileFile, ".xint")
      ) {
        if(FILE *f = fopen(CurrentCompileFile, "w")) {
            fclose(f);
            remove(CurrentCompileFile);
        } else {
            Error(_("Couldn't open file '%s'"), CurrentCompileFile);
            compileAs = TRUE;
        }
      }
    }

    if(compileAs || (strlen(CurrentCompileFile)==0)
      ||  (compile_MNU == MNU_COMPILE_AS)
      ||( (compile_MNU == MNU_COMPILE      )  && (!strstr(CurrentCompileFile,".hex")) )
      ||( (compile_MNU == MNU_COMPILE_IHEX )  && (!strstr(CurrentCompileFile,".hex")) )
      ||( (compile_MNU == MNU_COMPILE_ANSIC)  && (!strstr(CurrentCompileFile,".c"  )) )
      ||( (compile_MNU == MNU_COMPILE_ARDUINO)&& (!strstr(CurrentCompileFile,".cpp")) )
      ||( (compile_MNU == MNU_COMPILE_PASCAL) && (!strstr(CurrentCompileFile,".pas")) )
      ||( (compile_MNU == MNU_COMPILE_XINT)   && (!strstr(CurrentCompileFile, ".xint")) )
      ) {
        char *c;
        OPENFILENAME ofn;

        memset(&ofn, 0, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hInstance = Instance;
        ofn.lpstrTitle = _("Compile To");
        if((compile_MNU == MNU_COMPILE_ANSIC) ||
           (Prog.mcu && Prog.mcu->whichIsa == ISA_ANSIC)) {
            ofn.lpstrFilter = C_PATTERN;
            ofn.lpstrDefExt = "c";
            c = "c";
            compile_MNU = MNU_COMPILE_ANSIC;
        } else if(Prog.mcu && (Prog.mcu->whichIsa == ISA_INTERPRETED ||
                               Prog.mcu->whichIsa == ISA_NETZER)) {
            ofn.lpstrFilter = INTERPRETED_PATTERN;
            ofn.lpstrDefExt = "int";
            c = "int";
        } else if ((compile_MNU == MNU_COMPILE_XINT) ||
            (Prog.mcu && Prog.mcu->whichIsa == ISA_XINTERPRETED)) {
            ofn.lpstrFilter = XINT_PATTERN;
            ofn.lpstrDefExt = "xint";
            c = "xint";
            compile_MNU = MNU_COMPILE_XINT;
        } else if((compile_MNU == MNU_COMPILE_PASCAL) ||
                  (Prog.mcu && Prog.mcu->whichIsa == ISA_PASCAL)) {
            ofn.lpstrFilter = PASCAL_PATTERN;
            ofn.lpstrDefExt = "pas";
            c = "pas";
            compile_MNU = MNU_COMPILE_PASCAL;
        } else if((compile_MNU == MNU_COMPILE_ARDUINO) ||
                  (Prog.mcu && Prog.mcu->whichIsa == ISA_ARDUINO)) {
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

        strcpy(CurrentCompilePath,CurrentCompileFile);
        ExtractFileDir(CurrentCompilePath);

        // hex output filename is stored in the .ld file
        ProgramChangedNotSaved = TRUE;
        compileAs = FALSE;
        goto IsOpenAnable;
    }

    if(!GenerateIntermediateCode()) return;

    if((Prog.mcu == NULL)
    && (compile_MNU != MNU_COMPILE_PASCAL)
    && (compile_MNU != MNU_COMPILE_ANSIC)
    && (compile_MNU != MNU_COMPILE_ARDUINO)
    && (compile_MNU != MNU_COMPILE_XINT)) {
        Error(_("Must choose a target microcontroller before compiling."));
        return;
    }

    if((UartFunctionUsed() && (Prog.mcu) && Prog.mcu->uartNeeds.rxPin == 0)
    && (compile_MNU != MNU_COMPILE_PASCAL)
    && (compile_MNU != MNU_COMPILE_ANSIC)
    && (compile_MNU != MNU_COMPILE_ARDUINO)) {
        Error(_("UART function used but not supported for this micro."));
        return;
    }

    if((PwmFunctionUsed() && (Prog.mcu) && (Prog.mcu->pwmCount == 0) && Prog.mcu->pwmNeedsPin == 0)
    && (compile_MNU != MNU_COMPILE_PASCAL)
    && (compile_MNU != MNU_COMPILE_ANSIC)
    && (compile_MNU != MNU_COMPILE_ARDUINO)
    && (compile_MNU != MNU_COMPILE_XINT)
    && (Prog.mcu->whichIsa != ISA_XINTERPRETED)) {
        Error(_("PWM function used but not supported for this micro."));
        return;
    }
    if (compile_MNU == MNU_COMPILE_ANSIC) {
        CompileAnsiC(CurrentCompileFile);
        postCompile(ISA_ANSIC);
    } else if (compile_MNU == MNU_COMPILE_ARDUINO) {
        CompileAnsiC(CurrentCompileFile, ISA_ARDUINO);
        postCompile(ISA_ARDUINO);
    } else if (compile_MNU == MNU_COMPILE_XINT) {
        CompileXInterpreted(CurrentCompileFile);
        postCompile(ISA_XINTERPRETED);
    } else if (Prog.mcu) {
        switch(Prog.mcu->whichIsa) {
            case ISA_AVR:           CompileAvr(CurrentCompileFile); break;
            case ISA_PIC16:         CompilePic16(CurrentCompileFile); break;
            case ISA_ANSIC:         CompileAnsiC(CurrentCompileFile); break;
            case ISA_INTERPRETED:   CompileInterpreted(CurrentCompileFile); break;
            case ISA_XINTERPRETED:  CompileXInterpreted(CurrentCompileFile); break;
            case ISA_NETZER:        CompileNetzer(CurrentCompileFile); break;
            case ISA_ARDUINO:       CompileAnsiC(CurrentCompileFile, ISA_ARDUINO); break;
            default: ooops("0x%X", Prog.mcu->whichIsa);
        }
        postCompile(Prog.mcu->whichIsa);
    } else oops();

    RefreshControlsToSettings();
}

//-----------------------------------------------------------------------------
// If the program has been modified then give the user the option to save it
// or to cancel the operation they are performing. Return TRUE if they want
// to cancel.
//-----------------------------------------------------------------------------
BOOL CheckSaveUserCancels(void)
{
    if(!ProgramChangedNotSaved) {
        // no problem
        return FALSE;
    }

    int r = MessageBox(MainWindow,
        _("The program has changed since it was last saved.\r\n\r\n"
        "Do you want to save the changes?"), "LDmicro",
        MB_YESNOCANCEL | MB_ICONWARNING);
    switch(r) {
        case IDYES:
            if(SaveProgram(MNU_SAVE))
                return FALSE;
            else
                return TRUE;

        case IDNO:
            return FALSE;

        case IDCANCEL:
            return TRUE;

        default:
            oops();
            return FALSE;
    }
}

//-----------------------------------------------------------------------------
// Load a new program from a file. If it succeeds then set our default filename
// to that, else we end up with an empty file then.
//-----------------------------------------------------------------------------
static void OpenDialog(void)
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
        ProgramChangedNotSaved = FALSE;
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
void ProgramChanged(void)
{
    ProgramChangedNotSaved = TRUE;
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
                    if(IoListHeight < 50) IoListHeight = 50;
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
    if(code >= MNU_PROCESSOR_0 && code < MNU_PROCESSOR_0+NUM_SUPPORTED_MCUS) {
        strcpy(CurrentCompileFile, "");
        Prog.mcu = &SupportedMcus[code - MNU_PROCESSOR_0];
        RefreshControlsToSettings();
        return;
    }
    if(code == MNU_PROCESSOR_0+NUM_SUPPORTED_MCUS) {
        Prog.mcu = NULL;
        strcpy(CurrentCompileFile, "");
        RefreshControlsToSettings();
        return;
    }

    switch(code) {
        case MNU_NEW:
            if(CheckSaveUserCancels()) break;
            NewProgram();
            strcpy(CurrentSaveFile, "");
            strcpy(CurrentCompileFile, "");
            GenerateIoListDontLoseSelection();
            RefreshScrollbars();
            UpdateMainWindowTitleBar();
            break;

        case MNU_OPEN:
            if(CheckSaveUserCancels()) break;
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

        case MNU_FLASH_BAT:
            flashBat(CurrentSaveFile, Prog.mcu ? Prog.mcu->whichIsa : 0);
            break;

        case MNU_READ_BAT:
            readBat(CurrentSaveFile, Prog.mcu ? Prog.mcu->whichIsa : 0);
            break;

        case MNU_NOTEPAD_TXT:
            notepad(CurrentSaveFile, "txt");
            break;

        case MNU_NOTEPAD_HEX:
            notepad(CurrentSaveFile, "hex");
            break;

        case MNU_NOTEPAD_ASM:
            notepad(CurrentSaveFile, "asm");
            break;

        case MNU_NOTEPAD_C:
            notepad(CurrentSaveFile, "c");
            break;

        case MNU_NOTEPAD_H:
            notepad(CurrentSaveFile, "h");
            break;

        case MNU_NOTEPAD_PAS:
            notepad(CurrentSaveFile, "pas");
            break;

        case MNU_NOTEPAD_LD:
            if(CheckSaveUserCancels()) break;
            notepad(CurrentSaveFile, "ld");
            break;

        case MNU_NOTEPAD_PL:
            notepad(CurrentSaveFile, "pl");
            break;

        case MNU_EXIT:
            if(CheckSaveUserCancels()) break;
            PostQuitMessage(0);
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

        case MNU_INSERT_DELAY:
            CHANGING_PROGRAM(AddDelay());
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

        case MNU_INSERT_STRING:
            CHANGING_PROGRAM(AddString());
            break;

        case ELEM_CPRINTF:
        case ELEM_SPRINTF:
        case ELEM_FPRINTF:
        case ELEM_PRINTF:
        case ELEM_I2C_CPRINTF:
        case ELEM_ISP_CPRINTF:
        case ELEM_UART_CPRINTF:
            CHANGING_PROGRAM(AddPrint(code));
            break;

        case MNU_INSERT_OSR:
            CHANGING_PROGRAM(AddEmpty(ELEM_ONE_SHOT_RISING));
            break;

        case MNU_INSERT_OSF:
            CHANGING_PROGRAM(AddEmpty(ELEM_ONE_SHOT_FALLING));
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

        case MNU_INSERT_SWAP:
            CHANGING_PROGRAM(AddBcd(ELEM_SWAP));
            break;

        case MNU_INSERT_BUS:
            CHANGING_PROGRAM(AddBus(ELEM_BUS));
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

        case MNU_INSERT_PWM_OFF:
            CHANGING_PROGRAM(AddEmpty(ELEM_PWM_OFF));
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

        case MNU_INSERT_UART_SENDn:
            CHANGING_PROGRAM(AddUart(ELEM_UART_SENDn));
            break;

        case MNU_INSERT_UART_RECVn:
            CHANGING_PROGRAM(AddUart(ELEM_UART_RECVn));
            break;

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
            case MNU_INSERT_SET_BIT      : elem = ELEM_SET_BIT     ; goto bit_ops;
            case MNU_INSERT_CLEAR_BIT    : elem = ELEM_CLEAR_BIT   ; goto bit_ops;
            case MNU_INSERT_IF_BIT_SET   : elem = ELEM_IF_BIT_SET  ; goto bit_ops;
            case MNU_INSERT_IF_BIT_CLEAR : elem = ELEM_IF_BIT_CLEAR; goto bit_ops;
bit_ops:
                CHANGING_PROGRAM(AddBitOps(elem));
                break;
        }

        {
            int elem;
            case MNU_INSERT_ADD: elem = ELEM_ADD; goto math;
            case MNU_INSERT_SUB: elem = ELEM_SUB; goto math;
            case MNU_INSERT_MUL: elem = ELEM_MUL; goto math;
            case MNU_INSERT_DIV: elem = ELEM_DIV; goto math;
            case MNU_INSERT_MOD: elem = ELEM_MOD; goto math;
            case MNU_INSERT_AND: elem = ELEM_AND; goto math;
            case MNU_INSERT_OR : elem = ELEM_OR ; goto math;
            case MNU_INSERT_XOR: elem = ELEM_XOR; goto math;
            case MNU_INSERT_NOT: elem = ELEM_NOT; goto math;
            case MNU_INSERT_NEG: elem = ELEM_NEG; goto math;
            case MNU_INSERT_SHL: elem = ELEM_SHL; goto math;
            case MNU_INSERT_SHR: elem = ELEM_SHR; goto math;
            case MNU_INSERT_SR0: elem = ELEM_SR0; goto math;
            case MNU_INSERT_ROL: elem = ELEM_ROL; goto math;
            case MNU_INSERT_ROR: elem = ELEM_ROR; goto math;
math:
                CHANGING_PROGRAM(AddMath(elem));
                break;
        }

        // Special function register
        {
            int esfr;
            case MNU_INSERT_SFR: esfr = ELEM_RSFR; goto jcmp;
            case MNU_INSERT_SFW: esfr = ELEM_WSFR; goto jcmp;
            case MNU_INSERT_SSFB: esfr = ELEM_SSFR; goto jcmp;
            case MNU_INSERT_csFB: esfr = ELEM_CSFR; goto jcmp;
            case MNU_INSERT_TSFB: esfr = ELEM_TSFR; goto jcmp;
            case MNU_INSERT_T_C_SFB: esfr = ELEM_T_C_SFR; goto jcmp;
jcmp:
                CHANGING_PROGRAM(AddSfr(esfr));
                break;
        }
        // Special function register

        {
            int elem;
            case MNU_INSERT_EQU: elem = ELEM_EQU; goto cmp;
            case MNU_INSERT_NEQ: elem = ELEM_NEQ; goto cmp;
            case MNU_INSERT_GRT: elem = ELEM_GRT; goto cmp;
            case MNU_INSERT_GEQ: elem = ELEM_GEQ; goto cmp;
            case MNU_INSERT_LES: elem = ELEM_LES; goto cmp;
            case MNU_INSERT_LEQ: elem = ELEM_LEQ; goto cmp;
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
            CHANGING_PROGRAM(InsertRung(FALSE));
            break;

        case MNU_INSERT_RUNG_AFTER:
            CHANGING_PROGRAM(InsertRung(TRUE));
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
            if(i >= 0)
                if(Prog.rungSelected[i] == ' ')
                    Prog.rungSelected[i] = '*';
                else
                    Prog.rungSelected[i] = ' ';
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
            SimulateOneCycle(TRUE);
            break;

        case MNU_COMPILE_ANSIC:
        case MNU_COMPILE_IHEX:
        case MNU_COMPILE_ARDUINO:
        case MNU_COMPILE:
        case MNU_COMPILE_XINT:
            CompileProgram(FALSE, code);
            break;

        case MNU_PROCESSOR_NEW_PIC12:
            ShellExecute(0,"open","https://github.com/LDmicro/LDmicro/wiki/HOW-TO:-Soft-start-and-smooth-stop-of-LED-with-software-PWM",NULL,NULL,SW_SHOWNORMAL);
            break;

        case MNU_PROCESSOR_NEW:
            ShellExecute(0,"open","https://github.com/LDmicro/LDmicro/wiki/TODO-&-DONE",NULL,NULL,SW_SHOWNORMAL);
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
"On the question of payment, please contact LDmicro.GitHub@gmail.com.\n"
    );
            break;

        case MNU_COMPILE_AS:
            CompileProgram(TRUE, code);
            break;

        case MNU_MANUAL:
            ShowHelpDialog(FALSE);
            break;

        case MNU_SCHEME_BLACK:
        case MNU_SCHEME_WHITE:
        case MNU_SCHEME_SYS:
        case MNU_SCHEME_USER:
            scheme = code & 0xff;
            InitForDrawing();
            InvalidateRect(MainWindow, NULL, FALSE);
            RefreshControlsToSettings();
            break;

        case MNU_SELECT_COLOR:
            ShowColorDialog();
            break;

        case MNU_ABOUT:
            ShowHelpDialog(TRUE);
            break;

        case MNU_HOW:
            ShellExecute(0,"open","https://github.com/LDmicro/LDmicro/wiki/HOW-TO",NULL,NULL,SW_SHOWNORMAL);
            break;

        case MNU_FORUM:
            ShellExecute(0,"open","http://cq.cx/ladder-forum.pl",NULL,NULL,SW_SHOWNORMAL);
            break;

        case MNU_CHANGES:
            ShellExecute(0,"open","https://raw.githubusercontent.com/LDmicro/LDmicro/master/ldmicro/CHANGES.txt",NULL,NULL,SW_SHOWNORMAL);
            break;

        case MNU_EMAIL:
            ShellExecute(0,"open","mailto:LDmicro.GitHub@gmail.com",NULL,NULL,SW_SHOWNORMAL);
            break;

        case MNU_EXPLORE_DIR:
            ////ShellExecute(0, "open", CurrentLdPath, NULL, NULL, SW_SHOWNORMAL);
            ShellExecute(0, "explore", CurrentLdPath, NULL, NULL, SW_SHOWNORMAL);
            //ShellExecute(0, "find", CurrentLdPath, NULL, NULL, 0);
            break;

        case MNU_RELEASE:
            ShellExecute(0,"open","https://github.com/LDmicro/LDmicro/releases",NULL,NULL,SW_SHOWNORMAL);
            char str[1024];
            sprintf(str,"Date: %s\n\nSHA-1: %s\n\n"
                "Compiled: " __TIME__ " " __DATE__ ".",
                git_commit_date, git_commit_str);
            MessageBox(MainWindow, str, _("Release"),
                MB_OK | MB_ICONINFORMATION);
            break;
    }
}

//-----------------------------------------------------------------------------
void ScrollUp()
{
    if(ScrollYOffset > 0)
        ScrollYOffset--;
    RefreshScrollbars();
    InvalidateRect(MainWindow, NULL, FALSE);

    int gx=0, gy=0;
    if (!InSimulationMode && FindSelected(&gx, &gy)) {
      if (gy>ScrollYOffset+ScreenRowsAvailable()-1) {
        gy=ScrollYOffset+ScreenRowsAvailable()-1;
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
    InvalidateRect(MainWindow, NULL, FALSE);

    int gx=0, gy=0;
    if (!InSimulationMode && FindSelected(&gx, &gy)) {
      if (gy<ScrollYOffset) {
        gy=ScrollYOffset;
        MoveCursorNear(&gx, &gy);
      }
    }
}
//-----------------------------------------------------------------------------
void ScrollPgUp()
{
    int gx=0, gy=0;
    FindSelected(&gx, &gy);

    ScrollYOffset = 0;
    RefreshScrollbars();
    InvalidateRect(MainWindow, NULL, FALSE);

    SelectedGxAfterNextPaint = gx;
    SelectedGyAfterNextPaint = 0;
}
//-----------------------------------------------------------------------------
void ScrollPgDown()
{
    int gx=0, gy=0;
    FindSelected(&gx, &gy);

    ScrollYOffset = ScrollYOffsetMax;
    RefreshScrollbars();
    InvalidateRect(MainWindow, NULL, FALSE);

    SelectedGxAfterNextPaint = gx;
    SelectedGyAfterNextPaint = totalHeightScrollbars-1;
}
//-----------------------------------------------------------------------------
void RollHome()
{
    int gx=0, gy=0;
    if (FindSelected(&gx, &gy)) {
      gy=ScrollYOffset;
      MoveCursorNear(&gx, &gy);
    }
}
//-----------------------------------------------------------------------------
void RollEnd()
{
    int gx=0, gy=0;
    if (FindSelected(&gx, &gy)) {
      gy=ScrollYOffset+ScreenRowsAvailable()-1;
      MoveCursorNear(&gx, &gy);
    }
}
//-----------------------------------------------------------------------------
void TestSelections(UINT msg, int rung1)
{
    int rung2;
    if(SelectedGyAfterNextPaint>=0)
        rung2 = SelectedGyAfterNextPaint;
    else
        rung2 = RungContainingSelected();

    int i;
    switch (msg) {
        case WM_LBUTTONDOWN: {
            if(GetAsyncKeyState(VK_SHIFT) & 0x8000) {
                 if((rung1 >= 0) && (rung2 >= 0)) {
                      if(!(GetAsyncKeyState(VK_CONTROL) & 0x8000))
                         for(i = 0; i < Prog.numRungs; i++)
                             if(Prog.rungSelected[i] == '*')
                                 Prog.rungSelected[i] = ' ';
                      int d = (rung2 < rung1) ? -1 : +1;
                      for(i = rung1; ; i += d) {
                         Prog.rungSelected[i] = '*';
                         if(i==rung2)
                             break;
                      }
                 }
            } else if(GetAsyncKeyState(VK_CONTROL) & 0x8000) {
                 if((rung2 >= 0)) {
                      if(Prog.rungSelected[rung2]==' ')
                          Prog.rungSelected[rung2] = '*';
                      else
                          Prog.rungSelected[rung2] = ' ';
                 }
            } else {
                for(i = 0; i < Prog.numRungs; i++)
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
                      int i;
                      int d = (rung2 < rung1) ? -1 : +1;
                      for(i = rung1; ; i += d) {
                         Prog.rungSelected[i] = '*';
                         if(i==rung2)
                             break;
                      }
                 }
            } else {
                for(i = 0; i < Prog.numRungs; i++)
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
        if((LastWriteTime & PrevWriteTime)
        && (LastWriteTime != PrevWriteTime)) {
             tGetLastWriteTime(CurrentSaveFile, (PFILETIME)&LastWriteTime);
             PrevWriteTime = LastWriteTime;

             char buf[1024];
             sprintf(buf, _("File '%s' modified by another application.\r\n"
                 "Its disk timestamp is newer then the editor one.\n"
                 "Reload from disk?"), CurrentSaveFile);
             int r = MessageBox(MainWindow,
                 buf, "LDmicro",
                 MB_YESNO | MB_ICONWARNING);
             switch(r) {
                 case IDYES:
                     if(!LoadProjectFromFile(CurrentSaveFile)) {
                         Error(_("Couldn't reload '%s'."), CurrentSaveFile);
                     } else {
                         ProgramChangedNotSaved = FALSE;
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
    switch (msg) {
        case WM_ERASEBKGND:
            break;

        case WM_SETFOCUS:
            InvalidateRect(MainWindow, NULL, FALSE);
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
                        notepad(CurrentSaveFile, "asm");
                        return 1;
                    }
                break;

                case VK_F5:
                    if(GetAsyncKeyState(VK_ALT) & 0x8000) {
                        notepad(CurrentSaveFile, "pl");;
                        return 1;
                    }
                break;

                case VK_F6:
                    if(GetAsyncKeyState(VK_ALT) & 0x8000) {
                        notepad(CurrentSaveFile, "hex");
                        return 1;
                    }
                break;

                case VK_F7:
                    if(GetAsyncKeyState(VK_ALT) & 0x8000) {
                        notepad("acceleration_deceleration", "txt");;
                        return 1;
                    }
                break;

                case VK_BACK:
                    if((GetAsyncKeyState(VK_ALT) & 0x8000)
                    && (GetAsyncKeyState(VK_SHIFT) & 0x8000)) {
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
                        ToggleSimulationMode(TRUE);
                    else
                        ToggleSimulationMode();
                    break;
            } else if(wParam == VK_TAB) {
                SetFocus(IoList);
                BlinkCursor(0, 0, 0, 0);
                break;
            } else if(wParam == VK_F1) {
                ShowHelpDialog(FALSE);
                break;
            } else if(wParam == VK_F3) {
                ExportDialog();
                notepad(CurrentSaveFile, "txt");
                break;
            } else if(wParam == VK_F4) {
                if(CheckSaveUserCancels()) break;
                notepad(CurrentSaveFile, "ld");
                break;
            } else if(wParam == VK_F6) {
                if(GetAsyncKeyState(VK_CONTROL) & 0x8000)
                    readBat(CurrentSaveFile, Prog.mcu ? Prog.mcu->whichIsa : 0);
                else
                    flashBat(CurrentSaveFile, Prog.mcu ? Prog.mcu->whichIsa : 0);
                break;
            }

            switch(wParam) {
                case VK_DOWN:
                  if((GetAsyncKeyState(VK_CONTROL) & 0x8000) || InSimulationMode) {
                    ScrollDown();
                  } else {
                    rung1 = RungContainingSelected();
                    MoveCursorKeyboard(wParam);
                    TestSelections(msg,rung1);
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
                    TestSelections(msg,rung1);
                  }
                  break;

                case VK_LEFT:
                  if((GetAsyncKeyState(VK_CONTROL) & 0x8000) || InSimulationMode) {
                    ScrollXOffset -= FONT_WIDTH;
                    if(ScrollXOffset < 0) ScrollXOffset = 0;
                    RefreshScrollbars();
                    InvalidateRect(MainWindow, NULL, FALSE);
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
                    InvalidateRect(MainWindow, NULL, FALSE);
                  } else {
                    MoveCursorKeyboard(wParam);
                  }
                  break;

                case VK_HOME:
                  if(GetAsyncKeyState(VK_CONTROL) & 0x8000) {
                    RollHome();
                  } else {
                    int gx=0, gy=0;
                    if (FindSelected(&gx, &gy)) {
                        gx=0;
                        MoveCursorNear(&gx, &gy);
                        SelectElement(gx, gy, SELECTED_RIGHT);
                    }
                  }
                  break;

                case VK_END:
                  if(GetAsyncKeyState(VK_CONTROL) & 0x8000) {
                    RollEnd();
                  } else {
                    int gx=0, gy=0;
                    if (FindSelected(&gx, &gy)) {
                        gx=ColsAvailable;
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
                    int gx=0, gy=0;
                    FindSelected(&gx, &gy);

                    if(ScrollYOffset-ScreenRowsAvailable()-1 > 0) {
                        ScrollYOffset-=ScreenRowsAvailable()-1;
                    } else {
                        ScrollYOffset=0;
                    }
                    RefreshScrollbars();
                    InvalidateRect(MainWindow, NULL, FALSE);

                    if(gy-ScreenRowsAvailable()-1 > 0) {
                        gy-=ScreenRowsAvailable()-1;
                    } else {
                        gy=0;
                    }
                    SelectedGxAfterNextPaint = gx;
                    SelectedGyAfterNextPaint = gy;
                  }
                  TestSelections(msg,rung1);
                  break;

                case VK_NEXT:
                  rung1 = RungContainingSelected();
                  if(GetAsyncKeyState(VK_CONTROL) & 0x8000) {
                    ScrollPgDown();
                  } else {
                    int gx=0, gy=0;
                    FindSelected(&gx, &gy);

                    if(ScrollYOffset+ScreenRowsAvailable()-1 < ScrollYOffsetMax) {
                        ScrollYOffset+=ScreenRowsAvailable()-1;
                    } else {
                      ScrollYOffset = ScrollYOffsetMax;
                    }
                    RefreshScrollbars();
                    InvalidateRect(MainWindow, NULL, FALSE);

                    if(gy+ScreenRowsAvailable()-1 < totalHeightScrollbars-1) {
                        gy+=ScreenRowsAvailable()-1;
                    } else {
                        gy=totalHeightScrollbars-1;
                    }
                    SelectedGxAfterNextPaint = gx;
                    SelectedGyAfterNextPaint = gy;
                  }
                  TestSelections(msg,rung1);
                  break;
            }

            if(InSimulationMode) {
                switch(wParam) {
                    case ' ':
                        SimulateOneCycle(TRUE);
                        break;

                    case VK_F8:
                        StartSimulation();
                        break;

                    case 'R':
                        if(GetAsyncKeyState(VK_CONTROL) & 0x8000)
                            StartSimulation();
                        break;

                    case VK_F9:
                        StopSimulation();
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
                    CompileProgram(FALSE, MNU_COMPILE);
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
                    }
                    break;

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
                        if(CheckSaveUserCancels()) break;
                        if(!ProgramChangedNotSaved) {
                            int r = MessageBox(MainWindow,
                                _("Start new program?"),
                                "LDmicro", MB_YESNO | MB_DEFBUTTON2 |
                                MB_ICONQUESTION);
                            if(r == IDNO) break;
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
                        if(CheckSaveUserCancels()) break;
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
                        CHANGING_PROGRAM(InsertRung(TRUE));
                    }
                    break;

                case '6':
                    if(GetAsyncKeyState(VK_SHIFT) & 0x8000) {
                        CHANGING_PROGRAM(InsertRung(FALSE));
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
                InvalidateRect(MainWindow, NULL, FALSE);
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
            InvalidateRect(MainWindow, NULL, FALSE);
            break;
        }

        case WM_LBUTTONDOWN: {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            if((y > (IoListTop - 9)) && (y < (IoListTop + 3))) {
                POINT pt;
                pt.x = x; pt.y = y;
                ClientToScreen(MainWindow, &pt);
                MouseY = pt.y;
                MouseHookHandle = SetWindowsHookEx(WH_MOUSE_LL,
                        (HOOKPROC)MouseHook, Instance, 0);
            }
            if(!InSimulationMode) {
                rung1 = RungContainingSelected();
                MoveCursorMouseClick(x, y);
                TestSelections(msg,rung1);
            }
            SetFocus(MainWindow);
            InvalidateRect(MainWindow, NULL, FALSE);
            break;
        }
        case WM_MOUSEMOVE: {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);

            if((y > (IoListTop - 9)) && (y < (IoListTop + 3))) {
                SetCursor(LoadCursor(NULL, IDC_SIZENS));
            } else {
                SetCursor(LoadCursor(NULL, IDC_ARROW));
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
            InvalidateRect(MainWindow, NULL, FALSE);
            break;

        case WM_CLOSE:
        case WM_DESTROY:
            WipeIntMemory();
            if(CheckSaveUserCancels()) break;

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
static BOOL MakeWindowClass()
{
    WNDCLASSEX wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(wc);

    wc.style            = CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW | CS_OWNDC |
                            CS_DBLCLKS;
    wc.lpfnWndProc      = (WNDPROC)MainWndProc;
    wc.hInstance        = Instance;
    wc.hbrBackground    = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName    = "LDmicro";
    wc.lpszMenuName     = NULL;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon            = (HICON)LoadImage(Instance, MAKEINTRESOURCE(4000),
                            IMAGE_ICON, 32, 32, 0);
    wc.hIconSm          = (HICON)LoadImage(Instance, MAKEINTRESOURCE(4000),
                            IMAGE_ICON, 16, 16, 0);

    return RegisterClassEx(&wc);
}

//-----------------------------------------------------------------------------

static LPSTR _getNextCommandLineArgument(LPSTR lpBuffer)
{
    BOOL argFound = FALSE;
    while(*lpBuffer)
    {
        if (isspace(*lpBuffer))
        {
            argFound = FALSE;

        }
        else if ((*lpBuffer == '-') || (*lpBuffer == '/'))
        {
            argFound = TRUE;
        }
        else if (argFound)
        {
            return lpBuffer;
        }

        lpBuffer++;
    }
    return NULL;
}

//-----------------------------------------------------------------------------

static LPSTR _getNextPositionalArgument(LPSTR lpBuffer)
{
    BOOL argFound = FALSE;
    while(*lpBuffer)
    {
        if (isspace(*lpBuffer))
        {
            argFound = TRUE;
        }
        else if ((*lpBuffer == '-') || (*lpBuffer == '/'))
        {
            argFound = FALSE;
        }
        else if (argFound)
        {
            return lpBuffer;
        }
        lpBuffer++;
    }
    return NULL;
}

//-----------------------------------------------------------------------------

static char * _removeWhitespace(char * pBuffer)
{
    // Check from left:
    char * pStart = pBuffer;
    while(*pBuffer)
    {
        if (isspace(*pBuffer) || *pBuffer == '"' || *pBuffer == '\'')
        {
            pStart++;
        }
        else
        {
            break;
        }

        pBuffer++;
    }

    if (*pBuffer == 0)
    {
        // No alphanumeric characters in this string.
        return NULL;
    }


    // Check from right.
    {
        int len = strlen(pBuffer);
        char * pEnd = &pBuffer[len-1];

        while(pEnd > pStart)
        {
            if (isspace(*pEnd) || *pEnd == '"' || *pEnd == '\'')
            {
                *pEnd = 0;
            }
            else
            {
                break;
            }

            pEnd--;
        }
    }

    if (strlen(pStart) == 0)
    {
        return NULL;
    }

    return pStart;
}

//-----------------------------------------------------------------------------

static void _parseArguments(LPSTR lpCmdLine, char ** pSource, char ** pDest)
{
    // Parse for command line arguments.
    LPSTR lpArgs = lpCmdLine;

    while(1)
    {
        lpArgs = _getNextCommandLineArgument(lpArgs);

        if(lpArgs == NULL)
        {
            break;
        }
        if(*lpArgs == 'c')
        {
            RunningInBatchMode = TRUE;
        }
        if(*lpArgs == 't')
        {
            RunningInTestMode = TRUE;
        }
    }



    // Parse for positional arguments (first is source, second destination):
    *pSource = NULL;
    *pDest = NULL;

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

//---------------------------------------------------------------------------
void abortHandler( int signum )
{
    // associate each signal with a signal name string.
    const char* name = NULL;
    switch( signum )
    {
    case SIGABRT: name = "SIGABRT";  break;
    case SIGSEGV: name = "SIGSEGV";  break;
  //case SIGBUS:  name = "SIGBUS";   break;
    case SIGILL:  name = "SIGILL";   break;
    case SIGFPE:  name = "SIGFPE";   break;
    case SIGINT:  name = "SIGINT";   break;
    case SIGTERM: name = "SIGTERM";  break;
    }

    // Notify the user which signal was caught. We use printf, because this is the
    // most basic output function. Once you get a crash, it is possible that more
    // complex output systems like streams and the like may be corrupted. So we
    // make the most basic call possible to the lowest level, most
    // standard print function.
    if ( name )
       dbp("Caught signal %d (%s)\n", signum, name );
    else
       dbp("Caught signal %d\n", signum );

    // Dump a stack trace.
    // This is the function we will be implementing next.
    //printStackTrace();

    // If you caught one of the above signals, it is likely you just
    // want to quit your program right now.
    exit( signum );
}

//-----------------------------------------------------------------------------
void KxStackTrace()
{
    signal( SIGABRT, abortHandler );
    signal( SIGSEGV, abortHandler );
  //signal( SIGBUS,  abortHandler );
    signal( SIGILL,  abortHandler );
    signal( SIGFPE,  abortHandler );
    signal( SIGINT,  abortHandler );
    signal( SIGTERM, abortHandler );
}

//-----------------------------------------------------------------------------
void CheckPwmPins()
{
return;
    int i,j;
    for(i = 0; i < NUM_SUPPORTED_MCUS ; i++) {
       for(j = 0; j < SupportedMcus[i].pwmCount ; j++) {
           if(!SupportedMcus[i].pwmNeedsPin && SupportedMcus[i].pwmCount) {
               ooops("1 %s", SupportedMcus[i].mcuName)
           }
           else if(SupportedMcus[i].pwmNeedsPin)
               if(SupportedMcus[i].pwmNeedsPin == SupportedMcus[i].pwmInfo[j].pin)
                    break;
       }
       if(SupportedMcus[i].pwmCount)
           if(j >= SupportedMcus[i].pwmCount)
               ooops("2 %s", SupportedMcus[i].mcuName)
       }
}

//-----------------------------------------------------------------------------
// Entry point into the program.
//-----------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, INT nCmdShow)
{
    if((NUM_SUPPORTED_MCUS) != arraylen(SupportedMcus)) {
        Error("NUM_SUPPORTED_MCUS=%d != arraylen(SupportedMcus)=%d", NUM_SUPPORTED_MCUS, arraylen(SupportedMcus));
        oops();
    }

    CheckPwmPins();

  try
  {
    GetModuleFileName(hInstance,ExePath,MAX_PATH);
    ExtractFilePath(ExePath);

    Instance = hInstance;

    MainHeap = HeapCreate(0, 1024*64, 0);

    setlocale(LC_ALL,"");
    //RunningInBatchMode = FALSE;

    MakeWindowClass();
    MakeDialogBoxClass();
    HMENU top = MakeMainWindowMenus();

    MainWindow = CreateWindowEx(0, "LDmicro", "",
        WS_OVERLAPPED | WS_THICKFRAME | WS_CLIPCHILDREN | WS_MAXIMIZEBOX |
        WS_MINIMIZEBOX | WS_SYSMENU | WS_SIZEBOX,
        10, 10, 800, 600, NULL, top, Instance, NULL);
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
    if(memcmp(lpCmdLine, "/c", 2)==0) {
        RunningInBatchMode = TRUE;

        char *err =
            "Bad command line arguments: run 'ldmicro /c src.ld dest.ext'";

        char *source = lpCmdLine + 2;
        while(isspace(*source)) {
            source++;
        }
        if(*source == '\0') { Error(err); doexit(EXIT_FAILURE); }
        char *dest = source;
        while(!isspace(*dest) && *dest) {
            dest++;
        }
        if(*dest == '\0') { Error(err); doexit(EXIT_FAILURE); }
        *dest = '\0'; dest++;
        while(isspace(*dest)) {
            dest++;
        }
        if(*dest == '\0') { Error(err); doexit(EXIT_FAILURE); }
        char *l, *r;
        if((l=strchr(dest, '.')) != (r=strrchr(dest, '.'))) {
          while(*r) {
            *l = *r;
            r++; l++;
          }
          *l = '\0';
        }
        if(!LoadProjectFromFile(source)) {
            Error("Couldn't open '%s', running non-interactively.", source);
            doexit(EXIT_FAILURE);
        }
        strcpy(CurrentCompileFile, dest);
        GenerateIoList(-1);
        CompileProgram(FALSE, MNU_COMPILE);
        doexit(EXIT_SUCCESS);
    }
    if(memcmp(lpCmdLine, "/t", 2)==0) {
        RunningInBatchMode = TRUE;

        char exportFile[MAX_PATH];

        char *err =
            "Bad command line arguments: run 'ldmicro /t src.ld [dest.txt]'";

        char *source = lpCmdLine + 2;
        while(isspace(*source)) {
            source++;
        }
        if(*source == '\0') { Error(err); doexit(EXIT_FAILURE); }

        char *dest = source;
        while(!isspace(*dest) && *dest) {
            dest++;
        }
        *dest = '\0';
        if(!LoadProjectFromFile(source)) {
            Error("Couldn't open '%s', running non-interactively.", source);
            doexit(EXIT_FAILURE);
        }
        strcpy(CurrentSaveFile,source);
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
            strcpy(line, lpCmdLine+1);
        } else {
            strcpy(line, lpCmdLine);
        }
        if(strchr(line, '"')) *strchr(line, '"') = '\0';

        if(!strchr(line, '.')) strcat(line, ".ld");

        char *s;
        GetFullPathName(line, sizeof(CurrentSaveFile), CurrentSaveFile, &s);

        if(!LoadProjectFromFile(CurrentSaveFile)) {
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
    DWORD ret;
    while(ret = GetMessage(&msg, NULL, 0, 0)) {
        if(msg.hwnd == IoList && msg.message == WM_KEYDOWN) {
            if(msg.wParam == VK_TAB) {
                SetFocus(MainWindow);
                continue;
            }
        }
        if(msg.message == WM_KEYDOWN && msg.wParam != VK_UP &&
            msg.wParam != VK_NEXT && msg.wParam != VK_PRIOR &&
            msg.wParam != VK_DOWN && msg.wParam != VK_RETURN && msg.wParam
            != VK_SHIFT)
        {
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

    return 0;
  }
  catch(...)
  {
      abortHandler(EXCEPTION_EXECUTE_HANDLER);
  };
}
