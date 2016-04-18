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

#include "ldmicro.h"
#include "freeze.h"
#include "mcutable.h"
#include "git_commit.h"

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
char CurrentSaveFile[MAX_PATH];
static BOOL ProgramChangedNotSaved = FALSE;

#define HEX_PATTERN  "Intel Hex Files (*.hex)\0*.hex\0All files\0*\0\0"
#define C_PATTERN "C Source Files (*.c)\0*.c\0All Files\0*\0\0"
#define INTERPRETED_PATTERN \
    "Interpretable Byte Code Files (*.int)\0*.int\0All Files\0*\0\0"
#define PASCAL_PATTERN "PASCAL Source Files (*.pas)\0*.pas\0All Files\0*\0\0"
#define ARDUINO_C_PATTERN "ARDUINO C Source Files (*.cpp)\0*.cpp\0All Files\0*\0\0"
#define XINT_PATTERN \
    "Extended Byte Code Files (*.int)\0*.xint\0All Files\0*\0\0"
char CurrentCompileFile[MAX_PATH];

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

    if(!SaveProjectToFile(CurrentSaveFile)) {
        Error(_("Couldn't write to '%s'."), CurrentSaveFile);
        return FALSE;
    } else {
        ProgramChangedNotSaved = FALSE;
        return TRUE;
    }
}

//---------------------------------------------------------------------------
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
static BOOL SaveProgram(void)
{
    if(strlen(CurrentSaveFile)) {
        if(!SaveProjectToFile(CurrentSaveFile)) {
            Error(_("Couldn't write to '%s'."), CurrentSaveFile);
            return FALSE;
        } else {
            ProgramChangedNotSaved = FALSE;
            return TRUE;
        }
    } else {
        return SaveAsDialog();
    }
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
      Error("Error: %d - %s in >%s<",Err, s, r);
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
        default: oops()
    }
}
//-----------------------------------------------------------------------------
static void flashBat(char *name, int ISA)
{
    if(strlen(name) == 0) {
        Error(_(" Save ld before flash."));
        return;
    }

    char s[MAX_PATH];
    char r[MAX_PATH];

    s[0] = '\0';
    SetExt(s, name, "");
    sprintf(r,"\"%sflashMcu.bat\" %s \"%s\"",ExePath,GetIsaName(ISA),s);

    isErr(WinExec(r, SW_SHOWNORMAL), r);
}

//-----------------------------------------------------------------------------
static void readBat(char *name, int ISA)
{
    if(strlen(name) == 0) {
        name = "read";
    }

    char s[MAX_PATH];
    char r[MAX_PATH];

    s[0] = '\0';
    SetExt(s, name, "");
    sprintf(r,"\"%sreadMcu.bat\" %s \"%s\"",ExePath,GetIsaName(ISA),s);

    isErr(WinExec(r, SW_SHOWNORMAL/* | SW_SHOWMINIMIZED*/), r);
}

//-----------------------------------------------------------------------------
static void notepad(char *name, char *ext)
{
    char s[MAX_PATH]="";
    char r[MAX_PATH];

    s[0] = '\0';
    SetExt(s, name, ext);
    sprintf(r,"""%snotepad.bat %s""",ExePath,s);

    //_execl("notepad.exe",s);
    isErr(WinExec(r, SW_SHOWNORMAL), r);
}
//-----------------------------------------------------------------------------
static void postCompile()
{
    char r[MAX_PATH];
    char onlyName[MAX_PATH];

    strcpy(onlyName, ExtractFileName(CurrentSaveFile));
    SetExt(onlyName, onlyName, "");

    sprintf(r,"""%spostCompile.bat %s %s""", ExePath, CurrentLdPath, onlyName);

    isErr(WinExec(r, SW_SHOWNORMAL/* | SW_SHOWMINIMIZED*/), r);
}

