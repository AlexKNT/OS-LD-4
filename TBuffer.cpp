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
