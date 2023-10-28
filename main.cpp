#undef UNICODE // Pārslēgt projektu ASCII režīmā
#include <windows.h>
#include <stdio.h>
#include "resource.h"
#include <winuser.h>
#include <processthreadsapi.h>
#include <sysinfoapi.h>
#include "TBuffer.h"

#pragma warning(disable : 4996)

DWORD g_dwProcessId = 0;
HANDLE g_hProcess = NULL;

TBuffer g_Buffer;

bool g_Terminate = false;

DWORD FileTimeToMilliseconds(FILETIME ft) {
    ULARGE_INTEGER i;
    i.LowPart = ft.dwLowDateTime;
    i.HighPart = ft.dwHighDateTime;
    return i.QuadPart / 10000;
}

// Ziņojumu apstrādātāja prototips
BOOL CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);

LRESULT CALLBACK GraphWndProc(
    HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

// Galvenā funkcija
// ja galvenā loga šablonam ir piešķirts resursa identifikators IDD_MAINDIALOG
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {

    WNDCLASS wc = { 0 }; // klases struktūra, aizpildīta ar nullēm
    wc.lpfnWndProc = GraphWndProc; // elementa ziņojumu apstrādāšanas funkcija
    wc.lpszClassName = "GRAPH"; // elementa klases vārds
    RegisterClass(&wc); // iereģistrēt klasi

    DialogBox(
        hInstance,
        MAKEINTRESOURCE(IDD_MAINDIALOG),
        NULL,
        DLGPROC(MainWndProc));

    UnregisterClass("GRAPH", hInstance);

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

constexpr int g_timer_interval = 250;
DWORD WINAPI MonitorProcessThread(LPVOID lpParam) {
    HWND hWnd = (HWND)lpParam;
    char commandLine[MAX_PATH];
    GetDlgItemTextA(hWnd, IDC_COMMANDLINE, commandLine, MAX_PATH);

    SetTimer(hWnd, 1, g_timer_interval, NULL);

    // Create the process
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    if (CreateProcessA(NULL, commandLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        g_dwProcessId = pi.dwProcessId;
        g_hProcess = pi.hProcess;

        // Clear the buffer and Terminate flag at the start
        g_Buffer.Clear();
        g_Terminate = false;

        // Disable the Start button and enable the Terminate button
        EnableWindow(GetDlgItem(hWnd, IDC_START), false);
        EnableWindow(GetDlgItem(hWnd, IDC_TERMINATE), true);

        DWORD real_run_time1 = GetTickCount();
        DWORD time_in_user_mode1 = 0;
        DWORD time_in_kernel_mode1 = 0;

        while (!g_Terminate) {
            FILETIME creationTime, exitTime, kernelTime, userTime;

            if (GetProcessTimes(g_hProcess, &creationTime, &exitTime, &kernelTime, &userTime)) {
                // Calculate the percentage of time consumed
                DWORD real_run_time2 = GetTickCount();
                DWORD time_in_user_mode2 = static_cast<int>(FileTimeToMilliseconds(userTime));
                DWORD time_in_kernel_mode2 = static_cast<int>(FileTimeToMilliseconds(kernelTime));

                DWORD time_interval = real_run_time2 - real_run_time1;
                if (time_interval > 0) {
                    int user_mode_percentage = static_cast<int>((time_in_user_mode2 - time_in_user_mode1) * 100 / time_interval);
                    int kernel_mode_percentage = static_cast<int>((time_in_kernel_mode2 - time_in_kernel_mode1) * 100 / time_interval);

                    TBufItem item;
                    item.time_in_user_mode = user_mode_percentage;
                    item.time_in_kernel_mode = kernel_mode_percentage;
                    g_Buffer.Put(item);
                }

                real_run_time1 = real_run_time2;
                time_in_user_mode1 = time_in_user_mode2;
                time_in_kernel_mode1 = time_in_kernel_mode2;
            }

            Sleep(g_timer_interval); // Sleep for the specified interval

            // Check if the process is still running
            DWORD exitCode;
            if (GetExitCodeProcess(g_hProcess, &exitCode) && exitCode != STILL_ACTIVE) {
                // Process has finished, break out of the loop
                break;
            }
        }

        // If the process is still running, terminate it
        if (!g_Terminate) {
            TerminateProcess(g_hProcess, 0);
        }

        // Enable the Start button and disable the Terminate button
        EnableWindow(GetDlgItem(hWnd, IDC_START), true);
        EnableWindow(GetDlgItem(hWnd, IDC_TERMINATE), false);

        // Close process handles when the thread exits
        CloseHandle(g_hProcess);
        CloseHandle(pi.hThread);
    }

    KillTimer(hWnd, 1);

    return 0;
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
        {
            HANDLE hThread = CreateThread(NULL, 0, MonitorProcessThread, hWnd, 0, NULL);
            CloseHandle(hThread);
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

        case IDC_COMMANDLINE:
            //
            return TRUE;
        }

    case WM_DESTROY: // ja tas ir pabeigšanas pieprasījums
        PostQuitMessage(0); // pabeigt programmu
        return TRUE;

    case WM_TIMER:
        // Redraw the IDC_GRAPH control to update the animation
        InvalidateRect(GetDlgItem(hWnd, IDC_GRAPH), NULL, FALSE);
        break;
    }
    return FALSE;
}

LRESULT CALLBACK GraphWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    switch (Msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        RECT rect;
        GetClientRect(hWnd, &rect);

        // Calculate the coefficients for mapping data to screen coordinates
        int MaxN = g_BUF_SIZE - 1;
        int MaxT = 100; // Assuming percentages are in the range [0, 100]
        int Width = rect.right - rect.left;
        int Height = rect.bottom - rect.top;

        double Ax = static_cast<double>(Width) / MaxN;
        double Ay = -static_cast<double>(Height) / MaxT;
        double Bx = 0;
        double By = Height;

        // Draw the animation using data from g_Buffer
        int prevX = 0;
        int prevY = static_cast<int>(By); // Initialize with the starting point

        for (int i = 0; i < g_BUF_SIZE; ++i) {
            int x = static_cast<int>(Ax * i + Bx);
            int t = g_Buffer.buf[i].time_in_user_mode; // Use user mode data for the animation
            int y = static_cast<int>(Ay * t + By);

            // Draw a line segment from the previous point to the current point
            MoveToEx(hdc, prevX, prevY, NULL);
            LineTo(hdc, x, y);

            prevX = x;
            prevY = y;
        }

        EndPaint(hWnd, &ps);
        return 0;
    }
    }
    return DefWindowProc(hWnd, Msg, wParam, lParam);
}

