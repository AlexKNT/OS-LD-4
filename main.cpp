#undef UNICODE // Pārslēgt projektu ASCII režīmā
#include <windows.h>
#include <stdio.h>
#include "resource.h"
#include <winuser.h>
#include <processthreadsapi.h>
#include <sysinfoapi.h>
#include "TBuffer.h"

#pragma warning(disable : 4996)

CRITICAL_SECTION g_CriticalSection;

DWORD g_dwProcessId = 0;
HANDLE g_hProcess = NULL;

TBuffer g_Buffer;

DWORD FileTimeToMilliseconds(FILETIME ft) {
    ULARGE_INTEGER i;
    i.LowPart = ft.dwLowDateTime;
    i.HighPart = ft.dwHighDateTime;
    return i.QuadPart / 10000;
}

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

    WNDCLASS wc = { 0 }; // klases struktūra, aizpildīta ar nullēm
    wc.lpfnWndProc = GraphWndProc; // elementa ziņojumu apstrādāšanas funkcija
    wc.lpszClassName = "GRAPH"; // elementa klases vārds
    RegisterClass(&wc); // iereģistrēt klasi
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

LRESULT CALLBACK GraphWndProc(
    HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    switch (Msg) {
    case WM_PAINT:
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hWnd, &ps); // pieprasīt ierīces kontekstu zīmēšanai
        RECT rect;
        GetClientRect(hWnd, &rect);

        EndPaint(hWnd, &ps);
        return 0;
    }
    return DefWindowProc(hWnd, Msg, wParam, lParam);

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

// Since the buffer will be used in different threads, thread-safety must be ensured.
// This means that all public methods(except the constructor and destructor) must be placed in a critical section.
// A critical section must be initialized in the constructor of the class, it must be deleted in the destructor, 
// a critical section must be entered at the beginning of each opened method and exited at the end. 

// To draw a frame with the Draw method, you need to review all positions of the buffer(starting from the head) and recalculate the coordinates of the screen point from the data from each position.
// Then, if it is the first point, you need to move the pen to this point using the MoveToEx function, otherwise - draw a line on this point using the LineTo function.


// Coordinate point calculation
// The relationship between the data and the coordinates of the screen point is linear. 
// This means that in order to calculate the coordinates, you need to know two coefficients A and B, which must be calculated in advance.
// For example, we need to recalculate data (n, t) to coordinate (x, y), where n – point number, t – point data (percentage of spent time), x, y – screen point coordinates.

//Linear dependence between data (n, t) and coordinates (x, y) is described by formulas:
// x = Axn + Bx
// y = Ayt + By

//The coefficients Ax, Bx, Ay and By can be determined from the known relations :
// n = 0 → x = 0
// n = MaxN → x = Width
// t = 0 → y = Height
// t = MaxT → y = 0

// MaxN – maximum point number(MaxN = BUF_SIZE - 1),
// MaxT – maximum reflected percentages(e.g.MaxT = 50 %),
// Width – frame width,
// Height – frame height.

// To calculate the coefficients Ax and Bx, it is therefore necessary to solve the system of equations
// 0 = Ax * 0 + Bx
// Width = Ax * MaxN + Bx

// From where:
// Ax = Width / MaxN
// Bx = 0

// As such, to convert the point number n to the x-coordinate, use the formula:
// x = Width / MaxN * n

// Similarly, the coefficients Ay and By must be calculated and the formula for converting t to y determined.


// Elapsed time calculation
// To determine the time spent by a process, use the GetProcessTimes function. 
// The time spent by the process is divided into two parts: how long the process worked in user time)and how long in kernel kernel time. 
// Both times will be in FILETIME format, which contains two 32-bit parts. Together, these are 64-bit integers representing numbers of 100-nanosecond (0.1 microsecond or 10^-4 millisecond) intervals. 
// Before processing this time, it must be converted to milliseconds.
// You can use DWORD FileTimeToMilliseconds(FILETIME ft)

// To determine the real-time interval, use GetTickCount function 

