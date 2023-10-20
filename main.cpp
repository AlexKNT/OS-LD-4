#undef UNICODE // Pārslēgt projektu ASCII režīmā
#include <windows.h>
#include <stdio.h>
#include "resource.h"
#pragma warning(disable : 4996)

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
        DLGPROC(MainWndProc)
    );
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

    case WM_TIMER:
        //
        return TRUE;
        
    case WM_DESTROY: // ja tas ir pabeigšanas pieprasījums
        PostQuitMessage(0); // pabeigt programmu
        return TRUE;
    }
    return FALSE;
}

/*
I have C++ WINAPI code
#undef UNICODE // Pārslēgt projektu ASCII režīmā
#include <windows.h>
#include <stdio.h>
#include "resource.h"
#pragma warning(disable : 4996)
// Ziņojumu apstrādātāja prototips
BOOL CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
// Galvenā funkcija
// ja galvenā loga šablonam ir piešķirts resursa identifikators IDD_MAINDIALOG
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    DialogBox(
        hInstance,
        MAKEINTRESOURCE(IDD_MAINDIALOG),
        NULL,
        DLGPROC(MainWndProc)
    );
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
        }
        return FALSE;
    case WM_DESTROY: // ja tas ir pabeigšanas pieprasījums
        PostQuitMessage(0); // pabeigt programmu
        return TRUE;
    }
    return FALSE;
}

And resource contains
Edit control IDC_COMMANDLINE, Button IDC_START, Button IDC_TERMINATE, Static text IDC_MAX, Static text IDC_MID, Static text IDC_MIN, Button IDOK, Button IDC_BROWSE
*/

// TODO Task: Write a program that runs a process and outputs the dynamic graphics on the screentime consumed by these processes.
// 1 Implement IDC_BROWSE
// 2 Implement starting a process from IDC_COMMANDLINE. It should create a thread to run the process
// 3 Implement the Terminate buttom
// 4 Create a global buffer
// 5 The created thread must periodically receive information about the CPU time spent and write this information to the global buffer
// 6 The IDD_MAINDIALOG from needs to read information from that buffer periodically and need to draw the next frame in IDC_GRAPH to make an animation.
// 7 Working with the buffer would require making and utilizing critical sections