/*
With resource.h containing a dialog with: IDD_MAINDIALOG, Edit Control IDC_COMMANDLINE, Button IDC_BROWSE, Button IDC_START, Button IDC_TERMINATE, Custom Control IDC_GRAPH with Class GRAPH
TBuffer.h containing this code:
#pragma once
#include <windows.h>

struct TBufItem {
    int time_in_user_mode;
    int time_in_kernel_mode;
};

constexpr int g_BUF_SIZE = 100;

class TBuffer {
private:
    TBufItem buf[g_BUF_SIZE]; // Array for storing data
    int head; // Position of the oldest data
    CRITICAL_SECTION cs; // Critical section for thread safety

public:
    TBuffer(); // Constructor
    ~TBuffer(); // Destructor
    void Draw(HDC hdc, RECT rect); // Draw the buffer contents
    void Put(TBufItem AItem); // Insert data at the head
    void Clear(); // Clear the buffer
};

and TBuffer.cpp containing this code:

#include "TBuffer.h"
#include <windows.h>

CRITICAL_SECTION g_CriticalSection;

TBuffer::TBuffer() {
    InitializeCriticalSection(&g_CriticalSection);
    head = 0; // Initialize the head to the beginning of the buffer

    // Initialize the buf array to default values if needed
    for (int i = 0; i < g_BUF_SIZE; i++) {
        buf[i].time_in_user_mode = 0; // or any other default value
        buf[i].time_in_kernel_mode = 0; // or any other default value
    }
}

TBuffer::~TBuffer() {
    DeleteCriticalSection(&g_CriticalSection);
}

void TBuffer::Draw(HDC hdc, RECT rect) {
    EnterCriticalSection(&g_CriticalSection);

    // Draw the buffer contents here using hdc and rect

    LeaveCriticalSection(&cs);
}

void TBuffer::Put(TBufItem AItem) {
    EnterCriticalSection(&g_CriticalSection);

    // Insert the data at the head
    buf[head] = AItem;

    // Increment the head and wrap around if needed
    head = (head + 1) % g_BUF_SIZE;

    LeaveCriticalSection(&g_CriticalSection);
}

void TBuffer::Clear() {
    EnterCriticalSection(&g_CriticalSection);

    // Clear the buffer by resetting the head
    head = 0;

    LeaveCriticalSection(&g_CriticalSection);
}


*/


// TODO Task: Write a program that runs a process and outputs the dynamic graphics on the screentime consumed by these processes.
// 1 Implement IDC_BROWSE DONE
// 2 Implement starting a process from IDC_COMMANDLINE. It should create a thread to run the process DONE
// 3 Implement the Terminate buttom DONE
// 4 Create a global buffer DONE
// 5 The created thread must periodically receive information about the CPU time spent and write this information to the global buffer DONE
// 6 The IDD_MAINDIALOG from needs to read information from that buffer periodically and need to draw the next frame in IDC_GRAPH to make an animation.
// 7 Working with the buffer would require making and utilizing critical sections


// Program architecture

// Process thread
// 1 User selects a .exe file via IDC_BROWSE or types the name of the file in IDC_COMMANDLINE and then presses IDC_START
// 2 The program creates a thread that runs the process that it reads from IDC_COMMANDLINE. 
// 3 This thread runs the process
// 4 The thread calculates the CPU time consumed by the process every 250 milliseconds and writes the calculated values to a global buffer.

// Main Thread
// The main thread is created automatically at program startup
// It serves the main IDC_MAINDIALOG
// It periodically reads data from the global buffer and draws on a IDC_GRAPH, creating an animation <-

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