// Thus, to calculate what part of the time the process spent on work, you need :
// 1. Store running times: real_run_time1 = GetTickCount() and consumed time_in_user_mode1 and time_in_kernel_mode1
// 2. After some time(for example, on the next step of the cycle), determine the real and consumed times again (real_run_time2, time_in_user_mode2 and time_in_kernel_mode2).
// 3. Calculate the percentage of the time interval :
// time_in_user_mode = 100 * (time_in_user_mode2 - time_in_user_mode1) / (real_run_time2 - real_run_time1)
// time_in_kernel_mode = 100 * (time_in_kernel_mode2 - time_in_kernel_mode1) / (real_run_time2 - real_run_time1)


// IDC_GRAPH

// Use custom control for graphics output. 
// It allows you to create dialogue elements with non-standard appearance and behavior. 
 
// To add a custom control element to the window, you need to perform the following steps

// 1 Write a message handling function for a new element
// The message handling function must process the messages that will be sent to the control element. 
// This function shall perform a role analogous to the MainWndProc message handling function of the main window. 
// You can describe the response to different messages, but you must necessarily write a message WM_PAINT processing to draw an image of the element on the window surface. 
// If the message has not been processed the system function DefWindowProc shall be called to provide the default response.

// 2. Describe drawing a new element
// The program code to draw the element shall be written in the WM_PAINT message response. 
// It shall request the control the device context of the element with the BeginPaint function ,
//  using the received device context to draw the appearance of the element and release the device context with the EndPaint function.

// 3. Register a new class of control element
// Register a window class for a new element at the beginning of the program (in the WinMain function) with the RegisterClass

// 4. Place the element on the window template
// Place an element of type Custom Control on the dialog template and specify the registered class name GRAPH.

// 5. Register the class of the control element
// At the end of the program (at the end of the WinMain function), unregister the window class of the control element with the function UnregisterClass :
// UnregisterClass("GRAPH");


/*
3. Creating a new control class
As described above, create and register a control element class for drawing a dynamic schedule:
- Write a message handling function to describe the drawing of the graph (WM_PAINT message). For this, clear the background (draw a rectangle the size of the control) and call the buffer method Draw to display the current buffer state.
- Register the class of the control element (GRAPH) at the beginning of the WinMain function.
- Unregister the class of the control element (GRAPH) at the end of the WinMain function.
*/


/*
4. Process service thread
1. Create a global variable Terminate to store the thread completion flag.
2. clear the termination request flag and buffer at the start of the thread.
3. Before the thread loop starts, run the process (CreateProcess) with the command line from the main form and clear its hold. If the process fails to start, complete the thread
4. Start the drawing timer for the main shape:
SetTimer(MainWnd, NULL, 125, NULL);
5. Disable the IDC_START button and enable the IDC_TERMINATE button on the main window. For this use the EnableWindow and GetDlgItem functions
EnableWindow(MainWnd, GetDlgItem(MainWnd, IDC_START), false);
EnableWindow(MainWnd, GetDlgItem(MainWnd, IDC_TERMINATE), true);
6. Repeat the cycle until there is no thread completion request. In cycle:
- Calculate the percentage of time consumed and post it to the global buffer,
- check if the process is still running with GetExitCodeProcess and if the process has already finished - complete the cycle,
- execute a pause (approx. 50 ms)
7. after the cycle, if the process is still running - terminate it with TerminateProcess.
8. Close the process.
9. Stop the drawing timer on the main form:
KillTimer(MainWnd, NULL);
10. Enable the IDC_START button and disable the IDC_TERMINATE button on the main window.
*/

/*
5. Response to pressing the IDC_START button - starting the process 
Create a process service thread and close its handle.
*/

/*
Response to IDC_TERMINATE button press - Process completion
Terminate the process service thread with the Terminate request flag.
*/

/*
* 6. Program completion (response to WM_DESTROY message
1. Terminate the process service thread with the Terminate request flag.
2. Execute a pause to allow the process thread to finish (approx. 10 ms).
*/

/*
Animation (response to WM_TIMER message)
Tell the operating system how the control element IDC_GRAPH has changed and needs to be redrawn. 
This can be be done with the InvalidateRect function. 
WM_PAINT message to the control element
InvalidateRect(GetDlgItem(MainWnd, IDC_GRAPH), NULL, FALSE);

*/