#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>

#define BUFSIZE 1000
#define CMDSIZE 1000

#define BACK_COLOR RGB(240, 240, 240)

typedef struct buflist // liste chainée de buffers
{
    //void *prev;
    void *next;
    CHAR *buffer;
} buflist;

void Capture(const char * title, char *batchfile, char *fpath1, char *fname2, const char *target3, char *compiler4, char *progtool5);

int  CreateChildPiped(char *cmdline);
void CreateChildProcess(char *cmdline);
void ReadFromPipe(BOOL convert);
void CodePage(LPSTR lpString);

DWORD WINAPI ThreadFunction(LPVOID lpParam);

LRESULT CALLBACK WndProcTexte(HWND hwndTexte, UINT message, WPARAM wParam, LPARAM lParam);
/*
int min(int x, int y);
int max(int x, int y);
*/