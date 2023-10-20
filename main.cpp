#undef UNICODE // Pārslēgt projektu ASCII režīmā
#include <windows.h>
#include <stdio.h>
#include "resource.h"
#include <winuser.h>

#pragma warning(disable : 4996)

CRITICAL_SECTION g_CriticalSection;

DWORD g_dwProcessId = 0;
HANDLE g_hProcess = NULL;


// Ziņojumu apstrādātāja prototips
BOOL CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
// Galvenā funkcija
// ja galvenā loga šablonam ir piešķirts resursa identifikators IDD_MAINDIALOG
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    DialogBox(
        hInstance,
        MAKEINTRESOURCE(IDD_MAINDIALOG),
        NULL,
        DLGPROC(MainWndProc));
    return 0;
}


bool BrowseForFileName(HWND hWnd, char* fileName) {
    OPENFILENAME ofn = { 0 };
    char szFileName[MAX_PATH] = { 0 };

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = "Executable Files (*.exe)\0*.exe\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    ofn.lpstrDefExt = "exe";

    if (GetOpenFileNameA(&ofn)) {
        // Copy the selected file name to the output parameter 'fileName'
        strcpy(fileName, szFileName);
        return true;
    }

    return false;
}

// Function to terminate the process if it's running
void TerminateRunningProcess() {
    if (g_dwProcessId != 0 && g_hProcess != NULL) {
        TerminateProcess(g_hProcess, 0);
        CloseHandle(g_hProcess);
        g_hProcess = NULL;
    }
}

// Ziņojumu apstrādātājs
BOOL CALLBACK MainWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    switch (Msg) { // pārbaudīt ziņojuma identifikatoru
    case WM_INITDIALOG: // ja tas ir inicializācijas paziņojums
        // izpildīt inicializācijas darbības
        return TRUE;
    case WM_COMMAND: // ja tas ir paziņojums no vadības elementa
        switch (LOWORD(wParam)) { // pārbaudīt vadības elementa ID

            case IDOK: // ja bija uzspiesta OK poga
                DestroyWindow(hWnd); // aizvērt logu
                return TRUE;

            case IDC_START: // Ja bija uzspiesta Start poga (If the Start button was pressed)
                g_dwProcessId = 0; // Iestatām sākumvērtību 0 (Set the initial value to 0)

                // Iegūstam ievades lauka tekstu un saglabājam to mainīgajā szCommandLine (Get the text from the input field and store it in the szCommandLine variable)
                char szCommandLine[MAX_PATH];
                GetDlgItemTextA(hWnd, IDC_COMMANDLINE, szCommandLine, MAX_PATH);

                STARTUPINFOA si;
                PROCESS_INFORMATION pi;
                ZeroMemory(&si, sizeof(si));
                si.cb = sizeof(si);
                ZeroMemory(&pi, sizeof(pi));

                // Izveidojam jaunu procesu, izmantojot iegūto komandrindu (Create a new process using the provided command line)
                if (CreateProcessA(NULL, szCommandLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                    // Aizvērt iepriekšējā procesa turu, ja tas nav vienāds nullei (Close the previous process handle if it exists)
                    TerminateRunningProcess();

                    // Saglabāt jauna procesa turu globālajā mainīgajā (Save the new process handle in the global variable)
                    g_dwProcessId = pi.dwProcessId;
                    g_hProcess = pi.hProcess; // Store the process handle
                    CloseHandle(pi.hThread);
                }
                return TRUE;

            case IDC_BROWSE:
                char fileName[MAX_PATH];
                if (BrowseForFileName(hWnd, fileName)) {
                    // Update the command line field with the selected file name
                    SetDlgItemTextA(hWnd, IDC_COMMANDLINE, fileName);
                    return TRUE;
                }

            case IDC_TERMINATE:
                TerminateRunningProcess();
                return TRUE;
            }
        
    case WM_DESTROY: // ja tas ir pabeigšanas pieprasījums
        PostQuitMessage(0); // pabeigt programmu
        return TRUE;
    }
    return FALSE;
}


// TODO Task: Write a program that runs a process and outputs the dynamic graphics on the screentime consumed by these processes.
// 1 Implement IDC_BROWSE
// 2 Implement starting a process from IDC_COMMANDLINE. It should create a thread to run the process
// 3 Implement the Terminate buttom
// 4 Create a global buffer
// 5 The created thread must periodically receive information about the CPU time spent and write this information to the global buffer
// 6 The IDD_MAINDIALOG from needs to read information from that buffer periodically and need to draw the next frame in IDC_GRAPH to make an animation.
// 7 Working with the buffer would require making and utilizing critical sections


// Program architecture

// Process thread
// 1 User selects a .exe file via IDC_BROWSE or types the name of the file in IDC_COMMANDLINE and then presses IDC_START
// 2 The program creates a thread that runs the process that it reads from IDC_COMMANDLINE. 
// 3 This thread runs the process
// 4 The thread calculates the CPU time consumed by the process every 250 milliseconds and writes the calculated values to a global buffer.

// The buffer
// A buffer class is defined.
// It is a thread-safe global cyclic FIFO singleton buffer that stores the time values from the thread
// Thread safety is ensured with critical sections

// Main Thread
// The main thread is created automatically at program startup
// It serves the main IDC_MAINDIALOG
// It periodically reads data from the global buffer and draws on a IDC_GRAPH, creating an animation


// Critical sections
// To ensure thread-safety, mutual exceptions have to be addressed. The simplest solution is critical sections (Windows only).
// A critical section is a fragment (or fragments) of program code that can be executed by only one thread at any given time

// To make a critical section, you need:
// 1. declare a global variable of type CRITICAL_SECTION,
// 2. at the beginning of the program, initialize this variable with the InitializeCriticalSection function,
// 3. at the beginning of the critical section, call the EnterCriticalSection function,
// 4. at the end of the critical section, call the LeaveCriticalSection function,
// 5. at the end of the program, delete the critical section with the DeleteCriticalSection function.


// Graphical device interface
// The set of tools with which Win32 API programs can draw on the window surface is called the Graphics Device Interface (GDI). 
// The work of GDI is based on the concept of device context (DC: Device Context). 
// A device context is a Windows data structure that contains information about drawing attributes for visualization devices such as displays or printers. 
// In Windows programs, all drawing operations are performed through the device context object, which contains interfaces for line, shape, and text drawing.

// Before drawing, the program must request the device context. 
// It can be done with winuser.h function GetDC to draw in a window.
// After the drawing is finished, the device context must be released with the ReleaseDC function.

//To tune the device context, drawing objects can be created and attached to it: a brush, pen, font, etc.

//After setting the device context, you can use the drawing functions to draw the image elements (figures, lines, text).

// Buffer creation
// An array-based cyclic global FIFO buffer must be constructed to store information.
// FIFO is I -> IIIIII -> I

// A cyclic buffer has a Head. 
// It is the position of the array with the oldest data. 
// Whenever new data needs to be inserted, it must be written to the buffer Head. 
// After writing the new data, the Head index number must be incremented by one.
// Then the Head - 1 index will always contain the most recent data. 
// After the data is written in the last position of the array, the Head of the buffer should be moved to the beginning of the array (Head = 0). 
// A cyclic increment operation (Head = (Head + 1) % BUF_SIZE) can be used for this. 
// Thus, the circular buffer is based on two variables: the array and the Head element index. 
// Since multiple numbers must be stored in each position of the buffer, it is best to use a struct:

// struct TBufItem {int tu; int tk;};

// tu – the percentage of time that the process consumes while working in user mode,
// tk – the percentage of time that the process consumes while working in kernel mode.