//-----------------------------------------------------------------------------
// Compile the program to a hex file for the target micro. Get the output
// file name if necessary, then call the micro-specific compile routines.
//-----------------------------------------------------------------------------
static void CompileProgram(BOOL compileAs, int compile_MNU)
{
    if(compile_MNU==MNU_COMPILE){
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

    if( compileAs || (strlen(CurrentCompileFile)==0)
      ||  (compile_MNU==MNU_COMPILE_AS)
      ||( (compile_MNU==MNU_COMPILE      )  && (!strstr(CurrentCompileFile,".hex")) )
      ||( (compile_MNU==MNU_COMPILE_IHEX )  && (!strstr(CurrentCompileFile,".hex")) )
      ||( (compile_MNU==MNU_COMPILE_ANSIC)  && (strlen(strstr(CurrentCompileFile,".c"))!=2) )
      ||( (compile_MNU==MNU_COMPILE_ARDUINO)&& (!strstr(CurrentCompileFile,".cpp")) )
      ||( (compile_MNU==MNU_COMPILE_PASCAL) && (!strstr(CurrentCompileFile,".pas")) )
	  || ((compile_MNU==MNU_COMPILE_XINT)   && (!strstr(CurrentCompileFile, ".xint")) )
      ){
        char *c;
        OPENFILENAME ofn;

        memset(&ofn, 0, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hInstance = Instance;
        ofn.lpstrTitle = _("Compile To");
        if((compile_MNU==MNU_COMPILE_ANSIC) ||
           (Prog.mcu && Prog.mcu->whichIsa == ISA_ANSIC)) {
            ofn.lpstrFilter = C_PATTERN;
            ofn.lpstrDefExt = "c";
            c = "c";
        } else if(Prog.mcu && (Prog.mcu->whichIsa == ISA_INTERPRETED ||
							Prog.mcu->whichIsa == ISA_XINTERPRETED || 
							Prog.mcu->whichIsa == ISA_NETZER)) {
            ofn.lpstrFilter = INTERPRETED_PATTERN;
            ofn.lpstrDefExt = "int";
            c = "int";
        } else if((compile_MNU==MNU_COMPILE_PASCAL) ||
                  (Prog.mcu && Prog.mcu->whichIsa == ISA_PASCAL)) {
            ofn.lpstrFilter = PASCAL_PATTERN;
            ofn.lpstrDefExt = "pas";
            c = "pas";
		}
		else if ((compile_MNU == MNU_COMPILE_ARDUINO) ||
			(Prog.mcu && Prog.mcu->whichIsa == ISA_ARDUINO)) {
			ofn.lpstrFilter = ARDUINO_C_PATTERN;
			ofn.lpstrDefExt = "cpp";
			c = "cpp";
		} 
		else if ((compile_MNU == MNU_COMPILE_XINT) ||
			(Prog.mcu && Prog.mcu->whichIsa == ISA_XINTERPRETED)) {
			ofn.lpstrFilter = XINT_PATTERN;
			ofn.lpstrDefExt = "xint";
			c = "xint";
        /*
        } else if((compile_MNU==MNU_COMPILE_ARDUINO) ||
                  (Prog.mcu && Prog.mcu->whichIsa == ISA_ARDUINO)) {
            ofn.lpstrFilter = ARDUINO_PATTERN;
            ofn.lpstrDefExt = "ino";
            c = "ino";
        */
        } else {
            ofn.lpstrFilter = HEX_PATTERN;
            ofn.lpstrDefExt = "hex";
            c = "hex";
        }
        SetExt(CurrentCompileFile, CurrentSaveFile, c);
        ofn.lpstrFile = CurrentCompileFile;
        ofn.nMaxFile = sizeof(CurrentCompileFile);
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

        if(!GetSaveFileName(&ofn))
            return;

        // hex output filename is stored in the .ld file
        ProgramChangedNotSaved = TRUE;
    }

    if(!GenerateIntermediateCode()) return;

    if((Prog.mcu == NULL)
    && (compile_MNU!=MNU_COMPILE_PASCAL)
    && (compile_MNU!=MNU_COMPILE_ANSIC)
    && (compile_MNU!=MNU_COMPILE_ARDUINO)
	&& (compile_MNU != MNU_COMPILE_XINT)) {
			Error(_("Must choose a target microcontroller before compiling."));
        return;
    }

    if((UartFunctionUsed() && (Prog.mcu) && Prog.mcu->uartNeeds.rxPin == 0)
    && (compile_MNU!=MNU_COMPILE_PASCAL)
    && (compile_MNU!=MNU_COMPILE_ANSIC)
    && (compile_MNU!=MNU_COMPILE_ARDUINO)) {
        Error(_("UART function used but not supported for this micro."));
        return;
    }

    if((PwmFunctionUsed() && (Prog.mcu) && Prog.mcu->pwmNeedsPin == 0)
    && (compile_MNU!=MNU_COMPILE_PASCAL)
    && (compile_MNU!=MNU_COMPILE_ANSIC)
    && (compile_MNU!=MNU_COMPILE_ARDUINO)
	&& (compile_MNU != MNU_COMPILE_XINT)
	&& (Prog.mcu->whichIsa != ISA_XINTERPRETED)) {
        Error(_("PWM function used but not supported for this micro."));
        return;
    }

    if (compile_MNU==MNU_COMPILE_ANSIC)
		CompileAnsiC(CurrentCompileFile);
    else if (compile_MNU==MNU_COMPILE_ARDUINO)
		CompileAnsiC(CurrentCompileFile, ISA_ARDUINO);
	else if (compile_MNU == MNU_COMPILE_XINT)
		CompileXInterpreted(CurrentCompileFile);
	else if (Prog.mcu)
    switch(Prog.mcu->whichIsa) {
        case ISA_AVR:           CompileAvr(CurrentCompileFile); break;
        case ISA_PIC16:         CompilePic16(CurrentCompileFile); break;
        case ISA_ANSIC:         CompileAnsiC(CurrentCompileFile); break;
        case ISA_INTERPRETED:   CompileInterpreted(CurrentCompileFile); break;
		case ISA_XINTERPRETED:  CompileXInterpreted(CurrentCompileFile); break;
        case ISA_NETZER:        CompileNetzer(CurrentCompileFile); break;
        case ISA_ARDUINO:       CompileAnsiC(CurrentCompileFile, ISA_ARDUINO); break;

        default: oops();
    } else oops();

    postCompile();

//    IntDumpListing("t.pl");
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
            if(SaveProgram())
                return FALSE;
            else
                return TRUE;

        case IDNO:
            return FALSE;

        case IDCANCEL:
            return TRUE;

        default:
            oops();
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
        strcpy(CurrentSaveFile, tempSaveFile);
        UndoFlush();
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
            SaveProgram();
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

        case MNU_EXIT:
            if(CheckSaveUserCancels()) break;
            PostQuitMessage(0);
            break;

        case MNU_INSERT_COMMENT:
            CHANGING_PROGRAM(AddComment(_("--add comment here--")));
            break;

        case MNU_INSERT_CONTACTS:
            CHANGING_PROGRAM(AddContact());
            break;

        case MNU_INSERT_COIL:
            CHANGING_PROGRAM(AddCoil());
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

        case MNU_INSERT_SHIFT_REG:
            CHANGING_PROGRAM(AddShiftRegister());
            break;

        case MNU_INSERT_LUT:
            CHANGING_PROGRAM(AddLookUpTable());
            break;

        case MNU_INSERT_PWL:
            CHANGING_PROGRAM(AddPiecewiseLinear());
            break;

        case MNU_INSERT_FMTD_STR:
            CHANGING_PROGRAM(AddFormattedString());
            break;

        case MNU_INSERT_STRING:
            CHANGING_PROGRAM(AddString());
            break;

        case MNU_INSERT_OSR:
            CHANGING_PROGRAM(AddEmpty(ELEM_ONE_SHOT_RISING));
            break;

        case MNU_INSERT_OSF:
            CHANGING_PROGRAM(AddEmpty(ELEM_ONE_SHOT_FALLING));
            break;

        case MNU_INSERT_MOV:
            CHANGING_PROGRAM(AddMove());
            break;

        case MNU_INSERT_SET_PWM:
            CHANGING_PROGRAM(AddSetPwm());
            break;

        case MNU_INSERT_READ_ADC:
            CHANGING_PROGRAM(AddReadAdc());
            break;

        case MNU_INSERT_UART_SEND:
            CHANGING_PROGRAM(AddUart(ELEM_UART_SEND));
            break;

        case MNU_INSERT_UART_RECV:
            CHANGING_PROGRAM(AddUart(ELEM_UART_RECV));
            break;

        case MNU_INSERT_PERSIST:
            CHANGING_PROGRAM(AddPersist());
            break;

        {
            int elem;
            case MNU_INSERT_ADD: elem = ELEM_ADD; goto math;
            case MNU_INSERT_SUB: elem = ELEM_SUB; goto math;
            case MNU_INSERT_MUL: elem = ELEM_MUL; goto math;
            case MNU_INSERT_DIV: elem = ELEM_DIV; goto math;
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
            CHANGING_PROGRAM(DeleteSelectedRung());
            break;

        case MNU_PUSH_RUNG_UP:
            CHANGING_PROGRAM(PushRungUp());
            break;

        case MNU_PUSH_RUNG_DOWN:
            CHANGING_PROGRAM(PushRungDown());
            break;

        case MNU_DELETE_ELEMENT:
            CHANGING_PROGRAM(DeleteSelectedFromProgram());
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

        case MNU_COMPILE_AS:
            CompileProgram(TRUE, code);
            break;

        case MNU_MANUAL:
            ShowHelpDialog(FALSE);
            break;

        case MNU_ABOUT:
            ShowHelpDialog(TRUE);
            break;

        case MNU_RELEASE:
            char str[1024];
            sprintf(str,"Tag: %s\n\n%s\n\nSHA-1: %s\n\n"
                "Compiled: " __TIME__ " " __DATE__ ".",
                git_commit_tag, git_commit_date, git_commit_str);
            MessageBox(MainWindow, str, _("Release"),
                MB_OK | MB_ICONINFORMATION);
            break;
    }
}

//-----------------------------------------------------------------------------
// WndProc for MainWindow.
//-----------------------------------------------------------------------------
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
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
            } else if(wParam == VK_F6) {
                if(GetAsyncKeyState(VK_CONTROL) & 0x8000)
                    readBat(CurrentSaveFile, Prog.mcu ? Prog.mcu->whichIsa : 0);
                else
                    flashBat(CurrentSaveFile, Prog.mcu ? Prog.mcu->whichIsa : 0);
                break;
            }

            if(InSimulationMode) {
                switch(wParam) {
                    case ' ':
                        SimulateOneCycle(TRUE);
                        break;

                    case 'R':
                        if(GetAsyncKeyState(VK_CONTROL) & 0x8000)
                            StartSimulation();
                        break;

                    case 'H':
                        if(GetAsyncKeyState(VK_CONTROL) & 0x8000)
                            StopSimulation();
                        break;

                    case VK_DOWN:
                        if(ScrollYOffset < ScrollYOffsetMax)
                            ScrollYOffset++;
                        RefreshScrollbars();
                        InvalidateRect(MainWindow, NULL, FALSE);
                        break;

                    case VK_UP:
                        if(ScrollYOffset > 0)
                            ScrollYOffset--;
                        RefreshScrollbars();
                        InvalidateRect(MainWindow, NULL, FALSE);
                        break;

                    case VK_LEFT:
                        ScrollXOffset -= FONT_WIDTH;
                        if(ScrollXOffset < 0) ScrollXOffset = 0;
                        RefreshScrollbars();
                        InvalidateRect(MainWindow, NULL, FALSE);
                        break;

                    case VK_RIGHT:
                        ScrollXOffset += FONT_WIDTH;
                        if(ScrollXOffset >= ScrollXOffsetMax)
                            ScrollXOffset = ScrollXOffsetMax;
                        RefreshScrollbars();
                        InvalidateRect(MainWindow, NULL, FALSE);
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

                case VK_UP:
                    if(GetAsyncKeyState(VK_SHIFT) & 0x8000) {
                        CHANGING_PROGRAM(PushRungUp());
                    } else {
                        MoveCursorKeyboard(wParam);
                    }
                    break;

                case VK_DOWN:
                    if(GetAsyncKeyState(VK_SHIFT) & 0x8000) {
                        CHANGING_PROGRAM(PushRungDown());
                    } else {
                        MoveCursorKeyboard(wParam);
                    }
                    break;

                case VK_RIGHT:
                case VK_LEFT:
                    MoveCursorKeyboard(wParam);
                    break;

                case VK_RETURN:
                    CHANGING_PROGRAM(EditSelectedElement());
                    break;

                case VK_DELETE:
                    if(GetAsyncKeyState(VK_SHIFT) & 0x8000) {
                        CHANGING_PROGRAM(DeleteSelectedRung());
                    } else {
                        CHANGING_PROGRAM(DeleteSelectedFromProgram());
                    }
                    break;

                case VK_OEM_1:
                    CHANGING_PROGRAM(AddComment(_("--add comment here--")));
                    break;

                case 'C':
                    CHANGING_PROGRAM(AddContact());
                    break;

                // TODO: rather country-specific here
                case VK_OEM_2:
                    CHANGING_PROGRAM(AddEmpty(ELEM_ONE_SHOT_RISING));
                    break;

                case VK_OEM_5:
                    CHANGING_PROGRAM(AddEmpty(ELEM_ONE_SHOT_FALLING));
                    break;

                case 'L':
                    CHANGING_PROGRAM(AddCoil());
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
                        SaveProgram();
                        UpdateMainWindowTitleBar();
                    }
                    break;

                case 'S':
                    if(GetAsyncKeyState(VK_CONTROL) & 0x8000) {
                        SaveProgram();
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

                case 'A':
                    CHANGING_PROGRAM(MakeNormalSelected());
                    break;

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

                case 'M':
                    CHANGING_PROGRAM(AddMove());
                    break;

                case 'P':
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

                case VK_DIVIDE:
                case 'D':
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
                    if(GetAsyncKeyState(VK_SHIFT) & 0x8000) {
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
            if(!InSimulationMode) MoveCursorMouseClick(x, y);

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

//-----------------------------------------------------------------------------
// Entry point into the program.
//-----------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, INT nCmdShow)
{
    GetModuleFileName(hInstance,ExePath,MAX_PATH);
    ExtractFilePath(ExePath);

    Instance = hInstance;

    MainHeap = HeapCreate(0, 1024*64, 0);

    RunningInBatchMode = FALSE;

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
        if(*source == '\0') { Error(err); exit(-1); }
        char *dest = source;
        while(!isspace(*dest) && *dest) {
            dest++;
        }
        if(*dest == '\0') { Error(err); exit(-1); }
        *dest = '\0'; dest++;
        while(isspace(*dest)) {
            dest++;
        }
        if(*dest == '\0') { Error(err); exit(-1); }
        if(!LoadProjectFromFile(source)) {
            Error("Couldn't open '%s', running non-interactively.", source);
            exit(-1);
        }
        strcpy(CurrentCompileFile, dest);
        GenerateIoList(-1);
        CompileProgram(FALSE, MNU_COMPILE);
        exit(0);
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
