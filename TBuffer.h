#pragma once
#include <windows.h>

struct TBufItem { int time_in_user_mode; int time_in_kernel_mode; };

class TBuffer
{
private:
    TBufItem buf[g_BUF_SIZE]; // masīvs datu glabāšanai
    int head; // visvecāko datu pozīcija
    CRITICAL_SECTION cs; // kritiska sekcija

public:
    TBuffer(); // konstruktors
    ~TBuffer(); // destruktors
    void Draw(HDC hdc, RECT rect); // kadra zīmēšana
    void Put(TBufItem AItem); // datu ievietošana visvecāko datu vietā
    void Clear(); // bufera attīrīšana
};

