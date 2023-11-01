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

    // Set the pen color to red
    HPEN hRedPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hRedPen);

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
    int prevY = static_cast<int>(By); // Initialize with the starting point

    for (int i = 0; i < g_BUF_SIZE; ++i) {
        int x = static_cast<int>(Ax * i + Bx);
        int t = buf[i].time_in_user_mode; // Use user mode data for the animation
        int y = static_cast<int>(Ay * t + By);

        // Draw a line segment from the previous point to the current point
        MoveToEx(hdc, prevX, prevY, NULL);
        LineTo(hdc, x, y);

        prevX = x;
        prevY = y;
    }

    // Restore the original pen and clean up
    SelectObject(hdc, hOldPen);
    DeleteObject(hRedPen);

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
