/*******************************************************************************
*  file    : undoredo.hpp
*  created : 29.04.2019
*  author  : Slyshyk Oleksiy (alexslyshyk@gmail.com)
*******************************************************************************/
#ifndef UNDOREDO_HPP
#define UNDOREDO_HPP

#include <array>

#define MAX_LEVELS_UNDO 32

class PlcProgram;

void UndoUndo();
void UndoRedo();
void UndoRemember();
void UndoFlush();
bool CanUndo();

struct UndoStruct {
    int gx;
    int gy;
    PlcProgram* prog;

    UndoStruct() {reset();}
    void reset();
};

class ProgramStack {
public:
    ProgramStack():
        write(0),
        count(0)
    {
    }

    void push();
    void pop();
    void empty();

    int size() {return count;}
private:
    std::array<UndoStruct, MAX_LEVELS_UNDO> undo;
    int write;
    int count;
};

// Store a `deep copy' of the entire program before every change, in a
// circular buffer so that the first one scrolls out as soon as the buffer
// is full and we try to push another one.
class Undo {
    Undo();
    Undo(const Undo& ) {}
    Undo& operator=(const Undo&) {return *this;}
    static Undo instance_;
  public:
    ~Undo();
  public:
    static void pushUndo();
    static void popUndo();
    static void emptyUndo();
    static void pushRedo();
    static void popRedo();
    static void emptyRedo();
    static int undoSize();
    static int redoSize();

  private:
    ProgramStack undo;
    ProgramStack redo;
};

#endif // UNDOREDO_HPP
