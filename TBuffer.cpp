#include "TBuffer.h"
#include <windows.h>

TBuffer::TBuffer() {
    InitializeCriticalSection(&cs);
    head = 0; // Initialize the head to the beginning of the buffer
}

TBuffer::~TBuffer() {
    DeleteCriticalSection(&cs);
}

void TBuffer::Draw(HDC hdc, RECT rect) {
    EnterCriticalSection(&cs);

    // Draw the buffer contents here using hdc and rect

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
