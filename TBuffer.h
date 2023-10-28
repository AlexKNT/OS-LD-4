#pragma once
#include <windows.h>

struct TBufItem {
    int time_in_user_mode;
    int time_in_kernel_mode;
};

constexpr int g_BUF_SIZE = 100;

class TBuffer {
private:
    int head; // Position of the oldest data
    CRITICAL_SECTION cs; // Critical section for thread safety

public:
    TBufItem buf[g_BUF_SIZE]; // Array for storing data
    TBuffer(); // Constructor
    ~TBuffer(); // Destructor
    void Draw(HDC hdc, RECT rect); // Draw the buffer contents
    void Put(TBufItem AItem); // Insert data at the head
    void Clear(); // Clear the buffer
};
