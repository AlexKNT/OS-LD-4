#include "TBuffer.h"
#include <windows.h>

TBuffer::TBuffer() {
    InitializeCriticalSection(&cs);
    head = 0; // Initialize the head to the beginning of the buffer

    // Initialize the buf array to default values if needed
    for (int i = 0; i < g_BUF_SIZE; i++) {
        buf[i].time_in_user_mode = 0; // or any other default value
        buf[i].time_in_kernel_mode = 0; // or any other default value
    }
}

TBuffer::~TBuffer() {
    DeleteCriticalSection(&cs);
}

void TBuffer::Draw(HDC hdc, RECT rect) {
    EnterCriticalSection(&cs);

    // Set the pen colors
    HPEN hUserModePen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0)); // Red pen for user mode
    HPEN hKernelModePen = CreatePen(PS_SOLID, 1, RGB(0, 255, 0)); // Green pen for kernel mode
    HPEN hOldPen;

    // Calculate the coefficients for mapping data to screen coordinates
    int MaxN = g_BUF_SIZE - 1;
    int MaxT = 100; // Assuming percentages are in the range [0, 100]
    int Width = rect.right - rect.left;
    int Height = rect.bottom - rect.top;

    double Ax = static_cast<double>(Width) / MaxN;
    double Ay = -static_cast<double>(Height) / MaxT;
    double Bx = 0;
    double By = Height;

    // Draw the animation using data from the buffer
    int prevX = 0;
    int prevYUserMode = static_cast<int>(By); // Initialize with the starting point
    int prevYKernelMode = static_cast<int>(By); // Initialize with the starting point

    for (int i = 0; i < g_BUF_SIZE; ++i) {
        int x = static_cast<int>(Ax * i + Bx);
        int tUserMode = buf[i].time_in_user_mode; // Use user mode data for the animation
        int tKernelMode = buf[i].time_in_kernel_mode; // Use kernel mode data for the animation

        int yUserMode = static_cast<int>(Ay * tUserMode + By);
        int yKernelMode = static_cast<int>(Ay * tKernelMode + By);

        // Draw a line segment from the previous point to the current point with the respective pen
        if (i > 0) {
            hOldPen = (HPEN)SelectObject(hdc, hUserModePen);
            MoveToEx(hdc, prevX, prevYUserMode, NULL);
            LineTo(hdc, x, yUserMode);
            SelectObject(hdc, hOldPen);

            hOldPen = (HPEN)SelectObject(hdc, hKernelModePen);
            MoveToEx(hdc, prevX, prevYKernelMode, NULL);
            LineTo(hdc, x, yKernelMode);
            SelectObject(hdc, hOldPen);
        }

        prevX = x;
        prevYUserMode = yUserMode;
        prevYKernelMode = yKernelMode;
    }

    // Clean up
    DeleteObject(hUserModePen);
    DeleteObject(hKernelModePen);

    LeaveCriticalSection(&cs);
}




void TBuffer::Put(TBufItem AItem) {
    EnterCriticalSection(&cs);

    // Insert the data at the head
    buf[head] = AItem;

    // Increment the head and wrap around if needed
    head = (head + 1) % g_BUF_SIZE;

    LeaveCriticalSection(&cs);
}

void TBuffer::Clear() {
    EnterCriticalSection(&cs);

    // Clear the buffer by resetting the head
    head = 0;

    LeaveCriticalSection(&cs);
}
