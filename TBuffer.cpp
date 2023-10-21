#include "TBuffer.h"
#include <windows.h>

struct TBufItem { int time_in_user_mode; int time_in_kernel_mode; };

constexpr int g_BUF_SIZE = 100;

TBuffer::TBuffer() {
    //
}

TBuffer::~TBuffer() {
    //
}

void TBuffer::Draw(HDC hdc, RECT rect) {
    //
}
void TBuffer::Put(TBufItem AItem) {
    //
}
void TBuffer::Clear() {
    //
}