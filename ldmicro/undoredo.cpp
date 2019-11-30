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
// Routines to maintain the stack of recent versions of the program that we
// use for the undo/redo feature. Don't be smart at all; keep deep copies of
// the entire program at all times.
// Jonathan Westhues, split May 2005
//-----------------------------------------------------------------------------
#include "stdafx.h"

#include "ldmicro.h"
#include "undoredo.hpp"

//-----------------------------------------------------------------------------
// Push a copy of the PLC program onto the undo history, replacing (and
// freeing) the oldest one if necessary.
//-----------------------------------------------------------------------------
void UndoRemember()
{
    // can't redo after modifying the program
    Undo::emptyRedo();
    Undo::pushUndo();

    SetUndoEnabled(true, false);
}

//-----------------------------------------------------------------------------
// Pop the undo history one level, or do nothing if we're out of levels of
// undo. This means that we push the current program on the redo stack, and
// pop the undo stack onto the current program.
//-----------------------------------------------------------------------------
void UndoUndo()
{
    if(Undo::undoSize() <= 0)
        return;

    ForgetEverything();

    Undo::pushRedo();
    Undo::popUndo();

    if(Undo::undoSize() > 0) {
        SetUndoEnabled(true, true);
    } else {
        SetUndoEnabled(false, true);
    }
    RefreshControlsToSettings();
    RefreshScrollbars();
    InvalidateRect(MainWindow, nullptr, false);
}

//-----------------------------------------------------------------------------
// Redo an undone operation. Push the current program onto the undo stack,
// and pop the redo stack into the current program.
//-----------------------------------------------------------------------------
void UndoRedo()
{
    if(Undo::redoSize() <= 0)
        return;

    ForgetEverything();

    Undo::pushUndo();
    Undo::popRedo();

    if(Undo::redoSize() > 0) {
        SetUndoEnabled(true, true);
    } else {
        SetUndoEnabled(true, false);
    }
    RefreshControlsToSettings();
    RefreshScrollbars();
    InvalidateRect(MainWindow, nullptr, false);
}

//-----------------------------------------------------------------------------
// Empty out our undo history entirely, as when loading a new file.
//-----------------------------------------------------------------------------
void UndoFlush()
{
    Undo::emptyRedo();
    Undo::emptyUndo();
    SetUndoEnabled(false, false);
}

//-----------------------------------------------------------------------------
// Is it possible to undo some operation? The display code needs to do that,
// due to an ugly hack for handling too-long lines; the only thing that
// notices that easily is the display code, which will respond by undoing
// the last operation, presumably the one that added the long line.
//-----------------------------------------------------------------------------
bool CanUndo()
{
    return (Undo::undoSize() > 0);
}

//-----------------------------------------------------------------------------
// Push the current program onto a program stack. Can either make a deep or
// a shallow copy of the linked data structures.
//-----------------------------------------------------------------------------
void ProgramStack::push()
{
    if(count == MAX_LEVELS_UNDO)
        undo[write].reset();
    else
        count++;

    undo[write].prog = new PlcProgram;
    *(undo[write].prog) = Prog;
    int gx, gy;
    if(FindSelected(&gx, &gy)) {
        undo[write].gx = gx;
        undo[write].gy = gy;
    } else {
        undo[write].gx = -1;
        undo[write].gy = -1;
    }

    int a = (write + 1);
    if(a >= MAX_LEVELS_UNDO)
        a -= MAX_LEVELS_UNDO;
    write = a;
}

//-----------------------------------------------------------------------------
// Pop a program stack onto the current program. Always does a shallow copy.
// Internal error if the stack was empty.
//-----------------------------------------------------------------------------
void ProgramStack::pop()
{
    int a = (write - 1);
    if(a < 0)
        a += MAX_LEVELS_UNDO;
    write = a;
    count--;

    Prog = *(undo[write].prog);
    SelectedGxAfterNextPaint = undo[write].gx;
    SelectedGyAfterNextPaint = undo[write].gy;

    undo[write].reset();
}

//-----------------------------------------------------------------------------
// Empty out a ProgramStack data structure, either .undo or .redo: set the
// count to zero and free all the program copies in it.
//-----------------------------------------------------------------------------
void ProgramStack::empty()
{
    while(count > 0) {
        int a = write - 1;
        if(a < 0)
            a += MAX_LEVELS_UNDO;
        write = a;
        count--;

        undo[write].reset();
    }
}

void UndoStruct::reset()
{
    if(prog) {
        delete prog;
        prog = nullptr;
    }
    gx = -1;
    gy = -1;
}

Undo Undo::instance_;

Undo::Undo()
{
}

Undo::~Undo()
{
    undo.empty();
    redo.empty();
}

void Undo::pushUndo()
{
    instance_.undo.push();
}

void Undo::popUndo()
{
    instance_.undo.pop();
}

void Undo::emptyUndo()
{
    instance_.undo.empty();
}

void Undo::pushRedo()
{
    instance_.redo.push();
}

void Undo::popRedo()
{
    instance_.redo.pop();
}

void Undo::emptyRedo()
{
    instance_.redo.empty();
}

int Undo::undoSize()
{
    return instance_.undo.size();
}

int Undo::redoSize()
{
    return instance_.redo.size();
}
